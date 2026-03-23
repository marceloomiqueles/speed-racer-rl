# AGENTS.md

## Propósito del repositorio

Este repositorio es un proyecto de RL de carreras 2D top-down en C++.

Stack principal:
- C++17
- CMake
- LibTorch
- Raylib

Binarios principales:
- `racing_trainer`
- `racing_replay`

## Reglas de trabajo

- Haz cambios acotados y enfocados.
- Conserva la estructura actual del proyecto, salvo que la tarea pida explícitamente un refactor.
- Prefiere modificar archivos existentes antes que introducir nuevas abstracciones.
- No reescribas el proyecto en otro lenguaje.
- No reemplaces DQN ni rediseñes el simulador, salvo que se pida explícitamente.
- Mantén claramente separadas las responsabilidades del trainer, del replay y de la lógica compartida.

## Build

Build local típico:

```
mkdir -p build
cd build
cmake ..
cmake --build . --config Release
```

Si LibTorch o raylib están instalados en ubicaciones no estándar, pasa las variables de prefijo apropiadas a CMake.

## Ejecución

Ejecución típica de replay desde `build/`:

```
./racing_replay ../sampleModels/best_time.pt
```

Ejecución típica de trainer desde `build/`:

```
./racing_trainer
```

## Validación

Después de cambios en el código, valida lo que corresponda:

- que la configuración de CMake siga funcionando
- que el proyecto siga compilando
- que el replay siga iniciando si cambió código de replay
- que el trainer siga iniciando si cambió código de trainer

No afirmes mejoras de entrenamiento sin evidencia.

## Notas sobre el sistema de pistas

El manejo actual de pistas no es completamente configurable.

Asume que los siguientes elementos hoy están acoplados a la pista activa y pueden requerir cambios coordinados:
- ruta del asset de pista
- posición inicial
- ángulo inicial
- checkpoints
- supuestos de colisión/superficie

Si agregas soporte para nuevas pistas, conserva compatibilidad hacia atrás con el comportamiento por defecto actual cuando sea posible.

## Compatibilidad de modelos

Ten cuidado al cambiar:
- arquitectura de la red
- shapes de tensores
- lógica de serialización/carga

Si cambia la compatibilidad de checkpoints, indícalo explícitamente.

## Guía por archivo

Propiedad preferida por archivo:
- `racing_trainer.cpp`: comportamiento del loop de entrenamiento
- `racing_replay.cpp`: comportamiento de replay/render
- `dqn.h`: red y lógica del agente
- `replay_buffer.h`: lógica del replay buffer
- `assets/`: pistas y assets visuales
- `sampleModels/`: checkpoints de ejemplo

Mantén los cambios en el archivo que ya es dueño de ese comportamiento.

## Expectativas de respuesta

Al hacer cambios, indica claramente:
- qué archivos cambiaron
- si el cambio afecta al trainer, al replay o a ambos
- si cambió la compatibilidad de modelos
- si cambió la compatibilidad de pistas
- cómo ejecutar una verificación manual rápida
