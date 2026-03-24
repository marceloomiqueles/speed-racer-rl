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
When replay sets a better AI lap time for a track, it updates `tracks_ai_records.csv`
with the best `time_seconds`, formatted time, model name, UTC timestamp, and
`clean_lap` (`true` if that lap had no wall hits).

## Training

To train a new agent:

```bash
./racing_trainer --track sandbox --profile base
```

Apple Silicon (M1/M2/M3) performance notes:
- Build in `Release` mode (`-O3` + LTO enabled in `CMakeLists.txt`)
- Trainer uses all CPU threads by default
- You can override LibTorch threads with env var `RACING_TORCH_THREADS`
- M1 short sweep recommendation (this repo): `RACING_TORCH_THREADS=8`

Example:

```bash
RACING_TORCH_THREADS=8 ./racing_trainer --track sandbox --profile base
```

To fine-tune a track from a base checkpoint:

```bash
./racing_trainer --track australian-gp --profile finetune --init-model ../trainedModels/base/best.pt
```

`--profile auto` chooses `base` for `sandbox` and `finetune` for non-sandbox tracks.

To enable automatic curriculum stages:

```bash
./racing_trainer --track sandbox --profile base --curriculum auto
```

Curriculum stages (`auto`) advance on milestone evaluations:
- `drive` -> `drive_strict` when driving is safe and race-oriented (`avg_wall_hits <= 0.80`, `lap_gt1_rate >= 20%`, and `avg_steps_all <= 2600`) for `4` consecutive evals
- `drive_strict` -> `clean` when it starts finishing with strict wall discipline (`finish_rate >= 20%`, `avg_wall_hits <= 0.15`, and `avg_steps_all <= 2300`) for `2` consecutive evals
- `clean` -> `pace` when consistency and pace are both good (`finish_rate >= 70%`, `avg_wall_hits <= 0.50`, and `avg_steps_all <= 2400`)
- `pace` -> `corner` when finish pace improves consistently
Reward shaping prioritizes completing the full 3-lap race: `finish_reward` and `lap_reward` are weighted above speed-only incentives.

Wall-hit DNF policy (training):
- `drive`: DNF after `30` wall hits (exploration stage)
- `drive_strict`: DNF after `3` wall hits
- `clean`, `pace`, `corner`: DNF on first wall hit

Greedy milestone evaluation wall-hit policy:
- `drive`: DNF after `30` wall hits
- `drive_strict`: DNF after `3` wall hits
- `drive_strict`, `clean`, `pace`, `corner`: DNF on first wall hit

Optional top-speed reward (off by default):

```bash
./racing_trainer --track sandbox --reward-top-speed
```

Recommended safe start values:

```bash
./racing_trainer --track sandbox --reward-top-speed \
  --reward-top-speed-gain 0.003 \
  --reward-top-speed-peak-bonus 0.30
```

Top-speed reward is only applied when there is forward checkpoint progress and no wall hit on that step.
With `--curriculum auto`, top-speed reward is activated automatically in later stages
(`pace` and `corner`) and stays off in early stability stages (`drive`, `clean`).

### Episode Budget and Evaluation

Recommended episode budgets (practical ranges):
- Base (`sandbox`, profile `base`): `3,000` to `8,000` episodes
- Track fine-tune (`finetune` + `--init-model`): `500` to `2,500` episodes per track

Evaluation cadence:
- Keep `--milestone 50` (default) so every 50 episodes you get a greedy eval (`20` episodes)
- Do not tune hyperparameters before at least `1,000` episodes unless there is an obvious bug

Primary metrics to watch:
- `finish_rate` (first priority)
- `avg_wall_hits` (safety / clean driving)
- `avg_steps_finish` (pace proxy; lower is better)
- `avg_score` (overall trend, secondary)

Decision rules:
- Continue training when metrics are still improving every 2-3 milestone evals
- Stop a run when `finish_rate >= 70%` is stable for 2-3 evals and `avg_steps_finish` improvement is <5%
- Consider reward/profile adjustments when after `1,000-3,000` episodes you still have `finish_rate = 0%` or flat safety/pace metrics

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

To render LIDAR rays in trainer:

```bash
./racing_trainer --track sandbox --render --render-lidar
```

In render mode, press `L` to toggle LIDAR visibility.
Trainer HUD also shows the current AI lap record for the selected track and updates
`tracks_ai_records.csv` when a better lap is achieved during training, including
the `clean_lap` field.

To reset a track training run (checkpoints + scheduler state) and restart from episode 1:

```bash
./racing_trainer --track sandbox --reset-training
```

`--reset-training` resets only the active profile (`base` or `finetune`) for that track.

Training runs headless and periodically saves model checkpoints.
Checkpoints are stored per track and profile in `build/models/<track_name>/<profile>/` (for example `build/models/sandbox/base/best_time.pt`).
Best-time checkpoints are also exported outside `build` to `trainedModels/<track_name>/<profile>/best_time.pt`.
For replay compatibility, fine-tune best-time is mirrored to `build/models/<track_name>/best_time.pt` and `trainedModels/<track_name>/best_time.pt`.
Training scheduler state (`episode_next`, `epsilon`, learning-rate schedule) is saved in `build/models/<track_name>/<profile>/training_state.txt`.
Per-track runtime overrides can be set in `track_overrides.csv` (for example `australian-gp,0.08` to shrink the car sprite on that track) without recompiling.

Exact behavior (episode length, epsilon schedule, learning rate, etc.) is defined in code and can be adjusted in `racing_trainer.cpp`.

## Tracks

Track definitions are centralized in `track_config.h` (name, image path, spawn, checkpoints, viewport).

- Default track: `sandbox`
- Replay CLI: `./racing_replay <model_path> [--track <track_name>]`
- Trainer CLI: `./racing_trainer [--milestone <episodes>] [--track <track_name>] [--profile auto|base|finetune] [--init-model <path>] [--curriculum off|auto] [--render] [--render-trace] [--render-lidar] [--reward-top-speed] [--reward-top-speed-gain <float>] [--reward-top-speed-peak-bonus <float>] [--reset-training]`

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
