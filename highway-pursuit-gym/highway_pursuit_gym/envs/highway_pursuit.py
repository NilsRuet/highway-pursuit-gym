import gymnasium as gym
import numpy as np
import os
from dataclasses import dataclass
from warnings import warn
from highway_pursuit_gym.envs._remote.highway_pursuit_client import HighwayPursuitClient

class HighwayPursuitEnv(gym.Env):
    """
    A gym env for the game Highway Pursuit by Adam Dawes.
    """
    metadata = {"render_modes": ["rgb_array"], "render_fps": 12}

    def get_default_options():
        """
        Gets the default options for an highway pursuit env.
        """
        return {
            "real_time": False,
            "frameskip": 4,
            "resolution": "640x480",
            "enable_rendering": False,
            "server_restart_frequency": int(5e6),
            "max_memory_usage": 300.0,
        }

    def _add_options_no_override(defaults, new_params):
        return {**defaults, **new_params}

    def __init__(self, launcher_path, highway_pursuit_path, dll_path, render_mode=None, options: dict = None):
        """
        Initializes the env.

        Args:
            launcher_path (str): Path to the launcher executable.
            highway_pursuit_path (str): Path to the highway pursuit executable.
            dll_path (str): Path to the server DLL file.
            options (dict): dict with env options
                - is_real_time (bool): If highway pursuit runs in real time. Defaults to False.
                - frameskip (int): the number of frames to repeat an action for. Defaults to 4.
                - server_restart_frequency (int): the number of steps before the game is restarted.
                - max_memory_usage (float): the maximum memory the server process can use before being restarted.
                - log_dir (str): Directory for storing server logs. If not provided, defaults to a 'logs' folder in the same directory as the DLL path.
        """        
        # Store calling parameters
        self._launcher_path = launcher_path
        self._highway_pursuit_path = highway_pursuit_path
        self._dll_path = dll_path

        # Get default options and override with the provided ones
        self._options = options
        if(options != None):
            HighwayPursuitEnv._add_options_no_override(self._options, self._get_full_default_options())

        # Create process, get observation/action format
        self._client = HighwayPursuitClient(launcher_path, highway_pursuit_path, dll_path, self._options)
        image_shape, action_count = self._client.create_process_and_connect()

        # Process monitoring
        self._server_total_ellapsed_steps = 0
        self._last_info = None

        # render options
        self._last_observation = None

        # gym env members
        self.observation_space = gym.spaces.Box(low=0, high=255, dtype=np.uint8, shape=image_shape)
        self.action_space = gym.spaces.MultiBinary(action_count)
        assert render_mode is None or render_mode in self.metadata["render_modes"]
        self.render_mode = render_mode

    def _get_full_default_options(self):
        return {
            **HighwayPursuitEnv.get_default_options(),
            "log_dir": os.path.join(os.path.abspath(os.path.dirname(self._dll_path)), 'logs')
        }

    def _restart_server(self):
        """
        Restarts the Highway Pursuit app. This is mainly a workaround for memory leaks.
        """
        self._client.close()

        self._client = HighwayPursuitClient(self._launcher_path, self._highway_pursuit_path, self._dll_path, self._options)
        self._client.create_process_and_connect()

        self._server_total_ellapsed_steps = 0

    def _should_restart_server(self):
        """
        Checks if the server should be restarted e.g. high memory usage or enough steps ellapsed.
        """
        time_ellapsed = self._server_total_ellapsed_steps > self._options["server_restart_frequency"]
        
        if(self._last_info != None):
            memory_usage_too_high = self._last_info["memory_usage"] > self._options["max_memory_usage"]
        else:
            memory_usage_too_high = False

        return time_ellapsed or memory_usage_too_high

    def reset(self, seed=None, options: dict = {"new_game": False}):
        """
        Resets the environment, and retrieves the initial observation. Note: does not support seeding.

        Args:
            options (dict): Reset options with keys:
                - new_game (bool): wether to start a new episode by starting a new_game or by respawning the player

        Returns:
            tuple: A tuple containing:
                - observation (array-like): The initial observation after the reset.
                - info (dict): Additional environment information.
        """
        super().reset(seed=seed)

        if seed != None:
            warn("A seed was provided, but this env does not support seeding.")

        # restart the server if necessary
        has_restarted = False
        if(self._should_restart_server()):
            self._restart_server()
            has_restarted = True

        new_game = False
        if options != None:
            new_game = options.get("new_game", False)

        # get observation and info
        observation, info = self._client.reset(new_game)

        # add restart status to the info
        if(has_restarted):
            info["server_restarted"] = True

        # update state
        self._last_observation = observation
        self._last_info = info

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
        
        observation, reward, terminated, truncated, info = self._client.step(action)

        # update state
        self._last_observation = observation
        self._last_info = info

        # Update total ellapsed steps
        self._server_total_ellapsed_steps += 1
        return observation, reward, terminated, truncated, info

    def render(self):
        """
        Renders the current state of the environment. 
        """
        if self.render_mode == "rgb_array":
            return self._last_observation[..., ::-1] # BGR to RGB

    def close(self):
        """
        Closes the environment and free all resources.
        """
        self._client.close()
