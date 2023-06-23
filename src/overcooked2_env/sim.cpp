#include "sim.hpp"
#include <madrona/mw_gpu_entry.hpp>

#include<cmath>

using namespace madrona;
using namespace madrona::math;


namespace Simplecooked {

    
    void Sim::registerTypes(ECSRegistry &registry, const Config &cfg)
    {
        base::registerTypes(registry);

        registry.registerSingleton<WorldReset>();
        registry.registerSingleton<WorldState>();
    
        registry.registerComponent<Action>();
        registry.registerComponent<PlayerState>();
        registry.registerComponent<AgentID>();
        registry.registerComponent<ActionMask>();
        registry.registerComponent<ActiveAgent>();
        registry.registerComponent<Reward>();

        registry.registerComponent<LocationXObservation>();
        registry.registerComponent<LocationXID>();
        
        registry.registerComponent<LocationData>();

        registry.registerComponent<PotInfo>();

        int num_pots = 0;
        for (int x = 0; x < cfg.height * cfg.width; x++) {
            if (cfg.terrain[x] == TerrainT::POT) {
                num_pots++;
            }
        }
        registry.registerFixedSizeArchetype<Agent>(cfg.num_players);
        registry.registerFixedSizeArchetype<LocationType>(cfg.width * cfg.height);
        registry.registerFixedSizeArchetype<LocationXPlayer>(cfg.width * cfg.height * cfg.num_players);
        registry.registerFixedSizeArchetype<PotType>(num_pots);

        // Export tensors for pytorch
        registry.exportSingleton<WorldReset>(0);
        registry.exportColumn<Agent, ActiveAgent>(1);
        registry.exportColumn<Agent, Action>(2);
        registry.exportColumn<Agent, ActionMask>(4);
        registry.exportColumn<Agent, Reward>(5);
        registry.exportColumn<Agent, WorldID>(6);
        registry.exportColumn<Agent, AgentID>(7);

        registry.exportColumn<LocationXPlayer, LocationXObservation>(3);
        registry.exportColumn<LocationXPlayer, WorldID>(8);
        registry.exportColumn<LocationXPlayer, LocationXID>(9);
    }

    inline int32_t get_time(WorldState &ws, Object &soup)
    {
        return ws.recipe_times[soup.get_recipe()];
    }

    inline bool is_cooking(WorldState &ws, Object &soup)
    {
        return soup.cooking_tick >= 0 && soup.cooking_tick < get_time(ws, soup);
    }

    inline bool is_ready(WorldState &ws, Object &soup)
    {
        return soup.cooking_tick >= 0 && soup.cooking_tick >= get_time(ws, soup);
    }

    inline void observationSystem(Engine &ctx, LocationXObservation &obs, LocationXID &id)
    {
        WorldState &ws = ctx.getSingleton<WorldState>();

        int32_t loc = id.id % (ws.size);
        int32_t current_player = id.id / (ws.size);
        
        int32_t shift = 5 * ws.num_players;
        LocationData &dat = ctx.getUnsafe<LocationData>(ctx.data().locations[loc]);
        Object &obj = dat.object;
        
        obs.x[shift + 5] = 0;
        obs.x[shift + 6] = 0;
        obs.x[shift + 7] = 0;
        obs.x[shift + 8] = 0;
        obs.x[shift + 9] = 0;

        if (obj.name == ObjectT::SOUP) {
            if (dat.terrain == TerrainT::POT) {
                obs.x[shift + 5] = obj.num_onions;
                if (obj.cooking_tick < 0) {
                    obs.x[shift + 6] = 0;
                } else {
                    obs.x[shift + 6] = obj.cooking_tick;
                }
            } else {
                obs.x[shift + 7] = 1;
            }
        } else if (obj.name == ObjectT::DISH) {
            obs.x[shift + 8] = 1;
        } else if (obj.name == ObjectT::ONION) {
            obs.x[shift + 9] = 1;
        }

        if (dat.past_player != -1) {
            int32_t relative_player;
            if (dat.past_player == current_player) {
                relative_player = 0;
            } else if (dat.past_player < current_player) {
                relative_player = dat.past_player + 1;
            } else {
                relative_player = dat.past_player;
            }

            obs.x[relative_player] = 0;
            obs.x[ws.num_players + 4 * relative_player + dat.past_orientation] = 0;
        }

        if (dat.current_player != -1) {
            int other_player = dat.current_player;
            int i;
            if (other_player == current_player) {
                i = 0;
            } else if (other_player < current_player) {
                i = other_player + 1;
            } else {
                i = other_player;
            }
            PlayerState &ps = ctx.getUnsafe<PlayerState>(ctx.data().agents[other_player]);

            obs.x[i] = 1;
            obs.x[ws.num_players + 4 * i + ps.orientation] = 1;

            if (ps.has_object()) {
                Object &obj2 = ps.get_object();
                if (obj2.name == ObjectT::SOUP) {
                    obs.x[shift + 7] = 1;
                } else if (obj2.name == ObjectT::DISH) {
                    obs.x[shift + 8] = 1;
                } else if (obj2.name == ObjectT::ONION) {
                    obs.x[shift + 9] = 1;
                }
            }
        }
    }

    inline int32_t deliver_soup(WorldState &ws, PlayerState &ps, Object &soup)
    {
        ps.remove_object();
        return ws.recipe_values[soup.get_recipe()];
    }

    inline bool soup_to_be_cooked_at_location(WorldState &ws, Object &obj)
    {
        return obj.name == ObjectT::SOUP && !is_cooking(ws, obj) && !is_ready(ws, obj) && obj.num_ingredients() > 0;
    }

    inline bool soup_ready_at_location(WorldState &ws, Object &obj)
    {
        return obj.name == ObjectT::SOUP && is_ready(ws, obj);
    }

    inline int32_t move_in_direction(int32_t point, int32_t direction, int64_t width)
    {
        if (direction == ActionT::NORTH) {
            return point - width;
        } else if (direction == ActionT::SOUTH) {
            return point + width;
        } else if (direction == ActionT::EAST) {
            return point + 1;
        } else if (direction == ActionT::WEST) {
            return point - 1;
        }
        return point;
    }

    inline int get_pot_states(Engine &ctx, WorldState &ws)
    {
        int non_empty_pots = 0;
        for (int p = 0; p < ws.num_pots; p++) {
            int id = ctx.getUnsafe<PotInfo>(ctx.data().pots[p]).id;
            LocationData &dat = ctx.getUnsafe<LocationData>(ctx.data().locations[id]);
            if (dat.object.name != ObjectT::NONE && (dat.object.cooking_tick >= 0 || dat.object.num_ingredients() < MAX_NUM_INGREDIENTS)) {
                non_empty_pots++;
            }
        }
        return non_empty_pots;
    }

    inline bool is_dish_pickup_useful(Engine &ctx, int non_empty_pots)
    {
        int num_player_dishes = 0;
        for (int p = 0; p < 2; p++) {
            if (ctx.getUnsafe<PlayerState>(ctx.data().agents[p]).held_object.name == ObjectT::DISH) {
                num_player_dishes++;
            }
        }
        return num_player_dishes < non_empty_pots;
    }

    inline void resolve_interacts(Engine &ctx, WorldState &ws)
    {
        int32_t pot_states = get_pot_states(ctx, ws);

        int rew = 0;

        for (int i = 0; i < ws.num_players; i++) {
            PlayerState &player = ctx.getUnsafe<PlayerState>(ctx.data().agents[i]);
            Action &action = ctx.getUnsafe<Action>(ctx.data().agents[i]);

            if (action.choice != ActionT::INTERACT) {
                continue;
            }

            int32_t pos = player.position;
            int32_t o = player.orientation;

            int32_t i_pos = move_in_direction(pos, o, ws.width);

            LocationData &dat = ctx.getUnsafe<LocationData>(ctx.data().locations[i_pos]);
            TerrainT terrain_type = dat.terrain;
            
            Object &soup = ctx.getUnsafe<LocationData>(ctx.data().locations[i_pos]).object;

            if (terrain_type == TerrainT::COUNTER) {
                if (player.has_object() && soup.name == ObjectT::NONE) {
                    soup = player.remove_object();
                    if (soup.name == ObjectT::DISH) {
                        ws.num_dishes_out++;
                    }
                } else if (!player.has_object() && soup.name != ObjectT::NONE) {
                    if (soup.name == ObjectT::DISH) {
                        ws.num_dishes_out--;
                    }
                    player.set_object(soup);
                    soup = { .name = ObjectT::NONE };
                }
            } else if (terrain_type == TerrainT::ONION_SOURCE) {
                if (player.held_object.name == ObjectT::NONE) {
                    player.held_object = { .name = ObjectT::ONION };
                }
            } else if (terrain_type == TerrainT::TOMATO_SOURCE) {
                if (player.held_object.name == ObjectT::NONE) {
                    player.held_object = { .name = ObjectT::TOMATO };
                }
            } else if (terrain_type == TerrainT::DISH_SOURCE) {
                if (player.held_object.name == ObjectT::NONE) {
                    if (ws.num_dishes_out == 0 && is_dish_pickup_useful(ctx, pot_states)) {
                        rew += ws.dish_pickup_rew;
                    }
                    player.held_object = { .name = ObjectT::DISH };
                }
            } else if (terrain_type == TerrainT::POT) {
                if (player.get_object().name == ObjectT::DISH && soup_ready_at_location(ws, soup)) {
                    player.set_object(soup);
                    soup = { .name = ObjectT::NONE };
                    rew += ws.soup_pickup_rew;
                } else if (player.get_object().name == ObjectT::ONION || player.get_object().name == ObjectT::TOMATO) {
                    if (soup.name == ObjectT::NONE) {
                        soup = { .name = ObjectT::SOUP };
                    }

                    if (!(soup.cooking_tick >= 0 || soup.num_ingredients() == MAX_NUM_INGREDIENTS)) {
                        Object obj = player.remove_object();
                        if (obj.name == ObjectT::ONION) {
                            soup.num_onions++;
                        } else {
                            soup.num_tomatoes++;
                        }
                        rew += ws.placement_in_pot_rew;
                    }

                    if (soup_to_be_cooked_at_location(ws, soup) && soup.num_ingredients() == MAX_NUM_INGREDIENTS) {
                        soup.cooking_tick = 0;
                    }
                }
                // }
            } else if (terrain_type == TerrainT::SERVING) {
                if (player.has_object()) {
                    Object obj = player.get_object();
                    if (obj.name == ObjectT::SOUP) {
                        rew += deliver_soup(ws, player, obj);
                    }
                }
            }

        }
        ws.calculated_reward.store_relaxed(rew);
    }

    // REQUIRES: original player position, orientation
    // MODIFIES: proposed position, orientation, LocationData future_player
    inline void _move_if_direction(Engine &ctx, PlayerState &ps, Action &action, AgentID &id)
    {
        if (action.choice == ActionT::INTERACT) {
            ps.propose_pos_and_or(ps.position, ps.orientation);
        } else {
            WorldState &ws = ctx.getSingleton<WorldState>();
            
            int32_t new_pos = move_in_direction(ps.position, action.choice, ws.width);

            int32_t new_orientation = (action.choice == ActionT::STAY ? ps.orientation : (int32_t) action.choice);

            TerrainT terrain_type = ctx.getUnsafe<LocationData>(ctx.data().locations[new_pos]).terrain;
            ps.propose_pos_and_or((terrain_type != TerrainT::AIR ? ps.position : new_pos), new_orientation);
        }

        ctx.getUnsafe<LocationData>(ctx.data().locations[ps.proposed_position]).future_player.store_relaxed(id.id);
    }

    // REQUIRES: proposed_position, unmodified position
    // MODIFIES: ws.should_update_pos
    inline void _check_collisions(Engine &ctx, PlayerState &ps, AgentID &id)
    {
        WorldState &ws = ctx.getSingleton<WorldState>();

        LocationData &origloc = ctx.getUnsafe<LocationData>(ctx.data().locations[ps.position]);
        LocationData &proploc = ctx.getUnsafe<LocationData>(ctx.data().locations[ps.proposed_position]);

        int comp_id = proploc.current_player;
        
        if (proploc.future_player.load_relaxed() != id.id ||
            (comp_id != -1 && comp_id != id.id && origloc.future_player.load_relaxed() == comp_id)) {
            ws.should_update_pos.store_relaxed(false);
        }
    }

    // REQUIRES: proposed_position, unmodified position
    // MODIFIES: current_player and future_player of LocationData
    inline void _unset_loc_info(Engine &ctx, PlayerState &ps, AgentID &id)
    {        
        ctx.getUnsafe<LocationData>(ctx.data().locations[ps.position]).current_player = -1;
        ctx.getUnsafe<LocationData>(ctx.data().locations[ps.proposed_position]).future_player.store_relaxed(-1);
        ctx.getUnsafe<LocationData>(ctx.data().locations[ps.position]).past_player = id.id;
        ctx.getUnsafe<LocationData>(ctx.data().locations[ps.position]).past_orientation = ps.orientation;
    }

    // REQUIRES: proposed_position, unmodified position, reset current_player of new Location
    // MODIFIES: player position and orientation
    inline void _handle_collisions(Engine &ctx, PlayerState &ps, AgentID &id)
    {
        if (ctx.getSingleton<WorldState>().should_update_pos.load_relaxed()) {
            ps.update_pos_and_or();
        } else {
            ps.update_or();
        }
        int new_pos = ps.position;
        ctx.getUnsafe<LocationData>(ctx.data().locations[new_pos]).current_player = id.id;
    }

    // REQUIRES: finished interactions
    // MODIFIES: cooking_tick of pots
    inline void step_pot_effects(Engine &ctx, PotInfo &pi)
    {
        WorldState &ws = ctx.getSingleton<WorldState>();
        int pos = pi.id;
        Object &obj = ctx.getUnsafe<LocationData>(ctx.data().locations[pos]).object;
        if (obj.name == ObjectT::SOUP && is_cooking(ws, obj)) {
            obj.cooking_tick++;
        }
    }

    // MODIFIES: should_update_pos and timestep
    inline void _reset_world_system(Engine &ctx, WorldState &ws)
    {
        ws.should_update_pos.store_relaxed(true);
        if (ctx.getSingleton<WorldReset>().resetNow) {
            ws.timestep = 0;
            ws.num_dishes_out = 0;
        }
    }

    // MODIFIES: num_interacting_players and object
    inline void _reset_objects_system(Engine &ctx, LocationData &dat)
    {
        dat.num_interacting_players.store_relaxed(0);
        if (ctx.getSingleton<WorldReset>().resetNow) {
            dat.object = { .name = ObjectT::NONE };
        }
    }

    // MODIFIES: current_player for locations
    inline void _pre_reset_actors_system(Engine &ctx, PlayerState &p)
    {
        if (ctx.getSingleton<WorldReset>().resetNow) {
            ctx.getUnsafe<LocationData>(ctx.data().locations[p.position]).current_player = -1;
        }
    }

    // REQUIRES: current_player is reset for all locations
    // MODIFIES: current_player, agent rew, all agent properties
    inline void _reset_actors_system(Engine &ctx, PlayerState &p, AgentID &id)
    {
        WorldState &ws = ctx.getSingleton<WorldState>();
        int i = id.id;
        ctx.getUnsafe<Reward>(ctx.data().agents[i]).rew = ws.calculated_reward.load_relaxed();
        if (ctx.getSingleton<WorldReset>().resetNow) {
            p.position = ws.start_player_y[i] * ws.width + ws.start_player_x[i];
            ctx.getUnsafe<LocationData>(ctx.data().locations[p.position]).current_player = i;
            p.orientation = ActionT::NORTH;
            p.proposed_position = p.position;
            p.proposed_orientation = p.orientation;
        
            p.held_object = { .name = ObjectT::NONE };
        }
    }

    // MODIFIES: timestep of world and WorldReset
    inline void check_reset_system(Engine &ctx, WorldState &ws)
    {
        ws.timestep += 1;
        ctx.getSingleton<WorldReset>().resetNow = (ws.timestep >= ws.horizon);
    }

    inline void postObservationSystem(Engine &, LocationData &dat)
    {
        dat.past_player = -1;
        dat.past_orientation = -1;
    }
    

    void Sim::setupTasks(TaskGraph::Builder &builder, const Config &)
    {
        // Handle "Interactions"
        auto pre_interact_sys = builder.addToGraph<ParallelForNode<Engine, resolve_interacts, WorldState>>({});

        // Calculate next movement
        auto move_sys = builder.addToGraph<ParallelForNode<Engine, _move_if_direction, PlayerState, Action, AgentID>>({});
        auto check_collision_sys = builder.addToGraph<ParallelForNode<Engine, _check_collisions, PlayerState, AgentID>>({move_sys});
        auto unset_loc_info = builder.addToGraph<ParallelForNode<Engine, _unset_loc_info, PlayerState, AgentID>>({check_collision_sys});

        // Modify position (need to do after all interactions are done)
        auto collision_sys = builder.addToGraph<ParallelForNode<Engine, _handle_collisions, PlayerState, AgentID>>({unset_loc_info, pre_interact_sys});

        // Step time of cooking pots (does not rely on player locations)
        auto env_step_sys = builder.addToGraph<ParallelForNode<Engine, step_pot_effects, PotInfo>>({pre_interact_sys});

        // Should terminate in next timestep? (don't need to do whole step to make judgement)
        auto terminate_sys = builder.addToGraph<ParallelForNode<Engine, check_reset_system, WorldState>>({});

        // Updates should_update_pos so must come after collision_sys
        auto reset_world_sys = builder.addToGraph<ParallelForNode<Engine, _reset_world_system, WorldState>>({terminate_sys,  collision_sys});
        // Modifies objects in the world and num_interacting_players, so should come after interactions
        auto reset_obj_sys = builder.addToGraph<ParallelForNode<Engine, _reset_objects_system, LocationData>>({terminate_sys, env_step_sys});
        // Relies on positions of players, so must come after collision_sys
        auto pre_reset_actors_sys = builder.addToGraph<ParallelForNode<Engine, _pre_reset_actors_system, PlayerState>>({terminate_sys, collision_sys});
        auto reset_actors_sys = builder.addToGraph<ParallelForNode<Engine, _reset_actors_system, PlayerState, AgentID>>({pre_reset_actors_sys});

        // Get most up-to-date observations
        auto obs_sys = builder.addToGraph<ParallelForNode<Engine, observationSystem, LocationXObservation, LocationXID>>({reset_world_sys, reset_obj_sys, reset_actors_sys});

        auto post_obs_sys = builder.addToGraph<ParallelForNode<Engine, postObservationSystem, LocationData>>({obs_sys});

        (void)post_obs_sys;
    }

    static void resetWorld(Engine &ctx)
    {
        WorldState &ws = ctx.getSingleton<WorldState>();    
        _reset_world_system(ctx, ws);
        
        for (int i = 0; i < ws.size; i++) {
            _reset_objects_system(ctx, ctx.getUnsafe<LocationData>(ctx.data().locations[i]));
        }

        for (int i = 0; i < ws.num_players; i++) {
            PlayerState &p = ctx.getUnsafe<PlayerState>(ctx.data().agents[i]);
            AgentID &id = ctx.getUnsafe<AgentID>(ctx.data().agents[i]);
            _reset_actors_system(ctx, p, id);
        }
    }


    Sim::Sim(Engine &ctx, const Config& cfg, const WorldInit &init)
        : WorldBase(ctx),
          episodeMgr(init.episodeMgr)
    {
        // Make a buffer that will last the duration of simulation for storing
        // agent entity IDs
        agents = (Entity *)rawAlloc(cfg.num_players * sizeof(Entity));
        locations = (Entity *)rawAlloc(cfg.width * cfg.height * sizeof(Entity));
        locationXplayers = (Entity *)rawAlloc(cfg.num_players * cfg.width * cfg.height * sizeof(Entity));
        
        WorldState &ws = ctx.getSingleton<WorldState>();

        for (int r = 0; r < NUM_RECIPES; r++) {
            ws.recipe_values[r] = cfg.recipe_values[r];
            ws.recipe_times[r] = cfg.recipe_times[r];
        }

        ws.height = cfg.height;
        ws.width = cfg.width;
        ws.size = cfg.height * cfg.width;

        int num_pots = 0;
    
        for (int x = 0; x < ws.size; x++) {
            if ((TerrainT) cfg.terrain[x] == TerrainT::POT) {
                num_pots++;
            }
        }
        ws.num_players = cfg.num_players;
        ws.num_pots = num_pots;

        pots = (Entity *)rawAlloc(num_pots * sizeof(Entity));
        int pot_i = 0;
        for (int x = 0; x < ws.size; x++) {

            if (cfg.terrain[x] == TerrainT::POT) {
                pots[pot_i] = ctx.makeEntityNow<PotType>();
                ctx.getUnsafe<PotInfo>(pots[pot_i]).id = x;
                pot_i++;
            }
        }
    
        for (int p = 0; p < ws.num_players; p++) {
            ws.start_player_x[p] = cfg.start_player_x[p];
            ws.start_player_y[p] = cfg.start_player_y[p];
        }

        ws.horizon = cfg.horizon;
        ws.placement_in_pot_rew = cfg.placement_in_pot_rew;
        ws.dish_pickup_rew = cfg.dish_pickup_rew;
        ws.soup_pickup_rew = cfg.soup_pickup_rew;
        ws.calculated_reward.store_release(0);
        ws.num_dishes_out = 0;

    
        // Set Everything Else
    
        for (int i = 0; i < cfg.num_players; i++) {
            agents[i] = ctx.makeEntityNow<Agent>();
            ctx.getUnsafe<Action>(agents[i]).choice = ActionT::NORTH;
        
            ctx.getUnsafe<AgentID>(agents[i]).id = i;
            for (int t = 0; t < NUM_MOVES; t++) {
                ctx.getUnsafe<ActionMask>(agents[i]).isValid[t] = true;
            }
            ctx.getUnsafe<ActiveAgent>(agents[i]).isActive = true;
            ctx.getUnsafe<Reward>(agents[i]).rew = 0.f;
        }
    
        //  Base Observation
        for (int p = 0; p < cfg.height * cfg.width; p++) {
            locations[p] = ctx.makeEntityNow<LocationType>();
            ctx.getUnsafe<LocationData>(locations[p]).terrain = cfg.terrain[p];
            ctx.getUnsafe<LocationData>(locations[p]).past_player = -1;
            ctx.getUnsafe<LocationData>(locations[p]).past_orientation = -1;
            ctx.getUnsafe<LocationData>(locations[p]).current_player = -1;
            ctx.getUnsafe<LocationData>(locations[p]).future_player.store_relaxed(-1);

            for (int i = 0; i < cfg.num_players; i++){
                int lxp_id = p + i * cfg.height * cfg.width;
                locationXplayers[lxp_id] = ctx.makeEntityNow<LocationXPlayer>();
                ctx.getUnsafe<LocationXID>(locationXplayers[lxp_id]).id = lxp_id;
                LocationXObservation& obs = ctx.getUnsafe<LocationXObservation>(locationXplayers[lxp_id]);
                
                for (int j = 0; j < 5 * cfg.num_players + 10; j++) {
                    obs.x[j] = 0;
                }

                TerrainT t = cfg.terrain[p];
                if (t) {
                    obs.x[t - 1 + 5 * cfg.num_players] = 1;
                }
            }
        }
        // Initial reset
        ctx.getSingleton<WorldReset>().resetNow = true;    
        resetWorld(ctx);
        ctx.getSingleton<WorldReset>().resetNow = false;

        for (int p = 0; p < cfg.height * cfg.width * cfg.num_players; p++) {
            LocationXObservation& obs = ctx.getUnsafe<LocationXObservation>(locationXplayers[p]);
            LocationXID& id = ctx.getUnsafe<LocationXID>(locationXplayers[p]);
            observationSystem(ctx, obs, id);
        }

    }

    MADRONA_BUILD_MWGPU_ENTRY(Engine, Sim, Config, WorldInit);

}
