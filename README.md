# DashBot 3.0
Geometry Dash bot to play &amp; finish levels - Now training much faster!

## How it works
DashBot 3.0 uses a very simplified version of a genetic algorithm, which uses random evolution to slowly create better generations for future species to evolve over. DashBot 2.0 used a full genetic algorithm (with generations and species), but DashBot 3.0 removes the species concept and defines a new generation as one that is better than the last. This means that overall training runs much faster than 2.0, but it may take a while if the bot gets stuck somewhere, especially near the end.

Basically, the bot first follows a list of jumps that have been saved and are confirmed to work. (More about this later.) After it does all of the jumps, it starts to randomly click. If these new clicks help it go further through the level then it saves the new clicks, and those are now used for the jump list.

Note that the screen is not used as an input at all - the bot plays blind. The only inputs it uses are the current X position, the mode of the player, and whether the level was finished.

If the bot can't get further after trying random jumps for 10 attempts, it will delete the last jump in the list. To allow the possibility of multiple jump reversions, it will only save the new jumps if it passes the point it got stuck at. If it still can't get un-stuck, it continues reverting until it can make progress. This is what makes this bot able to learn and be self-correcting.

For cube, ball, UFO, and spider sections, the stored clicks tell the program to click down & up. For ship, wave, and robot sections, the stored clicks tell the program to toggle the mouse, allowing the bot to hold the mouse button down. This is why the mode string argument is required for the bot to run properly.

## Videos
[![Teaching my new bot how to play Geometry Dash](https://img.youtube.com/vi/zr5mRVt-b2s/0.jpg)](https://www.youtube.com/watch?v=zr5mRVt-b2s)  
[Twitch Highlight of moment Stereo Madness was finished](https://www.twitch.tv/videos/581659084)
[YouTube playlist of the bot's progress](https://www.youtube.com/playlist?list=PLkpRr6F2XV5408ZFj1Hm-Phf2G9MymztX)

## Anti-virus note
DashBot uses functions to read the memory of the Geometry Dash process. Some viruses take advantage of these same functions, so many anti-virus programs will flag programs that use those as malware. If your browser refuses to download the executables, you can try either downloading with a different browser (e.g. IE), or compile the code yourself.

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
To run DashBot, you need to open it from a console window. First, `cd` to the directory where `dashbot.exe` is located. Make sure  that Geometry Dash is running before starting DashBot. Then run `dashbot <modestr> [save.dbj]`. Finally, start the level in GD and make sure the mouse is on top of the window. You can pause the bot anytime just by pausing Geometry Dash. The bot will not run unless Geometry Dash is playing a level unpaused.

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

If you want to train DashBot to play a non-default level, you will have to specify the portal order yourself. The string must be a binary string specifying whether the bot should tap (0: cube, ball, UFO, spider) or hold (1: ship, wave, robot) the mouse. Each digit represents a region between two of either \[the start of the map, the end of the map, or a portal\]. For example, Stereo Madness's portal string would be '0101' since its mode sequence is cube, ship, cube, ship. You can also specify an odds multiplier at the end by typing a colon (`:`) followed by a decimal number (default is 4). Higher numbers mean the bot jumps more often, so you'll want to raise it on levels that require lots of jumps.

There can also be a second argument specifying a file path to read/write a level save. This keeps track of all of the jumps that have been recorded, as well as some other information about the current progress. This file can be used to allow leaving and coming back to the bot. When specified, the data will be saved every time any progress is made in completion. This means that you can quit DashBot at any time without losing your progress. These saves can also be replayed later by DashBot Player.

### DashBot Player
DashBot Player is a stripped-down version of DashBot that only plays saved jump maps. It can be run using the same steps as running DashBot. It requires exactly one argument: the file path to read from. It does not require the portal list argument because that data is located inside the save file. If you want to just play back a save file without worrying about corrupting or rewriting the data, you can use DashBot Player.

### mkdbj
This tool is mainly intended to recover lost progress in the event of data loss. It does not provide any output, but it expects this input in the following order (separated by newlines):
1. Best fitness level (floating point)
2. Last jump location (floating point)
3. Level type/portal string (string)
4. List of all jumps in the save, separated by newlines (integer)
5. Type `0` to save the map and exit

## Notes
I've experienced some instability when playing back levels, especially when restarting Geometry Dash. You may need to edit the save files manually to fix this if it happens. To do this, you can put the save in a hex editor and find where DashBot is getting stuck (the values are 32-bit LE integers). Then adjust the lowest byte by +- 5 to see if that will solve the issue.
