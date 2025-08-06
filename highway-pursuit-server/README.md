## Highway Pursuit server 
This project is the server/game side of the python gymnasium env that interacts with Highway Pursuit (v1.2.3, Windows 10/Windows 11).
It contains:
- A project that builds a to-be-injected DLL that fully synchronizes the game with commands received from the python env.
- A small launcher that injects the DLL into the given executable, waits for the game hooks to be initialized, and starts the server.

## Building
Both the launcher and the dll can be built using CMake. Make sure you are compiling in **32 bits**, otherwise the dll won't be compatible with the game.
For instance, using CMake to build a Visual Studio 2022 solution and compile the projects:

- `cmake -S . -B build-x86 -G "Visual Studio 17 2022" -A Win32` (needs to be done only once)
- `cmake --build build-x86 --config Debug` / `cmake --build build-x86 --config Release` (compile the projects)

The executable and dlls can then be found in `\build-x86\output\bin\Debug` (or `Release`).