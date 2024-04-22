# Dead Island 2 Fix
[![Patreon-Button](https://github.com/Lyall/DeadIsland2Fix/assets/695941/6c97db50-da3d-4762-9f41-eddf522a4ce5)](https://www.patreon.com/Wintermance) [![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/W7W01UAI9)<br />
[![Github All Releases](https://img.shields.io/github/downloads/Lyall/DeadIsland2Fix/total.svg)](https://github.com/Lyall/DeadIsland2Fix/releases)

This is a fix to remove pillarboxing/letterboxing in Dead Island 2.<br />

## Features
- Removes pillarboxing/letterboxing in cutscenes.
- Removes limit on resizing HUD. (Especially useful at wider than 21:9)

## Installation
- Grab the latest release of DeadIsland2Fix from [here.](https://github.com/Lyall/DeadIsland2Fix/releases)
- Extract the contents of the release zip in to the the game folder. <br />(e.g. "**steamapps\common\DRAGON QUEST XI S**" for Steam).

### Steam Deck/Linux Additional Instructions
🚩**You do not need to do this if you are using Windows!**
- Open up the game properties in Steam and add `WINEDLLOVERRIDES="dsound=n,b" %command%` to the launch options.

## Configuration
- See **DeadIsland2Fix.ini** to adjust settings for the fix.

## Known Issues
Please report any issues you see.
This list will contain bugs which may or may not be fixed.

## Screenshots

| ![cutscene gif](https://user-images.githubusercontent.com/695941/233657454-46a9effd-4168-4169-a168-8489942cc0a8.gif) |
|:--:|
| Cutscene |

## Credits
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading. <br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini reading. <br />
[spdlog](https://github.com/gabime/spdlog) for logging. <br />
[safetyhook](https://github.com/cursey/safetyhook) for hooking.
