# LofiWatch

Pebble Time Steel watchface — lo-fi cozy night scene.

## CloudPebble Setup

### Source
- `src/main.c`

### Resources

| Name | File | Type | Notes |
|---|---|---|---|
| `IMG_LOFI_BG` | `images/lofi_bg.png` | bitmap | 144×168 background |
| `FONT_DS_20` | `fonts/DS-DIGI.TTF` | font size 20 | Time HH:MM |
| `FONT_DS_7` | `fonts/DS-DIGI.TTF` | font size 7 | Date MM/DD |

### Clock screen area (144×168)
- Top-left: (71, 116)
- Bottom-right: (134, 136)
- Size: 63×20px

Time drawn left side, date drawn right side, both inside the clock screen.
