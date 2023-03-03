#pragma once

#include <madrona/taskgraph.hpp>
#include <madrona/math.hpp>
#include <madrona/custom_context.hpp>
#include <madrona/components.hpp>

#include "init.hpp"
#include "rng.hpp"

#define NUM_AGENTS 3
#define NUM_LANDMARKS NUM_AGENTS
#define DIM_P 2
#define DIM_V DIM_P
#define DIM_C 2
#define OBS_SIZE (DIM_P + DIM_V + (NUM_LANDMARKS * DIM_P) + (NUM_AGENTS - 1) * (DIM_P + DIM_C))

#define DT 0.1f
#define CONTACT_MARGIN 0.001f
#define CONTACT_FORCE 100

#define AGENT_SIZE 0.15f
#define LANDMARK_SIZE 0.05f
#define VEL_DAMPING 0.25f
#define MASS 1.0f

#define EPISODE_LENGTH 25

namespace SimpleSpread {

    struct RendererInitStub {};

    class Engine;

    // singletons

    struct WorldReset {
        int32_t resetNow;
    };

    struct WorldState {
        int32_t time;
        madrona::math::Vector2 landmark_pos[NUM_LANDMARKS];
        madrona::math::Vector2 landmark_vel[NUM_LANDMARKS];
    };

    // per-agent

    struct AgentID {
        int32_t id;
    };

    struct Action {
        int32_t choice; // choice from discrete action space
        int32_t comm;
    };

    struct Observation {
        float statevec[OBS_SIZE];
    };

    struct Kinematics {
        madrona::math::Vector2 pos;
        madrona::math::Vector2 vel;
    };

    struct Communication {
        uint32_t comm[DIM_C];
    };

    struct Reward {
        float rew;
    };

    struct Agent : public madrona::Archetype<Action, Observation, Kinematics, Communication, Reward, AgentID> {};

    struct Config {};

    struct Sim : public madrona::WorldBase {
        static void registerTypes(madrona::ECSRegistry &registry, const Config &cfg);

        static void setupTasks(madrona::TaskGraph::Builder &builder, const Config &cfg);

        Sim(Engine &ctx, const Config& cfg, const WorldInit &init);

        EpisodeManager *episodeMgr;
        RNG rng;

        madrona::Entity *agents;
    };

    class Engine : public ::madrona::CustomContext<Engine, Sim> {
        using CustomContext::CustomContext;
    };

}
