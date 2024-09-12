import numpy as np
from highway_pursuit_gym.envs import HighwayPursuitEnv
from highway_pursuit_gym.wrappers import NoRewardTimeoutWrapper
import matplotlib.pyplot as plt
import os
import threading
import time

def run_config(launcher_path, app_path, dll_path, options, record_step, episode_count=100):
    env = HighwayPursuitEnv(launcher_path, app_path, dll_path, options=options)
    # env = NoRewardTimeoutWrapper(env, timeout=(60*45) / options["frameskip"])

    for _ in range(episode_count):
        observation, info = env.reset()
        record_step(0, observation)

        done = False
        step_count = 0

        while not done:
            action = env.action_space.sample()
            observation, reward, terminated, truncated, info = env.step(action)
            done = truncated or terminated
            step_count += 1
            record_step(step_count, observation)
    
    print(info)
    env.close()

def main():
    launcher_path = "..\\highway-pursuit-server\\highway-pursuit-launcher\\bin\\Debug\\HighwayPursuitLauncher.exe"
    dll_path = "..\\highway-pursuit-server\\highway-pursuit-server\\bin\\Debug\\HighwayPursuitServer.dll"
    app_path = "C:\\Program Files (x86)\\HighwayPursuit\\HighwayPursuit.exe"

    images = []
    max_images = 50
    image_skip = 1

    # callback to keep some observations
    def record_step(step, img):
        if(len(images) < max_images and (step % image_skip) == 0):
            images.append(img) 

    log_dir = os.path.join(os.getcwd(), "logs") 

    options = HighwayPursuitEnv.get_default_options()
    options["real_time"] = False
    options["frameskip"] = 4
    options["log_dir"] = log_dir
    
    options["resolution"] = "320x240"
    options["enable_rendering"] = True

    run_config(launcher_path, app_path, dll_path, options, lambda *args: None)

    input("Waiting for key press...")
    for image in images:
        plt.imshow(image)
        plt.show()


if __name__ == "__main__":
    main()