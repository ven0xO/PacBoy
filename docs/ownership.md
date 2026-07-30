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

`getGrid()`, `getPlayer()`, and `getEnemies()` return non-owning references.
The grid reference is always valid while its `Game` object is alive. Player
and enemy references may only be requested after `isInitialized()` returns
`true`.

## Player

`Player` does not own `GameState`.

Its `GameState&` is a required, non-owning reference. It cannot be null or
reassigned. The referenced state must remain alive for the entire lifetime
of `Player`; `Game` guarantees this in normal gameplay.

## Enemy

`Enemy` does not own `Grid`, `Player`, or the red ghost.

`Grid&` and `Player&` are required, non-owning references. They cannot be
null or reassigned. Both referenced objects must outlive `Enemy`; `Game`
guarantees this in normal gameplay.

`red_ghost` is an optional `const Enemy*` because only the blue ghost uses
this relationship. `Game` assigns the red ghost before the blue ghost is
updated and keeps it alive for the entire lifetime of the blue ghost. The
other ghost types legitimately leave this pointer null.

## GameRenderer

`GameRenderer` owns `Hud`.

It does not own `Game`, the world shader, or the shared cube VAO.
These objects are borrowed only for the duration of `render()`.

## Remaining raw pointers

- `GLFWwindow*` is required by GLFW's C API. `main` owns the window and
  destroys it with GLFW; input functions and callbacks only borrow it.
- The `CameraController*` retrieved from GLFW's window user pointer may be
  null, so every callback checks it before use.
- `PixelFont::findGlyph()` returns a nullable pointer to a glyph stored in
  the static font table. The caller borrows it and must not delete it.
- Pointer-shaped arguments used as OpenGL buffer offsets belong to the
  OpenGL C API and do not represent C++ object ownership.

## Destruction order

The required destruction order is:

1. stop the game loop,
2. destroy VAO, VBO, EBO, shaders, HUD, and `GameRenderer`,
3. destroy `Game`,
4. destroy the GLFW window,
5. call `glfwTerminate()`.
