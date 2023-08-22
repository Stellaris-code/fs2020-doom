# fs2020-doom: Doom in a Microsoft Flight Simulator aircraft!


![20230822093249_1](https://github.com/Stellaris-code/fs2020-doom/assets/8571612/95cd9235-b3f5-400f-8cc8-c7981609e782)
![20230822093759_1](https://github.com/Stellaris-code/fs2020-doom/assets/8571612/50b9a8ca-615c-4821-af92-e66e1c713fa3)


## Instructions

Build the VS project and copy the built .wasm file to the GaugeAircraft SDK sample.

Put any (legally acquired, duh!) WAD file you'd like to run in the Packages/<package_name>/work directory. 

Controls are as follows:

* Numpad 4/8/6/2 : move
* Numpad 7 : escape
* Numpad 5 : fire
* Numpad 1 : use
* Numpad 0 : enter
* alphanumeric keys (for example, to type save file names or switch between weapons)

Controls are defined in the code and are "easily" remapped through modifiying the doom/doomgeneric_fs2020.cpp file.

All game features are working (such as save files), with the exception of sound and music.
