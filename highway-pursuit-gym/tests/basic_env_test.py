from highway_pursuit_gym.envs import HighwayPursuitEnv
from highway_pursuit_gym.wrappers import (
    NoRewardTimeoutWrapper,
    )
import matplotlib.pyplot as plt
import os

def run_config(launcher_path, app_path, dll_path, options, record_step = None, episode_count=5):
    env = HighwayPursuitEnv(launcher_path, app_path, dll_path, options=options)
    env = NoRewardTimeoutWrapper(env, 600)
    try:
        for _ in range(episode_count):
            observation, info = env.reset()

            if record_step != None:
                record_step(0, observation)

            done = False
            step_count = 0

            while not done:
                action = env.action_space.sample()
                observation, reward, terminated, truncated, info = env.step(action)

                done = truncated or terminated
                step_count += 1

                if record_step != None:
                    record_step(step_count, observation)

                if(step_count > 10000):
                    break
        
            print(info)
    finally:
        env.close()

def main():
    launcher_path = os.environ.get("HP_LAUNCHER_PATH")
    app_path = os.environ.get("HP_APP_PATH")
    dll_path = os.environ.get("HP_DLL_PATH")

    if not (launcher_path and app_path and dll_path):
        print("Error: Please set HP_LAUNCHER_PATH, HP_APP_PATH, and HP_DLL_PATH environment variables.")
        return

    images = []
    max_images = 50
    image_skip = 5

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

    run_config(launcher_path, app_path, dll_path, options, record_step=record_step)

    input("Waiting for key press...")
    for image in images:
        plt.imshow(image)
        plt.show()

if __name__ == "__main__":
    main()