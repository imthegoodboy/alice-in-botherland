"""Generate narrator lines with ElevenLabs (rotating keys from .env), convert to normalised mono WAV, record durations.
usage: gen_voice.py [ids...]   (default: every line in SourceAssets/Audio/narration.json that has no WAV yet)"""
import json, os, subprocess, sys, urllib.error, urllib.request

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FFMPEG = os.environ.get("FFMPEG", "ffmpeg")
VOICE = "Xb7hH8MSUJpSbSDYk0k2"  # Alice - clear, engaging educator (British)
OUT = os.path.join(ROOT, "SourceAssets", "Audio", "VO")
env = {}
for line in open(os.path.join(ROOT, ".env"), encoding="utf-8", errors="replace"):
    if "=" in line and not line.strip().startswith("#"):
        k, v = line.split("=", 1)
        env[k.strip()] = v.strip().strip('"').strip("'")
keys = [env[k] for k in ("ELEVENLABS_API_KEY1", "ELEVENLABS_API_KEY2", "ELEVENLABS_API_KEY3", "ELEVENLABS_API_KEY4", "ELEVENLABS_API_KEY") if env.get(k)]
script = json.load(open(os.path.join(ROOT, "SourceAssets", "Audio", "narration.json"), encoding="utf-8"))["lines"]
ids = sys.argv[1:] or [i for i in script if not os.path.exists(os.path.join(OUT, i + ".wav"))]
key_index = 0

def tts(text):
    global key_index
    body = {"text": text, "model_id": "eleven_multilingual_v2",
            "voice_settings": {"stability": 0.55, "similarity_boost": 0.8, "style": 0.25, "use_speaker_boost": True}}
    while key_index < len(keys):
        req = urllib.request.Request(f"https://api.elevenlabs.io/v1/text-to-speech/{VOICE}?output_format=mp3_44100_128",
                                     data=json.dumps(body).encode(), method="POST",
                                     headers={"xi-api-key": keys[key_index], "Content-Type": "application/json", "Accept": "audio/mpeg"})
        try:
            return urllib.request.urlopen(req, timeout=120).read()
        except urllib.error.HTTPError as e:
            msg = e.read()[:200].decode(errors="replace")
            if e.code in (401, 402, 403, 429) or "quota" in msg.lower():
                print(f"  key #{key_index + 1} unavailable ({e.code}), switching", flush=True)
                key_index += 1
                continue
            raise RuntimeError(f"HTTP {e.code} {msg}")
    raise RuntimeError("all ElevenLabs keys exhausted")

for line_id in ids:
    mp3 = os.path.join(OUT, line_id + ".mp3")
    wav = os.path.join(OUT, line_id + ".wav")
    open(mp3, "wb").write(tts(script[line_id]))
    # Mono 48 kHz, gentle high-pass, loudness-normalised to match the other voice lines, 0.15 s lead-in silence.
    subprocess.run([FFMPEG, "-y", "-loglevel", "error", "-i", mp3, "-af",
                    "highpass=f=70,loudnorm=I=-17:TP=-1.5:LRA=9,adelay=150", "-ac", "1", "-ar", "48000", "-c:a", "pcm_s16le", wav], check=True)
    os.remove(mp3)
    print("ok", line_id, flush=True)

# Durations for every narrator line (seconds), used to time captions and phases.
durations = {}
for line_id in script:
    wav = os.path.join(OUT, line_id + ".wav")
    if os.path.exists(wav):
        out = subprocess.run([os.environ.get("FFPROBE", "ffprobe"), "-v", "error", "-show_entries", "format=duration", "-of", "csv=p=0", wav],
                             capture_output=True, text=True).stdout.strip()
        durations[line_id] = round(float(out), 2)
json.dump(durations, open(os.path.join(ROOT, "SourceAssets", "Audio", "narration_durations.json"), "w"), indent=1)
print("durations:", len(durations), "total", round(sum(durations.values()), 1), "s")
