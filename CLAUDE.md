# Alice in Botherland

High-quality third-person game built in Unreal Engine 5.8. The game described in `game.md` ("The Nomad: The Last Island", 5-player trials) is implemented in the C++ module `Source/AliceInBotherland` (prefix AIB); the template Blueprints are no longer used by the game.

## Tools
- **Unreal MCP** (`unreal-mcp`, http://127.0.0.1:8000/mcp): Epic's ModelContextProtocol plugin + AllToolsets, auto-starts when this project is open in the editor. Tool search is on: use `list_toolsets` / `describe_toolset` / `call_tool`.
- **Blender MCP** (`blender`, `uvx blender-mcp`): Blender 5.2, add-on "MCP for Blender" — click Connect in the N-panel before use. Use for modelling/editing characters and props, export FBX to `SourceAssets/`.
- **Unity** (`unity` MCP + Unity plugin/CLI): installed but not the main engine for this game.

## Layout
- `Content/AliceInBotherland/` — all game-owned assets (Characters, Environment, Blueprints, Maps, Materials, UI, VFX, Audio, Data, Cinematics).
- `SourceAssets/` — raw files outside the engine (FBX, .blend, textures, reference images). Not imported automatically.
- Bought Fab packs (git-ignored, licensed per user) stay in the folders Fab installs them into: `Content/Survival_Character/` and `Content/JustBStudios/`. The C++ loads pack assets by these paths, so moving a pack (e.g. to `Content/ThirdParty/Fab/`) also means updating those paths in code. Never edit pack assets in place: duplicate into `Content/AliceInBotherland/` first.

## Characters
- Player character: **Survival Character** by Arberry (Fab).
- Environment: **Modular Post Apocalyptic Desert** by JustB Studios (Fab).

## Current setup
- **Map**: one map, `L_Island` (`Maps`, copy of the desert pack's `PostApo` level, git-ignored; recreate by duplicating `/Game/JustBStudios/PostApo/Levels/PostApo`). Main menu = `L_Island?game=Menu` (`AAIBMenuGameMode`); a match = `L_Island?listen?game=Match` (`AAIBGameMode`). The lobby is the first phase of the match game mode (no server travel).
- **Code layout** (`Source/AliceInBotherland`): `Core/` (types, timers, assets, save game, game instance), `Game/` (server-authoritative phase machine split into `AIBGameMode*.cpp`, game/player state, bots), `Player/` (character, race vehicle, player controller with code-built Enhanced Input), `World/` (self-building set pieces spawned locally by `UAIBWorldSubsystem`; positions in `AIBLayout.h`, race route in `AIBRacePath.h`), `Audio/`, `Online/` (sessions: Steam with Null/LAN fallback), `UI/` (all UI is Slate in C++).
- **Content** the code loads by path: `Content/AliceInBotherland/{Audio,Environment,Materials,Textures,UI,VFX}`; raw UI fonts/images/credits are staged as UFS files.
- **Building**: close the editor, then `Engine\Build\BatchFiles\Build.bat AliceInBotherlandEditor Win64 Development -Project=...`. Avoid editing `AliceInBotherland.Build.cs`: Windows Smart App Control on this PC blocks UBT's recompiled rules DLL.
- **Testing flags** (non-Shipping only): `-AIBTimeScale=0.5` shortens phase timers, `-AIBJump=stage1|stage2|race|final` skips ahead; URL options `?Bots=N`, `?AutoStart` (fill with AI + ready), `?AutoReady` (ready everyone, keep seats open).
- **Gotchas**: ProceduralMeshComponent front faces have Cross(V1-V0, V2-V0) pointing *away* from the viewer. Graphics scalability must be applied after engine init (`UAIBGameInstance::OnStart`). The target laptop is an RTX 3050 4 GB: auto-detected quality is capped at High.
- **Packaging**: `RunUAT.bat BuildCookRun ... -clientconfig=Shipping ... -archivedirectory=<project>/Build` (see README); add `-skipcook` after code-only changes. Shipping ignores map URLs on the command line; `-QuickPlay` starts a solo match.
