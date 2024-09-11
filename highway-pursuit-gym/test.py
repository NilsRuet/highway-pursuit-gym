import numpy as np
from envs import HighwayPursuitEnv
import matplotlib.pyplot as plt
import time
import os

def main():
    launcher_path = "..\\highway-pursuit-server\\highway-pursuit-launcher\\bin\\Debug\\HighwayPursuitLauncher.exe"
    dll_path = "..\\highway-pursuit-server\\highway-pursuit-server\\bin\\Debug\\HighwayPursuitServer.dll"
    app_path = "C:\\Program Files (x86)\\HighwayPursuit\\HighwayPursuit.exe"

    images = []
    max_images = 50
    image_skip = 20
    def record_step(step, img):
        if(len(images) < max_images and step % image_skip == 0):
            images.append(img) 


    log_dir = os.path.join(os.getcwd(), "logs") 
    env = HighwayPursuitEnv(launcher_path, app_path, dll_path, real_time=False, log_dir=log_dir)

    episode_limit = 1000
    episode_count = 2
    for _ in range(episode_count):
        t0 = time.time()
        observation, info = env.reset()

        print(f"tps: {info.tps}, memory: {info.memory}")
        record_step(0, observation)

        done = False
        step_count = 0

        while not done and step_count < episode_limit:
            action = env.action_space.sample()
            observation, reward, terminated, truncated, info = env.step(action)
            done = truncated or terminated
            step_count += 1
            record_step(step_count, observation)


        print(f"{step_count / (time.time() - t0)} steps/s")

    env.close()

    input("Waiting for key press...")
    for image in images:
        plt.imshow(image)
        plt.show()


if __name__ == "__main__":
    main()