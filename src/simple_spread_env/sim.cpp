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
    registry.registerComponent<ObjectID>();

    registry.registerFixedSizeArchetype<Agent>(NUM_AGENTS);
    registry.registerFixedSizeArchetype<Landmark>(NUM_LANDMARKS);

    // Export tensors for pytorch
    registry.exportSingleton<WorldReset>(0);
    registry.exportColumn<Agent, Action>(1);
    registry.exportColumn<Agent, Observation>(2);
    registry.exportColumn<Agent, Reward>(3);
    registry.exportColumn<Agent, ObjectID>(4);
    registry.exportColumn<Agent, WorldID>(5);
}

inline void observationSystem(Engine &ctx, Observation &obs, Kinematics &k, ObjectID &id)
{
    WorldState &state = ctx.getSingleton<WorldState>();

    int i = 0;
    obs.statevec[i++] = k.pos.x;
    obs.statevec[i++] = k.pos.y;
    obs.statevec[i++] = k.vel.x;
    obs.statevec[i++] = k.vel.y;

    for (int l = 0; l < NUM_LANDMARKS; l++) {
        Entity landmark = ctx.data().landmarks[l];
        Kinematics kL = ctx.getUnsafe<Kinematics>(landmark);
        obs.statevec[i++] = kL.pos.x;
        obs.statevec[i++] = kL.pos.y;
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
                AGENT_SIZE,
                {(float) ctx.data().rng.rand() * 2 - 1, (float) ctx.data().rng.rand() * 2 - 1},
                {0, 0}
        };
        ctx.getUnsafe<Communication>(agent) = {0, 0};
    }

    for (int landmarkID = 0; landmarkID < NUM_LANDMARKS; landmarkID++) {
        Entity landmark = ctx.data().landmarks[landmarkID];
        Kinematics &kL = ctx.getUnsafe<Kinematics>(landmark);
        kL.size = LANDMARK_SIZE;
        kL.pos = {(float) ctx.data().rng.rand() * 2 - 1, (float) ctx.data().rng.rand() * 2 - 1};
        kL.vel = {0, 0};
    }

    for (int agentID = 0; agentID < NUM_AGENTS; agentID++) {
        Entity agent = ctx.data().agents[agentID];
        Observation &obs = ctx.getUnsafe<Observation>(agent);
        Kinematics &k = ctx.getUnsafe<Kinematics>(agent);
        ObjectID &id_obj = ctx.getUnsafe<ObjectID>(agent);
        observationSystem(ctx, obs, k, id_obj);
    }
}

inline void actionSystem(Engine &ctx, ObjectID &id, Action &action, Communication &comm) {
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

    ctx.data().total_object_forces[id.id] = actionForce;

    for (uint32_t &i : comm.comm) i = 0;
    comm.comm[action.comm] = 1;
}

inline void objectInteractionSystem(Engine &ctx, ObjectID &id, Kinematics &k) {
    for (int b = 0; b < NUM_OBJECTS; b++) {
        if (b == id.id) continue;
        Entity objectB = ctx.data().objects[b];
        Kinematics kinematicsB = ctx.getUnsafe<Kinematics>(objectB);
        auto collisionForces = getCollisionForce(k.pos, k.size, kinematicsB.pos, kinematicsB.size);
        ctx.data().total_object_forces[id.id] += std::get<0>(collisionForces);
    }
}

inline void applyForcesSystem(Engine &ctx, ObjectID &id, Kinematics &k) {
    k.vel = k.vel * (1 - VEL_DAMPING);
    k.vel += ctx.data().total_object_forces[id.id] / MASS * DT;
    k.pos += k.vel * DT;
}

inline void timeSystem(Engine &ctx, WorldState &state)
{
    state.time -= 1;
}

inline void calculateRewardSystem(Engine &ctx, ObjectID &id, Kinematics &k) {
    float reward = 0.0f;
    float minDist = -1.0f; // for landmarks

    for (int b = 0; b < NUM_AGENTS; b++) {
        if (b == id.id) continue;
        Entity agentB = ctx.data().agents[b];
        Kinematics kB = ctx.getUnsafe<Kinematics>(agentB);
        float dist = (k.pos - kB.pos).length();

        if (id.id < NUM_AGENTS) {
            // agent; calculate collision reward
            if (dist < 2 * AGENT_SIZE) {
                reward -= 1.0f / 2.0f; // divide by two to handle double-counting
            }
        } else {
            // landmark; calculate min distance reward
            if (minDist == -1 || dist < minDist) {
                minDist = dist;
            }
        }
    }
    if (id.id >= NUM_AGENTS) reward -= minDist;

    ctx.data().intermediate_rews[id.id] = reward;
}

inline void applyRewardAndCheckDoneSystem(Engine &ctx, WorldReset &reset) {
    float totalReward = 0.0f;
    for (float intermediate_rew : ctx.data().intermediate_rews) {
        totalReward += intermediate_rew;
    }

    for (int a = 0; a < NUM_AGENTS; a++) {
        Entity agent = ctx.data().agents[a];
        ctx.getUnsafe<Reward>(agent).rew = totalReward;
    }

    WorldState state = ctx.getSingleton<WorldState>();
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

    auto action_sys = builder.addToGraph<ParallelForNode<Engine, actionSystem, ObjectID, Action, Communication>>({});
    auto object_interaction_sys = builder.addToGraph<ParallelForNode<Engine, objectInteractionSystem, ObjectID, Kinematics>>({action_sys});
    auto apply_forces_sys = builder.addToGraph<ParallelForNode<Engine, applyForcesSystem, ObjectID, Kinematics>>({object_interaction_sys});
    auto time_sys = builder.addToGraph<ParallelForNode<Engine, timeSystem, WorldState>>({apply_forces_sys});
    auto update_obs_sys = builder.addToGraph<ParallelForNode<Engine, observationSystem, Observation, Kinematics, ObjectID>>({time_sys});
    auto calculate_reward_sys = builder.addToGraph<ParallelForNode<Engine, calculateRewardSystem, ObjectID, Kinematics>>({update_obs_sys});
    auto terminate_sys = builder.addToGraph<ParallelForNode<Engine, applyRewardAndCheckDoneSystem, WorldReset>>({calculate_reward_sys});

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
    objects = (Entity *)rawAlloc((NUM_OBJECTS) * sizeof(Entity));

    agents = objects;
    for (int i = 0; i < NUM_AGENTS; i++) {
        objects[i] = ctx.makeEntityNow<Agent>();
        ctx.getUnsafe<ObjectID>(objects[i]).id = i;
    }

    landmarks = &objects[NUM_AGENTS];
    for (int i = NUM_AGENTS; i < NUM_OBJECTS; i++) {
        objects[i] = ctx.makeEntityNow<Landmark>();
        ctx.getUnsafe<ObjectID>(objects[i]).id = i;
    }

    // Initial reset
    resetWorld(ctx);
    ctx.getSingleton<WorldReset>().resetNow = false;
    ctx.getSingleton<WorldState>().time = EPISODE_LENGTH;
}

    MADRONA_BUILD_MWGPU_ENTRY(Engine, Sim, Config, WorldInit);

}
