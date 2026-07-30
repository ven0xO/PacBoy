# Support CMake 3.20+ and use policies tested through CMake 3.28.
cmake_minimum_required(VERSION 3.20...3.28)
project(PacBoy VERSION 0.1.0)

option(
    PACBOY_ENABLE_WARNINGS
    "Enable strict compiler warnings"
    ON
)

option(
    PACBOY_ENABLE_SANITIZERS
    "Enable AddressSanitizer and UndefinedBehaviorSanitizer"
    OFF
)

option(
    PACBOY_ENABLE_CLANG_TIDY
    "Run clang-tidy during compilation"
    OFF
)

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

# create exe
add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_features(
    ${PROJECT_NAME}
    PRIVATE
    cxx_std_17
)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/external/GLAD/include
    ${CMAKE_SOURCE_DIR}/external
)

# linking
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    OpenGL::GL
    glfw
    ${CMAKE_DL_LIBS}
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

    target_compile_features(
        enemy_regression_tests
        PRIVATE
        cxx_std_17
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

    add_test(
        NAME enemy_regression
        COMMAND enemy_regression_tests
    )

endif()

add_custom_target(
    copy_runtime_resources ALL

    COMMAND
        ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        "${CMAKE_CURRENT_BINARY_DIR}/shaders"

    COMMAND
        ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/levels"
        "${CMAKE_CURRENT_BINARY_DIR}/assets/levels"

    COMMAND
        ${CMAKE_COMMAND} -E make_directory
        "${CMAKE_CURRENT_BINARY_DIR}/assets/scores"

    COMMENT "Copying runtime resources"
    VERBATIM
)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "."
)

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/shaders/"
    DESTINATION "shaders"
)

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/levels/"
    DESTINATION "assets/levels"
)
