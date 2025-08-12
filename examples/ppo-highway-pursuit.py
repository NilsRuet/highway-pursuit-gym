# Modified from the PPO implementation by Weight & Biases
# https://www.youtube.com/watch?v=MEt6rrxH8W4
# https://github.com/vwxyzjn/ppo-implementation-details 

import argparse
from distutils.util import strtobool
import os
import random
import time
import gymnasium as gym
import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
from torch.distributions.bernoulli import Bernoulli
from torch.distributions import Categorical
from torch.utils.tensorboard import SummaryWriter
from highway_pursuit_gym import HighwayPursuitEnv
from highway_pursuit_gym.wrappers import (
    NoRewardTimeoutWrapper,
    RemovePowerupsWrapper,
    LivesPerGameWrapper,
    TransformRewardWrapper,
    RGBDownsampleWrapper
    )

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--exp-name",
        type=str,
        default=os.path.basename(__file__).rstrip(".py"),
        help="Name of the experiment",
    )
    parser.add_argument(
        "--learning-rate",
        type=float,
        default=1e-4,
        help="Learning rate of the optimizer",
    )
    parser.add_argument(
        "--seed", type=int, default=1, help="RNG seed of the experiment"
    )
    parser.add_argument(
        "--total-timesteps",
        type=int,
        default=10000000,
        help="Total timesteps of the experiments",
    )
    parser.add_argument(
        "--torch-deterministic",
        type=lambda x: bool(strtobool(x)),
        default=True,
        nargs="?",
        const=True,
        help="If toggled, torch.backends.cudnn.deterministic=False",
    )
    parser.add_argument(
        "--cuda",
        type=lambda x: bool(strtobool(x)),
        default=True,
        nargs="?",
        const=True,
        help="If toggled, cuda will not be enabled by default",
    )
    parser.add_argument(
        "--capture-video",
        type=lambda x: bool(strtobool(x)),
        default=True,
        nargs="?",
        const=True,
        help="If toggled, records some of the agent performances in ./pixel-videos",
    )
    parser.add_argument(
        "--track",
        type=lambda x: bool(strtobool(x)),
        default=True,
        nargs="?",
        const=True,
        help="If toggled, tracks training for tensorboard",
    )
    parser.add_argument(
        "--num-envs",
        type=int,
        default=4,
        help="The number of parallel game environments",
    )
    parser.add_argument(
        "--batch-size",
        type=int,
        default=1024,
        help="batch size",
    )
    parser.add_argument(
        "--anneal-lr",
        type=lambda x: bool(strtobool(x)),
        default=True,
        nargs="?",
        const=True,
        help="Toggles learning rate annealing for policy and value networks",
    )
    parser.add_argument(
        "--gae",
        type=lambda x: bool(strtobool(x)),
        default=True,
        nargs="?",
        const=True,
        help="Use general advantage estimation for advantage computation",
    )
    parser.add_argument(
        "--gae-lambda",
        type=float,
        default=0.95,
        help="The lambda for the general advantage estimation",
    )
    parser.add_argument(
        "--gamma",
        type=float,
        default=0.99,
        help="The discount factor gamma",
    )
    parser.add_argument(
        "--num-minibatches",
        type=int,
        default=4,
        help="The number of mini-batches",
    )
    parser.add_argument(
        "--update-epochs",
        type=int,
        default=4,
        help="The K epochs to update the policy",
    )
    parser.add_argument(
        "--norm-adv",
        type=lambda x: bool(strtobool(x)),
        default=True,
        nargs="?",
        const=True,
        help="Toggles advantage normalization",
    )
    parser.add_argument(
        "--clip-coef",
        type=float,
        default=0.2,
        help="The surrogate clipping coefficient",
    )
    parser.add_argument(
        "--clip-vloss",
        type=lambda x: bool(strtobool(x)),
        default=False,
        nargs="?",
        const=False,
        help="Toggles wether or not to use a clipped loss for the value function",
    )
    parser.add_argument(
        "--ent-coef",
        type=float,
        default=0.1,
        help="coefficient of the entropy",
    )
    parser.add_argument(
        "--vf-coef",
        type=float,
        default=0.5,
        help="coefficient of the value function",
    )
    parser.add_argument(
        "--max-grad-norm",
        type=float,
        default=0.5,
        help="The maximum norm for the gradient clipping",
    )
    parser.add_argument(
        "--num-frame-stack",
        type=int,
        default=4,
        help="The number of stacked observations",
    )
    parser.add_argument(
        "--num-frame-skip",
        type=int,
        default=3,
        help="The number of skipped frame observations",
    )
    parser.add_argument(
        "--artifact-folder",
        type=str,
        default="./artifacts",
        help="Folder to save artifacts (videos, checkpoints, etc.) in",
    )
    parser.add_argument(
        "--run-folder-name",
        type=str,
        default=None,
        help="Name of the subfolder in the artifacts folder. Defaults to a generated run name.",
    )
    parser.add_argument(
        "--video-folder-name",
        type=str,
        default="videos",
        help="name of the folder videos are saved in",
    )
    parser.add_argument(
        "--checkpoint-folder-name",
        type=str,
        default="checkpoints",
        help="name of the folder checkpoints are saved in",
    )
    parser.add_argument(
        "--checkpoint-count",
        type=int,
        default=10,
        help="The number of checkpoints during training"
    )

    args = parser.parse_args()
    if(args.batch_size % args.num_envs != 0):
        print("/!\ Warning: batch size may be incompatible with selected number of envs")
    args.num_steps = int(args.batch_size // args.num_envs)
    args.minibatch_size = int(args.batch_size // args.num_minibatches)
    return args

# Utility class to track various quantities during training
class Tracker:
    def __init__(self, exp_name):
        self.writer = SummaryWriter(f"runs/{exp_name}")

    def report_text(self, name, text):
        self.writer.add_text(name, text)

    def report_scalar(self, name, value, step):
        self.writer.add_scalar(name, value, step)

# Scales raw pixels from [0,255] to [0,1]
class NormalizePixelsWrapper(gym.ObservationWrapper):
    def __init__(self, env):
        super(NormalizePixelsWrapper, self).__init__(env)

        # Assuming the observation space is a Box space
        assert isinstance(self.observation_space, gym.spaces.Box)
        low, high = self.observation_space.low, self.observation_space.high
        assert (low == 0).all() and (
            high == 255
        ).all(), "This wrapper assumes pixel values in [0, 255]"

        # Update the observation space to reflect normalized values
        self.observation_space = gym.spaces.Box(
            low=0.0, high=1.0, shape=self.observation_space.shape, dtype=np.float32
        )

    def observation(self, obs):
        return obs.astype(np.float32) / 255.0

# Reshapes observations to fit the channels first convention
class ChannelsFirstWrapper(gym.ObservationWrapper):
    def __init__(self, env):
        super(ChannelsFirstWrapper, self).__init__(env)
        old_shape = self.observation_space.shape  # (frames, height, width, channels)
        self.new_shape = (old_shape[0] * old_shape[3], old_shape[1], old_shape[2])

        # Update the observation space with the new shape
        self.observation_space = gym.spaces.Box(
            low=self.observation_space.low.min(),
            high=self.observation_space.high.max(),
            shape=self.new_shape,
            dtype=self.observation_space.dtype,
        )

    def observation(self, obs):
        # Obs shape: (frames, height, width, channels)
        # Permute to (frames, channels, height, width)
        obs = np.transpose(obs, (0, 3, 1, 2))
        # Reshape to (frames * channels, height, width)
        obs = obs.reshape(self.new_shape)
        return obs
    
# Measures time spent running the step method of an environment
class StepTimerWrapper(gym.Wrapper):
    def __init__(self, env):
        super().__init__(env)
    
    def step(self, action):
        start_time = time.time()
        obs, reward, terminated, truncated, info = self.env.step(action)
        step_time = time.time() - start_time
        info["step_time"] = step_time
        return obs, reward, terminated, truncated, info

class Agent(nn.Module):
    def _layer_init(layer, std=np.sqrt(2), bias_const=0.0):
        torch.nn.init.orthogonal_(layer.weight, std)
        torch.nn.init.constant_(layer.bias, bias_const)
        return layer

    def __init__(self, envs, num_input_channels, is_multibinary):
        super(Agent, self).__init__()
        self.envs = envs
        self.num_input_channels = num_input_channels
        self.is_multibinary = is_multibinary
        # Networks
        dense_width = 512
        action_count = self.envs.single_action_space.n
        obs_shape = self.envs.single_observation_space.shape

        self.hidden = self._init_hidden(dense_width, obs_shape)
        self.actor = Agent._layer_init(nn.Linear(dense_width, action_count), std=0.01)
        self.critic = Agent._layer_init(nn.Linear(dense_width, 1), std=1)
        
    def _init_hidden(self, dense_width, input_shape):
        conv_layers = nn.Sequential(
            Agent._layer_init(
                nn.Conv2d(
                    in_channels=self.num_input_channels,
                    out_channels=32,
                    kernel_size=8,
                    stride=4,
                )
            ),
            nn.ReLU(),
            Agent._layer_init(
                nn.Conv2d(in_channels=32, out_channels=64, kernel_size=4, stride=2)
            ),
            nn.ReLU(),
            Agent._layer_init(
                nn.Conv2d(in_channels=64, out_channels=64, kernel_size=3, stride=1)
            ),
            nn.ReLU(),
        )

        with torch.no_grad():
            n_features = Agent._get_conv_output_shape(conv_layers, input_shape)

        hidden = nn.Sequential(
            conv_layers,  # Add the convolutional layers
            nn.Flatten(),  # Flatten the output of the conv layers
            Agent._layer_init(nn.Linear(n_features, dense_width)),
            nn.ReLU(),
        )
        return hidden

    def _get_conv_output_shape(conv_layers, input_shape):
        dummy_input = torch.zeros(1, *input_shape)
        # Pass the dummy input through the model to get the output
        output = conv_layers(dummy_input)
        # Flatten the output and get the number of features
        return output.reshape(output.size(0), -1).size(1)

    def get_value(self, x):
        return self.critic(self.hidden(x))

    def get_action_and_value(self, x, action=None):
        hidden = self.hidden(x)
        logits = self.actor(hidden)

        if(self.is_multibinary):
            probs = Bernoulli(logits=logits)
            if action is None:
                action = probs.sample()
            return action, torch.sum(probs.log_prob(action), axis=-1), probs.entropy(), self.critic(hidden)
        
        else: #Discrete
            probs = Categorical(logits=logits)
            if action is None:
                action = probs.sample()
            return action, probs.log_prob(action), probs.entropy(), self.critic(hidden)


def make_env(frame_stack, frame_skip, seed, idx, capture_video, video_folder, total_envs):
    def make_env_thunk():
        launcher_path = os.environ.get("HP_LAUNCHER_PATH")
        app_path = os.environ.get("HP_APP_PATH")
        dll_path = os.environ.get("HP_DLL_PATH")

        log_dir = os.path.join(os.getcwd(), "logs") 

        # Env options
        options = HighwayPursuitEnv.get_default_options()
        options["real_time"] = False
        options["frameskip"] = frame_skip
        options["log_dir"] = log_dir
        options["resolution"] = "320x240"
        
        # Enable rendering for one env only
        if idx == total_envs - 1:
            options["enable_rendering"] = True
        else:
            options["enable_rendering"] = False

        # Env wrappers
        env = HighwayPursuitEnv(launcher_path, app_path, dll_path, render_mode='rgb_array', options=options)
        env = StepTimerWrapper(env) # Measure time without other wrappers/preprocessing

        # Game-related wrappers
        env = LivesPerGameWrapper(env, lives = 1) # new game every death
        env = NoRewardTimeoutWrapper(env, timeout=int(60*45 / frame_skip)) # 45s of gameplay
        env = RemovePowerupsWrapper(env)

        # Episode statistics and recording
        env = gym.wrappers.RecordEpisodeStatistics(env)
        if capture_video:
            if idx == 0:
                env = gym.wrappers.RecordVideo(
                    env,
                    video_folder,
                    episode_trigger=lambda ep_id: ep_id % 50 == 0,
                )

        # Preprocessing observations
        env = RGBDownsampleWrapper(env)
        env = NormalizePixelsWrapper(env)
        env = gym.wrappers.FrameStack(env, num_stack=frame_stack)
        env = ChannelsFirstWrapper(env)
        
        # Pseudo reward-normalization
        env = TransformRewardWrapper(env, lambda reward: (reward / 500.0))

        # Seeding
        env.action_space.seed(seed)
        env.observation_space.seed(seed)
        return env
    
    return make_env_thunk

def check_env_var():
    launcher_path = os.environ.get("HP_LAUNCHER_PATH")
    app_path = os.environ.get("HP_APP_PATH")
    dll_path = os.environ.get("HP_DLL_PATH")

    if not (launcher_path and app_path and dll_path):
        print("Error: Please set HP_LAUNCHER_PATH, HP_APP_PATH, and HP_DLL_PATH environment variables.")
        exit()
    
    if(not os.path.isfile(launcher_path)):
        print("HP_LAUNCHER_PATH env var error: launcher not found")
        exit()

    if(not os.path.isfile(dll_path)):
        print("HP_DLL_PATH env var error: DLL not found")
        exit()

    if(not os.path.isfile(app_path)):
        print("HP_APP_PATH env var error: game executable not found")
        exit()

def main():
    args = parse_args()
    check_env_var()
    print(args)
    run_name = f"{args.exp_name}_{args.seed}_{int(time.time())}"
    run_folder_name = args.run_folder_name if args.run_folder_name != None else run_name

    num_updates = args.total_timesteps // args.batch_size

    run_folder = os.path.join(args.artifact_folder, run_folder_name)
    video_folder = os.path.join(run_folder, args.video_folder_name)
    checkpoint_folder = os.path.join(run_folder, args.checkpoint_folder_name)

    # Checkpoints
    if args.checkpoint_count > 0:
        os.makedirs(checkpoint_folder, exist_ok=True)
        checkpoint_path_builder = lambda i: os.path.join(checkpoint_folder, "{}_{}.pt".format(run_name, str(i)))
        checkpoint_frequency = num_updates / args.checkpoint_count
        next_checkpoint = 1

    # Seeding
    random.seed(args.seed)
    np.random.seed(args.seed)
    torch.manual_seed(args.seed)
    torch.cuda.manual_seed(args.seed)
    torch.backends.cudnn.deterministic = args.torch_deterministic

    device = torch.device("cuda" if torch.cuda.is_available() and args.cuda else "cpu")

    # Environment setup
    seed = 1

    envs = gym.vector.SyncVectorEnv(
        [
        make_env(
            args.num_frame_stack,
            args.num_frame_skip,
            seed+i,
            i,
            args.capture_video,
            video_folder,
            args.num_envs
            )
            for i in range(args.num_envs)
        ]
    )

    assert isinstance(envs.single_action_space, gym.spaces.MultiBinary) or isinstance(envs.single_action_space, gym.spaces.Discrete), "Only MultiBinary and Discrete action spaces are supported"
    is_multibinary = isinstance(envs.single_action_space, gym.spaces.MultiBinary)

    # Compute input size
    num_input_channels = envs.single_observation_space.shape[
        0
    ]  # By convention the channels are always first for the CNNs

    agent = Agent(envs, num_input_channels, is_multibinary).to(device)
    optimizer = optim.Adam(agent.parameters(), lr=args.learning_rate, eps=1e-5)

    # Init tracker
    if(args.track):
        tracker = Tracker(run_name)

    # Report config
    if(args.track):
        agent_summary = str(agent)
        opitmizer_summary = str(optimizer)
        tracker.report_text("Agent architecture", agent_summary)
        tracker.report_text("Optimizer", opitmizer_summary)

    print(
        "Total trainable parameters: ",
        sum(p.numel() for p in agent.parameters() if p.requires_grad),
    )


    # TRAINING LOOP
    # Storage setup
    obs = torch.zeros(
        (args.num_steps, args.num_envs) + envs.single_observation_space.shape
    ).to(device)
    actions = torch.zeros(
        (args.num_steps, args.num_envs) + envs.single_action_space.shape
    ).to(device)
    logprobs = torch.zeros((args.num_steps, args.num_envs)).to(device)
    rewards = torch.zeros((args.num_steps, args.num_envs)).to(device)
    dones = torch.zeros((args.num_steps, args.num_envs)).to(device)
    values = torch.zeros((args.num_steps, args.num_envs)).to(device)

    # Start the game
    global_step = 0
    start_time = time.time()

    # Client side profiling
    total_rollout_time = 0
    total_rollout_forward_pass_time = 0
    total_rollout_transfer_time = 0
    total_rollout_step_time = 0
    total_raw_env_step_time = 0
    total_model_training_time = 0

    # Server side profiling
    total_server_time = 0
    total_game_time = 0

    # Prepare the environment
    reset_obs, _ = envs.reset(seed=seed)
    next_obs = torch.Tensor(reset_obs).to(device)
    next_done = torch.zeros(args.num_envs).to(device)

    # Main training loop
    for update in range(1, num_updates + 1):
        print(f"Update {update} / {num_updates}\r")

        # Update learning rate
        if args.anneal_lr:
            frac = 1.0 - (update - 1.0) / num_updates
            lrnow = frac * args.learning_rate
            optimizer.param_groups[0]["lr"] = lrnow

        ### POLICY ROLLOUT
        rollout_t0 = time.time()
        for step in range(0, args.num_steps):
            global_step += 1 * args.num_envs
            obs[step] = next_obs
            dones[step] = next_done

            # Action logic
            forward_pass_t0 = time.time()

            with torch.no_grad():
                action, logprob, _, value = agent.get_action_and_value(next_obs)
                values[step] = value.flatten()
            actions[step] = action
            logprobs[step] = logprob

            total_rollout_forward_pass_time += time.time() - forward_pass_t0 

            # Execute the rollout policy and store data
            rollout_step_t0 = time.time()
            next_obs, reward, truncated, terminated, info = envs.step(
                action.cpu().numpy()
            )
            total_rollout_step_time += time.time() - rollout_step_t0

            rollout_transfer_t0 = time.time()
            rewards[step] = torch.tensor(reward).to(device).view(-1)
            next_obs = torch.Tensor(next_obs).to(device)
            next_done = torch.Tensor(np.logical_or(terminated, truncated)).to(device)
            total_rollout_transfer_time += time.time() - rollout_transfer_t0

            # Update server-side metrics
            if "step_time" in info:
                total_raw_env_step_time += np.sum(info["step_time"])

            if "server_time" in info:
                total_server_time = np.sum(info["server_time"])

            if "game_time" in info:
                total_game_time = np.sum(info["game_time"])

            # Track performance
            if(args.track):
                if "memory_usage" in info:
                    for item in info["memory_usage"]:
                        if(not item is None):
                            tracker.report_scalar(
                                "performance/memory_usage", item, global_step
                            )
                            break

                if "tps" in info:
                    for item in info["tps"]:
                        if(not item is None):
                            tracker.report_scalar(
                                "performance/server_tps", item, global_step
                            )
                            break


            # Track return and episodic length
            if "final_info" in info:
                for item in info["final_info"]:
                    if item is None:
                        continue

                    if "episode" in item:
                        print(
                            f'global_step={global_step}, episodic_return={item["episode"]["r"]}, {int(global_step / (time.time() - start_time))} steps/s'
                        )
                        
                        if(args.track):
                            tracker.report_scalar(
                                "charts/episodic_return", item["episode"]["r"], global_step
                            )
                            tracker.report_scalar(
                                "charts/episodic_length", item["episode"]["l"], global_step
                            )
                        break

        total_rollout_time += time.time() - rollout_t0

        ### POLICY AND VALUE NETWORK UPDATES
        training_t0 = time.time()
        # ADVANTAGE COMPUTATION
        with torch.no_grad():
            next_value = agent.get_value(next_obs).reshape(1, -1)
            if args.gae:
                advantages = torch.zeros_like(rewards).to(device)
                lastgaelam = 0
                for t in reversed(range(args.num_steps)):
                    if t == args.num_steps - 1:
                        nextnonterminal = 1.0 - next_done
                        nextvalues = next_value
                    else:
                        nextnonterminal = 1.0 - dones[t + 1]
                        nextvalues = values[t + 1]
                    delta = (
                        rewards[t]
                        + args.gamma * nextvalues * nextnonterminal
                        - values[t]
                    )
                    advantages[t] = lastgaelam = (
                        delta
                        + args.gamma * args.gae_lambda * nextnonterminal * lastgaelam
                    )
                returns = advantages + values
            else:
                returns = torch.zeros_like(rewards).to(device)
                for t in reversed(range(args.num_steps)):
                    if t == args.num_steps - 1:
                        nextnonterminal = 1.0 - next_done
                        next_return = next_value
                    else:
                        nextnonterminal = 1.0 - dones[t + 1]
                        next_return = returns[t + 1]
                    returns[t] = rewards[t] + args.gamma * nextnonterminal * next_return
                advantages = returns - values

        # BATCH STORAGE
        b_obs = obs.reshape((-1,) + envs.single_observation_space.shape)
        b_logprobs = logprobs.reshape(-1)
        b_actions = actions.reshape((-1,) + envs.single_action_space.shape)
        b_advantages = advantages.reshape(-1)
        b_returns = returns.reshape(-1)
        b_values = values.reshape(-1)

        ### POLICY UPDATE : MINIBATCH UPDATES
        b_inds = np.arange(args.batch_size)
        clipfracs = []
        for epoch in range(args.update_epochs):
            np.random.shuffle(b_inds)
            for start in range(0, args.batch_size, args.minibatch_size):
                end = start + args.minibatch_size
                mb_inds = b_inds[start:end].tolist()

                _, newlogprob, entropy, newvalue = agent.get_action_and_value(
                    b_obs[mb_inds], b_actions[mb_inds]
                )

                logratio = newlogprob - b_logprobs[mb_inds]
                ratio = logratio.exp()
                with torch.no_grad():
                    # calculate approx_kl http://joschu.net/blog/kl-approx.html
                    old_approx_kl = (-logratio).mean()
                    approx_kl = ((ratio - 1) - logratio).mean()
                    clipfracs += [
                        ((ratio - 1.0).abs() > args.clip_coef).float().mean().item()
                    ]

                # Advantage normalization
                mb_advantages = b_advantages[mb_inds]
                if args.norm_adv:
                    mb_advantages = (mb_advantages - mb_advantages.mean()) / (
                        mb_advantages.std() + 1e-8
                    )

                # Policy loss (objective clipping)
                pg_loss1 = -mb_advantages * ratio
                pg_loss2 = -mb_advantages * torch.clamp(
                    ratio, 1 - args.clip_coef, 1 + args.clip_coef
                )

                pg_loss = torch.max(pg_loss1, pg_loss2).mean()

                # Value loss clipping
                newvalue = newvalue.view(-1)
                if args.clip_vloss:
                    v_loss_unclipped = ((newvalue - b_returns[mb_inds]) ** 2)
                    v_clipped = b_values[mb_inds] + torch.clamp(
                        newvalue - b_values[mb_inds], -args.clip_coef, args.clip_coef
                    )
                    v_loss_clipped = (v_clipped - b_returns[mb_inds]) ** 2
                    v_loss_max = torch.max(v_loss_unclipped, v_loss_clipped)
                    v_loss = 0.5 * v_loss_max.mean()
                else:
                    v_loss = 0.5 * ((newvalue - b_returns[mb_inds]) ** 2).mean()

                # Full loss
                entropy_loss = entropy.mean()
                loss = pg_loss - args.ent_coef * entropy_loss + v_loss * args.vf_coef
                # Backpropagation
                optimizer.zero_grad()
                loss.backward()
                nn.utils.clip_grad_norm_(agent.parameters(), args.max_grad_norm)
                optimizer.step()

        total_model_training_time += time.time() - training_t0

        # Explained variance
        y_pred, y_true = b_values.cpu().numpy(), b_returns.cpu().numpy()
        var_y = np.var(y_true)
        explained_var = np.nan if var_y == 0 else 1 - np.var(y_true - y_pred) / var_y

        # Display total training time
        total_time = total_rollout_time + total_model_training_time
        h, remaining = divmod(total_time, 3600)
        m, s = divmod(remaining, 60)
        duration_text = f"{int(h)}h {int(m)}m {s:.0f}s"
        print(f"### Total training time: {duration_text} / {100*total_rollout_time / total_time :.1f}% collecting / {100*total_model_training_time / total_time:.1f}% updating")
        
        ### PROFILING
        profiling_total_time = total_time

        # total rollout/training
        profiling_env_collect = total_rollout_time
        profiling_train_loop = profiling_net_train = total_model_training_time 
        
        # forward pass
        profiling_policy_infer = total_rollout_forward_pass_time
        # cpu->gpu transfer for forward passes
        profiling_rollout_transfer = total_rollout_transfer_time
        
        # total time spent in the game loop
        profiling_game_loop = total_game_time

        # server-side processing overhead (total server time excluding the game loop)
        profiling_server_proc = total_server_time - total_game_time
        
        # preprocessing (actual time spent in the "step" method, minus the time spent in the unwrapped "step" method)
        profiling_obs_preproc = total_rollout_step_time - total_raw_env_step_time

        # client-side overhead and interprocess communication (total unprocessed step time minus server-side operations)
        profiling_ipc =  total_raw_env_step_time - total_server_time

        # On-screen feedback
        print(f"     ## EnvCollect: {100 * profiling_env_collect / total_time :.1f}%")
        print(f"         # GameLoop   : {100 * profiling_game_loop / total_time :.1f}%")
        print(f"         # ServerProc : {100 * profiling_server_proc / total_time :.1f}%")
        print(f"         # IPC        : {100 * profiling_ipc / total_time :.1f}%")
        print(f"         # ObsPreproc : {100 * profiling_obs_preproc / total_time :.1f}%")
        print(f"         # RolloutXfer: {100 * profiling_rollout_transfer / total_time :.1f}%")
        print(f"         # PolicyInfer: {100 * profiling_policy_infer / total_time :.1f}%")
        print(f"     ## TrainLoop: {100 * profiling_train_loop / total_time :.1f}%")
        print(f"         # NetTrain   : {100*profiling_net_train / total_time:.1f}%")

        # Useful metrics for evaluating if the agent is learning
        if(args.track):
            tracker.report_scalar(
                "charts/learning_rate", optimizer.param_groups[0]["lr"], global_step
            )
            tracker.report_scalar("losses/value_loss", v_loss.item(), global_step)
            tracker.report_scalar("losses/policy_loss", pg_loss.item(), global_step)
            tracker.report_scalar("diagnostics/entropy", entropy_loss.item(), global_step)
            tracker.report_scalar("diagnostics/old_approx_kl", old_approx_kl.item(), global_step)
            tracker.report_scalar("diagnostics/approx_kl", approx_kl.item(), global_step)
            tracker.report_scalar("diagnostics/clipfrac", np.mean(clipfracs), global_step)
            tracker.report_scalar("diagnostics/explained_variance", explained_var, global_step)
            tracker.report_scalar(
                "performance/SPS", int(global_step / (time.time() - start_time)), global_step
            )

            # profiling
            tracker.report_scalar("profiling/env_collect", profiling_env_collect / total_time, global_step)
            tracker.report_scalar("profiling/train_loop", profiling_train_loop / total_time, global_step)
            tracker.report_scalar("profiling/env_collect/game_loop", profiling_game_loop / total_time, global_step)
            tracker.report_scalar("profiling/env_collect/server_proc", profiling_server_proc / total_time, global_step)
            tracker.report_scalar("profiling/env_collect/ipc", profiling_ipc / total_time, global_step)
            tracker.report_scalar("profiling/env_collect/obs_preproc", profiling_obs_preproc / total_time, global_step)
            tracker.report_scalar("profiling/env_collect/rollout_transfer", profiling_rollout_transfer / total_time, global_step)
            tracker.report_scalar("profiling/env_collect/policy_infer", profiling_policy_infer / total_time, global_step)
            tracker.report_scalar("profiling/train_loop/net_train", profiling_net_train / total_time, global_step)

        # Save checkpoint if needed
        if(args.track and (args.checkpoint_count > 0) and (update >= next_checkpoint * checkpoint_frequency)):
            path = checkpoint_path_builder(next_checkpoint)
            print(f"Saving checkpoint {next_checkpoint}/{args.checkpoint_count} ... at {path}")
            torch.save({
                'exp_name': args.exp_name,
                'step': global_step,
                'total_steps': args.total_timesteps,
                'model_state_dict': agent.state_dict(),
                'optimizer_state_dict': optimizer.state_dict(),
                'loss': pg_loss.item(),
            }, path)

            next_checkpoint += 1

    # Close the envs
    envs.close()

if __name__ == "__main__":
    main()
