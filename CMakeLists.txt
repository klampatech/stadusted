cmake_minimum_required(VERSION 3.15)
project(stardust_falling_sand)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Godot headers via environment variable
set(GODOT_INCLUDE_DIRS "$ENV{GODOT_INCLUDE_DIRS}" CACHE PATH "Godot include directories")
set(GODOT_LIBRARIES "$ENV{GODOT_LIBRARIES}" CACHE FILEPATH "Godot library")

if(NOT GODOT_INCLUDE_DIRS)
    message(WARNING "GODOT_INCLUDE_DIRS not set - using default paths")
    if(APPLE)
        set(GODOT_INCLUDE_DIRS "/usr/local/include/godotcpp" CACHE PATH "" FORCE)
    endif()
endif()

# Source files
set(SOURCES
    src/FallingSandEngine.cpp
    src/BlackHoleEngine.cpp
    src/godot_extension/register_types.cpp
    src/godot_extension/FallingSandSimulation.cpp
    src/godot_extension/GridRenderer.cpp
)

# Create shared library
add_library(godot_falling_sand SHARED ${SOURCES})

target_include_directories(godot_falling_sand PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${GODOT_INCLUDE_DIRS}
)

target_link_libraries(godot_falling_sand PRIVATE
    ${GODOT_LIBRARIES}
)

# Platform-specific settings
if(APPLE)
    set_target_properties(godot_falling_sand PROPERTIES
        FRAMEWORK TRUE
        MACOSX_FRAMEWORK_IDENTIFIER "com.stardust.falling-sand"
        LINKER_FLAGS "-undefined dynamic_lookup"
    )
endif()

if(WIN32)
    target_compile_definitions(godot_falling_sand PRIVATE WIN32_LEAN_AND_MEAN)
endif()
