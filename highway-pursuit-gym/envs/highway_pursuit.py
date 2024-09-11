import gymnasium as gym
import numpy as np
from warnings import warn
from envs._remote.highway_pursuit_client import HighwayPursuitClient


class HighwayPursuitEnv(gym.Env):
    """
    A gym env for the game Highway Pursuit by Adam Dawes.
    """
    metadata = {"render_modes": ["rgb_array"], "render_fps": 12}

    def __init__(self, launcher_path, highway_pursuit_path, dll_path, render_mode=None, real_time=False, frameskip=4, log_dir=None):
        """
        Initializes the env.

        Args:
            launcher_path (str): Path to the launcher executable.
            highway_pursuit_path (str): Path to the highway pursuit executable.
            dll_path (str): Path to the server DLL file.
            is_real_time (bool, optional): If highway pursuit runs in real time. Defaults to False.
            frameskip (int, optional): the number of frames to repeat an action for. Defaults to 4.
            log_dir (str, optional): Directory for storing logs. If not provided, defaults to a 'logs' folder in the same directory as the DLL path.
            no_reward_timeout (int, optional): Duration in frames (not steps) for which not receiving rewards truncates the episode
        """
        # Create process, get observation/action format
        self.client = HighwayPursuitClient(launcher_path, highway_pursuit_path, dll_path, real_time, frameskip, log_dir)
        image_shape, action_count = self.client.create_process_and_connect()
        
        # local env options
        self._frameskip = frameskip

        # render options
        self._last_observation = None

        # gym env members
        self.observation_space = gym.spaces.Box(low=0, high=255, dtype=int, shape=image_shape)
        self.action_space = gym.spaces.MultiBinary(action_count)
        assert render_mode is None or render_mode in self.metadata["render_modes"]
        self.render_mode = render_mode

    def reset(self, seed=None, options=None):
        """
        Resets the environment, and retrieves the initial observation. Note: does not support seeding.

        Returns:
            tuple: A tuple containing:
                - observation (array-like): The initial observation after the reset.
                - info (dict): Additional environment information.
        """
        super().reset(seed=seed)

        if seed != None:
            warn("A seed was provided, but this env does not support seeding.")

        # get observation and info
        observation, info = self.client.reset()

        # update state
        self._last_observation = observation

        return observation, info

    def step(self, action):
        """
        Performs one step in the environment by sending the given action to the server, retrieves the reward and next observation.

        Args:
            action (any): The action to take in the environment.

        Returns:
            tuple: A tuple containing:
                - observation (array-like): The observation after the step.
                - reward (float): The reward received after the action.
                - terminated (bool): Whether the episode has terminated.
                - truncated (bool): Whether the episode has been truncated.
                - info (dict): Additional environment information.
        """
        
        observation, reward, terminated, truncated, info = self.client.step(action)

        # update state
        self._last_observation = observation

        return observation, reward, terminated, truncated, info

    def render(self):
        """
        Renders the current state of the environment. 
        """
        if self.render_mode == "rgb_array":
            return self._last_observation

    def close(self):
        """
        Closes the environment and free all resources.
        """
        self.client.close()
