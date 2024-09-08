from gymnasium.envs.registration import register

register(
    id="exyl-exe/highway-pursuit-v0",
    entry_point="gym_examples.envs:HighwayPursuitEnv",
)
