# unrealcv

We use Unreal Game Engine 4.22.3

## Build
* Download/Purchase UE4 Scene Model from Unreal Game Engine Market Place
* Create "Plugins" directory within the project root directory
* GoTo "Plugins/" directory, do "git clone https://gitlab.inf.ethz.ch/liup/unrealcv.git"
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
### Sample control points for spline trajectory interpolation
* Navigate camera to desired location
* Type vget /pose [path2LogFile] from unreal console terminal
* Repeat above steps for more points, sampled pose will be appended to "path2LogFile"
### Compute inpterpolated spline trajectory based on above sampled control knots
* Compile apps/splineInterpolation from autovision_vio https://github.com/autovision/autovision_vio
* Follow apps/splineInterpolation/data/controlKnots_realisticRendering.txt format to config new sampled control knots log file
* Run "splineInterpolation.exe [path2controlKnotsFile]" to generate spline trajectory
### Render synthetic dataset based on above interpolated trajectory
* Open Plugins/unrealcv/python/generator.py
* Config "datasetFolder", "trajectoryFile", "translationScaleFactor", "trajectorySamplingRate", "exposureTime", "imageSamplingRate"
* Click "Play" from UE4 editor
* Run "python generator.py"
* Dataset will then be created and saved to specified destination
