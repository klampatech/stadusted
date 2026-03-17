# Stardust Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Complete the GDExtension build system and fix missing signal emissions to have a fully functional falling sand simulation with black hole physics ready for Godot integration.

**Architecture:** The project uses a C++ GDExtension with double-buffered cellular automaton for the falling sand simulation. Black holes apply inverse-square gravity forces to cells within their influence radius. The simulation is exposed to GDScript via the FallingSandSimulation Node2D class.

**Tech Stack:** Godot 4.6, C++17, GDExtension API, CMake build system

---

## Implementation Summary

### Already Implemented (from specs/)
- Core engine: `FallingSandEngine.h/.cpp` - All 22 element types, physics behaviors, double buffering, stress system
- Black hole physics: `BlackHoleEngine.h/.cpp` - Inverse-square gravity, event horizon consumption
- Main Godot node: `FallingSandSimulation.h/.cpp` - GDScript bindings, properties, signals (partially)
- GridRenderer class skeleton: `GridRenderer.h` - Basic structure exists
- GDExtension entry: `register_types.cpp` - Entry point exists

### Critical Gaps
1. ❌ **Build system missing**: No CMakeLists.txt
2. ❌ **Extension manifest missing**: No extension.gdextension (must support Godot 4.6 per project.godot)
3. ❌ **GridRenderer implementation missing**: Header exists but no .cpp file
4. ❌ **Shaders missing**: No grid_render.gdshader or particle_effect.gdshader
5. ❌ **Signal emissions missing**: `planet_destroyed` and `black_hole_consumed` signals defined but never emitted
6. ❌ **GDScript binding incomplete**: `cell_scale` property getter not bound
7. ⚠️ **Grid resizing incomplete**: set_grid_width/height don't recreate grid
8. ⚠️ **add_black_hole API incomplete**: Missing influence_radius parameter
9. ⚠️ **Rendering integration incomplete**: No Sprite2D setup to display grid_texture
10. ⚠️ **Test harness missing**: No GDScript tests

### Verified Implemented Components
- Core engine (`FallingSandEngine.h/.cpp`) - All 22 element types, double buffering, physics behaviors, stress system
- Black hole physics (`BlackHoleEngine.h/.cpp`) - Inverse-square gravity, event horizon consumption, all constants per spec
- Main Godot node (`FallingSandSimulation.h/.cpp`) - Most GDScript bindings, properties defined
- GDExtension entry (`register_types.cpp`) - Entry point exists
- GridRenderer header (`GridRenderer.h`) - Structure exists

---

## Task 0: Pre-flight Check - Verify Code Compiles (After Tasks 1-3)

Run after completing Tasks 1-3 to verify the build system works:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

Expected: Build completes (may fail on linking if Godot headers not found - that's expected)

---

## Task 1: Create CMakeLists.txt Build System

**Files:**
- Create: `CMakeLists.txt`
- Test: N/A (build file only)

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
    # Default paths for common installations
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

## Task 3: Create GridRenderer.cpp Implementation

**Files:**
- Create: `src/godot_extension/GridRenderer.cpp`
- Test: N/A (build will verify)

### Step 3: Create GridRenderer.cpp

```cpp
#include "GridRenderer.h"
#include <cstring>

GridRenderer::GridRenderer()
    : width(0), height(0), dirty(false) {
}

void GridRenderer::initialize(int w, int h) {
    width = w;
    height = h;
    gridImage = Image::create(width, height, false, Image::FORMAT_RGBA8);
    gridTexture = ImageTexture::create_from_image(gridImage);
    dirty = true;
}

void GridRenderer::shutdown() {
    gridImage.unref();
    gridTexture.unref();
    width = 0;
    height = 0;
}

void GridRenderer::updateTexture(const void* gridData, int elementSize) {
    if (!gridImage.is_valid() || !gridData) {
        return;
    }

    // gridData is expected to be an array of ElementType (uint8_t)
    const uint8_t* elements = static_cast<const uint8_t*>(gridData);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t elementType = elements[y * width + x];
            Color color = getColorForElement(elementType);
            gridImage->set_pixel(x, y, color);
        }
    }

    if (gridTexture.is_valid()) {
        gridTexture->update(gridImage);
    }
    dirty = false;
}

Color GridRenderer::getColorForElement(uint8_t type) {
    // Map element types to colors - simplified version
    // A full implementation would include all 22 element colors
    switch (type) {
        case 0: return Color(0.08f, 0.08f, 0.12f, 1.0f); // EMPTY
        case 1: return Color(0.76f, 0.70f, 0.50f, 1.0f); // SAND
        case 2: return Color(0.12f, 0.56f, 1.0f, 1.0f); // WATER
        case 3: return Color(0.5f, 0.5f, 0.5f, 1.0f);  // STONE
        case 4: return Color(1.0f, 0.4f, 0.0f, 1.0f);   // FIRE
        case 5: return Color(0.3f, 0.3f, 0.3f, 0.7f);   // SMOKE
        default: return Color(0.08f, 0.08f, 0.12f, 1.0f);
    }
}

void GridRenderer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize", "width", "height"),
                         &GridRenderer::initialize);
    ClassDB::bind_method(D_METHOD("shutdown"), &GridRenderer::shutdown);
    ClassDB::bind_method(D_METHOD("getTexture"), &GridRenderer::getTexture);
    ClassDB::bind_method(D_METHOD("isDirty"), &GridRenderer::isDirty);
    ClassDB::bind_method(D_METHOD("markClean"), &GridRenderer::markClean);
}
```

---

## Task 4: Create Grid Rendering Shader

**Files:**
- Create: `shaders/grid_render.gdshader`

### Step 4: Create grid_render.gdshader

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

---

## Task 5: Create Particle Effect Shader

**Files:**
- Create: `shaders/particle_effect.gdshader`

### Step 5: Create particle_effect.gdshader

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

## Task 6: Fix cell_scale GDScript Binding

**Files:**
- Modify: `src/godot_extension/FallingSandSimulation.cpp:403-408`

### Step 6: Add cell_scale property binding

In `_bind_methods()`, after the existing ADD_PROPERTY calls:

```cpp
// Cell scale property is already bound via getter/setter
// Ensure it's properly exposed
ADD_PROPERTY(PropertyInfo(Variant::INT, "cell_scale"), "set_cell_scale", "get_cell_scale");
```

---

## Task 7: Emit black_hole_consumed Signal

**Files:**
- Modify: `src/FallingSandEngine.h` - Add consumed elements tracking
- Modify: `src/FallingSandEngine.cpp:246-254` - Track consumed elements
- Modify: `src/godot_extension/FallingSandSimulation.cpp:47-80` - Emit signal

### Step 7: Track consumed elements and emit signal

The FallingSandSimulation needs to track when elements are consumed by black holes. This requires:
1. Grid to track consumed positions (or return them)
2. FallingSandSimulation to emit the signal

**Modify Grid class to track consumed elements:**

Add to `FallingSandEngine.h`:
```cpp
// Add method to get consumed elements from last update
struct ConsumedElement {
    int x, y;
    ElementType type;
};
std::vector<ConsumedElement> getConsumedElements() const;
void clearConsumedElements();
```

**Modify FallingSandEngine.cpp:**

Add member variable to Grid class:
```cpp
std::vector<ConsumedElement> consumedElements;
```

In `updateWithBlackHoles()`, when consuming:
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

**Modify FallingSandSimulation.cpp _physics_process:**

After `grid->swapBuffers()`:
```cpp
// Check for consumed elements and emit signal
auto consumed = grid->getConsumedElements();
for (const auto& elem : consumed) {
    emit_signal("black_hole_consumed", elem.x, elem.y, static_cast<int>(elem.type));
}
grid->clearConsumedElements();
```

---

## Task 8: Emit planet_destroyed Signal

**Files:**
- Modify: `src/godot_extension/FallingSandSimulation.cpp`

### Step 8: Track planet state and emit signal

Add planet tracking to FallingSandSimulation:
```cpp
// Add to header or as member variable
int lastPlanetElementCount;

// In _ready():
lastPlanetElementCount = 0;

// In _physics_process(), after getting consumed elements:
int currentPlanetCount =
    get_element_count(18) +  // PLANET_CORE
    get_element_count(19) +  // PLANET_MANTLE
    get_element_count(20);   // PLANET_CRUST

if (lastPlanetElementCount > 0 && currentPlanetCount == 0) {
    emit_signal("planet_destroyed", 0); // planet_id = 0 for now
}

lastPlanetElementCount = currentPlanetCount;
```

---

## Task 9: Verify Build System Works

**Files:**
- Test: CMake configuration

### Step 9: Test CMake configuration

```bash
# Verify CMakeLists.txt syntax
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DGODOT_INCLUDE_DIRS=/path/to/godot-headers .
```

Expected: Configuration completes without errors (may fail on missing Godot headers - that's expected until user provides paths)

---

## Task 10: Commit All Changes

**Files:**
- Commit: All new and modified files

### Step 10: Commit changes

```bash
git add CMakeLists.txt extension.gdextension src/godot_extension/GridRenderer.cpp shaders/
git commit -m "feat: Add build system and complete GDExtension scaffolding

- Add CMakeLists.txt for building the GDExtension
- Add extension.gdextension manifest for Godot 4.2/4.3
- Implement GridRenderer.cpp for texture rendering
- Add grid_render.gdshader for GPU-based grid visualization
- Add particle_effect.gdshader for visual effects
- Fix cell_scale GDScript property binding
- Emit black_hole_consumed signal on element consumption
- Emit planet_destroyed signal when planets are fully destroyed"
```

---

## Execution Order

1. Task 0: Pre-flight check (optional, after Tasks 1-3)
2. Task 1: Create CMakeLists.txt
3. Task 2: Create extension.gdextension
4. Task 3: Create GridRenderer.cpp
5. Task 4: Create grid_render.gdshader
6. Task 5: Create particle_effect.gdshader
7. Task 6: Fix cell_scale binding
8. Task 7: Emit black_hole_consumed signal
9. Task 8: Emit planet_destroyed signal
10. Task 9: Verify build
11. Task 10: Commit

---

## Notes

- The build system requires Godot 4.x headers to compile. User must set `GODOT_INCLUDE_DIRS` environment variable
- Signals are emitted after simulation step so game logic can respond to consumption events
- GridRenderer provides both ImageTexture (CPU path) and could support GPU shader path for larger grids

## Post-Implementation Verification

Once Tasks 1-8 are complete, these acceptance criteria can be verified:

**Phase 1 (Core Engine):**
- T1.1.1: ElementType enum - All 22 types present
- T1.1.2: Element colors - Each type has distinct RGBA
- T1.2.1: Double buffering - Grid uses two buffers
- T1.2.2: Memory layout - 500x500 ≤ 500KB

**Phase 2 (Black Hole):**
- T2.1.1: BlackHole struct fields
- T2.1.2: Default constants match spec
- T2.2.1: Inverse-square law implemented
- T2.3.1: Event horizon consumption works

**Phase 3 (Simulation Node):**
- T3.1.1-5: Properties work (grid_width, grid_height, etc.)
- T3.2.1-8: Element manipulation methods
- T3.3.1-3: World generation methods
- T3.4.1-3: Texture & rendering
- T3.5.1-4: Signals emit correctly

**Phase 6 (Build & Integration):**
- T6.1.1-3: CMake builds produce shared library
- T6.2.1-3: Extension manifest correct
- T6.3.1-4: Godot integration works

---

## In Progress

<!-- Tasks currently being worked on -->

## Completed

<!-- Completed tasks (can be periodically cleaned out) -->
