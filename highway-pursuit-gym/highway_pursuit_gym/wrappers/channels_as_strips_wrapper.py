import gymnasium as gym
import numpy as np

class ChannelsAsStripsWrapper(gym.ObservationWrapper):
    def __init__(self, env):
        super(ChannelsAsStripsWrapper, self).__init__(env)
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
        new_obs = np.zeros(self.new_shape)
        width = self.new_shape[1]
        for channel_i in range(self.n_channels):
            # Copy the strips matching the given channel
            channel = obs[:, channel_i::self.n_channels, channel_i]

            # pad if necessary
            if(channel.shape[1] < width):
                padding_width = width - channel.shape[1]
                padding = np.zeros((channel.shape[0], padding_width))
                channel = np.concatenate((channel, padding), axis=1)

            new_obs[:,:,channel_i] = channel
        return new_obs