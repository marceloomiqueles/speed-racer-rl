# speed-racer-rl

A reinforcement learning project built around a custom 2D top-down racing simulator written in C++.  
The environment uses LIDAR-style raycasts for perception and trains a Deep Q-Network (DQN) agent using libtorch.  
Training runs headless, while trained models can be replayed visually using Raylib.

This repository focuses on the learning system and simulator, not the standalone game implementation.

## Overview

The project consists of two main executables:

- **Training** (`racing_trainer.cpp`)  
  Headless reinforcement learning using a vanilla DQN agent.

- **Replay** (`racing_replay.cpp`)  
  Loads a trained model and visualizes behavior in real time.

The environment is fully custom, including physics, collision handling, checkpoint logic, and reward shaping.

## Environment

- 2D pixel-based racing track
- Custom physics (speed, friction/drag, steering, wall/grass collisions)
- Checkpoint + lap system
- Deterministic step-based simulation

Tracks and assets live in `assets/`.

## Observation Space

The agent observes an 18-dimensional state vector:

- normalized speed
- `sin(heading)`, `cos(heading)`
- normalized position `(x, y)`
- 13 LIDAR-style raycasts

Raycasts span −90° to +90° relative to the car’s heading and return a normalized danger value:

```text
danger = 1 / ((distance / reference_distance) + 0.1)
```

Values are clamped to `[0, 1]`.

## Action Space

Discrete action space with 7 actions:

0. accelerate forward  
1. reverse  
2. steer left  
3. steer right  
4. forward + left  
5. forward + right  
6. no input  

## Reward Structure

The reward function is shaped using:

- progress toward next checkpoint
- small speed incentive (scaled conservatively)
- checkpoint reward
- lap completion reward
- finish reward
- time penalty
- wall collision penalty
- grass penalty
- anti-idle penalty

Episodes may terminate early if the vehicle becomes stuck or stops making progress.

## Training Behavior

Observed training characteristics:

- early models tend to drive conservatively but complete laps reliably
- later models learn higher speeds but often degrade and crash
- fine-tuning from a stable checkpoint (lower learning rate + lower epsilon floor) significantly improves stability

Several example models are included for comparison.

## Repository Structure

```text
.
├── assets/              # Track images and environment assets
├── sampleModels/        # Example trained models
├── CMakeLists.txt
├── LICENSE
├── README.md
├── analyze_training.cpp # Training log analysis utilities
├── dqn.h                # DQN network and agent implementation
├── main.cpp             # Shared entry point / utilities
├── racing_replay.cpp    # Visual replay executable
├── racing_trainer.cpp   # Headless training executable
└── replay_buffer.h      # Experience replay buffer
```

## Building

The project uses CMake.

### Dependencies

- C++17 or newer
- Raylib
- libtorch (PyTorch C++)

### Build

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

This produces separate trainer and replay executables.

## Running Replay

To visualize a file from `sampleModels/`:

```bash
./racing_replay sampleModels/best_time.pt --track sandbox
```

Or use auto mode (loads `models/<track>/best_time.pt` and falls back to `../trainedModels/<track>/best_time.pt`):

```bash
./racing_replay --track sandbox
```

Replay runs in real time and renders the environment, vehicle, and perception rays.

## Training

To train a new agent:

```bash
./racing_trainer --track sandbox
```

To train with live visualization (slower):

```bash
./racing_trainer --track sandbox --render
```

To train with live visualization and short trajectory trace overlay (last ~100 steps):

```bash
./racing_trainer --track sandbox --render --render-trace
```

Trace colors:
- blue: normal driving
- orange: grass frames
- red: wall-hit frames

To reset a track training run (checkpoints + scheduler state) and restart from episode 1:

```bash
./racing_trainer --track sandbox --reset-training
```

Training runs headless and periodically saves model checkpoints.
Checkpoints are stored per track in `build/models/<track_name>/` (for example `build/models/sandbox/best_time.pt`).
Best-time checkpoints are also exported outside `build` to `trainedModels/<track_name>/best_time.pt`.
Training scheduler state (`episode_next`, `epsilon`, learning-rate schedule) is saved in `build/models/<track_name>/training_state.txt`.

Exact behavior (episode length, epsilon schedule, learning rate, etc.) is defined in code and can be adjusted in `racing_trainer.cpp`.

## Tracks

Track definitions are centralized in `track_config.h` (name, image path, spawn, checkpoints, viewport).

- Default track: `sandbox`
- Replay CLI: `./racing_replay <model_path> [--track <track_name>]`
- Trainer CLI: `./racing_trainer [--milestone <episodes>] [--track <track_name>] [--render] [--render-trace] [--reset-training]`

F1 tracks are generated from `f1-circuits.geojson` with:

```bash
./scripts/generate_f1_assets.py
```

This generates:

- `assets/tracks/<slug>.png`
- `generated/track_layouts.json` (spawn + checkpoints)
- `generated/track_layouts_generated.h` (compiled into `track_config.h`)

To regenerate from scratch, ensure `/tmp/f1-circuits.geojson` is available and rerun the script.

To refresh normalized lap records in `tracks_manifest.json`:

```bash
./scripts/update_lap_records.py
```

## Sample Models

The `sampleModels/` directory contains trained checkpoints from different stages of learning.

Typical progression:

- early episodes: safe but slow driving
- mid training: inconsistent lap completion
- fine-tuned models: stable multi-lap behavior

These models are included for demonstration and replay purposes.
