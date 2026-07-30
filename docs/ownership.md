# Ownership and lifetime rules

## Main application

`main` owns:

- the GLFW lifetime, window, and OpenGL context,
- the world shader,
- the shared cube VAO, VBO, and EBO,
- the `Game` object,
- the `GameRenderer` object,
- the `CameraController`,
- the `AudioManager`,
- the `FrameTimer`.

The OpenGL context must exist before shaders and renderers are created.
All OpenGL resources must be destroyed before the GLFW window and context.
`GlfwGuard` is created before the objects that use GLFW or OpenGL, so its
destructor calls `glfwTerminate()` only after those objects have been
destroyed. The current implementation relies on `glfwTerminate()` to destroy
the remaining window instead of calling `glfwDestroyWindow()` explicitly.

## Game

`Game` owns:

- `Grid`,
- `GameState`,
- `Scoreboard`,
- `Player`,
- all four `Enemy` objects.

`Game` contains gameplay state and rules only. It does not own rendering
or audio resources and does not depend on OpenGL, GLFW, or miniaudio.

`Game` receives backend-independent `GameInput` values. It queues semantic
`GameEvent` values for the application layer, and `takeEvents()` transfers and
clears that queue. This lets gameplay tests run without a window, an OpenGL
context, or an audio device.

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
Cached shader IDs and uniform locations are non-owning OpenGL handles.

## Hud

`Hud` owns its shader, VAO, and VBO. Its destructor deletes the buffers, and
the owned `Shader` deletes its program. These destructors must run while the
OpenGL context still exists.

## AudioManager

`AudioManager` owns its private implementation through `std::unique_ptr`.
The implementation owns the miniaudio engine and all preloaded sounds. Its
destructor releases every initialized sound before shutting down the engine.

`AudioManager` does not inspect gameplay state. The application loop retrieves
`GameEvent` values from `Game` and passes each event to `AudioManager::play()`.

## Shader

`Shader` owns the OpenGL program stored in `ID`. It is non-copyable and deletes
the program in its destructor. A zero `ID` represents failed initialization
and owns no program.

## Remaining raw pointers

- `GLFWwindow*` is required by GLFW's C API. `main` owns the window and
  GLFW lifetime; input functions and callbacks only borrow the window.
- The `CameraController*` retrieved from GLFW's window user pointer may be
  null, so every callback checks it before use.
- `PixelFont::findGlyph()` returns a nullable pointer to a glyph stored in
  the static font table. The caller borrows it and must not delete it.
- Pointer-shaped arguments used as OpenGL buffer offsets belong to the
  OpenGL C API and do not represent C++ object ownership.

## Destruction order

The required destruction order is:

1. stop the game loop,
2. delete the shared cube VAO, VBO, and EBO,
3. destroy `AudioManager`, shaders, `GameRenderer`, `Hud`, and `Game`,
4. let `GlfwGuard` call `glfwTerminate()`, which also destroys the remaining
   window and OpenGL context.
