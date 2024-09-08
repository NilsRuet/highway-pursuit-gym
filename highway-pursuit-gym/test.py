from envs import HighwayPursuitEnv

def main():
    env = HighwayPursuitEnv()
    observation, info = env.reset()
    done = False
    while not done:
        action = env.action_space.sample()
        observation, reward, terminated, truncated, info = env.step(action)
        done = truncated or terminated
    env.close()


if __name__ == "__main__":
    main()