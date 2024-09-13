import gymnasium as gym
import numpy as np
from gymnasium.spaces import MultiBinary

class RemovePowerupsWrapper(gym.Wrapper):
    def __init__(self, env, n_powerup_actions = 3):
        super(RemovePowerupsWrapper, self).__init__(env)
        
        self.n_powerup_actions = n_powerup_actions
        assert isinstance(env.action_space, MultiBinary), "This wrapper only works with MultiBinary action spaces."

        original_n = env.action_space.n
        assert original_n > self.n_powerup_actions, "Action space must have more than {} actions.".format(self.n_powerup_actions)
        self.action_space = MultiBinary(original_n - self.n_powerup_actions)

    def step(self, action):
        extended_action = np.concatenate([action, np.zeros(self.n_powerup_actions, dtype=int)])
        obs, reward, terminated, truncated, info = self.env.step(extended_action)
        return obs, reward, terminated, truncated, info