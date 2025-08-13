# DR_CursorTracker (Crosshair Tracker)

### [**Language:English | [簡介語言:繁體中文](README.md)**]

DR_CursorTracker is an OBS Studio video source plugin that visualizes the mouse crosshair and movement trail in real time.
It provides low-overhead rendering and a context-aware settings UI, ideal for tutorials, game streaming, and product/UI demos.

### [**[Settings Guide](SETTINGS_GUIDE_EN.md)**]

![Preview](/images/show_2.jpg)

## Highlights
- Crosshair modes: Movement (delta + rebound), Coordinate (follow screen cursor)
- Tracking modes: Linear (line from center), Path (small circles along the trail)
- Visuals: Custom image crosshair, circle, and box (each toggleable)
- Smart UI: Shows only relevant options based on current selections/modes
- Motion controls: Rebound speed, center/outer move speeds, sensitivity, idle recenter boost
- Performance: Pre-generated alpha textures and caching
- Localization: zh-TW, en-US, ja-JP

## Use Cases
- Step-by-step tutorials and mouse trail display
- Crosshair/aim visualization in game streaming
- Product demos and UI/UX behavior showcases

## Installation
1. Download or extract the plugin zip (it contains `data/` and `obs-plugins/`).
2. Copy these two folders directly into the OBS Studio installation root.
   - Example: `C:/Program Files/obs-studio/`
3. Restart OBS and add “Crosshair Tracker” from the Add Source dialog.

> Platform: Windows (OBS Studio x64)

## Quick Start
- Switch crosshair mode (Movement/Coordinate) and tracking mode (Linear/Path) in Source Properties.
- Use a custom image as the crosshair (irrelevant options auto-hide).
- Toggle Box and Circle independently.
- Fine-tune rebound speed, sensitivity, and idle acceleration to your preference.

## License & Credits
- Copyright © Demon Realm / Godpriest

## AI Assistance
Portions of this plugin’s code and documentation were assisted by AI. All final content was reviewed and integrated by the author.

