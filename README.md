# GTA2 Inspired Map Watchface

Pebble Time Steel (basalt) watchface using the GTA2 Industrial Sector map as background.

- Platform: basalt (Pebble Time / Pebble Time Steel)
- Resolution: 144x168
- Time displayed in bottom-right corner (yellow, LECO font)
- Date displayed above time (white, Gothic 14)

## CloudPebble Setup

1. New project → Pebble C SDK → basalt
2. Add `src/main.c`
3. Upload `resources/images/gta2map.png` as:
   - Type: Bitmap
   - Name: IMAGE_GTA2MAP
   - File: images/gta2map.png

## Local SDK Build

```bash
pebble build
pebble install --emulator basalt
```
