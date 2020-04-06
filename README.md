# UnrealCV
This is a customized version of the original [UnrealCV project](https://unrealcv.org).

## Settings
The code is tested and built with Unreal Game Engine 4.22.3 and Visual studio community 2017 in Windows 10.

* Download/Purchase UE4 Scene Model from Unreal Game Engine Market Place
    * An example model and detailed instruction from Carla simulator can be found from [CarlaUE4](https://github.com/ethliup/CarlaUE4)
* Create "Plugins" directory within the project root directory
* GoTo "Plugins/" directory, do "git clone https://github.com/ethliup/UnrealCV"
* Open \*.uproject, UnrealCV will be promopted to ask for compilation. Choose to build it.

## Set-up Game Mode
### Edit->Project Settings->Project->Maps & Modes
* Select "UE4CVGameMode" as default gamemode 
### Window->World Settings->Game Mode
* Select "UE4CVGameMode" as GameMode Override

## Set-up Axis Mappings 
### Edit->Project Settings->Engine->Input->Axis Mappings
* Add "Pitch", choose Mouse-Y
* Add "Yaw", choose Mouse-X
* Add "MoveForward", choose W (+1) & S (-1)
* Add "MoveRight", choose A (-1) & D (+1)
* Add "MoveUpward", choose Q (+1) & E (-1)

## Usages
### Start Unreal game engine by clicking the start button from the Unreal editor
* Render rolling shutter dataset 
```
cd Plugins/unrealcv/Python/
python capture_rolling_shutter.py
```
