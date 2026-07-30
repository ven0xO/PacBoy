# Ownership and lifetime rules

## Main application

`main` owns:

- the GLFW window and OpenGL context,
- the world shader,
- the shared cube VAO, VBO, and EBO,
- the `Game` object,
- the `GameRenderer` object.

The OpenGL context must exist before shaders and renderers are created.
All OpenGL resources must be destroyed before the GLFW window and context.

## Game

`Game` owns:

- `Grid`,
- `GameState`,
- `Scoreboard`,
- `Player`,
- all four `Enemy` objects.

`Game` contains gameplay state and rules only. It does not own rendering
resources and does not depend on OpenGL or GLFW.

## Player

`Player` does not own `GameState`.

Its `GameState*` is a non-owning pointer. The `GameState` owned by `Game`
must remain alive for the entire lifetime of `Player`.

## Enemy

`Enemy` does not own `Grid`, `Player`, or the red ghost.

These are non-owning pointers. `Game` guarantees that these objects remain
alive while enemies are being updated.

## GameRenderer

`GameRenderer` owns `Hud`.

It does not own `Game`, the world shader, or the shared cube VAO.
These objects are borrowed only for the duration of `render()`.

## Destruction order

The required destruction order is:

1. stop the game loop,
2. destroy VAO, VBO, EBO, shaders, HUD, and `GameRenderer`,
3. destroy `Game`,
4. destroy the GLFW window,
5. call `glfwTerminate()`.
