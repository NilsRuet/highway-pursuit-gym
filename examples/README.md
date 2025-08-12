This folder contains an example use of the custom environment by an implementation of PPO by [Weight & Biases](https://github.com/vwxyzjn/ppo-implementation-details).
The dependencies can be installed with:
- `pip install -r requirements.txt`
This will additionally install the highway_pursuit_gym library locally from the repo in editable mode.

> **Note:** If you have an NVIDIA GPU, install the matching CUDA build of PyTorch from https://pytorch.org/get-started/locally/.

Running the environment also requires the launcher, DLL and game executable. Their path must be set as environment variables before running the training script.
For instance, using Powershell:
-`$env:HP_LAUNCHER_PATH = "C:\path\to\launcher.exe"`
-`$env:HP_APP_PATH = "C:\path\to\game.exe"`
-`$env:HP_DLL_PATH = "C:\path\to\inject.dll"`

The training script can be started with:
-`python ppo-highway-pursuit.py --exp-name <run_name>`

The full list of options and arguments can be accessed with:
-`python ppo-highway-pursuit.py --help`