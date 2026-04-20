# CSE472 Project 2: Interactive 2D Water Ripple Simulator

This project implements a real-time 2D water ripple simulator in MFC/C++.

## Features

- Discrete height-field wave simulation using a finite-difference update.
- Fixed boundary conditions.
- Reflective internal obstacles with multiple preset layouts.
- Interactive disturbances by mouse clicks.
- Runtime obstacle editing (add/remove while running).
- Per-cell normal reconstruction from height gradients.
- Real-time lighting-based water shading.
- Alternate reflective-tint rendering mode.
- Disturbance presets: manual, rain, line splash, pulse source.

## Build and Run (Visual Studio)

1. Open `Project2/Project2.sln` in Visual Studio (Windows).
2. Select `Debug` or `Release` and build the solution.
3. Run the `Project2` target.

## Controls

- `Left Click`: Add water disturbance (or add obstacle in obstacle edit mode)
- `Right Click`: Add negative disturbance (or remove obstacle in obstacle edit mode)
- `Space`: Pause/Resume simulation
- `R`: Reset water state
- `L`: Cycle obstacle layout (`None`, `Pillars`, `Circles`, `Channel`)
- `O`: Toggle obstacle edit mode
- `M`: Toggle rendering mode (`Lit water` / `Reflective tint`)
- `Q / A`: Increase/Decrease wave speed
- `W / S`: Increase/Decrease damping
- `E / D`: Increase/Decrease disturbance radius
- `1`: Manual click disturbances
- `2`: Rain preset
- `3`: Line splash preset
- `4`: Pulse source preset

## Implementation Notes

- Wave update uses three height buffers (`prev`, `curr`, `next`):
  - `next = (2 * curr - prev + waveSpeed * Laplacian(curr)) * damping`
- Obstacles are treated as reflective by mirroring neighboring samples in the Laplacian calculation.
- Surface normals are reconstructed from finite differences of the height field and used for diffuse + specular lighting.
