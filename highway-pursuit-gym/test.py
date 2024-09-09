import numpy as np
from envs import HighwayPursuitEnv
import matplotlib.pyplot as plt

def main():
    launcher_path = "..\\highway-pursuit-server\\highway-pursuit-launcher\\bin\\Debug\\HighwayPursuitLauncher.exe"
    dll_path = "..\\highway-pursuit-server\\highway-pursuit-server\\bin\\Debug\\HighwayPursuitServer.dll"
    app_path = "C:\\Program Files (x86)\\HighwayPursuit\\HighwayPursuit.exe"

    images = []

    env = HighwayPursuitEnv(launcher_path, app_path, dll_path, real_time=False)
    observation, info = env.reset()
    print(observation.shape)
    print(f"tps: {info.tps}, memory: {info.memory}")

    images.append(observation)

    done = False
    step_count = 0
    while not done and step_count < 1000:
        action = env.action_space.sample()
        observation, reward, terminated, truncated, info = env.step(action)
        done = truncated or terminated
        images.append(observation)
        step_count+=1
    env.close()

    input("Waiting for key press..")
    for image in images[::100]:
        plt.imshow(image)
        plt.axis('off')
        plt.show()

if __name__ == "__main__":
    main()