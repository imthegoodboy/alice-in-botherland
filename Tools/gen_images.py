"""Generate game images with OpenAI. usage: gen_images.py <model> <id> [<id> ...]  (ids from PROMPTS; 'all' for every image)
Reads OPENAI_API_KEY from the project .env (never printed). Writes SourceAssets/Images/Story/<id>.png"""
import base64, json, os, sys, time, urllib.request, urllib.error
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "SourceAssets", "Images", "Story")
env = {}
for line in open(os.path.join(ROOT, ".env"), encoding="utf-8", errors="replace"):
    if "=" in line and not line.strip().startswith("#"):
        k, v = line.split("=", 1)
        env[k.strip()] = v.strip().strip('"').strip("'")
KEY = env["OPENAI_API_KEY"]

STYLE = ("Cinematic digital matte painting, widescreen film still, dramatic volumetric light, muted teal and burnt amber colour palette, "
         "dust and haze in the air, painterly but realistic detail, consistent art direction for a story-driven survival game. "
         "Absolutely no text, no letters, no numbers, no captions, no logos, no watermark.")
PROMPTS = {
    "S1_DyingEarth": "The year 2100: a dying Earth. A vast barren cracked plain where farmland used to be, dead crops and withered trees, "
                     "a dust storm swallowing an abandoned harvester, a dim swollen sun behind smog.",
    "S1_Hunger": "An overcrowded megacity in crisis: endless towers packed together, huge desperate crowds queuing for food and water at a fenced "
                 "ration point, armed guards, smoke from riots and distant war fires on the skyline. Seen from above, sombre and human.",
    "S1_AcidSea": "A poisoned acidic ocean: sickly green-yellow water lapping a dead coastline, rusted ships stranded in toxic foam, "
                  "dead coral and bleached rocks, people with masks collecting the last clean rainwater in barrels.",
    "S2_Fleets": "The great exodus: a fleet of patched-together ships and floating towns made of scrap, sails and solar panels, "
                 "drifting across a dark ocean at dawn. Five separate communities, each ship group flying banners of a different colour "
                 "(teal, orange, gold, steel blue, green). Families on the decks looking at the horizon.",
    "S2_Search": "A lone scrap ship in a storm at night, lightning over huge waves, a lookout with a lantern scanning the horizon, "
                 "a hand-drawn map pinned in the wheelhouse window, feeling of years of searching.",
    "S3_Island": "The Last Island discovered at sunrise: a lush green island rising from a calm turquoise sea, waterfalls of fresh water, "
                 "fertile fields and fruit trees, ancient ruins on the hills, five small fleets approaching the bay with their colourful banners, hope.",
    "S4_Guardian": "An ancient colossal stone Guardian statue, hooded, standing on a hilltop of the island with a pillar of green light "
                   "rising from a stone altar into the dusk sky; five small champion silhouettes stand before it, each holding a different coloured banner.",
    "L_Loading": "Wide establishing shot of the Last Island at dusk seen from the sea: five ships anchored in a cove, warm lanterns, "
                 "a green beam of light rising from the island's highest point, calm reflective water, mysterious and inviting.",
    "T1_Numbers": "A dark circular chamber inside the island's ruins: an ancient round stone table with five glowing pedestals around it, "
                  "red holographic rings floating above the table, five figures standing in shadow, tension. No digits visible.",
    "T2_Symbols": "The grand lobby of a ruined luxury hotel swallowed by sand: marble columns, a broken chandelier, red carpets, "
                  "four glowing card-suit symbols (heart, diamond, club, spade) hovering in the dim air above shadowy figures.",
    "T3_Race": "Two rugged offroad buggies racing across the island's red desert dunes at golden hour, dust trails behind them, "
               "green flare markers along the route, a towering beam of green light on the horizon where the Guardian waits.",
    "T4_Choice": "Two paths before the Guardian at blood-red sunset: on the left a golden crown resting on a stone throne, "
                 "on the right a line of ships sailing away carrying a community to safety. A lone champion silhouette stands between them, deciding.",
    "I_Welcome": "Champions stepping off a wooden pier onto the golden beach of the Last Island, ships moored behind them, "
                 "a winding path leading up towards ruins and a distant green beam of light, warm late afternoon light.",
    # Epilogue: the truth about the island.
    "E_Scientist": "Decades before the collapse: an elderly scientist with grey hair and round glasses in a cluttered research lab at night, "
                   "glowing screens showing a burning ocean and failing crops, papers, globes and climate charts everywhere, "
                   "he studies the screens with grim certainty.",
    "E_Warning": "The same elderly scientist alone at a podium in a vast, nearly empty conference hall, rows of empty seats, "
                 "a giant screen behind him showing a red, dying Earth; the few people present turn and walk away, ignoring him.",
    "E_Ark": "A secret construction on a remote green island decades ago: engineers and machines carving terraces and fresh water reservoirs, "
             "a hidden seed vault with thousands of glowing seed capsules, the elderly scientist overseeing from a cliff with blueprints in hand.",
    "E_Keeper": "In a vast cavern beneath the island, the frail elderly scientist places his hand on a colossal hooded stone Guardian statue "
                "as ancient machinery wakes and green light kindles in its eyes and chest; his final work, reverent and quiet.",
    "E_Crown": "A lone champion silhouette standing on the island's highest hill at dusk under a faint crown of golden light, "
               "the Guardian's green beam behind, the sea below empty of ships, triumphant but lonely.",
    "E_Dawn": "Dawn over the Last Island: families stepping onto the beach from colourful patched ships, children running on the sand, "
              "the first gardens being planted, the hooded stone Guardian watching peacefully from the hill, hopeful and warm.",
}

def generate(model, key_id):
    path = os.path.join(OUT, key_id + ".png")
    body = {"model": model, "prompt": PROMPTS[key_id] + " " + STYLE, "size": "1536x1024", "quality": "high", "n": 1}
    req = urllib.request.Request("https://api.openai.com/v1/images/generations", data=json.dumps(body).encode(), method="POST",
                                 headers={"Authorization": "Bearer " + KEY, "Content-Type": "application/json"})
    for attempt in range(3):
        try:
            t = time.time()
            r = json.loads(urllib.request.urlopen(req, timeout=600).read())
            item = r["data"][0]
            if "b64_json" in item:
                data = base64.b64decode(item["b64_json"])
            else:
                data = urllib.request.urlopen(item["url"], timeout=120).read()
            open(path, "wb").write(data)
            return f"{key_id}: ok {len(data)//1024} KB in {int(time.time()-t)} s"
        except urllib.error.HTTPError as e:
            msg = e.read()[:300].decode(errors="replace")
            if e.code in (429, 500, 502, 503) and attempt < 2:
                time.sleep(20)
                continue
            return f"{key_id}: HTTP {e.code} {msg}"
        except Exception as e:
            if attempt < 2:
                time.sleep(10)
                continue
            return f"{key_id}: error {e}"

model = sys.argv[1]
ids = list(PROMPTS) if sys.argv[2] == "all" else sys.argv[2:]
with ThreadPoolExecutor(max_workers=4) as pool:
    for line in pool.map(lambda i: generate(model, i), ids):
        print(line, flush=True)
