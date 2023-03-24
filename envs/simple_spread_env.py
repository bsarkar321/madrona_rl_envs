from gym.spaces import MultiDiscrete, Box
from gym.vector.vector_env import VectorEnv

import build.madrona_python as madrona_python
import build.madrona_simple_spread_example_python as simple_spread_python
import torch

import numpy as np

NUM_AGENTS = 3
NUM_LANDMARKS = NUM_AGENTS
DIM_P = 2
DIM_V = DIM_P
DIM_C = 2
OBS_SIZE = (DIM_P + DIM_V + (NUM_LANDMARKS * DIM_P) + (NUM_AGENTS - 1) * (DIM_P + DIM_C))


def to_torch(a):
    # print(a.shape)
    return a[:, 0].detach().clone()


class SimpleSpreadMadronaTorch(VectorEnv):

    def __init__(self, num_envs, gpu_id, debug_compile=True, use_cpu=False):

        self.device = torch.device('cuda', gpu_id) if not use_cpu and torch.cuda.is_available() else torch.device('cpu')

        self.observation_space = Box(low=-np.infty, high=np.infty, shape=(OBS_SIZE,))
        self.action_space = MultiDiscrete([5, DIM_C])
        # print(self.action_space)
        super().__init__(num_envs, self.observation_space, self.action_space)
        # print(self.action_space)

        self.sim = simple_spread_python.SimpleSpreadSimulator(
            exec_mode = simple_spread_python.ExecMode.CPU if use_cpu else simple_spread_python.ExecMode.CUDA,
            gpu_id = gpu_id,
            num_worlds = num_envs,
            debug_compile = debug_compile,
        )

        self.static_dones = self.sim.reset_tensor().to_torch()
        self.static_actions = self.sim.action_tensor().to_torch()
        self.static_observations = self.sim.observation_tensor().to_torch()
        self.static_rewards = self.sim.reward_tensor().to_torch()

        self.static_worldID = self.sim.world_id_tensor().to_torch().to(torch.long)
        print(self.static_worldID)

        self.static_gathers = self.static_dones.detach().clone()
        # print(self.static_gathers)

        self.infos = [{}] * self.num_envs

        self.share_observation_space = self.observation_space

    def step(self, actions):
        # print(actions.shape, actions)
        self.static_actions.copy_(actions)

        self.sim.step()

        # print(self.static_worldID)
        torch.gather(self.static_dones, 0, self.static_worldID, out=self.static_gathers)

        return to_torch(self.static_observations), to_torch(self.static_rewards), to_torch(self.static_gathers), self.infos

    def reset(self):
        return to_torch(self.static_observations)

    def close(self, **kwargs):
        pass