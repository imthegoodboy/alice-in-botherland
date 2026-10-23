# Alice in Botherland

High-quality third-person game built in Unreal Engine 5.8 (Blueprint project, started from the Third Person template).

## Tools
- **Unreal MCP** (`unreal-mcp`, http://127.0.0.1:8000/mcp): Epic's ModelContextProtocol plugin + AllToolsets, auto-starts when this project is open in the editor. Tool search is on: use `list_toolsets` / `describe_toolset` / `call_tool`.
- **Blender MCP** (`blender`, `uvx blender-mcp`): Blender 5.2, add-on "MCP for Blender" — click Connect in the N-panel before use. Use for modelling/editing characters and props, export FBX to `SourceAssets/`.
- **Unity** (`unity` MCP + Unity plugin/CLI): installed but not the main engine for this game.

## Layout
- `Content/AliceInBotherland/` — all game-owned assets (Characters, Environment, Blueprints, Maps, Materials, UI, VFX, Audio, Data, Cinematics).
- `SourceAssets/` — raw files outside the engine (FBX, .blend, textures, reference images). Not imported automatically.
- Fab packs install into their own top-level folder under `Content/` (e.g. `Content/RogueCharacter`). Reference them from there; build game Blueprints in `Content/AliceInBotherland/`.

## Characters
- Player character: **Survival Character – Arberry** (Fab). Not the Rogue Character (`Content/RogueCharacter`, kept but unused for now).
