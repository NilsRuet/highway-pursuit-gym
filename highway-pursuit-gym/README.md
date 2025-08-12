# Highway Pursuit gym
This package provides a custom Gymnasium-compatible interface for interacting with the game _Highway Pursuit_ by Adam Dawes. This works in tandem with the Highway Pursuit Server (from the same repo), and requires an install of Highway Pursuit v1.2.3, as well as the server's launcher and dll. Refer to the README in `highwayt-pursuit-server` to build the launcher and dll.

## Installation

Requires python 3.11 or newer.

Installing the python gym env:
- `git clone https://github.com/exyl-exe/highway-pursuit-gym`
- `cd highway_pursuit_gym\highway_pursuit_gym`
- `pip install .` / `pip install -e .` (development)

## Checking the installation
The basic environment test requires matplotlib, which can be installed with:
`pip install -r requirements-dev.txt`

Before running the environment, you need to set three environment variables to specify the required external files:
-`HP_LAUNCHER_PATH`: Path to the launcher
-`HP_DLL_PATH`: Path to the server DLL
-`HP_APP_PATH`: Path to the game executable

For instance, using Powershell:
-`$env:HP_LAUNCHER_PATH = "C:\path\to\launcher.exe"`
-`$env:HP_APP_PATH = "C:\path\to\game.exe"`
-`$env:HP_DLL_PATH = "C:\path\to\inject.dll"`
-`python tests/basic_env_test.py`

This script will:
    - Launch the game
    - Connect the Gym environment to it
    - Run a few random-action episodes
    - Render and show a few frames using matplotlib (blue and red channels will be inverted)