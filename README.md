# ESPHome Voice Assistant for the Guition JC3636K518C (K5 Knob)

A full-featured **Home Assistant Voice Assistant** for the **Guition JC3636K518C**
/ Waveshare ESP32-S3-Knob-Touch-LCD-1.8 — 1.8" round 360×360 touch display with a
**quadrature rotary knob**, **3.5 mm audio jack** (no built-in speaker), I²S microphone,
and **DRV2605 haptic** motor (no WS2812 LED ring).

Ported from [esphome-guition-jc3636k718c-va](https://github.com/MichalZaniewicz/esphome-guition-jc3636k718c-va)
(K718C hardware layer replaced for K518 pins and haptic feedback).

## Hardware notes (K518)

- **Knob:** quadrature encoder GPIO8 (A) / GPIO7 (B) — `rotary_encoder`, not K718 pulse pins.
- **Audio:** plug headphones/speakers into the **3.5 mm jack** for TTS and media.
- **Haptic:** DRV2605 on I²C (0x5A) replaces the K718 LED ring; Settings → **Haptic**.
- **First flash:** USB only (16 MB `partitions.csv`); flash the **ESP32-S3**, not the secondary MCU.

## Quick start

1. Copy `secrets.example.yaml` → `secrets.yaml` and fill in Wi‑Fi.
2. Edit `guition-va.yaml` (`substitutions:` for HA entities, `github_user` for your fork).
3. **In-repo build:** `packages:` uses local `files:` (default). **Thin deploy** from another
   folder: uncomment `url` / `ref` in `guition-va.yaml` so firmware pulls from your GitHub.
4. Generate sounds (once): `python scripts/make_sounds.py`
5. `esphome run guition-va.yaml` (first time over USB).

## Repository layout

```
guition-va.yaml          # thin config (entities + package list)
partitions.csv           # 16 MB dual-slot (local path required)
secrets.yaml             # Wi‑Fi (not committed)
base/core.yaml           # K518 hardware + VA + LVGL core
base/screens/*.yaml      # optional carousel screens
assets/sounds/           # wake.wav, alarm.wav
```

## GitHub packages

Set in `guition-va.yaml`:

```yaml
substitutions:
  github_user: "your-github-login"
  github_repo: "esphome-guition-jc3636k518c-va"
  github_ref: main

packages:
  core:
    url: https://github.com/${github_user}/${github_repo}
    ref: ${github_ref}
    files:
      - base/core.yaml
      # ...
```

After pushing this repo, copy only `guition-va.yaml`, `partitions.csv`, and `secrets.yaml`
to a working directory and run `esphome run guition-va.yaml`.

## Credits

- Original VA firmware architecture: Michal Zaniewicz (JC3636K718C).
- K518 pinout / init sequence: manufacturer demo `JC3636K518CN_knob_EN`, [ESPHome devices](https://devices.esphome.io/devices/ESP32S3-1.8-inch-JC3636K518C/).
- DRV2605 component: [RAR/esphome-drv2605](https://github.com/RAR/esphome-drv2605).
