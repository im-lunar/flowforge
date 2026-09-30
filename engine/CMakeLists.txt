cmake_minimum_required(VERSION 3.20)
project(WorkflowEngine VERSION 0.1.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Fetch nholmann json
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(json)

# Engine Library
add_library(workflow_engine 
    src/workflow/workflow.cpp
)

target_include_directories(workflow_engine PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(workflow_engine PUBLIC 
    nlohmann_json::nlohmann_json
)

# Tests
enable_testing()
add_subdirectory(tests)