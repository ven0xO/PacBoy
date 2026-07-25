cmake_minimum_required(VERSION 3.3)
project(PacBoy VERSION 0.1.0)

# C++ 17 as standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# exit path
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# set as .exe
set(CMAKE_EXECUTABLE_SUFFIX ".exe")

# required libraries
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# sources
set(SOURCES 
    src/main.cpp
    external/GLAD/glad.c
    src/Grid.cpp
    src/Player.cpp
    src/Enemy.cpp
    src/GameState.cpp
    src/Hud.cpp
    src/PixelFont.cpp
    src/Game.cpp
)

# headers
include_directories(
    ${CMAKE_SOURCE_DIR}/external/GLAD/include
    ${CMAKE_SOURCE_DIR}/external
)

# create exe
add_executable(${PROJECT_NAME} ${SOURCES})

# linking
target_link_libraries(${PROJECT_NAME} OpenGL::GL glfw dl)

# shaders
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_SOURCE_DIR})
