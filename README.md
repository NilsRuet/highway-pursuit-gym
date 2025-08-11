# Highway Pursuit Gym Environment

An in-depth presentation of the project is available on my [website](https://nilsruet.github.io/projects/highway-pursuit-rl/).

This repository provides a Python RL Gymnasium environment for the game *Highway Pursuit* by Adam Dawes, currently supporting version **1.2.3** and Windows 10.

The version of the game that is compatible with the game can be downloaded [here](http://retrospec.sgn.net/files/HighwayPursuit1_2.exe). I have also included a copy of the installer in the releases of this repository.

> **Note:** The latest version (NOT compatible with this project) is available on [Adam Dawes’ website](https://adamdawes.com/games/highway-pursuit.html).

The project consists of two main components:

- **Python package:** Implements the Gym environment and handles sending commands/actions to the game.
- **Mod/DLL:** Injected into the game to enable server functionality for communication.

Both components are required and should be installed together. Installation instructions are provided in the README files located in each subfolder.
