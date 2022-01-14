# OpenSpades+
# Revision 7

[PLEASE READ THE ENTIRE README.md FILE BEFORE USING](https://github.com/nonperforming/openspadesplus#important)
## What is it?
OpenSpades+ is a modification of [OpenSpades](https://github.com/yvt/openspades) that has

* No FOV cap
* No falling blocks hindering visibility
* No ejected shell or bullet casings
* A crosshair more akin to classic FPS games such as CS:GO (two to pick from currently, feel free to open a PR to add more!)
* No ragdolls or corpses whatsoever
* More concise and typically informative messages (Messages will **never** give less information than vanilla)
* Hides the view-model that is distracting and takes up precious screen space
* Encouraging words on the pause menu! (I swear it's not cheating)
* More info in the stats UI (FPS, Ping, OS+ version, (IP todo))
* Allow ghosts to use the flashlight
* Change teams in Limbo menu by using the Q, W, and E keys (todo: fix)
* Less harsh and digestible flashlight
* Streamer-friendly (disable chat, hide ip, sanitize player names) ((todo))

## How to build?
[Just build like normal OpenSpades.](https://github.com/yvt/openspades/wiki/Building)
If you are on Linux and have all the dependencies installed there is a convenient [file](https://github.com/nonperforming/openspadesplus/blob/master/build.sh) that will run all the necessary commands to build OpenSpades+. Occasionally there will be bugs preventing you from launching or building the game - in that case, please download the source code from the latest Release or fix the bug yourself (please do send a PR).

## Why create this?
The standard OpenSpades client is a very awesome Ace of Spades client while looking great at the same time, but in my opinion there are certain sacrifices or features that appeal to casual players but not always competitive players. A small lag spike when blocks are pulled from the ground (typically the barriers that drop in Arena), distracting bullet casings being ejected every time you fire, a view-model that takes up a fifth of the screen, no weapon information on kills with a headshot, etc. I hope OpenSpades+ is easy to switch between and from OpenSpades as possible, but at the same time more useful to competitive play.

## Useful console variables

### cg_particles
Default: 0

As simple as it gets. Turn particles on or off along with some extra settings. This can and will hide grenade particles, which may be useful.

### cg_ragdoll
Default: 0

Setting this to 0 (model instead of dynamic ragdoll) seemingly has marginally less impact than when set to 1. There is no point in setting this to 1 since OpenSpades+ removes ragdolls and said model. ***Highly recommended not to turn this on.***

### r_vsync
Default: 0

Unlimit refresh rate and decrease input delay. ***Highly recommended not to turn this on.***

### r_fullscreen
Default: 1

Launch the game in fullscreen and get immersed.

### cl_fps
Default: 0

No FPS cap.

### cg_environmentalAudio
Default: 1

This turns on those fancy calculations for more precise and accurate sound.

### cg_stats
Default: 1

Show extra information such as accuracy (todo), FPS, ping, etc. ***Highly recommended not to turn this off.***

### cg_fov
Default: 90

FOV is very a preferential thing. For most people 90 fov is fine; though nobody's judging you, go crazy and set it to 1 or 179 (anything more or less breaks the game - though I'm not stopping you though!)

### cg_ejectBrass
Default: 0

Disables the distracting shell casings when firing your weapon. ***Highly recommended not to turn this off.***

### cg_shake
Default: 0

Disables the screen shake on grenades or shooting. It's just an annoyance most of the time.

### cg_holdAimDownSight
Default: 1

This is the default for a reason. It is self explanatory.

### cg_tracersFirstPerson
Default: 0

Remove the bright yellow tracers that are shot by you, the player. Other tracers won't be affected.

### cg_skipDeadPlayersWhenDead
Default: 0

Spectate where a player died, which can be great for callouts.

### cg_stats
Default: 1

Show FPS, Ping, OS+ version, Accuracy and IP of connected server when enabled. ***Highly recommended not to turn this off.***

### cg_alertSounds
Default: 1

Are you in a clan which plays OpenSpades regularly in a VC? Consider turning this to off to prevent pubs from distracting you and making important sounds such as footsteps or gunfire harder to hear.

### cg_animations
Default: 1

Remove many animations, such at the scope-in animation when aiming down sights. This is enabled by default because it can be disorientating when quickly scoping in and out with a weapon.

## Other
### !!IMPORTANT!!
Save files and game files are kept seperate from OpenSpades. It is highly unlikely that OpenSpades+ will bork or modify an OpenSpades install in any way. To use your OpenSpades config file as a template/your config file for OpenSpades+, you can copy your SPConfig.cfg over to OSPlus.cfg. Please notice that OpenSpades+ uses different default options which you may find helpful. You can always set them manually however, see ["Useful console variables"](https://github.com/nonperforming/openspadesplus#useful-console-variables)

### To-do:
Remove ragdolls and corpses cleaner code

Accuracy in cg_stats or other

Players left in cg_stats or other

Reload progress bar below the crosshair or in cg_stats

Shots left below crosshair and replacing icons (bottom right)

### On commit:

Bump OpenSpades+ revision level in OpenSpadesPlus.cpp and in README.md
