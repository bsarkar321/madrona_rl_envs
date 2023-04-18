from envs.overcooked_env import validate_step, init_validation, SimplifiedOvercooked, get_base_layout_params, OvercookedMadrona

from pantheonrl_extension.asyncvectorenv import AsyncVectorEnv
from pantheonrl_extension.vectorobservation import VectorObservation


import torch
import time

import argparse

from tqdm import tqdm

import numpy as np


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--num-envs", type=int, default=32,
                        help="the number of parallel game environments")
    parser.add_argument("--num-steps", type=int, default=1000,
                        help="the number of steps to run in each environment per policy rollout")
    parser.add_argument("--verbose", default=False, nargs="?", const=True,
                        help="if toggled, enable assertions to validate correctness")
    parser.add_argument("--asserts", default=False, nargs="?", const=True,
                        help="if toggled, enable assertions to validate correctness")


    parser.add_argument("--use-cpu", default=False, nargs="?", const=True,
                        help="if toggled, use cpu version of madrona")
    parser.add_argument("--use-env-cpu", default=False, nargs="?", const=True,
            help="if toggled, use cpu for env outputs")
    parser.add_argument("--use-baseline", default=False, nargs="?", const=True,
                        help="if toggled, use baseline version")

    parser.add_argument("--validation", default=False, nargs="?", const=True,
                        help="if toggled, validate correctness")
    parser.add_argument("--debug-compile", default=False, nargs="?", const=True,
                        help="if toggled, use debug compilation mode")
    parser.add_argument("--layout", type=str, default="cramped_room",
                        # choices=['cramped_room', 'coordination_ring', 'asymmetric_advantages_tomato', 'bonus_order_test', 'corridor', 'multiplayer_schelling'],
                        help="Choice for overcooked layout.")

    parser.add_argument("--num-players", type=int, default=None,
                        help="the number of players in the game")
    args = parser.parse_args()

    print(get_base_layout_params(args.layout, 400, max_num_players=args.num_players))

    if args.use_baseline:
        env = AsyncVectorEnv(
                [lambda: SimplifiedOvercooked(args.layout, num_players=args.num_players) for _ in range(args.num_envs)],
                device = torch.device('cpu') if args.use_env_cpu else None
            )
    else:
        env = OvercookedMadrona(args.layout, args.num_envs, 0, args.debug_compile, args.use_cpu, args.use_env_cpu, num_players=args.num_players)
    old_state = env.n_reset()
    actions = torch.zeros((env.n_players, args.num_envs, 1), dtype=int).to(device=env.device)
    num_errors = 0

    if args.validation:
        orig_obs_valid = init_validation(args.layout, args.num_envs, args.num_players)

        old_state_numpy = np.array([x.obs.cpu().numpy() for x in old_state])

        for i in range(len(orig_obs_valid)):
            truevalid = np.array([orig[0] for orig in orig_obs_valid[i]])
            if not np.all(np.abs(truevalid - old_state_numpy[:, i]) == 0):
                print(np.abs(truevalid - old_state_numpy[:, i]).nonzero())
                print("madrona:", old_state_numpy[:, i][np.abs(truevalid - old_state_numpy[:, i]).nonzero()])
                print("numpy:", truevalid[np.abs(truevalid - old_state_numpy[:, i]).nonzero()])
                assert(not args.asserts)

    # warp up
    for i in range(5):
        actions[:, :, :] = torch.randint_like(actions, high=env.action_space.n)
        next_state, reward, next_done, _ = env.n_step(actions)

        if args.validation and not validate_step(old_state, actions, next_done, next_state, reward, verbose=args.verbose):
            num_errors += 1
            print("ASERTION FAILED", i)
            assert(not args.asserts)

        old_state = [VectorObservation(s.active.clone(), s.obs.clone(), s.state.clone(), s.action_mask) for s in next_state]

    time_stamps = [0 for i in range(args.num_steps * 2)]
    for iter in tqdm(range(args.num_steps), desc="Running Simulation"):
        actions[:, :, :] = torch.randint_like(actions, high=env.action_space.n)
        # print(actions)

        time_stamps[iter * 2] = time.time()
        next_state, reward, next_done, _ = env.n_step(actions)
        time_stamps[iter * 2 + 1] = time.time()

        if args.validation and not validate_step(old_state, actions, next_done, next_state, reward, verbose=args.verbose):
            num_errors += 1
            assert(not args.asserts)

        old_state = [VectorObservation(s.active.clone(), s.obs.clone(), s.state.clone(), s.action_mask) for s in next_state]

    time_difference = [time_stamps[i] - time_stamps[i-1] for i in range(1, len(time_stamps), 2)]
    assert(len(time_difference) == args.num_steps)
    print("step * worlds / sec:", args.num_envs / (sum(time_difference) / args.num_steps))
    if args.validation:
        print("Error rate:", num_errors/args.num_steps)
