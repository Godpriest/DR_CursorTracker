# DR_CursorTracker Settings Guide (English)

Below is an overview of all settings. The UI dynamically hides irrelevant options based on your selections/modes.

## Box Settings
- **Show Box**: Toggle box rendering. Box Size stays editable even when off (for layout planning).
- **Box Size**: Side length in pixels.
- **Box Color**: ARGB color of the box.
- **Box Thickness**: Border width in pixels.
- **Box Alpha**: 0.0–1.0 transparency.

## Crosshair Settings
- **Crosshair Mode**:
  - **Movement Mode**: Driven by mouse delta with rebound behavior.
  - **Coordinate Mode**: Follows the on-screen mouse position.
- **Max Offset**: Maximum distance from the center (px).
- **Use Custom Image**: Use an image as the crosshair; hides built‑in crosshair options and the entire Circle Settings group.
- **Crosshair Image**: File path to the custom image.
- Built‑in crosshair (only when not using a custom image):
  - **Crosshair Cross Length**: Arm length (px).
  - **Crosshair Color**: ARGB color.
  - **Crosshair Thickness**: Line width (px).
  - **Crosshair Alpha**: 0.0–1.0.

## Circle Settings (hidden when using a custom image)
- **Circle Color**: ARGB color of the outer circle.
- **Circle Thickness (pixels)**: Circle line width.
- **Circle Radius (pixels)**: Circle radius.
- **Circle Alpha**: 0.0–1.0.

## Tracking Line Settings
- **Show Tracking Line**: Toggle the tracking line.
- **Tracking Line Mode**:
  - **Linear Mode**: A line from center to the crosshair offset. Shows:
    - **Tracking Line Color**
    - **Tracking Line Thickness**
    - **Tracking Line Alpha**
  - **Path Mode**: Generates small circles along the movement path. Shows:
    - **Path Circle Color**
    - **Path Circle Radius**
    - **Path Generation Interval (pixels)**
    - **Path Lifetime (seconds)**

## Speed Settings
- **Rebound Speed**: Overall speed scale to recenter.
- **Center Rebound Speed**: Speed near the center.
- **Outer Rebound Speed**: Speed near the outer range.
- **Crosshair Sensitivity**: Sensitivity to mouse movement.
- **Crosshair Center Move Speed**: Move speed multiplier near center.
- **Crosshair Outer Move Speed**: Move speed multiplier near outer range.
- **Enable Idle Recenter**: When idle, rebound accelerates after a delay.
  - **Idle Recenter Delay (seconds)**: Idle time before acceleration starts.
  - **Idle Recenter Time (seconds)**: Time to reach maximum acceleration.
  - **Idle Recenter Boost**: Maximum added speed.

 
