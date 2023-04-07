import gym
import numpy as np

from pantheonrl_extension.multiagentenv import MultiAgentEnv
from pantheonrl_extension.vectorenv import MadronaEnv
from pantheonrl_extension.vectorobservation import VectorObservation

from overcooked_ai_py.utils import read_layout_dict

from overcooked_ai_py.mdp.actions import Action
from overcooked_ai_py.mdp.overcooked_mdp import OvercookedGridworld
from overcooked_ai_py.mdp.overcooked_env import OvercookedEnv

import build.madrona_python as madrona_python
import build.madrona_overcooked_example_python as overcooked_python


class OvercookedMadrona(MadronaEnv):

    def __init__(self, layout_name, num_envs, gpu_id, debug_compile=True, use_cpu=False, ego_agent_idx=0, horizon=400):
        self.layout_name = layout_name
        self.base_layout_params = base_layout_params(layout_name, horizon)
        self.width = self.base_layout_params['width']
        self.height = self.base_layout_params['height']
        self.num_players = self.base_layout_params['num_players']
        self.size = self.width * self.height

        self.horizon = horizon

        sim = overcooked_python.OvercookedSimulator(
            exec_mode = overcooked_python.ExecMode.CPU if use_cpu else overcooked_python.ExecMode.CUDA,
            gpu_id = gpu_id,
            num_worlds = num_envs,
            debug_compile = debug_compile,
            **self.base_layout_params
        )
        full_obs_size = self.width * self.height * (5 * self.num_players + 16)
        super().__init__(num_envs, gpu_id, sim, obs_size=full_obs_size, state_size=full_obs_size)

        self.ego_ind = ego_agent_idx

        self.observation_space = self._setup_observation_space()
        self.share_observation_space = self.observation_space
        self.action_space = gym.spaces.Discrete(Action.NUM_ACTIONS)

        self.n_reset()

    def _setup_observation_space(self):
        obs_shape = np.array([self.width, self.height, 5 * self.num_players + 16])
        return gym.spaces.MultiBinary(obs_shape)

    def n_step(self, actions):
        obs, rews, dones, infos = super().n_step(actions)

        for ob in obs:
            ob.obs = ob.obs.reshape((self.num_envs, self.height, self.width, -1)).transpose(1, 2)
            ob.state = ob.obs
        # modify obs and state
        return obs, rews, dones, infos

    def n_reset(self):
        obs = super().n_reset()

        for ob in obs:
            ob.obs = ob.obs.reshape((self.num_envs, self.height, self.width, -1)).transpose(1, 2)
            ob.state = ob.obs
        # modify obs and state
        return obs


from .overcooked_reimplement import DummyMDP

MAX_NUM_INGREDIENTS = 3

BASE_REW_SHAPING_PARAMS = {
    "PLACEMENT_IN_POT_REW": 3,
    "DISH_PICKUP_REWARD": 3,
    "SOUP_PICKUP_REWARD": 5,
}

TOMATO = "tomato"
ONION = "onion"

AIR = ' '
POT = 'P'
COUNTER = 'X'
ONION_SOURCE = 'O'
TOMATO_SOURCE = 'T'
DISH_SOURCE = 'D'
SERVING = 'S'
TERRAIN_TYPES = [AIR, POT, COUNTER, ONION_SOURCE, TOMATO_SOURCE, DISH_SOURCE, SERVING]


def get_true_orders(orders):
    ans = [0] * ((MAX_NUM_INGREDIENTS + 1) ** 2)
    for order in orders:
        num_onions = len([ingredient for ingredient in order['ingredients'] if ingredient == ONION])
        num_tomatoes = len([ingredient for ingredient in order['ingredients'] if ingredient == TOMATO])
        # ans.append((num_onions, num_tomatoes))
        ans[((MAX_NUM_INGREDIENTS + 1) * num_onions + num_tomatoes)] = 1
        # ans.append(num_onions)
        # ans.append(num_tomatoes)
    return ans


def base_layout_params(layout_name, horizon):
    base_layout_params = read_layout_dict(layout_name)
    grid = base_layout_params['grid']
    del base_layout_params['grid']
    grid = [layout_row.strip() for layout_row in grid.split('\n')]
    layout_grid = [[c for c in row] for row in grid]

    player_positions = [None] * 9
    for y, row in enumerate(layout_grid):
        for x, c in enumerate(row):
            if c in ["1", "2", "3", "4", "5", "6", "7", "8", "9"]:
                layout_grid[y][x] = " "
                player_positions[int(c) - 1] = (x, y)
    num_players = len([x for x in player_positions if x is not None])
    player_positions = player_positions[:num_players]

    base_layout_params['height'] = len(layout_grid)
    base_layout_params['width'] = len(layout_grid[0])
    base_layout_params['terrain'] = [TERRAIN_TYPES.index(x) for row in layout_grid for x in row]
    base_layout_params['num_players'] = len(player_positions)
    # base_layout_params['start_player_positions'] = player_positions
    base_layout_params['start_player_x'] = [p[0] for p in player_positions]
    base_layout_params['start_player_y'] = [p[1] for p in player_positions]

    if 'rew_shaping_params' not in base_layout_params or base_layout_params['rew_shaping_params'] is None:
        base_layout_params['rew_shaping_params'] = BASE_REW_SHAPING_PARAMS

    base_layout_params['placement_in_pot_rew'] = base_layout_params['rew_shaping_params']['PLACEMENT_IN_POT_REW']
    base_layout_params['dish_pickup_rew'] = base_layout_params['rew_shaping_params']['DISH_PICKUP_REWARD']
    base_layout_params['soup_pickup_rew'] = base_layout_params['rew_shaping_params']['SOUP_PICKUP_REWARD']

    del base_layout_params['rew_shaping_params']

    if 'start_all_orders' not in base_layout_params or base_layout_params['start_all_orders'] is None:
        base_layout_params['start_all_orders'] = []

    if 'start_bonus_orders' not in base_layout_params or base_layout_params['start_bonus_orders'] is None:
        base_layout_params['start_bonus_orders'] = []

    original_start_all_orders = base_layout_params['start_all_orders']

    base_layout_params['start_all_orders'] = get_true_orders(original_start_all_orders)
    base_layout_params['start_bonus_orders'] = get_true_orders(base_layout_params['start_bonus_orders'])

    if 'order_bonus' not in base_layout_params:
        base_layout_params['order_bonus'] = 2

    times = [20] * ((MAX_NUM_INGREDIENTS + 1) ** 2)

    if 'onion_time' in base_layout_params and 'tomato_time' in base_layout_params:
        times = []
        for o in range(MAX_NUM_INGREDIENTS + 1):
            for t in range(MAX_NUM_INGREDIENTS + 1):
                times.append(o * base_layout_params['onion_time'] + t * base_layout_params['tomato_time'])
        del base_layout_params['onion_time']
        del base_layout_params['tomato_time']

    if 'recipe_times' in base_layout_params:
        for order, time in zip(original_start_all_orders, base_layout_params['recipe_times']):
            num_onions = len([ingredient for ingredient in order['ingredients'] if ingredient == ONION])
            num_tomatoes = len([ingredient for ingredient in order['ingredients'] if ingredient == TOMATO])
            times[((MAX_NUM_INGREDIENTS + 1) * num_onions + num_tomatoes)] = time

    if 'cook_time' in base_layout_params:
        times = [base_layout_params['cook_time']] * ((MAX_NUM_INGREDIENTS + 1) ** 2)
        del base_layout_params['cook_time']

    base_layout_params['recipe_times'] = times

    values = [20] * ((MAX_NUM_INGREDIENTS + 1) ** 2)

    if 'onion_value' in base_layout_params and 'tomato_value' in base_layout_params:
        values = []
        for o in range(MAX_NUM_INGREDIENTS + 1):
            for t in range(MAX_NUM_INGREDIENTS + 1):
                values.append(o * base_layout_params['onion_value'] + t * base_layout_params['tomato_value'])
        del base_layout_params['onion_value']
        del base_layout_params['tomato_value']

    if 'recipe_values' in base_layout_params:
        for order, value in zip(original_start_all_orders, base_layout_params['recipe_values']):
            num_onions = len([ingredient for ingredient in order['ingredients'] if ingredient == ONION])
            num_tomatoes = len([ingredient for ingredient in order['ingredients'] if ingredient == TOMATO])
            values[((MAX_NUM_INGREDIENTS + 1) * num_onions + num_tomatoes)] = value

    if 'delivery_reward' in base_layout_params:
        values = [base_layout_params['delivery_reward']] * ((MAX_NUM_INGREDIENTS + 1) ** 2)
        del base_layout_params['delivery_reward']

    for i in range(len(values)):
        if base_layout_params['start_bonus_orders'][i]:
            values[i] *= base_layout_params['order_bonus']

        if not base_layout_params['start_all_orders'][i]:
            values[i] = 0

    del base_layout_params['order_bonus']

    base_layout_params['recipe_values'] = values

    base_layout_params['horizon'] = horizon

    del base_layout_params['start_all_orders']
    del base_layout_params['start_bonus_orders']

    return base_layout_params


class SimplifiedOvercooked(MultiAgentEnv):
    def __init__(self, layout_name, ego_agent_idx=0, horizon=400):
        self.layout_name = layout_name
        self.base_layout_params = base_layout_params(layout_name, horizon)
        self.width = self.base_layout_params['width']
        self.height = self.base_layout_params['height']
        self.size = self.width * self.height

        self.mdp = DummyMDP(**self.base_layout_params)
        self.horizon = horizon

        super().__init__(ego_ind=ego_agent_idx, n_players=2)

        self.featurize_fn = lambda x: self.mdp.lossless_state_encoding(x)

        self.observation_space = self._setup_observation_space()
        self.share_observation_space = self.observation_space
        self.action_space = gym.spaces.Discrete(Action.NUM_ACTIONS)

        self.n_reset()

    def _setup_observation_space(self):
        obs_shape = np.array([self.mdp.width, self.mdp.height, 5 * self.mdp.num_players + 16])
        return gym.spaces.MultiBinary(obs_shape)

    def get_mask(self):
        return np.array([1] * Action.NUM_ACTIONS, dtype=bool)

    def get_obs(self, state):
        ob0, ob1 = self.featurize_fn(state)
        ob0 = ob0[:self.size, :].reshape((self.height, self.width, -1)).transpose((1, 0, 2))
        ob1 = ob1[:self.size, :].reshape((self.height, self.width, -1)).transpose((1, 0, 2))
        return (ob0, ob0, self.get_mask()), (ob1, ob1, self.get_mask())

    def n_step(self, actions):
        actions = [act.item() for act in actions]

        next_state, mdp_infos = self.mdp.get_state_transition(self.state, actions)

        self.state = next_state

        done = self.state.timestep >= self.horizon

        reward = sum(mdp_infos)
        info = {}

        return (0, 1), self.get_obs(next_state), (reward, reward), done, info

    def n_reset(self):
        self.state = self.mdp.get_standard_start_state()
        return (0, 1), self.get_obs(self.state)


class PantheonOvercooked(MultiAgentEnv):

    def __init__(self, layout_name, ego_agent_idx=0, horizon=400):
        self.layout_name = layout_name

        self.mdp = OvercookedGridworld.from_layout_name(self.layout_name)
        self.base_env = OvercookedEnv.from_mdp(self.mdp, horizon=horizon, info_level=0)
        super().__init__(ego_ind=ego_agent_idx, n_players=2)

        self.featurize_fn = lambda x: self.mdp.lossless_state_encoding(
            x, horizon=horizon)

        self.observation_space = self._setup_observation_space()
        self.share_observation_space = self.observation_space
        self.action_space = gym.spaces.Discrete(Action.NUM_ACTIONS)

        self.n_reset()

    def _setup_observation_space(self):
        obs_shape = self.mdp.get_lossless_state_encoding_shape()
        return gym.spaces.MultiBinary(obs_shape)

    def get_mask(self):
        return np.array([1] * Action.NUM_ACTIONS, dtype=bool)

    def get_obs(self, state):
        ob0, ob1 = self.featurize_fn(state)
        return (ob0, ob0, self.get_mask()), (ob1, ob1, self.get_mask())

    def n_step(self, actions):
        a0 = Action.INDEX_TO_ACTION[actions[0]]
        a1 = Action.INDEX_TO_ACTION[actions[1]]

        next_state, reward, done, info = self.base_env.step((a0, a1))
        reward = reward + sum(info['shaped_r_by_agent'])

        return (0, 1), self.get_obs(next_state), (reward, reward), done, info

    def n_reset(self):
        self.base_env.reset()
        return (0, 1), self.get_obs(self.base_env.state)


def init_validation(layout_name, num_envs):
    global VALIDATION_ENVS
    VALIDATION_ENVS = [
        PantheonOvercooked(layout_name) for _ in range(num_envs)
    ]
    return [x.n_reset()[1] for x in VALIDATION_ENVS]


def validate_step(states, actions, dones, nextstates, rewards, verbose=True):
    global VALIDATION_ENVS

    states = np.array([x.obs.cpu().numpy() for x in states])
    nextstates = np.array([x.obs.cpu().numpy() for x in nextstates])

    retval = True
    for i in range(len(VALIDATION_ENVS)):
        # assume current state is fine: fix todo?

        _, truenext, truerewards, truedone, _ = VALIDATION_ENVS[i].n_step(actions[:, i])
        truenext = np.array([truenext[0][0], truenext[1][0]])
        # print(truenext.shape)
        truerewards = np.array([truerewards[0], truerewards[1]], dtype=np.float32)
        if not np.all(truerewards == rewards[:, i].cpu().numpy()):
            if verbose:
                # print("start state:", states[:, i], i)
                print("action:", actions[:, i])
                # print("madrona transition:", nextstates[:, i])
                # print("numpy transition:", truenext)
                print(f"Rewards mismatch: numpy={truerewards}, madrona={rewards[:, i]}")
            retval = False

        if truedone != dones[i]:
            if verbose:
                # print("start state:", states[:, i], i)
                print("action:", actions[:, i])
                # print("madrona transition:", nextstates[:, i])
                # print("numpy transition:", truenext)
                print(f"DONES mismatch: numpy={truedone}, madrona={dones[i] == 1}")
            retval = False
            # return False
            # pass
        if dones[i]:
            _, truenext = VALIDATION_ENVS[i].n_reset()
            truenext = np.array([truenext[0][0], truenext[1][0]])

        if not np.all(np.abs(truenext - nextstates[:,i]) == 0):
            if verbose:
                # print("start state:", states[:, i], i)
                print("action:", actions[:, i])
                print(np.abs(truenext - nextstates[:, i]).nonzero())
                print("madrona:", nextstates[:, i][np.abs(truenext - nextstates[:, i]).nonzero()])
                print("numpy:", truenext[np.abs(truenext - nextstates[:, i]).nonzero()])
                # print("madrona transition:", nextstates[:, i])
                # print("numpy transition:", truenext)
                print("TRANSITIONS are not equal")
            retval = False

    return retval
