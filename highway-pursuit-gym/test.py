import numpy as np
from envs import HighwayPursuitEnv
import matplotlib.pyplot as plt

def main():
    launcher_path = "..\\highway-pursuit-server\\highway-pursuit-launcher\\bin\\Debug\\HighwayPursuitLauncher.exe"
    dll_path = "..\\highway-pursuit-server\\highway-pursuit-server\\bin\\Debug\\HighwayPursuitServer.dll"
    app_path = "C:\\Program Files (x86)\\HighwayPursuit\\HighwayPursuit.exe"

    env = HighwayPursuitEnv(launcher_path, app_path, dll_path, real_time=False)
    observation, info = env.reset()
    print(observation)
    print(np.max(observation, axis=(0,1)))
    print(observation.shape)
    print(f"tps: {info.tps}, memory: {info.memory}")
    done = False
    while not done:
        action = env.action_space.sample()
        _, reward, terminated, truncated, info = env.step(action)
        done = truncated or terminated
    env.close()

    input("Waiting for key press..")
    plt.imshow(observation)
    plt.axis('off')
    plt.show()
if __name__ == "__main__":
    main()