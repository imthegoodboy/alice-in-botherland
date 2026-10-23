# Alice in Botherland

A third-person adventure game built in **Unreal Engine 5.8**, developed with an agentic workflow: Claude Code drives the Unreal Editor, Blender and Unity through MCP.

## Requirements
- Unreal Engine 5.8
- [Git LFS](https://git-lfs.com/) (`git lfs install` before cloning)
- Blender 5.2 with the "MCP for Blender" add-on (optional, for asset work)
- Claude Code (optional, for the agentic workflow)

## Getting started
```bash
git lfs install
git clone https://github.com/imthegoodboy/alice-in-botherland.git
```
Open `AliceInBotherland.uproject` with Unreal Engine 5.8.

### Fab assets (not included)
Paid Fab packs are licensed per user and are **not** in this repo. Add them from your own Fab library (Epic Games Launcher → Fab Library → *Add to Project* → AliceInBotherland):
- Survival Character – Arberry (player character)
- Island environment pack

After adding a new pack, list its `Content/<PackFolder>/` in `.gitignore`.

## Agentic setup
| Tool | How it connects |
|---|---|
| Unreal | Plugins `ModelContextProtocol` + `AllToolsets` are enabled in the project; the MCP server auto-starts at `http://127.0.0.1:8000/mcp` when the editor opens. |
| Blender | `uvx blender-mcp`; in Blender press **N → MCP for Blender → Connect**. |
| Unity | `unity mcp` (Unity CLI) + Unity plugin for Claude Code (`unity setup claude`). |

Claude Code MCP config (user scope):
```bash
claude mcp add --transport http unreal-mcp http://127.0.0.1:8000/mcp -s user
claude mcp add blender -s user -- uvx blender-mcp
claude mcp add unity -s user -- unity mcp
```

## Project layout
```
Content/AliceInBotherland/   Game assets: Characters, Environment, Blueprints, Maps, Materials, UI, VFX, Audio, Data, Cinematics
Content/ThirdPerson/         Third Person template content
SourceAssets/                Raw source files (FBX, .blend, textures, reference)
Config/                      Project settings
```
