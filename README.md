# Alice in Botherland: The Last Island

A multiplayer survival-trial game for Windows, built in **Unreal Engine 5.8** (C++ and Slate).

The world burned. The survivors took to the sea as five floating communities, the Nomads. When a beacon rises
from the last habitable island, each community sends one champion. Three trials decide who reaches the Guardian,
and at the end of it all a single choice remains: **become the King, or save your people.**

![Main menu](docs/screenshots/main-menu.png)

| | |
|---|---|
| ![Arrival](docs/screenshots/arrival.png) | ![Stage 1: the Number Game](docs/screenshots/stage1-number-game.png) |
| *The five fleets reach the island* | *Stage 1: the Number Game* |
| ![Stage 2: the Hidden Symbol](docs/screenshots/stage2-hidden-symbol.png) | ![Stage 3: the race](docs/screenshots/stage3-race.png) |
| *Stage 2: the Hidden Symbol* | *Stage 3: the race to the Guardian* |
| ![The final choice](docs/screenshots/final-choice.png) | ![Results](docs/screenshots/results.png) |
| *The Guardian's choice* | *The ending and the leaderboard* |

## The game

Five champions (human players, with AI filling any empty seats) play one match:

| Phase | What happens |
|---|---|
| **Lobby** | Up to 5 champions gather and press Ready. Empty seats can be filled with AI. A 15 s countdown starts when everyone is ready. |
| **Arrival** | A cinematic (skippable): the fleets sail through the channel into the cove and the champions land. |
| **The island** | Explore the island until the first trial begins. |
| **Stage 1: the Number Game** | Everyone secretly picks 0-100. Target = average × 0.8. The champion **farthest** from the target is eliminated. Ties go to sudden death. |
| **Stage 2: the Hidden Symbol** | Everyone sees every symbol except their own. Talk in chat, trust, lie, then name your own symbol. 3 rounds; the top two advance. |
| **Stage 3: the race** | The two finalists drive offroad buggies to the Guardian's beam and hold the switch for 3 s. Winning only earns the **first choice**. |
| **The Guardian's choice** | **Become the King** (you win; your people are lost) or **Save your people** (you die; they live, and the choice passes to the other finalist). No choice in time means King. |
| **Ending** | Three main endings (*The Crowned Exile*, *The Gift and the Crown*, *The Double Sacrifice*) plus two for when a finalist forfeits. A leaderboard records champions, sacrifices and eliminations. |

The full design document is [`game.md`](game.md).

### Playing

- **Solo vs AI**: you and four AI champions.
- **Host a lobby / join a lobby**: LAN discovery, or connect directly by IP address (the host forwards UDP port 7777 for internet play). Steam friend invites work when Steam is running.
- Reconnects: a disconnected player's seat is held by an AI "ghost" for 30 s in the lobby and 20 s during a match.

| Action | Keyboard / mouse | Gamepad |
|---|---|---|
| Move / look | WASD / mouse | Left / right stick |
| Sprint, jump | Left Shift, Space | Left stick click, A |
| Interact (hold the switch) | E | X |
| Chat | Enter or T | |
| Scoreboard, menu | Tab, Esc | View, Menu |
| Drive (Stage 3) | W/S, A/D, Space handbrake, R reset | Triggers, left stick, B, Y |
| Spectate next / previous | Left / right mouse button | Shoulder buttons |

A packaged game ships with a `HOW TO PLAY.txt` for players.

## Building from source

### Requirements

- **Unreal Engine 5.8** (Epic Games Launcher).
- **Visual Studio 2022** (or Build Tools) with the *Desktop development with C++* workload and a Windows SDK.
- **.NET Framework 4.8 SDK** (UnrealBuildTool needs it on this setup).
- **[Git LFS](https://git-lfs.com/)**: all `.uasset`/`.umap` files, textures, models and audio are stored in LFS.

### 1. Clone

```bash
git lfs install
git clone https://github.com/imthegoodboy/alice-in-botherland.git
```

### 2. Add the Fab packs (not included)

Two paid Fab packs are licensed per user and are **not** in this repository. Add both from your own Fab library
(Epic Games Launcher → Fab Library → *Add to Project* → AliceInBotherland):

| Pack | Used for | Must be at |
|---|---|---|
| **Survival Character** by Arberry | the champions | `Content/Survival_Character/` |
| **Modular Post Apocalyptic Desert** by JustB Studios | the island | `Content/JustBStudios/` |

Leave both in the folders Fab installs them into. The game's C++ loads pack assets by these paths. Both folders are git-ignored.

### 3. Create the island map

`L_Island` is derived from the desert pack's level, so it is git-ignored too. In the editor:

1. Duplicate `/Game/JustBStudios/PostApo/Levels/PostApo` to `/Game/AliceInBotherland/Maps/L_Island`.
2. Open it. In **World Settings**, clear **GameMode Override**. The game picks its mode from the URL: `?game=Menu` or `?game=Match`.
3. Save.

### 4. Build and run

Open `AliceInBotherland.uproject` and let it compile the C++ module, or build from a terminal with the editor closed:

```bat
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" AliceInBotherlandEditor Win64 Development -Project="%CD%\AliceInBotherland.uproject" -WaitMutex
```

Press **Play** in the editor, or run the game standalone:

```bat
UnrealEditor.exe "%CD%\AliceInBotherland.uproject" -game
```

### 5. Package a Windows build

```bat
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%CD%\AliceInBotherland.uproject" -noP4 -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -iostore -archive -archivedirectory="%CD%\Build" -unattended -nocompileeditor -skipbuildeditor -prereqs
```

The game lands in `Build/Windows/` (git-ignored). A full cook takes about 30 minutes. After code-only changes, add
`-skipcook` to restage in a few minutes. Share the whole `Windows` folder, not just the `.exe`.

## How it is built

Everything game-specific lives in the C++ module `Source/AliceInBotherland` (class prefix `AIB`).

| Folder | Contents |
|---|---|
| `Core/` | Shared types and timers (`AIBTypes`), asset loading, the save profile, and the game instance (settings, loading screen, graphics auto-detect). |
| `Game/` | The server-authoritative match: `AAIBGameMode` is a phase machine (lobby → arrival → trials → race → choice → ending), split across `AIBGameMode*.cpp`, plus game/player state, AI champions and reconnect ghosts. |
| `Player/` | The champion character, the Chaos offroad vehicle, and the player controller (input is built in code with Enhanced Input; cameras for every phase). |
| `World/` | Self-building set pieces spawned locally on every machine by `UAIBWorldSubsystem`: ocean, fleet, landing site, Number room, hotel, race course, Guardian site and environment lighting. Positions are in `AIBLayout.h`; the race route is in `AIBRacePath.h`. |
| `Audio/` | Music crossfades, voice-over with subtitles, ambience per zone. |
| `Online/` | Sessions: Steam (App ID 480 for development) with a LAN / direct-IP fallback. |
| `UI/` | All UI is Slate in C++: main menu, HUD, every stage panel, settings, the leaderboard. |

One map (`L_Island`) hosts the main menu, the lobby and the whole match. There is no level travel between phases.

Game assets live in `Content/AliceInBotherland/`: audio, imported models and textures, materials, UI images and fonts.
Raw source files (audio, glTF/GLB models, textures, credits) are in `SourceAssets/`.

### Development options

`-AIBTimeScale` and `-AIBJump` are compiled out of Shipping builds, and Shipping builds ignore map URLs on the command line.

| Option | Effect |
|---|---|
| `-AIBTimeScale=0.5` | Shortens every phase timer (fast end-to-end testing). |
| `-AIBJump=stage1\|stage2\|race\|final` | Starts the match straight at a trial. |
| URL `?Bots=N`, `?AutoStart`, `?AutoReady` | Pre-fill AI seats; ready everyone (`AutoStart` also fills every empty seat). Example: `L_Island?listen?game=Match?Bots=4?AutoStart` |
| `-QuickPlay` (also in Shipping) | Goes straight from the menu into a solo match. |

### Known issues and notes

- **Smart App Control**: Shipping builds are not code-signed. On PCs with Windows Smart App Control enabled, an individual build can be refused (`Microsoft-Windows-CodeIntegrity/Operational` shows event 3033). Rebuilding usually produces an accepted binary, and running through `UnrealEditor.exe -game` always works.
- **Graphics**: auto-detection caps quality at High, because Epic overflows 4 GB GPUs (stutter and flicker). Epic can still be chosen in Settings.
- **Steam**: with Steam not running, the game falls back to LAN / direct IP.

## Credits and licences

- **Music**: Kevin MacLeod ([incompetech.com](https://incompetech.com)), CC BY 4.0. Full attribution in [`SourceAssets/Audio/CREDITS.md`](SourceAssets/Audio/CREDITS.md) and the in-game Credits.
- **Voices and some sound effects**: generated with [ElevenLabs](https://elevenlabs.io) on the free plan, which requires attribution and does **not** include a commercial licence. Regenerate on a paid plan before any commercial release.
- **Models and textures**: [Poly Haven](https://polyhaven.com), CC0. See [`SourceAssets/ART_CREDITS.md`](SourceAssets/ART_CREDITS.md).
- **Fonts**: Bebas Neue, Barlow, Barlow Condensed, Cinzel and Noto Sans Symbols 2 (SIL Open Font License). **Icons**: Google Material Icons (Apache 2.0).
- **Vehicles**: Unreal Engine Vehicle template content (Epic Games).
- **Characters and island**: the Fab packs listed above (not distributed with this repository).

## Agentic workflow

The project was built with Claude Code driving Unreal Engine, Blender and Unity through MCP:

| Tool | How it connects |
|---|---|
| Unreal | Plugins `ModelContextProtocol` + `AllToolsets` are enabled; the MCP server auto-starts at `http://127.0.0.1:8000/mcp` when the editor opens. |
| Blender | `uvx blender-mcp`; in Blender press **N → MCP for Blender → Connect**. |
| Unity | `unity mcp` (Unity CLI) + Unity plugin for Claude Code (`unity setup claude`). |

```bash
claude mcp add --transport http unreal-mcp http://127.0.0.1:8000/mcp -s user
claude mcp add blender -s user -- uvx blender-mcp
claude mcp add unity -s user -- unity mcp
```

`Content/Python/init_unreal.py` adds an optional localhost automation bridge for scripted editor work. It only starts when
`Saved/ClaudeBridge.enabled` exists. See [`CLAUDE.md`](CLAUDE.md) for project conventions.
