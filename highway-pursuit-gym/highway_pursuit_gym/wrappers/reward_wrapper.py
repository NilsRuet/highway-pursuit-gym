import gymnasium as gym

class RewardWrapper(gym.Wrapper):
    def __init__(self, env, function: lambda r: r / 500.0):
        """
        Wrapper to apply a function to the reward signal. Defaults to dividing by 500.0.

        Args:
            env (gym.Env): The environment to wrap.
        """
        super(RewardWrapper, self).__init__(env)
        self.function = function

    def step(self, action):
        observation, reward, terminated, truncated, info = self.env.step(action)
        return observation, self.function(reward), terminated, truncated, info