# Godot 4.6 GDExtension Technical Specification

## Stardust - Black Hole Falling Sand Game

---

## 1. Executive Summary

This document specifies a GDExtension that ports the existing SDL2/C++ Falling Sand Engine to Godot 4.6, adding black hole attraction physics for the Stardust game. The extension exposes the high-performance cellular automaton simulation to GDScript while leveraging Godot's 2D rendering pipeline.

### Key Requirements

- **Performance**: Support 500x500+ grids at 60 FPS
- **Multiple Black Holes**: Support simultaneous attraction forces
- **Planet Destruction**: Mechanic for breaking planets piece-by-piece
- **Godot Integration**: Full GDScript control with signals for game events

---

## 2. Project Structure

### 2.1 Directory Layout

```
stardust/
├── project/
│   └── stardust.godot              # Godot project file
├── src/
│   ├── FallingSandEngine.h         # Core engine header (from ralph)
│   ├── FallingSandEngine.cpp       # Core engine impl (from ralph)
│   ├── BlackHoleEngine.h           # NEW: Black hole mechanics
│   ├── BlackHoleEngine.cpp         # NEW: Black hole impl
│   └── godot_extension/
│       ├── register_types.cpp      # GDExtension registration
│       ├── FallingSandSimulation.h # Main Godot class
│       ├── FallingSandSimulation.cpp
│       ├── GridRenderer.h          # Rendering integration
│       ├── GridRenderer.cpp
│       └── CMakeLists.txt          # Build config
├── shaders/
│   ├── grid_render.gdshader        # GPU-based grid rendering
│   └── particle_effect.gdshader   # Visual effects
├── icons/
│   └── icon.svg                    # Extension icon
├── extension.gdextension            # Extension manifest
└── README.md
```

### 2.2 File Purposes

| File | Purpose |
|------|---------|
| `FallingSandEngine.h/cpp` | Core cellular automaton (existing, from ralph) |
| `BlackHoleEngine.h/cpp` | NEW: Black hole attraction physics |
| `register_types.cpp` | GDExtension entry point, registers all classes |
| `FallingSandSimulation.h/cpp` | Main `FallingSandSimulation` node class |
| `GridRenderer.h/cpp` | Bridge between simulation and Godot rendering |
| `grid_render.gdshader` | GPU-accelerated grid visualization |
| `extension.gdextension` | Godot extension manifest |

---

## 3. C++ Class Design

### 3.1 Class Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           GDExtension Layer                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│  FallingSandSimulation (Godot Node2D)                                      │
│  ├── Extends: Node2D                                                       │
│  ├── Properties: grid_width, grid_height, time_scale                      │
│  ├── Methods: spawn_element(), erase_element(), get_element_at()         │
│  └── Signals: element_changed, simulation_stepped, black_hole_consumed    │
├─────────────────────────────────────────────────────────────────────────────┤
│  GridRenderer (Rendering Bridge)                                          │
│  ├── Uses: RenderingServer, Viewport                                      │
│  ├── Methods: render(), update_texture()                                  │
│  └── Internals: Image texture, GPU rendering                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                        Core Engine Layer                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│  Grid (Existing - Modified)                                                │
│  ├── Buffers: currentBuffer[], nextBuffer[]                               │
│  ├── Methods: update(), swapBuffers(), getCell(), setCell()              │
│  └── NEW: setBlackHole(), applyAttractionForces()                        │
├─────────────────────────────────────────────────────────────────────────────┤
│  BlackHoleEngine (NEW)                                                    │
│  ├── Properties: position, mass, event_horizon_radius                    │
│  ├── Methods: calculateAttraction(), isInRange()                         │
│  └── Internal: Inverse-square gravity with damping                       │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 4. Core Engine (From Ralph)

### 4.1 Element Types

```cpp
enum class ElementType : uint8_t {
    EMPTY = 0,
    SAND, WATER, STONE, FIRE, SMOKE, WOOD, OIL, ACID, GUNPOWDER,
    COAL, COPPER, GOLD, DIAMOND, DIRT,
    PICKUP_PARTICLES, PICKUP_POINTS, GOLD_CRACKED,
    // NEW: Black hole specific elements
    PLANET_CORE, PLANET_MANTLE, PLANET_CRUST, BLACK_HOLE
};
```

### 4.2 Grid Class

- Double-buffered with `currentBuffer` and `nextBuffer`
- Row-major memory layout
- Physics update toggles direction each frame to prevent bias

### 4.3 Physics Behaviors

| Element | Behavior |
|---------|----------|
| SAND | Falls down, piles at 45° angle |
| WATER | Falls, flows horizontally when blocked |
| FIRE | Rises, dies over time, ignites flammables |
| SMOKE | Rises, disperses |
| WOOD | Static, catches fire from adjacent FIRE |
| OIL | Liquid + flammable |
| ACID | Dissolves adjacent destructibles |
| GUNPOWDER | Falls, explodes when ignited |
| PLANET_CORE | Dense center - high mass, hard to destroy |
| PLANET_MANTLE | Middle layer - medium destruction resistance |
| PLANET_CRUST | Outer layer - breaks apart first under gravity |

---

## 5. Black Hole Engine

### 5.1 BlackHole Structure

```cpp
struct BlackHole {
    float x;              // Position X (grid coordinates)
    float y;              // Position Y (grid coordinates)
    float mass;           // Gravitational mass (affects pull strength)
    float event_horizon;  // Radius where elements are consumed
    float influence_radius; // Maximum distance of gravitational effect
    bool active;          // Whether this black hole is currently active

    // Visual properties (for Godot)
    float rotation_speed; // Visual rotation animation
    float pulse_phase;    // For pulsing animation
};
```

### 5.2 Constants

```cpp
static constexpr float DEFAULT_MASS = 1000.0f;
static constexpr float DEFAULT_EVENT_HORIZON = 8.0f;
static constexpr float DEFAULT_INFLUENCE_RADIUS = 150.0f;
static constexpr float MAX_BLACK_HOLES = 16;
static constexpr float MIN_BLACK_HOLE_MASS = 100.0f;
static constexpr float MAX_BLACK_HOLE_MASS = 10000.0f;
static constexpr float GRAVITATIONAL_CONSTANT = 6.674f; // Scaled for simulation
static constexpr float INVERSE_SQUARE_DAMPING = 0.5f; // Prevents extreme forces at close range
```

### 5.3 Attraction Force Calculation

```cpp
float BlackHoleEngine::calculateAttraction(float cell_x, float cell_y,
                                          float& out_dx, float& out_dy) const {
    float total_force_x = 0.0f;
    float total_force_y = 0.0f;
    float max_force = 0.0f;

    for (const BlackHole& bh : black_holes) {
        if (!bh.active) continue;

        // Calculate distance to black hole
        float dx = bh.x - cell_x;
        float dy = bh.y - cell_y;
        float distance_sq = dx * dx + dy * dy;
        float distance = std::sqrt(distance_sq);

        // Skip if too far or at exact center
        if (distance > bh.influence_radius || distance < 0.01f) continue;

        // Normalized direction
        float dir_x = dx / distance;
        float dir_y = dy / distance;

        // Force calculation (inverse square with damping at close range)
        float effective_distance = distance + INVERSE_SQUARE_DAMPING;
        float force_magnitude = (GRAVITATIONAL_CONSTANT * bh.mass) /
                                (effective_distance * effective_distance);

        // Clamp force to prevent extreme acceleration
        force_magnitude = std::min(force_magnitude, 2.0f);

        total_force_x += dir_x * force_magnitude;
        total_force_y += dir_y * force_magnitude;

        max_force = std::max(max_force, force_magnitude);
    }

    // Normalize output
    float total_force = std::sqrt(total_force_x * total_force_x +
                                   total_force_y * total_force_y);
    if (total_force > 0.001f) {
        float scale = std::min(total_force, 1.0f) / total_force;
        out_dx = total_force_x * scale;
        out_dy = total_force_y * scale;
    } else {
        out_dx = 0.0f;
        out_dy = 0.0f;
    }

    return std::min(max_force, 1.0f);
}
```

### 5.4 Event Horizon Consumption

```cpp
int BlackHoleEngine::checkEventHorizon(float x, float y) const {
    for (int i = 0; i < black_holes.size(); i++) {
        const BlackHole& bh = black_holes[i];
        if (!bh.active) continue;

        float dx = bh.x - x;
        float dy = bh.y - y;
        float distance = std::sqrt(dx * dx + dy * dy);

        if (distance < bh.event_horizon) {
            return i; // Return index of consuming black hole
        }
    }
    return -1; // Not consumed
}
```

---

## 6. FallingSandSimulation (Main Godot Class)

### 6.1 Class Definition

```cpp
class FallingSandSimulation : public godot::Node2D {
    GDCLASS(FallingSandSimulation, godot::Node2D)

private:
    // Core simulation
    Grid* grid;
    BlackHoleEngine* black_hole_engine;

    // Grid configuration
    int grid_width;
    int grid_height;
    int cell_scale;

    // Simulation control
    bool simulation_paused;
    float time_scale;
    int target_fps;

    // Rendering
    godot::Image* grid_image;
    godot::ImageTexture* grid_texture;
    bool texture_dirty;

    // Black hole management
    bool black_holes_enabled;

    // Statistics
    uint64_t frame_count;
    float update_accumulator;
    float updates_per_second;

public:
    FallingSandSimulation();
    ~FallingSandSimulation();

    // Godot lifecycle
    void _ready() override;
    void _physics_process(float delta) override;

    // Configuration properties
    void set_grid_width(int width);
    int get_grid_width() const;
    void set_grid_height(int height);
    int get_grid_height() const;
    void set_simulation_paused(bool paused);
    bool is_simulation_paused() const;
    void set_time_scale(float scale);
    float get_time_scale() const;

    // Element manipulation
    void spawn_element(int x, int y, int element_type, int radius = 1);
    void erase_element(int x, int y, int radius = 2);
    int get_element_at(int x, int y) const;

    // Texture access
    godot::ImageTexture* get_grid_texture();
    bool is_texture_dirty() const;
    void mark_texture_clean();

    // Black hole management
    int add_black_hole(float x, float y, float mass = 1000.0f,
                       float event_horizon = 8.0f);
    void remove_black_hole(int index);
    void set_black_hole_position(int index, float x, float y);
    void set_black_hole_mass(int index, float mass);
    int get_black_hole_count() const;
    Dictionary get_black_hole_info(int index) const;
    void clear_all_black_holes();
    void set_black_holes_enabled(bool enabled);

    // World generation
    void generate_planet(int center_x, int center_y, int radius);
    void clear_grid();

    // Statistics
    float get_ups() const;
    uint64_t get_frame_count() const;

protected:
    static void _bind_methods();
};
```

### 6.2 GDScript Bindings

```cpp
void FallingSandSimulation::_bind_methods() {
    // Configuration
    ClassDB::bind_method(D_METHOD("set_grid_width", "width"), &FallingSandSimulation::set_grid_width);
    ClassDB::bind_method(D_METHOD("get_grid_width"), &FallingSandSimulation::get_grid_width);
    ClassDB::bind_method(D_METHOD("set_grid_height", "height"), &FallingSandSimulation::set_grid_height);
    ClassDB::bind_method(D_METHOD("get_grid_height"), &FallingSandSimulation::get_grid_height);
    ClassDB::bind_method(D_METHOD("set_simulation_paused", "paused"), &FallingSandSimulation::set_simulation_paused);
    ClassDB::bind_method(D_METHOD("is_simulation_paused"), &FallingSandSimulation::is_simulation_paused);
    ClassDB::bind_method(D_METHOD("set_time_scale", "scale"), &FallingSandSimulation::set_time_scale);
    ClassDB::bind_method(D_METHOD("get_time_scale"), &FallingSandSimulation::get_time_scale);

    // Element operations
    ClassDB::bind_method(D_METHOD("spawn_element", "x", "y", "element_type", "radius"),
                         &FallingSandSimulation::spawn_element, DEFVAL(1));
    ClassDB::bind_method(D_METHOD("erase_element", "x", "y", "radius"),
                         &FallingSandSimulation::erase_element, DEFVAL(2));
    ClassDB::bind_method(D_METHOD("get_element_at", "x", "y"),
                         &FallingSandSimulation::get_element_at);

    // Texture
    ClassDB::bind_method(D_METHOD("get_grid_texture"),
                         &FallingSandSimulation::get_grid_texture);

    // Black holes
    ClassDB::bind_method(D_METHOD("add_black_hole", "x", "y", "mass", "event_horizon"),
                         &FallingSandSimulation::add_black_hole, DEFVAL(1000.0f), DEFVAL(8.0f));
    ClassDB::bind_method(D_METHOD("remove_black_hole", "index"),
                         &FallingSandSimulation::remove_black_hole);
    ClassDB::bind_method(D_METHOD("set_black_hole_position", "index", "x", "y"),
                         &FallingSandSimulation::set_black_hole_position);
    ClassDB::bind_method(D_METHOD("set_black_hole_mass", "index", "mass"),
                         &FallingSandSimulation::set_black_hole_mass);
    ClassDB::bind_method(D_METHOD("get_black_hole_count"),
                         &FallingSandSimulation::get_black_hole_count);
    ClassDB::bind_method(D_METHOD("get_black_hole_info", "index"),
                         &FallingSandSimulation::get_black_hole_info);
    ClassDB::bind_method(D_METHOD("clear_all_black_holes"),
                         &FallingSandSimulation::clear_all_black_holes);
    ClassDB::bind_method(D_METHOD("set_black_holes_enabled", "enabled"),
                         &FallingSandSimulation::set_black_holes_enabled);
    ClassDB::bind_method(D_METHOD("are_black_holes_enabled"),
                         &FallingSandSimulation::are_black_holes_enabled);

    // World generation
    ClassDB::bind_method(D_METHOD("generate_planet", "center_x", "center_y", "radius"),
                         &FallingSandSimulation::generate_planet);
    ClassDB::bind_method(D_METHOD("clear_grid"),
                         &FallingSandSimulation::clear_grid);

    // Statistics
    ClassDB::bind_method(D_METHOD("get_ups"), &FallingSandSimulation::get_ups);
    ClassDB::bind_method(D_METHOD("get_frame_count"), &FallingSandSimulation::get_frame_count);

    // Properties
    ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_width"), "set_grid_width", "get_grid_width");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_height"), "set_grid_height", "get_grid_height");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "simulation_paused"), "set_simulation_paused", "is_simulation_paused");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "time_scale"), "set_time_scale", "get_time_scale");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "black_holes_enabled"), "set_black_holes_enabled", "are_black_holes_enabled");

    // Signals
    ADD_SIGNAL(MethodInfo("element_changed", PropertyInfo(Variant::INT, "x"),
                         PropertyInfo(Variant::INT, "y"),
                         PropertyInfo(Variant::INT, "old_type"),
                         PropertyInfo(Variant::INT, "new_type")));
    ADD_SIGNAL(MethodInfo("simulation_stepped", PropertyInfo(Variant::FLOAT, "delta")));
    ADD_SIGNAL(MethodInfo("black_hole_consumed", PropertyInfo(Variant::INT, "x"),
                         PropertyInfo(Variant::INT, "y"),
                         PropertyInfo(Variant::INT, "element_type")));
    ADD_SIGNAL(MethodInfo("planet_destroyed", PropertyInfo(Variant::INT, "planet_id")));
}
```

---

## 7. GDScript API

### 7.1 Element Types Enum

```gdscript
enum ElementType {
    EMPTY = 0,
    SAND = 1,
    WATER = 2,
    STONE = 3,
    FIRE = 4,
    SMOKE = 5,
    WOOD = 6,
    OIL = 7,
    ACID = 8,
    GUNPOWDER = 9,
    COAL = 10,
    COPPER = 11,
    GOLD = 12,
    DIAMOND = 13,
    DIRT = 14,
    PICKUP_PARTICLES = 15,
    PICKUP_POINTS = 16,
    GOLD_CRACKED = 17,
    PLANET_CORE = 18,
    PLANET_MANTLE = 19,
    PLANET_CRUST = 20,
    BLACK_HOLE = 21
}
```

### 7.2 Complete GDScript Interface

```gdscript
# FallingSandSimulation.gd
extends Node2D

# === Configuration ===

## Grid width in cells (default: 500)
@export var grid_width: int = 500

## Grid height in cells (default: 500)
@export var grid_height: int = 500

## Visual scale of each cell in pixels (default: 4)
@export var cell_scale: int = 4

## Pause simulation (default: false)
@export var simulation_paused: bool = false

## Time scale multiplier (0.1 to 10.0, default: 1.0)
@export var time_scale: float = 1.0

## Enable black hole physics (default: true)
@export var black_holes_enabled: bool = true

# === Read-Only Properties ===

## Texture containing the grid visualization
var grid_texture: ImageTexture

## Updates per second (simulation speed)
var ups: float

## Total frames simulated
var frame_count: int

# === Signals ===

## Emitted when an element changes at a position
## (x: int, y: int, old_type: int, new_type: int)
signal element_changed(x: int, y: int, old_type: int, new_type: int)

## Emitted after each simulation step (delta: float)
signal simulation_stepped(delta: float)

## Emitted when an element is consumed by a black hole
## (x: int, y: int, element_type: int)
signal black_hole_consumed(x: int, y: int, element_type: int)

## Emitted when a planet is destroyed (planet_id: int)
signal planet_destroyed(planet_id: int)

# === Methods ===

func spawn_element(x: int, y: int, element_type: int, radius: int = 1) -> void
    """Spawn an element at grid position (x, y) with brush radius."""

func spawn_element_at_world(world_pos: Vector2, element_type: int, radius: int = 1) -> void
    """Spawn an element at world position, automatically converting to grid coordinates."""

func erase_element(x: int, y: int, radius: int = 2) -> void
    """Erase elements at grid position (set to EMPTY)."""

func erase_element_at_world(world_pos: Vector2, radius: int = 2) -> void
    """Erase elements at world position."""

func get_element_at(x: int, y: int) -> int
    """Get element type at grid position. Returns 0 (EMPTY) if out of bounds."""

func get_element_at_world(world_pos: Vector2) -> int
    """Get element type at world position."""

func fill_rect(x: int, y: int, width: int, height: int, element_type: int) -> void
    """Fill a rectangular area with element type."""

func fill_circle(cx: int, cy: int, radius: int, element_type: int) -> void
    """Fill a circular area with element type."""

func clear_grid() -> void
    """Clear all elements (set to EMPTY)."""

func generate_mining_world(seed: int = 12345) -> void
    """Generate a layered mining world with ores."""

func generate_planet(center_x: int, center_y: int, radius: int) -> void
    """Generate a planet with core, mantle, and crust layers."""

func add_black_hole(x: float, y: float, mass: float = 1000.0,
                    event_horizon: float = 8.0) -> int
    """Add a black hole at position. Returns index for management."""

func remove_black_hole(index: int) -> void
    """Remove black hole by index."""

func set_black_hole_position(index: int, x: float, y: float) -> void
    """Update black hole position (can be animated)."""

func set_black_hole_mass(index: int, mass: float) -> void
    """Update black hole mass (affects attraction strength)."""

func get_black_hole_count() -> int
    """Get number of active black holes."""

func get_black_hole_info(index: int) -> Dictionary
    """Get black hole properties as Dictionary."""

func clear_all_black_holes() -> void
    """Remove all black holes."""

func get_element_count(element_type: int) -> int
    """Count total cells of a specific element type."""
```

---

## 8. Rendering Architecture

### 8.1 Rendering Pipeline

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        Rendering Pipeline                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│  ┌─────────────┐    ┌──────────────┐    ┌─────────────┐    ┌────────────┐ │
│  │   Grid      │───▶│   Image      │───▶│ ImageTexture│───▶│ Sprite2D   │ │
│  │   (C++)     │    │   (Godot)    │    │  (Godot)    │    │  (Node)    │ │
│  └─────────────┘    └──────────────┘    └─────────────┘    └────────────┘ │
│                                                                              │
│  Alternative: GPU-based rendering                                           │
│  ┌─────────────┐    ┌──────────────┐    ┌─────────────────────────────┐   │
│  │   Grid      │───▶│ PackedByte   │───▶│ RenderingServer::canvas_item│   │
│  │   (C++)     │    │   Array       │    │ with custom shader          │   │
│  └─────────────┘    └──────────────┘    └─────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 8.2 ImageTexture Rendering (Recommended for 500x500)

```cpp
void FallingSandSimulation::update_grid_image() {
    for (int y = 0; y < grid_height; y++) {
        for (int x = 0; x < grid_width; x++) {
            ElementType type = grid->getCell(x, y);
            ElementColor color = getElementColor(type);

            // Special handling for dynamic elements
            if (type == ElementType::FIRE) {
                color = getFireColor(x, y);
            } else if (type == ElementType::SMOKE) {
                color = getSmokeColor(x, y);
            }

            grid_image->set_pixel(x, y, godot::Color(
                color.r / 255.0f,
                color.g / 255.0f,
                color.b / 255.0f,
                color.a / 255.0f
            ));
        }
    }
    texture_dirty = true;
}
```

### 8.3 GPU Shader Rendering (For Larger Grids)

```glsl
// shaders/grid_render.gdshader
shader_type canvas_item;

uniform sampler2D grid_data : filter_nearest; // Packed element IDs
uniform sampler2D color_palette : filter_nearest; // 256-color palette
uniform vec2 grid_size;

void fragment() {
    vec2 uv = UV;
    vec2 pixel_pos = floor(uv * grid_size);

    // Read element ID from grid data
    float element_id = texture(grid_data, pixel_pos / grid_size).r;

    // Look up color from palette
    vec4 color = texture(color_palette, vec2((element_id + 0.5) / 256.0, 0.5));

    COLOR = color;
}
```

---

## 9. Memory Layout & Performance

### 9.1 Grid Memory Structure

For a 500x500 grid (250,000 cells):

| Approach | Memory | Access Pattern | Cache Friendly |
|----------|--------|----------------|----------------|
| `vector<ElementType>` | 250 KB | Row-major | Yes |
| Flat `unique_ptr<ElementType[]>` | 250 KB | Row-major | Yes |
| SoA (Structure of Arrays) | 300 KB | Column access | Better for some ops |

### 9.2 Memory Budget

| Component | 500x500 | 1000x1000 | Notes |
|-----------|---------|------------|-------|
| Grid buffers | 500 KB | 2 MB | Two buffers |
| Color table | 88 B | 88 B | Constant |
| Image texture | 1 MB | 4 MB | RGBA8 |
| **Total** | ~2.5 MB | ~10 MB | Well within limits |

### 9.3 Performance Targets

| Grid Size | Target FPS | Rendering Method |
|-----------|------------|------------------|
| 500x500 | 60+ | ImageTexture |
| 1000x1000 | 60+ | GPU buffer + shader |

---

## 10. Planet Destruction Mechanics

### 10.1 Planet Structure

```cpp
struct Planet {
    int center_x;
    int center_y;
    int radius;
    int core_radius;
    int mantle_radius;
    int id;
    bool destroyed;

    // Gravitational stress accumulation per cell
    std::vector<float> stress_map;
};
```

### 10.2 Planet Generation

```cpp
void FallingSandSimulation::generate_planet(int center_x, int center_y, int radius) {
    int core_radius = radius / 4;
    int mantle_radius = radius / 2;

    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int dist = static_cast<int>(std::sqrt(dx*dx + dy*dy));
            if (dist > radius) continue;

            int x = center_x + dx;
            int y = center_y + dy;

            ElementType type;
            if (dist <= core_radius) {
                type = ElementType::PLANET_CORE;
            } else if (dist <= mantle_radius) {
                type = ElementType::PLANET_MANTLE;
            } else {
                type = ElementType::PLANET_CRUST;
            }

            // Add some variation
            if (type == ElementType::PLANET_CRUST && (rand() % 10) == 0) {
                type = ElementType::STONE;
            }

            grid->setCell(x, y, type);
        }
    }
}
```

### 10.3 Stress-Based Destruction

- PLANET_CRUST: Low stress threshold - breaks apart first
- PLANET_MANTLE: Medium stress threshold
- PLANET_CORE: High stress threshold - requires sustained gravity to destroy
- When stress exceeds threshold, chunk becomes detachrable (moves toward black hole)
- When near black hole, chunks are consumed

---

## 11. Build System

### 11.1 CMakeLists.txt (Alternative to SCons)

```cmake
cmake_minimum_required(VERSION 3.15)
project(stardust_falling_sand)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Godot
set(GODOT_INCLUDE_DIRS "$ENV{GODOT_INCLUDE_DIRS}" CACHE PATH "Godot include directories")
set(GODOT_LIBRARIES "$ENV{GODOT_LIBRARIES}" CACHE FILEPATH "Godot library")

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
    )
endif()
```

### 11.2 Extension Manifest

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
        "4.3",
        "4.2"
    ]
}
```

---

## 12. Implementation Phases

### Phase 1: Core Port (Week 1)

- [ ] Set up GDExtension project structure
- [ ] Integrate existing FallingSandEngine code
- [ ] Create basic FallingSandSimulation class with GDScript bindings
- [ ] Implement ImageTexture-based rendering
- [ ] Test basic element spawning and physics

### Phase 2: Rendering Optimization (Week 2)

- [ ] Implement GPU buffer rendering for large grids
- [ ] Add shader-based color lookup
- [ ] Optimize simulation loop performance
- [ ] Add dirty-region tracking for partial updates

### Phase 3: Black Hole Mechanics (Week 3)

- [ ] Implement BlackHoleEngine class
- [ ] Add black hole GDScript API
- [ ] Integrate attraction forces into simulation
- [ ] Implement event horizon consumption

### Phase 4: Planet Destruction (Week 4)

- [ ] Add PLANET_CORE, PLANET_MANTLE, PLANET_CRUST elements
- [ ] Implement stress calculation system
- [ ] Add debris spawning on destruction
- [ ] Implement planet generation utilities

### Phase 5: Polish & Testing (Week 5)

- [ ] Performance profiling and optimization
- [ ] Memory leak fixes
- [ ] Edge case handling
- [ ] Documentation and examples

---

## 13. Source Files Reference

### From Ralph Project

These files will be ported directly (with modifications):

- `/Users/kylelampa/Development/Games/falling-sand/ralph/src/FallingSandEngine.h`
- `/Users/kylelampa/Development/Games/falling-sand/ralph/src/FallingSandEngine.cpp`

### Reference Files

- `/Users/kylelampa/Development/Games/falling-sand/ralph/src/main.cpp` - Shows game loop structure
- `/Users/kylelampa/Development/Games/falling-sand/ralph/Makefile` - Build system reference

---

## 14. Verification

### Build Verification

```bash
# Development build (debug symbols)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)

# Release build (optimized)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Runtime Verification

1. Build extension and copy to Godot project `bin/` folder
2. Create test scene with `FallingSandSimulation` node
3. Spawn elements via GDScript - verify physics work
4. Add black holes - verify attraction force
5. Generate planets - verify layered structure
6. Place black hole near planet - verify destruction chain

### Performance Verification

- 500x500 grid at 60 FPS
- 1000x1000 grid at 60 FPS with GPU rendering
- Multiple black holes (up to 16) simultaneously
