import gymnasium as gym
import numpy as np

class RGBDownsampleWrapper(gym.ObservationWrapper):
    def __init__(self, env):
        super(RGBDownsampleWrapper, self).__init__(env)
        # Assuming the observation space is Box with shape (height, width, channels)
        old_shape = (
            self.observation_space.shape
        )

        height = old_shape[0]
        self.n_channels = old_shape[2] 
        new_width = int(np.ceil(old_shape[1] / self.n_channels))

        self.new_shape = (
            height,
            new_width,
            self.n_channels,
        )

        # Update the observation space with the new shape
        self.observation_space = gym.spaces.Box(
            low=self.observation_space.low.min(),
            high=self.observation_space.high.max(),
            shape=self.new_shape,
            dtype=self.observation_space.dtype,
        )

    def observation(self, obs):
        width = self.new_shape[1]
        new_obs = np.zeros((obs.shape[0], width, self.n_channels), dtype=obs.dtype)

        for channel_i in range(self.n_channels):
            # Copy matching pixels to the new_obs
            channel = obs[:, channel_i::self.n_channels, channel_i]
            current_width = channel.shape[1]
            new_obs[:, :current_width, channel_i] = channel

        return new_obs