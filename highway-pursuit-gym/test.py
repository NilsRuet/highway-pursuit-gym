from envs import HighwayPursuitEnv

def main():
    launcher_path = "..\\highway-pursuit-server\\highway-pursuit-launcher\\bin\\Debug\\HighwayPursuitLauncher.exe"
    dll_path = "..\\highway-pursuit-server\\highway-pursuit-server\\bin\\Debug\\HighwayPursuitServer.dll"
    app_path = "C:\\Program Files (x86)\\HighwayPursuit\\HighwayPursuit.exe"

    env = HighwayPursuitEnv(launcher_path, app_path, dll_path)
    observation, info = env.reset()
    done = False
    while not done:
        action = env.action_space.sample()
        observation, reward, terminated, truncated, info = env.step(action)
        done = truncated or terminated
    env.close()


if __name__ == "__main__":
    main()