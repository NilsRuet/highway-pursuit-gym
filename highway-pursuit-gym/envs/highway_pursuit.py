import gymnasium as gym
import numpy as np
from warnings import warn
from envs._remote.highway_pursuit_client import HighwayPursuitClient


class HighwayPursuitEnv(gym.Env):
    metadata = {"render_modes": ["rgb_array"], "render_fps": 12}

    def __init__(self, launcher_path, highway_pursuit_path, dll_path, render_mode=None):
        self.client = HighwayPursuitClient(launcher_path, highway_pursuit_path, dll_path, is_real_time = False)

        # Create process, get observation/action format
        image_shape, action_count = self.client.create_process_and_connect()

        self.observation_space = gym.spaces.Box(low=0, high=255, dtype=int, shape=image_shape)
        self.action_space = gym.spaces.MultiBinary(action_count)

        assert render_mode is None or render_mode in self.metadata["render_modes"]
        self.render_mode = render_mode

    def reset(self, seed=None, options=None):
        # Seed self.np_random
        super().reset(seed=seed)

        if seed != None:
            warn("A seed was provided, but this env does not support seeding.")

        observation, info = self.client.reset()
        return observation, info

    def step(self, action):
        observation, reward, terminated, truncated, info = self.client.step(action)
        return observation, reward, terminated, truncated, info

    def render(self):
        if self.render_mode == "rgb_array":
            return self._render_frame()

    def _render_frame(self):
        pass
        # TODO: render_mode rgb_array

    def close(self):
        self.client.close()
