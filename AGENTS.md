# AGENTS.md

## Purpose

This repository contains a custom 2D top-down racing reinforcement learning environment implemented in C++ with LibTorch and Raylib.

Core characteristics of the repo:
- C++17+ with CMake build system
- headless training executable: `racing_trainer`
- visual replay executable: `racing_replay`
- custom environment with pixel-based track, handcrafted physics, raycast perception, checkpoint/lap system, and reward shaping
- assets stored in `assets/`
- sample trained models stored in `sampleModels/`

Agents working in this repository must preserve the current architecture and optimize for correctness, determinism, and maintainability over abstraction for abstraction’s sake.

---

## Source of truth

When making decisions, use this priority order:

1. Existing code in the repository
2. `README.md`
3. `CMakeLists.txt`
4. This `AGENTS.md`

Do not invent architecture that is not already present in the repo.

---

## Repository shape

Current repository structure:

- `racing_trainer.cpp`: headless RL training loop
- `racing_replay.cpp`: real-time visual replay
- `dqn.h`: DQN network and agent logic
- `replay_buffer.h`: replay buffer implementation
- `analyze_training.cpp`: utilities for training analysis
- `main.cpp`: shared entry point / utilities
- `assets/`: track and rendering assets
- `sampleModels/`: model checkpoints for replay/testing

Agents must respect the current split between:
- training concerns
- replay/rendering concerns
- model/agent logic
- supporting utilities

Do not collapse these responsibilities into one file unless explicitly requested.

---

## Non-goals

Unless explicitly requested, do not:
- rewrite the project into another language
- introduce a game engine
- replace DQN with a different RL algorithm
- convert the project into a general-purpose framework
- add heavy abstraction layers for future-proofing
- move core runtime parameters into config files if the repo currently keeps them in code, unless the task explicitly asks for that refactor

This project is a focused hobby/research simulator, not an enterprise platform.

---

## Coding principles

### General
- Prefer direct, readable C++ over clever metaprogramming.
- Keep the code easy to inspect and debug.
- Preserve deterministic simulation behavior where possible.
- Make small, scoped changes.
- Avoid hidden side effects.

### Architecture
- Keep simulation logic separate from rendering logic.
- Keep agent/model logic separate from environment stepping.
- Keep training-only behavior out of replay code.
- Keep replay-only rendering/UI behavior out of trainer code.

### Dependencies
- Do not add new dependencies lightly.
- Prefer the current stack:
  - CMake
  - Raylib
  - LibTorch
- Any new dependency must have a clear justification tied to the requested task.

### Performance
- Avoid unnecessary heap allocations inside per-frame or per-step hot paths.
- Be careful with copies of tensors, state vectors, images, and replay data.
- Prefer predictable runtime behavior over premature micro-optimizations.

---

## Build and run expectations

The project builds with CMake and depends on:
- C++17 or newer
- Raylib
- LibTorch

Agents should assume the standard flow is:

```bash
mkdir -p build
cd build
cmake ..
cmake --build . --config Release
