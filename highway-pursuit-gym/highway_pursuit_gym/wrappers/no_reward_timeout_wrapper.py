import gymnasium as gym

class NoRewardTimeoutWrapper(gym.Wrapper):
    def __init__(self, env, timeout):
        """
        Wrapper to truncate episodes if no reward is received for a certain number of frames.

        Args:
            env (gym.Env): The environment to wrap.
            no_reward_timeout (int): Number of steps to wait for a reward before truncating the episode.
        """
        super(NoRewardTimeoutWrapper, self).__init__(env)
        self._no_reward_timeout = timeout
        self._step = 0
        self._last_rewarded_step = 0

    def reset(self, **kwargs):
        self._step = 0
        self._last_rewarded_step = 0
        return self.env.reset(**kwargs)

    def step(self, action):
        observation, reward, terminated, truncated, info = self.env.step(action)
        self._step += 1

        # Check if reward is non-zero, update last rewarded frame
        if reward != 0:
            self._last_rewarded_step = self._step

        # Truncate if no reward has been received for too long
        if self._step - self._last_rewarded_step > self._no_reward_timeout:
            truncated = True
            info['truncate_reason'] = 'no_reward_timeout'

        return observation, reward, terminated, truncated, info