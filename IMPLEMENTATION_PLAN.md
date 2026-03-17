# Stardust Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Complete the GDExtension build system and fix missing signal emissions to have a fully functional falling sand simulation with black hole physics ready for Godot integration.

**Architecture:** The project uses a C++ GDExtension with double-buffered cellular automaton for the falling sand simulation. Black holes apply inverse-square gravity forces to cells within their influence radius. The simulation is exposed to GDScript via the FallingSandSimulation Node2D class.

**Tech Stack:** Godot 4.6, C++17, GDExtension API, CMake build system

---

## Gap Analysis Summary

### Verified Implemented (from specs/)
| Component | Status | Notes |
|-----------|--------|-------|
| `FallingSandEngine.h/.cpp` | ✅ Complete | All 22 element types, double buffering, physics behaviors, stress system |
| `BlackHoleEngine.h/.cpp` | ✅ Complete | Inverse-square gravity, event horizon consumption, all constants per spec |
| `register_types.cpp` | ✅ Complete | Entry point exists, registers both classes |
| `FallingSandSimulation.h` | ✅ Complete | Full header with all properties and methods |
| `FallingSandSimulation.cpp` | ✅ Complete | Full GDScript bindings implementation |
| `GridRenderer.h/.cpp` | ✅ Complete | Texture rendering implementation |

### Critical Gaps Remaining

| # | Gap | Priority | Impact | Status |
|---|-----|----------|--------|--------|
| 1 | **CMakeLists.txt missing** | Critical | Cannot compile the extension | ✅ Complete |
| 2 | **extension.gdextension missing** | Critical | Godot won't load the extension | ✅ Complete |
| 3 | **Shaders directory missing** | Medium | No GPU rendering path | ✅ Complete |
| 4 | **Signal emissions incomplete** | Medium | black_hole_consumed and planet_destroyed not emitted | ✅ Complete |
| 5 | **Tests directory missing** | High | Cannot verify acceptance criteria | ✅ Complete |

---

## Task 1: Create CMakeLists.txt Build System

**Files:**
- Create: `CMakeLists.txt`

### Step 1: Create CMakeLists.txt

```cmake
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
```

---

## Task 2: Create extension.gdextension Manifest

**Files:**
- Create: `extension.gdextension`

### Step 2: Create extension.gdextension

```json
{
    "entry_point": "godot_falling_sand_gdextension_init",
    "toolbox": false,
    "rdp": false,
    "script_language": "gdscript",
    "dependencies": [],
    "product_name": "Falling Sand Simulation",
    "product_version": "1.0.0",
    "supported_api_versions": [
        "4.6",
        "4.3",
        "4.2"
    ]
}
```

---

## Task 3: Create Shaders Directory and Files

**Files:**
- Create: `shaders/grid_render.gdshader`
- Create: `shaders/particle_effect.gdshader`

### Step 3: Create grid_render.gdshader

```glsl
shader_type canvas_item;

uniform sampler2D grid_data : filter_nearest;
uniform sampler2D color_palette : filter_nearest;
uniform vec2 grid_size;

void fragment() {
    vec2 pixel_pos = floor(UV * grid_size);
    float element_id = texture(grid_data, pixel_pos / grid_size).r;
    vec4 color = texture(color_palette, vec2((element_id + 0.5) / 256.0, 0.5));
    COLOR = color;
}
```

### Step 4: Create particle_effect.gdshader

```glsl
shader_type canvas_item;

uniform vec4 tint : source_color = vec4(1.0);
uniform float emission_strength : hint_range(0.0, 5.0) = 1.0;

void fragment() {
    vec4 color = texture(TEXTURE, UV);
    color.rgb *= emission_strength;
    color *= tint;
    COLOR = color;
}
```

---

## Task 4: Implement Signal Emissions

**Files:**
- Modify: `src/FallingSandEngine.h` - Add consumed elements tracking
- Modify: `src/FallingSandEngine.cpp` - Track consumed elements
- Modify: `src/godot_extension/FallingSandSimulation.cpp` - Emit signals

### Step 5: Add consumed elements tracking to FallingSandEngine

Add to `FallingSandEngine.h`:
```cpp
// Add to Grid class
struct ConsumedElement {
    int x, y;
    ElementType type;
};
std::vector<ConsumedElement> getConsumedElements() const;
void clearConsumedElements();
```

Add member variable:
```cpp
std::vector<ConsumedElement> consumedElements;
```

### Step 6: Track consumption in updateWithBlackHoles

In `FallingSandEngine.cpp`, when consuming element:
```cpp
if (consumingIndex >= 0) {
    ConsumedElement consumed = {x, y, current};
    consumedElements.push_back(consumed);
    getNextCell(x, y) = ElementType::EMPTY;
    nextStressMap[y * width + x] = 0.0f;
    continue;
}
```

Add helper methods:
```cpp
std::vector<ConsumedElement> Grid::getConsumedElements() const {
    return consumedElements;
}

void Grid::clearConsumedElements() {
    consumedElements.clear();
}
```

### Step 7: Emit black_hole_consumed in FallingSandSimulation

In `_physics_process()`, after `grid->swapBuffers()`:
```cpp
auto consumed = grid->getConsumedElements();
for (const auto& elem : consumed) {
    emit_signal("black_hole_consumed", elem.x, elem.y, static_cast<int>(elem.type));
}
grid->clearConsumedElements();
```

### Step 8: Emit planet_destroyed signal

Add to FallingSandSimulation:
```cpp
int lastPlanetElementCount = 0;

int currentPlanetCount =
    grid->getElementCount(ElementType::PLANET_CORE) +
    grid->getElementCount(ElementType::PLANET_MANTLE) +
    grid->getElementCount(ElementType::PLANET_CRUST);

if (lastPlanetElementCount > 0 && currentPlanetCount == 0) {
    emit_signal("planet_destroyed", 0);
}
lastPlanetElementCount = currentPlanetCount;
```

---

## Task 5: Verify Build

**Files:**
- Test: CMake configuration

### Step 9: Test CMake configuration

```bash
# Verify CMakeLists.txt syntax
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DGODOT_INCLUDE_DIRS=/path/to/godot-headers .
```

Expected: Configuration completes (may fail on missing Godot headers - that's expected until user provides paths)

---

## Execution Order

1. Task 1: Create CMakeLists.txt ✅ COMPLETE
2. Task 2: Create extension.gdextension ✅ COMPLETE
3. Task 3: Create shaders ✅ COMPLETE
4. Task 4: Implement signal emissions ✅ COMPLETE
5. Task 5: Verify build ✅ COMPLETE
6. Task 6: Create test infrastructure ✅ COMPLETE

## Verification Results (2026-03-17)

- CMake configuration: PASSES
- Build artifacts exist: libgodot_falling_sand.dylib in build/
- Signal emissions verified in code:
  - black_hole_consumed: Implemented in FallingSandSimulation.cpp:72
  - planet_destroyed: Implemented in FallingSandSimulation.cpp:83
  - element_changed: Implemented in FallingSandSimulation.cpp:172,201
- ConsumedElement tracking: Implemented in FallingSandEngine.h:74-90
- Test infrastructure: tests/TestRunner.gd (681 lines, 50+ tests)
- Note: GDScript tests require Godot 4.6+ to run
- GDExtension loads successfully in Godot 4.6.1
- Fixed: cell_scale bind_method missing (now added)
- Fixed: extension.gdextension format updated for Godot 4.6
- Fixed: CMake builds dylib instead of framework for macOS compatibility
- Fixed: Extension loading - single instance now loads correctly

---

## Post-Implementation Verification

Once tasks are complete, verify these acceptance criteria:

**Phase 1 (Core Engine):**
- T1.1.1: ElementType enum - All 22 types present
- T1.1.2: Element colors - Each type has distinct RGBA
- T1.2.1: Double buffering - Grid uses two buffers

**Phase 2 (Black Hole):**
- T2.1.1: BlackHole struct fields
- T2.2.1: Inverse-square law implemented
- T2.3.1: Event horizon consumption works

**Phase 3 (Simulation Node):**
- T3.5.1-4: Signals emit correctly

**Phase 6 (Build & Integration):**
- T6.1.1-3: CMake builds produce shared library
- T6.2.1-3: Extension manifest correct
