# modorganizer-preview_nif
NIF preview plugin for Mod Organizer 2

For Qt6 builds of MO2 (version 2.5+)

## How to build
Follow the instructions at https://github.com/modorganizer2/mob. Then, open `CMakeLists.txt` and edit the variables at the top to the correct paths. Finally, build with cmake:

```shell
mkdir build
cmake -S . -B build
cmake --build build --config Release
cmake --install build --config Release
```
