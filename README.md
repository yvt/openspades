# OpenSpades+
# Revision 4

## What is it?
OpenSpades+ is a modification of [OpenSpades](https://github.com/yvt/openspades) that has

* No FOV cap
* No falling blocks hindering visibility
* No ejected shell or bullet casings
* A crosshair more akin to classic FPS games such as CS:GO
* No ragdolls or corpses whatsoever
* More concise and informative kill and death messages in both as the center message log and kill log
* Hides viewmodel
* Encouraging words on the pause menu! (I swear it's not cheating)
* More info in cg_stats (FPS and ping counter)
* Allow spectators and dead people to use the flashlight
* Change teams in Limbo menu by using the Q, W, and E keys

## How to build?
[Just build like normal OpenSpades.](https://github.com/yvt/openspades/wiki/Building)
If you are on Linux and have all the dependencies installed there is a convenient [file](https://github.com/nonperforming/openspadesplus/blob/master/build.sh) that will run all the necessary commands to build OpenSpades+.

## Useful console variables

### cg_particles
Recommended: 0

As simple as it gets. Turn particles on or off along with some extra settings. This can and will hide grenade particles, which may be useful.

### r_vsync
Recommended: 0

Unlimit refresh rate and decrease input delay.

### cl_fps
Recommended: 0

No FPS cap.

### cg_environmentalAudio
Recommended: 1

This turns on those fancy calculations for more precise and accurate sound.

### cg_stats
Recommended: 1

Show extra information such as accuracy (todo), FPS, ping, etc

### cg_fov
Recommended: N/A

FOV is very a preferential thing. Nobody's judging you, go crazy and set it to 1 or 179 (anything more or less breaks the game - though I'm not stopping you though!)

## Other
### To-do:
Remove ragdolls and corpses cleaner code

Bump OpenSpades revision level in Client_Draw.cpp (911) and in README.md

Accuracy in cg_stats or other

Players left in cg_stats or other

Reload progress bar below the crosshair or in cg_stats

Shots left below crosshair and replacing icons (bottom right)
