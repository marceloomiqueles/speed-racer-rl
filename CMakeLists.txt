cmake_minimum_required(VERSION 3.12)
project(RacingDQN)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default to Release for better CPU performance unless explicitly overridden.
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Enable stronger optimizations in Release builds.
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# Apple Silicon tuning.
if (APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mcpu=apple-m1")
endif()

# Find PyTorch (LibTorch)
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

# Find Raylib (installed via vcpkg)
find_package(raylib CONFIG REQUIRED)

# Create models directory at build time
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/models)

# Training executable (headless mode - no rendering during training)
add_executable(racing_trainer racing_trainer.cpp)
target_link_libraries(racing_trainer "${TORCH_LIBRARIES}" raylib)

# Replay executable (visual mode - watch trained agent)
add_executable(racing_replay racing_replay.cpp)
target_link_libraries(racing_replay "${TORCH_LIBRARIES}" raylib)

# Analysis tool (statistics viewer)
add_executable(analyze_training analyze_training.cpp)

# Copy assets to build directory at configure time
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets 
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Also copy assets to Release/Debug directories for MSVC (done at build time)
add_custom_command(TARGET racing_trainer POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_CURRENT_SOURCE_DIR}/assets
                   $<TARGET_FILE_DIR:racing_trainer>/assets)

add_custom_command(TARGET racing_replay POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_CURRENT_SOURCE_DIR}/assets
                   $<TARGET_FILE_DIR:racing_replay>/assets)

# Windows-specific: Copy LibTorch DLLs to executable directories
if (MSVC)
    file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
    add_custom_command(TARGET racing_trainer
                       POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${TORCH_DLLS}
                       $<TARGET_FILE_DIR:racing_trainer>)
    add_custom_command(TARGET racing_replay
                       POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${TORCH_DLLS}
                       $<TARGET_FILE_DIR:racing_replay>)
endif()
