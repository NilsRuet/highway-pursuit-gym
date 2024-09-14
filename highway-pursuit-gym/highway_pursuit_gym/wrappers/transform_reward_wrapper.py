import gymnasium as gym

class TransformRewardWrapper(gym.RewardWrapper):
    def __init__(self, env, function = lambda r: r / 500.0):
        """
        Wrapper to apply a function to the reward signal. Defaults to dividing by 500.0.

        Args:
            env (gym.Env): The environment to wrap.
        """
        super(TransformRewardWrapper, self).__init__(env)
        self.function = function

    def reward(self, reward):
        return self.function(reward)
