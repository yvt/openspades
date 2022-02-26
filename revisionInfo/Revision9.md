# OpenSpades+ Revision 9
* Renamed `p_hideViewmodel` to `p_viewmodel`
* Added numerous variables to the preferences menu
* Moved `p_hurtTint` and `p_hurtBlood` to be defined in `OpenSpadesPlus.cpp`
* Un-limited player names
  * Technically there is still a limit in for 1000 characters
  * Like anybody's gonna hit that
* Fixed CMake compilation warnings
* Changed crosshair names to be more clear
* Changed default crosshair to Classic Green (my crosshair is still the best though)
* The background map in the Main Menu is now classic Rainbow Hallway
  * It currently looks awful when the camera is inside the hallway though.
    * Could any mappers step up?
* Reversed hardpatch, you can now turn on ejecting shells again with `cg_ejectBrass`
* Merged latest OpenSpades commits
  * CPU changes or something
  * DragonFly BSD
* Custom client messages are now a thing with `p_showCustomClientMessage` and `p_customClientMessage`
* Updated `vcpkg`
