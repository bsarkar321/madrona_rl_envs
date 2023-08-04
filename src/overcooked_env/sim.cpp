#include "sim.hpp"
#include <madrona/mw_gpu_entry.hpp>

#include<cmath>

using namespace madrona;
using namespace madrona::math;


namespace Overcooked {

    
  void Sim::registerTypes(ECSRegistry &registry, const Config &)
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

    registry.registerArchetype<Agent>();
    registry.registerArchetype<LocationType>();
    registry.registerArchetype<LocationXPlayer>();
    registry.registerArchetype<PotType>();

    // Export tensors for pytorch
    registry.exportSingleton<WorldReset>((uint32_t)ExportID::WorldReset);
    registry.exportColumn<Agent, ActiveAgent>((uint32_t)ExportID::ActiveAgent);
    registry.exportColumn<Agent, Action>((uint32_t)ExportID::Action);
    registry.exportColumn<Agent, ActionMask>((uint32_t)ExportID::ActionMask);
    registry.exportColumn<Agent, Reward>((uint32_t)ExportID::Reward);
    registry.exportColumn<Agent, WorldID>((uint32_t)ExportID::WorldID);
    registry.exportColumn<Agent, AgentID>((uint32_t)ExportID::AgentID);

    registry.exportColumn<LocationXPlayer, LocationXObservation>((uint32_t)ExportID::Observation);
    registry.exportColumn<LocationXPlayer, WorldID>((uint32_t)ExportID::LocationWorldID);
    registry.exportColumn<LocationXPlayer, LocationXID>((uint32_t)ExportID::LocationID);
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

  inline void observationSystem(Engine &ctx, AgentID &id)
  {
    WorldState &ws = ctx.singleton<WorldState>();

    int32_t shift = 5 * ws.num_players;
    int32_t current_player = id.id;

    for (int loc = 0; loc < ws.size; loc++) {
      LocationXObservation &obs = ctx.get<LocationXObservation>(ctx.data().locationXplayers[current_player * ws.size + loc]);
      LocationData &dat = ctx.get<LocationData>(ctx.data().locations[loc]);
      Object &obj = dat.object;
      if (ws.horizon - ws.timestep < 40) {
	obs.x[shift + 15] = 1;
      } else {
	obs.x[shift + 15] = 0;
      }

      for (int to_reset = 0; to_reset < shift; to_reset++) {
	obs.x[to_reset] = 0;
      }
        
      obs.x[shift + 6] = 0;
      obs.x[shift + 7] = 0;
      obs.x[shift + 8] = 0;
      obs.x[shift + 9] = 0;
      obs.x[shift + 10] = 0;
      obs.x[shift + 11] = 0;
      obs.x[shift + 12] = 0;
      obs.x[shift + 13] = 0;
      obs.x[shift + 14] = 0;

      if (obj.name == ObjectT::SOUP) {
	if (dat.terrain == TerrainT::POT) {
	  if (obj.cooking_tick < 0) {
	    obs.x[shift + 6] = obj.num_onions;
	    obs.x[shift + 7] = obj.num_tomatoes;
	  } else {
	    obs.x[shift + 8] = obj.num_onions;
	    obs.x[shift + 9] = obj.num_tomatoes;
	    obs.x[shift + 10] = get_time(ws, obj) - obj.cooking_tick;
	    if (is_ready(ws, obj)) {
	      obs.x[shift + 11] = 1;
	    }
	  }
	} else {
	  obs.x[shift + 8] = obj.num_onions;
	  obs.x[shift + 9] = obj.num_tomatoes;
	  obs.x[shift + 10] = 0;
	  obs.x[shift + 11] = 1;
	}
      } else if (obj.name == ObjectT::DISH) {
	obs.x[shift + 12] = 1;
      } else if (obj.name == ObjectT::ONION) {
	obs.x[shift + 13] = 1;
      } else if (obj.name == ObjectT::TOMATO) {
	obs.x[shift + 14] = 1;
      }
    }

    int other_i = 1;
    for (int i = 0; i < ws.num_players; i++) {
      PlayerState &ps = ctx.get<PlayerState>(ctx.data().agents[i]);

      int32_t pos = ps.position;

      LocationXObservation &obs = ctx.get<LocationXObservation>(ctx.data().locationXplayers[current_player * ws.size + pos]);
        
      if (i == id.id) {
	obs.x[0] = 1;
	obs.x[ws.num_players + ps.orientation] = 1;
      } else {
	obs.x[other_i] = 1;
	obs.x[ws.num_players + 4 * other_i + ps.orientation] = 1;
	other_i++;
      }

      if (ps.has_object()) {
	Object &obj = ps.get_object();
	if (obj.name == ObjectT::SOUP) {
	  obs.x[shift + 8] = obj.num_onions;
	  obs.x[shift + 9] = obj.num_tomatoes;
	  obs.x[shift + 10] = 0;
	  obs.x[shift + 11] = 1;
	} else if (obj.name == ObjectT::DISH) {
	  obs.x[shift + 12] = 1;
	} else if (obj.name == ObjectT::ONION) {
	  obs.x[shift + 13] = 1;
	} else if (obj.name == ObjectT::TOMATO) {
	  obs.x[shift + 14] = 1;
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

  inline void resolve_interacts(Engine &ctx, WorldState &ws)
  {
    int rew = 0;
    for (int i = 0; i < ws.num_players; i++) {
      PlayerState &player = ctx.get<PlayerState>(ctx.data().agents[i]);
      Action &action = ctx.get<Action>(ctx.data().agents[i]);
      
      if (action.choice != ActionT::INTERACT) {
	continue;
      }

      int32_t pos = player.position;
      int32_t o = player.orientation;

      int32_t i_pos = move_in_direction(pos, o, ws.width);
      TerrainT terrain_type = ctx.get<LocationData>(ctx.data().locations[i_pos]).terrain;

      Object &soup = ctx.get<LocationData>(ctx.data().locations[i_pos]).object;

      if (terrain_type == TerrainT::COUNTER) {
	if (player.has_object() && soup.name == ObjectT::NONE) {
	  soup = player.remove_object();
	} else if (!player.has_object() && soup.name != ObjectT::NONE) {
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
	  player.held_object = { .name = ObjectT::DISH };
	}
      } else if (terrain_type == TerrainT::POT) {
	if (!player.has_object()) {
	  if (soup_to_be_cooked_at_location(ws, soup)) {
	    soup.cooking_tick = 0;
	  }
	} else {
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
	  }
	}
      } else if (terrain_type == TerrainT::SERVING) {
	if (player.has_object()) {
	  Object obj = player.get_object();
	  if (obj.name == ObjectT::SOUP) {
	    rew += deliver_soup(ws, player, obj);
	  }
	}
      }

    }

    for (int i = 0; i < ws.num_players; i++) {
      Reward &reward = ctx.get<Reward>(ctx.data().agents[i]);
      reward.rew = rew;
    }
  }

  // REQUIRES: original player position, orientation
  // MODIFIES: proposed position, orientation, LocationData future_player
  inline void _move_if_direction(Engine &ctx, PlayerState &ps, Action &action)
  {
    if (action.choice == ActionT::INTERACT) {
      ps.propose_pos_and_or(ps.position, ps.orientation);
    } else {
      WorldState &ws = ctx.singleton<WorldState>();
            
      int32_t new_pos = move_in_direction(ps.position, action.choice, ws.width);

      int32_t new_orientation = (action.choice == ActionT::STAY ? ps.orientation : (int32_t) action.choice);

      TerrainT terrain_type = ctx.get<LocationData>(ctx.data().locations[new_pos]).terrain;
      ps.propose_pos_and_or((terrain_type != TerrainT::AIR ? ps.position : new_pos), new_orientation);
    }
  }

  // // REQUIRES: proposed_position, unmodified position
  // // MODIFIES: ws.should_update_pos
  // inline void _check_collisions(Engine &ctx, PlayerState &ps, AgentID &id)
  // {
  //   WorldState &ws = ctx.singleton<WorldState>();

  //   LocationData &origloc = ctx.get<LocationData>(ctx.data().locations[ps.position]);
  //   LocationData &proploc = ctx.get<LocationData>(ctx.data().locations[ps.proposed_position]);

  //   int comp_id = proploc.current_player;
        
  //   if (proploc.future_player.load_relaxed() != id.id ||
  // 	(comp_id != -1 && comp_id != id.id && origloc.future_player.load_relaxed() == comp_id)) {
  //     ws.should_update_pos.store_relaxed(false);
  //   }
  // }

  // // REQUIRES: proposed_position, unmodified position
  // // MODIFIES: current_player and future_player of LocationData
  // inline void _unset_loc_info(Engine &ctx, PlayerState &ps, AgentID &id)
  // {
  //   // WorldState &ws = ctx.singleton<WorldState>();
        
  //   ctx.get<LocationData>(ctx.data().locations[ps.position]).current_player = -1;
  //   ctx.get<LocationData>(ctx.data().locations[ps.proposed_position]).future_player.store_relaxed(-1);

  //   // update relevant portions of obs
  //   // LocationObservation &obs = ctx.get<LocationObservation>(ctx.data().locations[ps.position]);
  //   // obs.x[id.id] = 0;
  //   // obs.x[ws.num_players + 4 * id.id + ps.orientation] = 0;
  //   ctx.get<LocationData>(ctx.data().locations[ps.position]).past_player = id.id;
  //   ctx.get<LocationData>(ctx.data().locations[ps.position]).past_orientation = ps.orientation;
  // }

  inline void _handle_collisions(Engine &ctx, WorldState &ws)
  {
    for (int idx0 = 0; idx0 < ws.num_players; idx0++) {
      for (int idx1 = idx0+1; idx1 < ws.num_players; idx1++) {
	PlayerState &ps0 = ctx.get<PlayerState>(ctx.data().agents[idx0]);
	PlayerState &ps1 = ctx.get<PlayerState>(ctx.data().agents[idx1]);

	if (ps0.proposed_position == ps1.proposed_position ||
	    (ps0.proposed_position == ps1.position && ps1.proposed_position == ps0.position)) {
	  for (int i = 0; i < ws.num_players; i++) {
	    ctx.get<PlayerState>(ctx.data().agents[i]).update_or();
	  }
	  return;
	}
      }
    }

    for (int i = 0; i < ws.num_players; i++) {
      ctx.get<PlayerState>(ctx.data().agents[i]).update_pos_and_or();
    }
  }

  // // REQUIRES: proposed_position, unmodified position, reset current_player of new Location
  // // MODIFIES: player position and orientation
  // inline void _handle_collisions(Engine &ctx, PlayerState &ps, AgentID &id)
  // {
  //   if (ctx.singleton<WorldState>().should_update_pos.load_relaxed()) {
  //     ps.update_pos_and_or();
  //   } else {
  //     ps.update_or();
  //   }
  //   int new_pos = ps.position;
  //   ctx.get<LocationData>(ctx.data().locations[new_pos]).current_player = id.id;
  // }

  // REQUIRES: finished interactions
  // MODIFIES: cooking_tick of pots
  inline void step_pot_effects(Engine &ctx, PotInfo &pi)
  {
    WorldState &ws = ctx.singleton<WorldState>();
    int pos = pi.id;
    Object &obj = ctx.get<LocationData>(ctx.data().locations[pos]).object;
    if (obj.name == ObjectT::SOUP && is_cooking(ws, obj)) {
      obj.cooking_tick++;
    }
  }

  // // MODIFIES: should_update_pos and timestep
  // inline void _reset_world_system(Engine &ctx, WorldState &ws)
  // {
  //   ws.should_update_pos.store_relaxed(true);
  //   if (ctx.singleton<WorldReset>().resetNow) {
  //     ws.timestep = 0;
  //   }
  // }

  // // MODIFIES: num_interacting_players and object
  // inline void _reset_objects_system(Engine &ctx, LocationData &dat)
  // {
  //   dat.num_interacting_players.store_relaxed(0);
  //   if (ctx.singleton<WorldReset>().resetNow) {
  //     dat.object = { .name = ObjectT::NONE };
  //   }
  // }

  // // MODIFIES: current_player for locations
  // inline void _pre_reset_actors_system(Engine &ctx, PlayerState &p)
  // {
  //   if (ctx.singleton<WorldReset>().resetNow) {
  //     ctx.get<LocationData>(ctx.data().locations[p.position]).current_player = -1;
  //   }
  // }

  // // REQUIRES: current_player is reset for all locations
  // // MODIFIES: current_player, agent rew, all agent properties
  // inline void _reset_actors_system(Engine &ctx, PlayerState &p, AgentID &id)
  // {
  //   WorldState &ws = ctx.singleton<WorldState>();
  //   int i = id.id;
  //   if (ctx.singleton<WorldReset>().resetNow) {
  //     p.position = ws.start_player_y[i] * ws.width + ws.start_player_x[i];
  //     ctx.get<LocationData>(ctx.data().locations[p.position]).current_player = i;
  //     p.orientation = ActionT::NORTH;
  //     p.proposed_position = p.position;
  //     p.proposed_orientation = p.orientation;
        
  //     p.held_object = { .name = ObjectT::NONE };
  //   }
  // }

  static void resetWorld(Engine &ctx)
  {
    WorldState &ws = ctx.singleton<WorldState>();    
    ws.timestep = 0;
    for (int i = 0; i < ws.size; i++) {
      LocationData &loc = ctx.get<LocationData>(ctx.data().locations[i]);
      loc.object = { .name = ObjectT::NONE };
    }

    for (int i = 0; i < ws.num_players; i++) {
      PlayerState &p = ctx.get<PlayerState>(ctx.data().agents[i]);
      p.position = ws.start_player_y[i] * ws.width + ws.start_player_x[i];
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
    ctx.singleton<WorldReset>().resetNow = (ws.timestep >= ws.horizon);

    if (ctx.singleton<WorldReset>().resetNow) {
      resetWorld(ctx);
    }
  }

  // inline void postObservationSystem(Engine &, LocationData &dat)
  // {
  //   dat.past_player = -1;
  //   dat.past_orientation = -1;
  // }
    

  void Sim::setupTasks(TaskGraphBuilder &builder, const Config &)
  {
    // Handle "Interactions"
    // auto pre_interact_sys = builder.addToGraph<ParallelForNode<Engine, pre_resolve_interacts, WorldState>>({});
    // auto resolve_interact_sys = builder.addToGraph<ParallelForNode<Engine, resolve_interacts, PlayerState, AgentID, Action>>({pre_interact_sys});
    // auto interact_time_sys = builder.addToGraph<ParallelForNode<Engine, setup_interact_time, PlayerState, AgentID, Action>>({resolve_interact_sys});
    // auto counter_pot_sys0 = builder.addToGraph<ParallelForNode<Engine, do_counter_pot_int0, PlayerState>>({interact_time_sys});
    // auto counter_pot_sys1 = builder.addToGraph<ParallelForNode<Engine, do_counter_pot_int1, PlayerState>>({counter_pot_sys0});
    // auto counter_pot_sys2 = builder.addToGraph<ParallelForNode<Engine, do_counter_pot_int2, PlayerState>>({counter_pot_sys1});
    // auto counter_pot_sys3 = builder.addToGraph<ParallelForNode<Engine, do_counter_pot_int3, PlayerState>>({counter_pot_sys2});
    auto interact_sys = builder.addToGraph<ParallelForNode<Engine, resolve_interacts, WorldState>>({});

    // Calculate next movement
    auto move_sys = builder.addToGraph<ParallelForNode<Engine, _move_if_direction, PlayerState, Action>>({});
    // auto check_collision_sys = builder.addToGraph<ParallelForNode<Engine, _check_collisions, PlayerState, AgentID>>({move_sys});
    // auto unset_loc_info = builder.addToGraph<ParallelForNode<Engine, _unset_loc_info, PlayerState, AgentID>>({check_collision_sys});

    // Modify position (need to do after all interactions are done)
    auto collision_sys = builder.addToGraph<ParallelForNode<Engine, _handle_collisions, WorldState>>({move_sys});

    // Step time of cooking pots (does not rely on player locations)
    auto env_step_sys = builder.addToGraph<ParallelForNode<Engine, step_pot_effects, PotInfo>>({interact_sys, collision_sys});

    // Should terminate in next timestep? (don't need to do whole step to make judgement)
    auto terminate_sys = builder.addToGraph<ParallelForNode<Engine, check_reset_system, WorldState>>({env_step_sys});

    // // Updates should_update_pos so must come after collision_sys
    // auto reset_world_sys = builder.addToGraph<ParallelForNode<Engine, _reset_world_system, WorldState>>({terminate_sys,  collision_sys});
    // // Modifies objects in the world and num_interacting_players, so should come after interactions
    // auto reset_obj_sys = builder.addToGraph<ParallelForNode<Engine, _reset_objects_system, LocationData>>({terminate_sys, env_step_sys});
    // // Relies on positions of players, so must come after collision_sys
    // auto pre_reset_actors_sys = builder.addToGraph<ParallelForNode<Engine, _pre_reset_actors_system, PlayerState>>({terminate_sys, collision_sys});
    // auto reset_actors_sys = builder.addToGraph<ParallelForNode<Engine, _reset_actors_system, PlayerState, AgentID>>({pre_reset_actors_sys});

    // Get most up-to-date observations
    auto obs_sys = builder.addToGraph<ParallelForNode<Engine, observationSystem, AgentID>>({terminate_sys});
    
    (void)obs_sys;
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
        
    WorldState &ws = ctx.singleton<WorldState>();

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

    pots = (Entity *)rawAlloc(num_pots * sizeof(Entity));
    int pot_i = 0;
    for (int x = 0; x < ws.size; x++) {

      if (cfg.terrain[x] == TerrainT::POT) {
	pots[pot_i] = ctx.makeEntity<PotType>();
	ctx.get<PotInfo>(pots[pot_i]).id = x;
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
    
    // Set Everything Else
    
    for (int i = 0; i < cfg.num_players; i++) {
      agents[i] = ctx.makeEntity<Agent>();
      ctx.get<Action>(agents[i]).choice = ActionT::NORTH;
        
      ctx.get<AgentID>(agents[i]).id = i;
      for (int t = 0; t < NUM_MOVES; t++) {
	ctx.get<ActionMask>(agents[i]).isValid[t] = true;
      }
      ctx.get<ActiveAgent>(agents[i]).isActive = true;
      ctx.get<Reward>(agents[i]).rew = 0.f;
    }
    
    //  Base Observation
    for (int p = 0; p < cfg.height * cfg.width; p++) {
      locations[p] = ctx.makeEntity<LocationType>();
      ctx.get<LocationData>(locations[p]).terrain = cfg.terrain[p];
      ctx.get<LocationData>(locations[p]).past_player = -1;
      ctx.get<LocationData>(locations[p]).past_orientation = -1;
      ctx.get<LocationData>(locations[p]).current_player = -1;
      ctx.get<LocationData>(locations[p]).future_player.store_relaxed(-1);

      for (int i = 0; i < cfg.num_players; i++){
	int lxp_id = p + i * cfg.height * cfg.width;
	locationXplayers[lxp_id] = ctx.makeEntity<LocationXPlayer>();
	ctx.get<LocationXID>(locationXplayers[lxp_id]).id = lxp_id;
	LocationXObservation& obs = ctx.get<LocationXObservation>(locationXplayers[lxp_id]);
                
	for (int j = 0; j < 5 * cfg.num_players + 16; j++) {
	  obs.x[j] = 0;
	}

	TerrainT t = cfg.terrain[p];
	if (t) {
	  obs.x[t - 1 + 5 * cfg.num_players] = 1;
	}
      }
    }
    // Initial reset
    ctx.singleton<WorldReset>().resetNow = true;    
    resetWorld(ctx);
    ctx.singleton<WorldReset>().resetNow = false;

    // for (int p = 0; p < cfg.height * cfg.width * cfg.num_players; p++) {
    //   LocationXObservation& obs = ctx.get<LocationXObservation>(locationXplayers[p]);
    //   LocationXID& id = ctx.get<LocationXID>(locationXplayers[p]);
    //   observationSystem(ctx, obs, id);
    // }

    for (int i = 0; i < cfg.num_players; i++) {
      observationSystem(ctx, ctx.get<AgentID>(agents[i]));
    }

  }

  MADRONA_BUILD_MWGPU_ENTRY(Engine, Sim, Config, WorldInit);

}
