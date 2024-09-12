from gymnasium.envs.registration import register
from highway_pursuit_gym.envs.highway_pursuit import HighwayPursuitEnv

register(
    id="exyl-exe/highway-pursuit-v0",
    entry_point="highway-pursuit-gym.envs:HighwayPursuitEnv",
)
