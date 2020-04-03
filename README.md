# DashBot 3.0
Geometry Dash bot to play &amp; finish levels - Now training much faster!

## How it works
DashBot 3.0 uses a very simplified version of a genetic algorithm, which uses random evolution to slowly create better generations for future species to evolve over. DashBot 2.0 used a full genetic algorithm (with generations and species), but DashBot 3.0 removes the species concept and defines a new generation as one that is better than the last. This means that overall training runs much faster than 2.0, but it may take a while if the bot gets stuck somewhere, especially near the end.

## Videos
[![Teaching my new bot how to play Geometry Dash](https://img.youtube.com/vi/zr5mRVt-b2s/0.jpg)](https://www.youtube.com/watch?v=zr5mRVt-b2s)  
[Twitch Highlight of moment the level was finished](https://www.twitch.tv/videos/581659084)

## Compiling
This program only works on Windows. Mac & Linux support may be available in the future.

You will need a C++ compiler to build. I suggest Visual Studio, though MinGW or other G++ ports may work. To compile with VS:
1. Open a new **x86** Native Tools command prompt. This is available in the Start Menu in the "Visual Studio \<year\>" folder.
2. `cd` to the folder with the files.
3. Compile
  * For DashBot: `cl /Fedashbot.exe dashbot.cpp HAPIH.cpp /linkuser32.lib`
  * For DashBot Player: `cl /Fedashbot_player.exe dashbot_player.cpp HAPIH.cpp /linkuser32.lib`
  * For `mkdbj` (level creator of sorts): `cl /Femkdbj.exe mkdbj.cpp`

## Usage
### DashBot
DashBot expects at least one argument. This argument specifies how DashBot should handle mode changes since ATM it can't detect which mode the player's in, but it can detect when the mode changes. DashBot has these settings built-in for the following default levels (use these exact strings):
* `StereoMadness`
* `BackOnTrack`
* `Polargeist`
* `DryOut`
* `BaseAfterBase`
* `CantLetGo`
* `Jumper`
* `TimeMachine`
* `Cycles`
* `xStep`
* `Clutterfunk`
* `TheoryOfEverything`
* `ElectromanAdventures`
* `Clubstep`
* `Electrodynamix`
* `HexagonForce`
* `BlastProcessing`
* `TheoryOfEverything2`
* `GeometricalDominator`

If you want to train DashBot to play a non-default level, you will have to specify the portal order yourself. The string must be a binary string specifying whether the bot should tap (0: cube, ball, UFO, spider) or hold (1: ship, wave, robot?) the mouse. Each digit represents a region between two of either \[the start of the map, the end of the map, or a portal\]. For example, Stereo Madness's portal string would be '0101' since its mode sequence is cube, ship, cube, ship.

There can also be a second argument specifying a file path to read/write a level save. This keeps track of all of the jumps that have been recorded, as well as some other information about the current progress. This file can be used to allow leaving and coming back to the bot. When specified, the data will be saved every time any progress is made in completion. This means that you can quit DashBot at any time without losing your progress. These saves can also be replayed later by DashBot Player.

### DashBot Player
DashBot Player is a stripped-down version of DashBot that only plays saved jump maps. It requires exactly one argument: the file path to read from. It does not require the portal list argument because that data is located inside the save file. If you want to just play back a save file without worrying about corrupting or rewriting the data, you can use DashBot Player.

### mkdbj
This tool is mainly intended to recover lost progress in the event of data loss. It does not provide any output, but it expects this input in the following order (separated by newlines):
1. Best fitness level (floating point)
2. Last jump location (floating point)
3. Level type/portal string (string)
4. List of all jumps in the save, separated by newlines (integer)
5. Type `0` to save the map and exit

## Notes
I've experienced some instability when playing back levels, especially when restarting Geometry Dash. You may need to edit the save files manually to fix this if it happens. To do this, you can put the save in a hex editor and find where DashBot is getting stuck (the values are 32-bit LE integers). Then adjust the lowest byte by +- 5 to see if that will solve the issue.
