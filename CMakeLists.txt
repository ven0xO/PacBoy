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

function(pacboy_enable_sanitizers target_name)
    if(NOT PACBOY_ENABLE_SANITIZERS)
        return()
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(
            ${target_name}
            PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
        )

        target_link_options(
            ${target_name}
            PRIVATE
            -fsanitize=address,undefined
        )
    else()
        message(
            FATAL_ERROR
            "Sanitizers are currently supported only with GCC or Clang"
        )
    endif()
endfunction()

if(PACBOY_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy REQUIRED)
endif()

function(pacboy_enable_clang_tidy target_name)
    if(NOT PACBOY_ENABLE_CLANG_TIDY)
        return()
    endif()

    set_property(
        TARGET ${target_name}
        PROPERTY CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE};--quiet"
    )
endfunction()

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

if(PACBOY_ENABLE_WARNINGS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(
            ${PROJECT_NAME}
            PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:-Wall>
            $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
            $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
        )
    elseif(MSVC)
        target_compile_options(
            ${PROJECT_NAME}
            PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:/W4>
            $<$<COMPILE_LANGUAGE:CXX>:/permissive->
        )
    endif()
endif()

# linking
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    OpenGL::GL
    glfw
    ${CMAKE_DL_LIBS}
    nlohmann_json::nlohmann_json
)

pacboy_enable_sanitizers(${PROJECT_NAME})
pacboy_enable_clang_tidy(${PROJECT_NAME})

include(CTest)

if(BUILD_TESTING)
    find_package(Catch2 3 CONFIG QUIET)

    if(NOT Catch2_FOUND)
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.15.0
            GIT_SHALLOW TRUE
        )

        FetchContent_MakeAvailable(Catch2)

        list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
    endif()

    include(Catch)

    add_executable(
        pacboy_tests
        tests/EnemyRegression.cpp
        tests/GameFlowTests.cpp
        tests/GameStateScoreboardTests.cpp
        tests/GridPlayerTests.cpp
        src/Grid.cpp
        src/Player.cpp
        src/Enemy.cpp
        src/GameState.cpp
        src/Game.cpp
        src/Scoreboard.cpp
    )

    target_compile_features(
        pacboy_tests
        PRIVATE
        cxx_std_17
    )

    if(PACBOY_ENABLE_WARNINGS)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(
                pacboy_tests
                PRIVATE
                -Wall
                -Wextra
                -Wpedantic
            )
        elseif(MSVC)
            target_compile_options(
                pacboy_tests
                PRIVATE
                /W4
                /permissive-
            )
        endif()
    endif()

    target_include_directories(
        pacboy_tests
        PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(
        pacboy_tests
        PRIVATE
        nlohmann_json::nlohmann_json
        Catch2::Catch2WithMain
    )

    pacboy_enable_sanitizers(pacboy_tests)
    pacboy_enable_clang_tidy(pacboy_tests)

    target_compile_definitions(
        pacboy_tests
        PRIVATE
        TEST_LEVEL_PATH="${CMAKE_SOURCE_DIR}/assets/levels/classic_inspired.txt"
    )

    catch_discover_tests(
        pacboy_tests
        DISCOVERY_MODE PRE_TEST
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
