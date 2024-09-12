import numpy as np
from envs import HighwayPursuitEnv
from wrappers import NoRewardTimeoutWrapper
import matplotlib.pyplot as plt
import os

def main():
    launcher_path = "..\\highway-pursuit-server\\highway-pursuit-launcher\\bin\\Debug\\HighwayPursuitLauncher.exe"
    dll_path = "..\\highway-pursuit-server\\highway-pursuit-server\\bin\\Debug\\HighwayPursuitServer.dll"
    app_path = "C:\\Program Files (x86)\\HighwayPursuit\\HighwayPursuit.exe"

    images = []
    max_images = 50
    image_skip = 1
    def record_step(step, img):
        if(len(images) < max_images and (step % image_skip) == 0):
            images.append(img) 

    log_dir = os.path.join(os.getcwd(), "logs") 
    options = HighwayPursuitEnv.get_default_options()
    options["real_time"] = False
    options["frameskip"] = 4
    options["log_dir"] = log_dir

    env = HighwayPursuitEnv(launcher_path, app_path, dll_path, options=options)
    # env = NoRewardTimeoutWrapper(env, timeout=(60*45) / options["frameskip"])

    episode_count = 3
    for _ in range(episode_count):
        observation, info = env.reset()
        print(info)
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

    input("Waiting for key press...")
    for image in images:
        plt.imshow(image)
        plt.show()


if __name__ == "__main__":
    main()