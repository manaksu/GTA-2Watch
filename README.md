# GTA2Watch

A Pebble Time Steel watchface inspired by the GTA2 "Level Complete" score screen.

## Features

- **Time** — Orbitron ExtraBold in a tight stadium border. Last digit in accent colour.
- **Date bar** — Blue pill banner, bleeds off the right edge of the screen.
- **Score rows** — PLAYER ONE (steps as high score) + two EMPTY slots.
- **Health stats** — Heart rate, calories, active mins, battery/steps, recovery. Centre-aligned, values in accent colour.
- **Menu** — PLAY NEXT LEVEL / SAVE CURRENT GAME / BACK TO MAIN MENU. First item in accent colour.

## Settings

| Setting | Options |
|---|---|
| Date Bar | Hide / Show |
| Menu Lines | Hide / Show |
| Stat Line 4 | Battery % / Steps count |
| Accent Color | Red / Gold |

## CloudPebble Setup

### Source files
- `src/main.c`
- `src/js/pebble-js-app.js`

### Font resources (add each separately)

| Resource name | File | Size |
|---|---|---|
| `FONT_ORB_EB_24` | Orbitron-ExtraBold.ttf | 24 |
| `FONT_ORB_BD_8` | Orbitron-Bold.ttf | 8 |
| `FONT_ORB_EB_8` | Orbitron-ExtraBold.ttf | 8 |

### Project settings
- Platform: **Basalt only**
- SDK: 3
- Type: Watchface
- Capabilities: configurable

## Memory estimate
- FONT_ORB_EB_24 (digits only `[0-9: ]`) ≈ 3.5 KB
- FONT_ORB_BD_8  (A-Z, 0-9, punctuation) ≈ 2.5 KB
- FONT_ORB_EB_8  (A-Z, 0-9, space) ≈ 2.0 KB
- Canvas + buffers ≈ 4 KB
- **Total ≈ ~12 KB** — well within 24 KB heap limit
