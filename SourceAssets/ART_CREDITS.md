# Art Credits — Alice in Botherland: The Last Island

Machine-readable details (sizes, triangle counts, facing, files): `SourceAssets/art_manifest.json`.

## Poly Haven (CC0 1.0, public domain)

All models and textures below come from [Poly Haven](https://polyhaven.com) and are released under CC0. No attribution is required, but we credit Poly Haven and its artists.

### Models (glTF, `SourceAssets/PolyHaven/<id>/`)

| Group | Asset IDs (resolution) |
|---|---|
| Ships | dutch_ship_large_01, dutch_ship_large_02, dutch_ship_medium, ship_pinnace (2k) |
| Coast | coastal_cliff_01, coastal_cliff_02, coastal_cliff_04, coast_rocks_01, coast_rocks_02, coast_line_01, coast_land_rocks_02 (2k) |
| Dock | modular_wooden_pier (2k); wooden_barrels_01, wooden_crate_01, wooden_crate_02, lateral_sea_marker, ocean_buoy, wooden_lantern_01 (1k) |
| Statues | gothic_statue, marble_bust_01, lion_head (2k) |
| Hotel lobby | Chandelier_01, Chandelier_02, Sofa_01, sofa_02, ArmChair_01, ClassicConsole_01, GothicCabinet_01 (2k); coffee_table_round_01, Ottoman_01, GreenChair_01, industrial_wall_lamp, vintage_oil_lamp, hanging_industrial_lamp (1k) |
| Number-game room | round_wooden_table_01, round_wooden_table_02 (2k); chess_set (1k) |

### Textures (2k JPG: diff, nor_gl, arm — `SourceAssets/Textures/PolyHaven/<id>/`)

floor_tiles_06, decrepit_wallpaper, peeling_painted_wall, dark_wooden_planks, dark_paneled_wood, rusty_metal_04, hangar_concrete_floor, stone_block_wall, dark_rock, velour_velvet, damp_beach_sand_02

## Fonts (`Content/AliceInBotherland/UI/Fonts/`)

| File | Font | License | Source |
|---|---|---|---|
| BebasNeue-Regular.ttf | Bebas Neue (Dharma Type) | SIL Open Font License 1.1 (`OFL_BebasNeue.txt`) | github.com/google/fonts |
| BarlowCondensed-SemiBold.ttf, BarlowCondensed-Bold.ttf | Barlow Condensed (Jeremy Tribby) | SIL OFL 1.1 (`OFL_BarlowCondensed.txt`) | github.com/google/fonts |
| Barlow-Regular.ttf, Barlow-Medium.ttf | Barlow (Jeremy Tribby) | SIL OFL 1.1 (`OFL_Barlow.txt`) | github.com/google/fonts |
| Cinzel-Regular.ttf, Cinzel-Bold.ttf | Cinzel (Natanael Gama); static instances (wght 400/700) made from the variable font with fontTools | SIL OFL 1.1 (`OFL_Cinzel.txt`) | github.com/google/fonts |
| Symbols.ttf | Noto Sans Symbols 2 Regular (Google), renamed. Contains ♠ ♥ ♦ ♣ | SIL OFL 1.1 (`OFL_NotoSansSymbols2.txt`) | github.com/google/fonts |

The OFL lets you bundle, embed and ship these fonts in the game. You may not sell the font files on their own. The modified Cinzel instances keep the original family name, which the OFL allows because "Cinzel" is not a Reserved Font Name.

## Icons

`UI/Images/Crest_0..4.png` are rendered from Google **Material Icons** glyphs (tsunami, local_fire_department, notifications, anchor, eco). They are licensed under the **Apache License 2.0** (github.com/google/material-design-icons).

## Original project assets

The following were made for this project with Blender 5.2 (headless Python), Pillow, numpy and shapely. They contain no third-party content:
- Custom models in `SourceAssets/Models/`: SM_Suit_Heart, SM_Suit_Diamond, SM_Suit_Club, SM_Suit_Spade, SM_Throne, SM_Altar, SM_SwitchPedestal, SM_Podium, SM_RoundTable_Big, SM_HoloRing, SM_BeamCylinder, SM_ColumnRuined, SM_ReceptionDesk, SM_GuardianStatue.
- UI images: Suit_* (and _Glow), Vignette, Grain, GradientUp, Glow_Soft, Ring, Logo_Emblem.
