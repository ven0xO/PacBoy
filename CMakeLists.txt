cmake_minimum_required(VERSION 3.14)
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

include(FetchContent)

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)

FetchContent_MakeAvailable(json)

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
    src/Scoreboard.cpp
    src/GameRenderer.cpp
    src/GlfwInput.cpp
    src/CameraController.cpp
    src/FrameTimer.cpp
)

# headers
include_directories(
    ${CMAKE_SOURCE_DIR}/external/GLAD/include
    ${CMAKE_SOURCE_DIR}/external
)

# create exe
add_executable(${PROJECT_NAME} ${SOURCES})

# linking
target_link_libraries(
    ${PROJECT_NAME}
    OpenGL::GL
    glfw
    dl
    nlohmann_json::nlohmann_json
)

include(CTest)

if(BUILD_TESTING)
    add_executable(
        enemy_regression_tests
        tests/EnemyRegression.cpp
        src/Grid.cpp
        src/Player.cpp
        src/Enemy.cpp
        src/GameState.cpp
        src/Game.cpp
        src/Scoreboard.cpp
    )

    target_include_directories(
        enemy_regression_tests
        PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(
        enemy_regression_tests
        PRIVATE
        nlohmann_json::nlohmann_json
    )

    target_compile_definitions(
        enemy_regression_tests
        PRIVATE
        TEST_LEVEL_PATH="${CMAKE_SOURCE_DIR}/assets/levels/classic_inspired.txt"
    )

    set_target_properties(
        enemy_regression_tests
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )

    add_test(
        NAME enemy_regression
        COMMAND enemy_regression_tests
    )

endif()

# shaders
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_SOURCE_DIR})
