import gymnasium as gym
import numpy as np
from gymnasium.spaces import MultiBinary, Discrete

class MultiBinaryToDiscreteWrapper(gym.Wrapper):
    def __init__(self, env):
        super(MultiBinaryToDiscreteWrapper, self).__init__(env)
        assert isinstance(env.action_space, MultiBinary), "This wrapper only works with MultiBinary action spaces."

        self.original_n = env.action_space.n
        self.action_space = Discrete(self.original_n+1) # Add 1 for NOOP

    def step(self, action):
        mbinary_action = np.zeros(self.original_n)
        if(action > 0):
            mbinary_action[action - 1] = 1
        obs, reward, terminated, truncated, info = self.env.step(mbinary_action)
        return obs, reward, terminated, truncated, info