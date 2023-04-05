#include "sim.hpp"
#include <madrona/mw_gpu_entry.hpp>
#include <random>

#include<cmath>

using namespace madrona;
using namespace madrona::math;


namespace SimpleSpread {


    void Sim::registerTypes(ECSRegistry &registry, const Config &cfg)
{
    base::registerTypes(registry);

    registry.registerSingleton<WorldReset>();
    registry.registerSingleton<WorldState>();

    registry.registerComponent<Action>();
    registry.registerComponent<Observation>();
    registry.registerComponent<Kinematics>();
    registry.registerComponent<Communication>();
    registry.registerComponent<Reward>();
    registry.registerComponent<AgentID>();

    registry.registerFixedSizeArchetype<Agent>(NUM_AGENTS);

    // Export tensors for pytorch
    registry.exportSingleton<WorldReset>(0);
    registry.exportColumn<Agent, Action>(1);
    registry.exportColumn<Agent, Observation>(2);
    registry.exportColumn<Agent, Reward>(3);
    registry.exportColumn<Agent, AgentID>(4);
    registry.exportColumn<Agent, WorldID>(5);
}

inline void updateObs(Engine &ctx, Observation &obs, Kinematics &k, AgentID &id)
{
    WorldState &state = ctx.getSingleton<WorldState>();

    int i = 0;
    obs.statevec[i++] = k.pos.x;
    obs.statevec[i++] = k.pos.y;
    obs.statevec[i++] = k.vel.x;
    obs.statevec[i++] = k.vel.y;

    for (int l = 0; l < NUM_LANDMARKS; l++) {
        obs.statevec[i++] = state.landmark_pos[l].x;
        obs.statevec[i++] = state.landmark_pos[l].y;
    }

    for (int b = 0; b < NUM_AGENTS; b++) {
        if (id.id == b) continue;
        Entity agentB = ctx.data().agents[b];
        Kinematics kB = ctx.getUnsafe<Kinematics>(agentB);
        Communication commB = ctx.getUnsafe<Communication>(agentB);

        obs.statevec[i++] = kB.pos.x;
        obs.statevec[i++] = kB.pos.y;
        obs.statevec[i++] = (float) commB.comm[0];
        obs.statevec[i++] = (float) commB.comm[1];
    }
}

inline void observationSystem(Engine &ctx, Observation &obs, Kinematics &k, AgentID &id)
{
    updateObs(ctx, obs, k, id);
}

inline std::tuple<Vector2, Vector2> getCollisionForce(Vector2 aPos, float aSize, Vector2 bPos, float bSize)
{
    Vector2 deltaPos = aPos - bPos;
    float dist = deltaPos.length();
    float minDist = aSize + bSize;

    float k = CONTACT_MARGIN;

    float penetration = log(1 + exp(-(dist - minDist) / k)) * k;
    Vector2 force = CONTACT_FORCE * deltaPos / dist * penetration;
    return std::make_tuple(force, -force);
}

static void resetWorld(Engine &ctx)
{
    // Update the RNG seed for a new episode
    EpisodeManager &episode_mgr = *ctx.data().episodeMgr;
    uint32_t episode_idx =
        episode_mgr.curEpisode.fetch_add(1, std::memory_order_relaxed);
    ctx.data().rng = RNG::make(episode_idx);

    WorldState &state = ctx.getSingleton<WorldState>();

    for (int agentID = 0; agentID < NUM_AGENTS; agentID++) {
        Entity agent = ctx.data().agents[agentID];

        ctx.getUnsafe<Kinematics>(agent) = {
                {(float) ctx.data().rng.rand() * 2 - 1, (float) ctx.data().rng.rand() * 2 - 1},
                {0, 0}
        };
        ctx.getUnsafe<Communication>(agent) = {0, 0};
    }

    for (int landmarkID = 0; landmarkID < NUM_LANDMARKS; landmarkID++) {
        state.landmark_pos[landmarkID] = {(float) ctx.data().rng.rand() * 2 - 1, (float) ctx.data().rng.rand() * 2 - 1};
        state.landmark_vel[landmarkID] = {0, 0};
    }

    for (int agentID = 0; agentID < NUM_AGENTS; agentID++) {
        Entity agent = ctx.data().agents[agentID];
        Observation &obs = ctx.getUnsafe<Observation>(agent);
        Kinematics &k = ctx.getUnsafe<Kinematics>(agent);
        AgentID &id_obj = ctx.getUnsafe<AgentID>(agent);
        updateObs(ctx, obs, k, id_obj);
    }
}

inline void actionSystem(Engine &ctx, WorldState state)
{
    Vector2 actionForces[NUM_AGENTS];

    for (int id = 0; id < NUM_AGENTS; id++) {
        Entity agent = ctx.data().agents[id];
        Action action = ctx.getUnsafe<Action>(agent);

        Vector2 actionForce = {0, 0};
        switch (action.choice) {
            case 0:
                actionForce.x = -1.0f;
                break;
            case 1:
                actionForce.x = 1.0f;
                break;
            case 2:
                actionForce.y = -1.0f;
                break;
            case 3:
                actionForce.y = 1.0f;
                break;
        }

        Communication &comm = ctx.getUnsafe<Communication>(agent);
        for (uint32_t &i : comm.comm) i = 0;
        comm.comm[action.comm] = 1;
    }

    // apply_action_force
    // TODO: add noise

    // apply_environment_force
    Vector2 totalAgentForces[NUM_AGENTS];
    Vector2 totalLandmarkForces[NUM_LANDMARKS];

    // agent-agent interactions
    for (int a = 0; a < NUM_AGENTS; a++) {
        Entity agentA = ctx.data().agents[a];
        Kinematics kinematicsA = ctx.getUnsafe<Kinematics>(agentA);
        for (int b = a + 1; b < NUM_AGENTS; b++) {
            Entity agentB = ctx.data().agents[b];
            Kinematics kinematicsB = ctx.getUnsafe<Kinematics>(agentB);
            auto collisionForces = getCollisionForce(kinematicsA.pos, AGENT_SIZE, kinematicsB.pos, AGENT_SIZE);
            totalAgentForces[a] = actionForces[a] + std::get<0>(collisionForces);
            totalAgentForces[b] = actionForces[b] + std::get<1>(collisionForces);
        }
    }

    // landmark-landmark interactions
    for (int a = 0; a < NUM_LANDMARKS; a++) {
        for (int b = a + 1; b < NUM_LANDMARKS; b++) {
            auto collisionForces = getCollisionForce(state.landmark_pos[a], LANDMARK_SIZE, state.landmark_pos[b], LANDMARK_SIZE);
            totalLandmarkForces[a] = std::get<0>(collisionForces);
            totalLandmarkForces[b] = std::get<1>(collisionForces);
        }
    }

    // agent-landmark interactions
    for (int a = 0; a < NUM_AGENTS; a++) {
        Entity agentA = ctx.data().agents[a];
        Kinematics kinematicsA = ctx.getUnsafe<Kinematics>(agentA);
        for (int b = 0; b < NUM_LANDMARKS; b++) {
            auto collisionForces = getCollisionForce(kinematicsA.pos, AGENT_SIZE, state.landmark_pos[b], LANDMARK_SIZE);
            totalAgentForces[a] = totalAgentForces[a] + std::get<0>(collisionForces);
            totalLandmarkForces[b] = totalLandmarkForces[b] + std::get<1>(collisionForces);
        }
    }

    // integrate_state

    // update agents
    for (int a = 0; a < NUM_AGENTS; a++) {
        Entity agent = ctx.data().agents[a];
        Kinematics &kinematics = ctx.getUnsafe<Kinematics>(agent);
        kinematics.vel = kinematics.vel * (1 - VEL_DAMPING);
        kinematics.vel += totalAgentForces[a] / MASS * DT;
        kinematics.pos += kinematics.vel * DT;
    }

    // update landmarks
    for (int l = 0; l < NUM_LANDMARKS; l++) {
        Vector2 &vel = state.landmark_vel[l];
        vel = vel * (1 - VEL_DAMPING);
        vel += totalLandmarkForces[l] / MASS * DT;
        state.landmark_pos[l] += vel * DT;
    }
}

inline void timeSystem(Engine &ctx, WorldState &state)
{
    state.time -= 1;
}

inline void checkDone(Engine &ctx, WorldReset &reset)
{
    reset.resetNow = false;
    WorldState state = ctx.getSingleton<WorldState>();

    // reward calculation

    // distance reward
    float reward = 0;
    for (int i = 0; i < NUM_LANDMARKS; i++) {
        float minDist = -1;
        for (int a = 0; a < NUM_AGENTS; a++) {
            Entity agent = ctx.data().agents[a];
            Kinematics k = ctx.getUnsafe<Kinematics>(agent);
            float curDist = (k.pos - state.landmark_pos[i]).length();
            if (minDist == -1 || curDist < minDist) {
                minDist = curDist;
            }
        }
        reward -= minDist;
    }

    // collision reward
    for (int a = 0; a < NUM_AGENTS; a++) {
        Entity agentA = ctx.data().agents[a];
        Kinematics kA = ctx.getUnsafe<Kinematics>(agentA);
        for (int b = a + 1; b < NUM_AGENTS; b++) {
            Entity agentB = ctx.data().agents[b];
            Kinematics kB = ctx.getUnsafe<Kinematics>(agentB);
            float dist = (kA.pos - kB.pos).length();
            if (dist < 2 * AGENT_SIZE) {
                reward -= 1;
            }
        }
    }

    // set reward
    for (int a = 0; a < NUM_AGENTS; a++) {
        Entity agent = ctx.data().agents[a];
        ctx.getUnsafe<Reward>(agent).rew = reward;
    }

    if (state.time == 0) {
        reset.resetNow = true;
    }

    if (reset.resetNow) {
        resetWorld(ctx);
    }
}



    void Sim::setupTasks(TaskGraph::Builder &builder, const Config &)
{
    // auto reset_sys =
    //     builder.addToGraph<ParallelForNode<Engine, resetSystem, WorldReset>>({});

    // auto sort_sys =
    //     builder.addToGraph<SortArchetypeNode<Agent, WorldID>>({reset_sys});

    // auto clear_tmp_alloc =
    //     builder.addToGraph<ResetTmpAllocNode>({sort_sys});

    auto action_sys = builder.addToGraph<ParallelForNode<Engine, actionSystem, WorldState>>({});
    auto time_sys = builder.addToGraph<ParallelForNode<Engine, timeSystem, WorldState>>({action_sys});
    auto update_obs = builder.addToGraph<ParallelForNode<Engine, observationSystem, Observation, Kinematics, AgentID>>({time_sys});
    auto terminate_sys = builder.addToGraph<ParallelForNode<Engine, checkDone, WorldReset>>({update_obs});

    (void)terminate_sys;
    // (void) action_sys;

    // printf("Setup done\n");
}


Sim::Sim(Engine &ctx, const Config& cfg, const WorldInit &init)
    : WorldBase(ctx),
      episodeMgr(init.episodeMgr)
{
    // Make a buffer that will last the duration of simulation for storing
    // agent entity IDs
    agents = (Entity *)rawAlloc(NUM_AGENTS * sizeof(Entity));

    for (int i = 0; i < NUM_AGENTS; i++) {
        agents[i] = ctx.makeEntityNow<Agent>();
        ctx.getUnsafe<AgentID>(agents[i]).id = i;
    }

    // Initial reset
    resetWorld(ctx);
    ctx.getSingleton<WorldReset>().resetNow = false;
}

    MADRONA_BUILD_MWGPU_ENTRY(Engine, Sim, Config, WorldInit);

}
