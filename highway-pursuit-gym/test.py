import numpy as np
from envs import HighwayPursuitEnv
import matplotlib.pyplot as plt
import time

def main():
    launcher_path = "..\\highway-pursuit-server\\highway-pursuit-launcher\\bin\\Debug\\HighwayPursuitLauncher.exe"
    dll_path = "..\\highway-pursuit-server\\highway-pursuit-server\\bin\\Debug\\HighwayPursuitServer.dll"
    app_path = "C:\\Program Files (x86)\\HighwayPursuit\\HighwayPursuit.exe"

    images = []
    env = HighwayPursuitEnv(launcher_path, app_path, dll_path, real_time=False)

    episode_limit = 2000
    episode_count = 1
    for i in range(episode_count):
        t0 = time.time()
        observation, info = env.reset()
        print(f"tps: {info.tps}, memory: {info.memory}")

        images.append(observation)

        done = False
        step_count = 0

        while not done and step_count < episode_limit:
            action = env.action_space.sample()
            observation, reward, terminated, truncated, info = env.step(action)
            done = truncated or terminated

            images.append(observation)
            step_count += 1

        print(f"{step_count / (time.time() - t0)} steps/s")

    env.close()
    input("Waiting for key press...")

    for image in images:
        plt.imshow(image)
        plt.show()


if __name__ == "__main__":
    main()