from gymnasium.envs.registration import register

register(
    id="exyl-exe/highway-pursuit-v0",
    entry_point="highway-pursuit-gym.envs:HighwayPursuitEnv",
)
