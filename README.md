# Alice in Botherland: The Last Island

A multiplayer survival-trial game for Windows, built in **Unreal Engine 5.8** (C++ and Slate).

The world burned. The survivors took to the sea as five floating communities, the Nomads. When a beacon rises
from the last habitable island, each community sends one champion. Three trials decide who reaches the Guardian,
and at the end of it all a single choice remains: **become the King, or save your people.**

![The story](docs/screenshots/story.png)

# story / Dialogue  writeen by @sarthak567


| | |
|---|---|
| ![Main menu](docs/screenshots/main-menu.png) | ![The room](docs/screenshots/lobby.png) |
| *The main menu* | *The room: share the code, wait for five champions* |
| ![Your champion](docs/screenshots/profile.png) | ![Settings](docs/screenshots/settings.png) |
| *Your champion: name and date of birth on scroll wheels* | *Settings: audio, voice and captions, controls, graphics* |
| ![Exploring the island](docs/screenshots/island.png) | ![Alice explains the rules](docs/screenshots/rules-card.png) |
| *Exploring the island while Alice welcomes you* | *Alice explains each trial's rules* |
| ![Arrival](docs/screenshots/arrival.png) | ![Stage 1: the Number Game](docs/screenshots/stage1-number-game.png) |
| *The five fleets reach the island* | *Stage 1: the Number Game* |
| ![Stage 2: the Hidden Symbol](docs/screenshots/stage2-hidden-symbol.png) | ![Stage 3: the race](docs/screenshots/stage3-race.png) |
| *Stage 2: the Hidden Symbol* | *Stage 3: the race to the Guardian* |
| ![The final choice](docs/screenshots/final-choice.png) | ![Results](docs/screenshots/results.png) |
| *The Guardian's choice* | *The ending and the leaderboard* |

**Download:** [latest release](https://github.com/imthegoodboy/alice-in-botherland/releases/latest). Unzip it and run
`AliceInBotherland.exe` in the `AliceInBotherland` folder.

## What's new in v1.3.0

A story-driven update: 108 files changed, about 3,300 lines added.

### New story features
- **The Guardian's judgement (the fall scene).** Every champion eliminated in Trial I or II gets a cinematic scene:
  - letterbox bars and a close-up of the champion;
  - they stand in grief and speak their **last words, voiced**. The first fall is always *"Today is not my day... but I hope my people survive."* Seven other lines cover later falls;
  - they look up as a red line of light finds them, the **red beam strikes from the sky**, the screen flashes red and **they collapse**;
  - a banner reads "KESTREL HAS FALLEN" and the Guardian gives its verdict;
  - if two fall together, each speaks in turn and then both are struck.
- **The truth of the island (epilogue)**, after the ending. Alice reveals that **Doctor Elias Varne** foresaw the dying Earth, secretly built the island as **humanity's backup**, and left the Guardian to find someone worthy. The trials tested the **mind** (Number Game), **trust and loyalty** (Hidden Symbol), and **resolve and the heart** (race and final choice).
  - Nine narrated picture cards (**six new paintings**), ending on **"THE ISLAND HAS CHOSEN: (name)"** or "the island remains unclaimed". Space skips it.
- **Story body language**: grief, looking up at the judgement, the chooser thinking, the crowned King's raised fists, and a hand on the heart for the sacrifice.
- **The Liar's Ledger**: after each symbol round, every claim is revealed as TRUE, LIE, or LIE - BELIEVED.
- **The match story**: the results screen shows a timeline of every round win, tie, fall, race win and final choice.
- **New voice recordings:**
  - 8 last-words lines in a new survivor voice;
  - 11 epilogue lines by Alice;
  - 4 of Alice's rule lines re-recorded for the new rules.

### The island
Gulls circling the cove, the town and the Guardian's beam, tumbleweeds rolling across the road, a dead power line,
wrecked cars, an abandoned outpost, signposts, a beached shipwreck, sparks over the signal fire, and scrub and rocks
along the road.

### Voice chat
- A **mic button with an on/off icon** at the top right (or the **M** key). It shows **LIVE** while you are heard.
- **Voice chat volume** slider in the Esc menu.
- A **Mute** button for each player, which hides their voice and their chat.
- Eliminated players can't be heard by champions still in the trials, because they can see every symbol.

### Gameplay changes
- **The race**: the first car into the Guardian's ring wins the first choice. There is no stopping and no holding E. If both cars arrive on the same frame, the one deeper in the ring wins.
- **The final choice**:
  - a pick you didn't confirm still counts when time runs out;
  - no stalling by picking and going back;
  - if the chooser leaves, the choice passes to the other finalist;
  - the correct ending and message show when the second finalist chooses.
- **Number Game sudden death**: everyone picks again, so the target keeps moving, but only the tied champions can fall.
- **A number left on your dial counts** even if you didn't press Lock In.
- **The host can End Match** from the Esc menu (click twice) to send everyone back to the room.

### Bugs fixed
1. After a match that went to a sudden-death race, the **next match's race froze**.
2. With no finalists left, the race **got stuck**.
3. A finalist who left before the race could have their AI ghost **drive and win**. Now the other finalist wins by walkover.
4. A **disconnected chooser could be crowned King** by default.
5. If someone left mid-round in Trial I, **two champions** could be eliminated.
6. **Anyone could take a disconnected player's seat** by typing the same name. Now only the same player can.
7. Players who left from the menu kept a ghost holding their seat for 30 s. Now the seat frees at once, and leaving mid-trial counts as a forfeit.
8. Players who left after being eliminated **vanished from the leaderboard**.
9. Old ghosts **stayed as AI players** in the next room.
10. Players who reconnected during the Hidden Symbol **couldn't see anyone's symbols**.
11. The round's "everyone answered" skip **waited forever** for players who had left.
12. The Trial II tie announcement was **instantly hidden**.
13. Chat name matching was too loose ("Ann" matched "Joanna"), so AI champions misread who was told what.
14. Marker stones **could be read from anywhere**. Now you must stand near them.
15. The car reset key **could be spammed**. It now has a 3-second cooldown.
16. Alice **repeated the whole race briefing** before the sudden-death race.
17. Signpost text **faced the wrong way**.
18. On-screen text still said "hold the switch for 3 seconds". Updated everywhere.

### Release
- Version 1.3.0, merged into `main` (pull request #1), tagged `v1.3.0`.
- GitHub release with the Windows zip (1.5 GB). The packaged `.exe` was tested and plays through the story, Trial I and the fall scene into Trial II.
- New tool: `Tools/gen_last_words.py` records the champions' last words.
- README, `HOW TO PLAY.txt` and the developer notes were updated.

## The game

Five champions (human players, with AI filling any empty seats) play one match:

| Phase | What happens |
|---|---|
| **Room** | The host creates a room and shares its **room code**; friends join by typing it. When all five seats are filled (AI test mode fills empty seats with AI) and everyone is ready, the host presses **Start**. |
| **The story** | A narrated picture story (skippable): the fall of Earth, the search for survival, the last island, the Guardian's trials. |
| **Arrival** | A short cinematic (skippable): the fleets sail through the channel into the cove and the champions land. |
| **The island** | 30 seconds to explore: gulls over the cove, tumbleweeds on the road, wrecks, a dead power line and an abandoned outpost. Five old marker stones (follow the green light pillars) each tell part of the island's past. |
| **Stage 1: the Number Game** | Everyone secretly picks 0-100. Target = average × 0.8. The champion **farthest** from the target is eliminated. A tie for farthest means sudden death: everyone picks again (so the target still moves), but only the tied champions can fall. A number dialled in but not locked counts. |
| **Stage 2: the Hidden Symbol** | Everyone sees every symbol except their own. Talk in chat, trust, lie, then name your own symbol. 3 rounds; the top two advance. After each round **the Liar's Ledger** reveals every claim: true, lie, or lie that was believed. |
| **Stage 3: the race** | The two finalists drive offroad buggies to the Guardian's beam. **Whoever drives into the glowing ring first** takes the first choice - no stopping, no button. Winning only earns the **first choice**. |
| **The Guardian's choice** | **Become the King** (you win; your people are lost) or **Save your people** (you die; they live, and the choice passes to the other finalist). No choice in time means King (a pick you didn't confirm still counts). If a chooser leaves, the choice passes on. |
| **Ending** | Three main endings (*The Crowned Exile*, *The Gift and the Crown*, *The Double Sacrifice*) plus two for when a finalist forfeits. |
| **The truth of the island** | After the ending Alice reveals why the island exists: long ago Doctor Elias Varne foresaw the dying Earth and built it in secret as humanity's backup, then left the Guardian to find someone worthy. The trials tested the mind (the Number Game), trust and loyalty (the Hidden Symbol), and resolve and the heart (the race and the final choice). Narrated pictures, ending on the champion the island chose (or that it stays unclaimed). Skippable. |
| **Results** | The leaderboard and **the match story**: a timeline of every round win, tie, fall and choice. |

Alice, the narrator, welcomes the champions and explains every trial's rules in plain words, with captions and a
picture card, while everyone can still walk around.

The full design document is [`game.md`](game.md).

### Playing

- **First launch**: enter your champion name and pick your date of birth on the scroll wheels (kept on your PC only).
- **Main menu**: pick a mode (Play vs AI, Create room, Join room) and press **START**; Rules, Hall, Settings, Credits and Quit are the icons at the top right.
- **Play vs AI**: you and four AI champions.
- **Create room / join room**: the room shows a short code such as `6C2M-9QZ`. Friends on the same network type it in **Join room**. For internet play, the host switches to the **internet code** and forwards UDP port 7777 on their router. Rooms on your network are also listed, and you can still connect by IP address.
- **Voice chat**: hold **V** to talk (or set the microphone to *always on* / *off*). The mic button at the top right (or **M**) turns your microphone on and off and lights up while you are live; a microphone icon shows who is talking. In the game menu you can set the voice chat volume and mute any player (their voice and chat). Champions still in the trials can't hear eliminated players, who see every hidden symbol.
- **The Guardian's judgement**: every eliminated champion gets a scene. Letterbox bars close in, the camera moves close, and the champion stands in grief and speaks their last words, voiced ("Today is not my day... but I hope my people survive."). They look up as a red beam gathers in the sky, then it strikes and they collapse. The Guardian speaks its verdict.
- **In a match**: press **Esc** or the gear at the top right for the game menu (resume, settings, how to play, leave; the host can also end the match for everyone) with quick sliders for mouse sensitivity, field of view, volume and voice.
- **Settings**: audio, voice and captions, controls (sensitivity, invert, field of view, key list) and graphics (quality, resolution, brightness, frame-rate limit, motion blur, FPS counter, fullscreen, VSync).
- Reconnects: a disconnected player's seat is held by an AI "ghost" for 30 s in the lobby and 20 s during a match (only the same player can take it back). Leaving from the menu frees the seat at once; leaving mid-trial forfeits.

| Action | Keyboard / mouse | Gamepad |
|---|---|---|
| Move / look | WASD / mouse | Left / right stick |
| Sprint, jump | Left Shift, Space | Left stick click, A |
| Interact (read a marker stone) | E | X |
| Voice chat (push to talk), mic on/off | V, M | D-pad up |
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
| `Player/` | The champion character (story poses for grief, judgement, the final choice and its outcome, `AIBPoses`: bones posed on a poseable copy of the animated mesh; the Guardian's judgement beam), the Chaos offroad vehicle, and the player controller (input is built in code with Enhanced Input; cameras for every phase including the judgement close-ups; voice chat). |
| `World/` | Self-building set pieces spawned locally on every machine by `UAIBWorldSubsystem`: ocean, fleet, landing site, Number room, hotel, race course, Guardian site, island life (gulls, tumbleweeds, roadside wrecks, signposts) and environment lighting. Positions are in `AIBLayout.h`; the race route is in `AIBRacePath.h`. |
| `Audio/` | Music crossfades, voice-over with captions, ambience per zone, and the narrator (`AIBNarration`: Alice's lines and the timed story / rules sequences). |
| `Online/` | Sessions: Steam (App ID 480 for development) with a LAN / direct-IP fallback, and room codes (`AIBRoomCode`: the host's address packed into a short code). |
| `UI/` | All UI is Slate in C++: main menu and profile, the room, story and rules cards, HUD, every stage panel, settings, the leaderboard. `SAIBGameUI` holds the game-style widgets (slanted buttons, icon buttons, sliders, switches, scroll wheels, reveal animations); icons come from the Material Icons font. |

One map (`L_Island`) hosts the main menu, the lobby and the whole match. There is no level travel between phases.

Game assets live in `Content/AliceInBotherland/`: audio, imported models and textures, materials, UI images and fonts.
Raw source files (audio, glTF/GLB models, textures, story pictures, credits) are in `SourceAssets/`.

### Story pictures and narration

`Tools/` holds the scripts that made the story art and Alice's voice. They read API keys from a git-ignored `.env`
in the project root (`OPENAI_API_KEY`, `ELEVENLABS_API_KEY` and optionally `ELEVENLABS_API_KEY1`..`4`).

| Script | Does |
|---|---|
| `Tools/gen_images.py <model> all` | Generates the story, loading and trial pictures into `SourceAssets/Images/Story/`. |
| `Tools/gen_voice.py [ids]` | Speaks the lines in `SourceAssets/Audio/narration.json` with ElevenLabs into `SourceAssets/Audio/VO/` (needs ffmpeg). |
| `Tools/gen_last_words.py [ids]` | Speaks the champions' last words (`SourceAssets/Audio/last_words.json`, a separate survivor voice) and writes `Audio/AIBLastWordsLines.inl`. |
| `Tools/gen_narration_table.py` | Writes `Source/AliceInBotherland/Audio/AIBNarrationLines.inl` (captions and spoken lengths) so the game's timing matches the recordings. |

Imported pictures live in `Content/AliceInBotherland/UI/Story/`, the voice lines in `Content/AliceInBotherland/Audio/VO/`.

### Development options

`-AIBTimeScale` and `-AIBJump` are compiled out of Shipping builds, and Shipping builds ignore map URLs on the command line.

| Option | Effect |
|---|---|
| `-AIBTimeScale=0.5` | Shortens every phase timer (fast end-to-end testing). |
| `-AIBJump=stage1\|stage2\|race\|final` | Starts the match straight at a trial. |
| URL `?Bots=N`, `?AutoStart`, `?AutoReady` | Pre-fill AI seats; ready everyone (`AutoStart` also fills every empty seat). Example: `L_Island?listen?game=Match?Bots=4?AutoStart` |
| `-QuickPlay` (also in Shipping) | Goes straight from the menu into a solo match. |
| `-JoinCode=6C2M-9QZ` (also in Shipping) | Joins that room straight from the menu. |
| `-AIBMic=2` | Forces an open microphone (voice chat testing). |

### Known issues and notes

- **Smart App Control**: builds are not code-signed, and Windows judges every new binary (the editor's game DLL too). A refused DLL shows "The game module could not be loaded" (error 4551); rebuilding with any small change usually gets an accepted binary. Shipping builds are not code-signed either. On PCs with Windows Smart App Control enabled, an individual build can be refused (`Microsoft-Windows-CodeIntegrity/Operational` shows event 3033). Rebuilding usually produces an accepted binary, and running through `UnrealEditor.exe -game` always works.
- **Graphics**: auto-detection caps quality at High, because Epic overflows 4 GB GPUs (stutter and flicker). Epic can still be chosen in Settings.
- **Steam**: with Steam not running, the game falls back to LAN / direct IP.

## Credits and licences

- **Music**: Kevin MacLeod ([incompetech.com](https://incompetech.com)), CC BY 4.0. Full attribution in [`SourceAssets/Audio/CREDITS.md`](SourceAssets/Audio/CREDITS.md) and the in-game Credits.
- **Voices and some sound effects**: generated with [ElevenLabs](https://elevenlabs.io) (the narrator is the ElevenLabs voice "Alice"). The free plan requires attribution and does **not** include a commercial licence. Regenerate on a paid plan before any commercial release.
- **Story pictures**: generated with OpenAI image models for this project.
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
