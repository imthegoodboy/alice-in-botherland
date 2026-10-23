# Alice in Botherland

High-quality third-person game built in Unreal Engine 5.8 (Blueprint project, started from the Third Person template).

## Tools
- **Unreal MCP** (`unreal-mcp`, http://127.0.0.1:8000/mcp): Epic's ModelContextProtocol plugin + AllToolsets, auto-starts when this project is open in the editor. Tool search is on: use `list_toolsets` / `describe_toolset` / `call_tool`.
- **Blender MCP** (`blender`, `uvx blender-mcp`): Blender 5.2, add-on "MCP for Blender" — click Connect in the N-panel before use. Use for modelling/editing characters and props, export FBX to `SourceAssets/`.
- **Unity** (`unity` MCP + Unity plugin/CLI): installed but not the main engine for this game.

## Layout
- `Content/AliceInBotherland/` — all game-owned assets (Characters, Environment, Blueprints, Maps, Materials, UI, VFX, Audio, Data, Cinematics).
- `SourceAssets/` — raw files outside the engine (FBX, .blend, textures, reference images). Not imported automatically.
- `Content/ThirdParty/Fab/<PackName>/` — bought Fab packs (git-ignored, licensed per user). Fab installs a pack into its own top-level `Content/` folder; move it here in the editor (Content Browser move, then Fix Up Redirectors). Never edit pack assets in place: duplicate into `Content/AliceInBotherland/` first.

## Characters
- Player character: **Survival Character** by Arberry (Fab).
- Environment: **Modular Post Apocalyptic Desert** by JustB Studios (Fab).

## Current setup
- `BP_AlicePlayer` (`Blueprints/Core`): child of template `BP_ThirdPersonCharacter`, mesh `SK_Survival_Character`, anim `ABP_Quinn` from the Survival pack demo.
- `BP_AliceGameMode` (`Blueprints/Core`): default pawn `BP_AlicePlayer`. Set as global default game mode.
- `L_Desert_Main` (`Maps`): copy of the desert pack's `PostApo` level; editor startup + game default map. Git-ignored (derived from licensed content), recreate it by duplicating `/Game/JustBStudios/PostApo/Levels/PostApo`.
- Fab packs currently sit at `Content/Survival_Character/` and `Content/JustBStudios/` (git-ignored). Moving them into `ThirdParty/Fab/` must be done in the editor with redirector fix-up.
