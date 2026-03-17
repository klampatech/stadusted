# Stardust — Game & Technical Specification

> **Version:** 0.2  
> **Engine:** Godot 4.6 + C++ GDExtension  
> **Status:** Pre-implementation

---

## Table of Contents

1. [Concept & Vision](#1-concept--vision)
2. [Architecture Overview](#2-architecture-overview)
3. [C++ GDExtension Spec](#3-c-gdextension-spec)
4. [Black Hole Engine](#4-black-hole-engine)
5. [FallingSandSimulation Node](#5-fallindsandsimulation-node)
6. [Rendering Architecture](#6-rendering-architecture)
7. [Planet Destruction System](#7-planet-destruction-system)
8. [Game Design — The Black Hole](#8-game-design--the-black-hole)
9. [Game Design — The Player Ship](#9-game-design--the-player-ship)
10. [Game Design — Celestial Objects](#10-game-design--celestial-objects)
11. [Game Design — Material Economy](#11-game-design--material-economy)
12. [Game Design — Scoring System](#12-game-design--scoring-system)
13. [Game Design — Progression](#13-game-design--progression)
14. [Performance Targets & Memory Budget](#14-performance-targets--memory-budget)
15. [Build System](#15-build-system)
16. [Acceptance Criteria](#16-acceptance-criteria)
17. [Implementation Phases](#17-implementation-phases)
18. [Open Questions & Risks](#18-open-questions--risks)

---

## 1. Concept & Vision

Stardust is a top-down space physics game where the player pilots a ship equipped with a gravity beam, hurling celestial bodies into a black hole that tears them apart particle by particle using a falling sand cellular automaton simulation. The destruction IS the spectacle — and the economy.

**Core loop:**

```
Pull object into tidal zone
  → watch falling sand engine tear it apart layer by layer
    → collect Hawking-ejected material output from far side
      → spend materials on ship upgrades
        → pull bigger, rarer, more valuable objects
```

The falling sand simulation is the differentiating mechanic. Every object is a genuine pixel-level particle grid. The black hole applies real (scaled) inverse-square gravity across that grid, creating spaghettification — the near side strips away faster than the far side, peeling atmosphere, then ocean, then rock, then core in sequence. Players learn to read the material layers and time their pulls to maximize yield.

---

## 2. Architecture Overview

```
┌──────────────────────────────────────────────────────────────────┐
│                   Godot Engine (GDScript host)                    │
│  Scene tree · rendering · input · audio · UI · camera            │
├──────────────────────────────────────────────────────────────────┤
│              C++ GDExtension  (godot_falling_sand.so)            │
│  FallingSandSimulation (Node2D)  ·  GridRenderer                 │
│  BlackHoleEngine  ·  Grid (double-buffered cellular automaton)   │
└──────────────────────────────────────────────────────────────────┘
```

### 2.1 Directory Layout

```
stardust/
├── project/
│   └── stardust.godot
├── src/
│   ├── FallingSandEngine.h / .cpp      # Cellular automaton (ported from ralph)
│   ├── BlackHoleEngine.h / .cpp        # NEW: gravity + event horizon
│   └── godot_extension/
│       ├── register_types.cpp          # GDExtension entry point
│       ├── FallingSandSimulation.h/.cpp # Main Godot Node2D class
│       ├── GridRenderer.h/.cpp         # Texture/shader bridge
│       └── CMakeLists.txt
├── shaders/
│   ├── grid_render.gdshader            # GPU palette lookup (large grids)
│   └── particle_effect.gdshader        # Visual effects
├── extension.gdextension               # Extension manifest
└── README.md
```

---

## 3. C++ GDExtension Spec

### 3.1 Element Types

```cpp
enum class ElementType : uint8_t {
    EMPTY          = 0,
    SAND           = 1,
    WATER          = 2,
    STONE          = 3,
    FIRE           = 4,
    SMOKE          = 5,
    WOOD           = 6,
    OIL            = 7,
    ACID           = 8,
    GUNPOWDER      = 9,
    COAL           = 10,
    COPPER         = 11,
    GOLD           = 12,
    DIAMOND        = 13,
    DIRT           = 14,
    PICKUP_PARTICLES = 15,
    PICKUP_POINTS  = 16,
    GOLD_CRACKED   = 17,
    PLANET_CORE    = 18,   // High density, hardest to destroy
    PLANET_MANTLE  = 19,   // Medium layer
    PLANET_CRUST   = 20,   // Outer layer, breaks first
    BLACK_HOLE     = 21    // Visual/zone marker element
};
```

Each element must have: a distinct RGBA color in the color table, and defined properties for density, flammability, and conductivity.

### 3.2 Grid Class

- **Double-buffered:** `currentBuffer[]` and `nextBuffer[]` of `ElementType`.
- **Memory layout:** Row-major flat array. `500 × 500 × 1 byte = 250 KB per buffer`.
- **Update direction:** Toggles left→right vs right→left each frame to prevent simulation bias.
- **Bounds checking:** All `getCell`/`setCell` validate coordinates before access.
- **Black hole integration:** `setBlackHole()` and `applyAttractionForces()` added to existing Grid class.

### 3.3 Physics Behaviors

| Element | Behavior |
|---------|----------|
| SAND | Falls down; piles at 45° angle; does not fall through WATER |
| WATER | Falls; flows horizontally when blocked; displaced by SAND |
| FIRE | Rises; lifespan 60–180 frames; ignites WOOD, OIL, GUNPOWDER |
| SMOKE | Rises; probability-based dispersal; passes through most elements |
| WOOD | Static; catches fire from adjacent FIRE (probability-based) |
| OIL | Liquid behavior; ignites from FIRE; burns faster than WOOD |
| ACID | Dissolves adjacent destructibles (SAND, WOOD, etc.); immune: STONE, DIAMOND |
| GUNPOWDER | Falls like SAND; explodes when ignited (expands to FIRE/SMOKE in radius) |
| PLANET_CORE | High density; stress threshold 1.0; last layer to break |
| PLANET_MANTLE | Medium density; stress threshold 0.6 |
| PLANET_CRUST | Low density; stress threshold 0.3; breaks first |

---

## 4. Black Hole Engine

### 4.1 BlackHole Struct

```cpp
struct BlackHole {
    float x;                // Grid-space position
    float y;
    float mass;             // Gravitational mass (affects pull strength)
    float event_horizon;    // Radius where elements are consumed (destroyed)
    float influence_radius; // Maximum distance of gravitational effect
    bool  active;

    // Visual (Godot layer)
    float rotation_speed;
    float pulse_phase;
};
```

### 4.2 Constants

```cpp
static constexpr float DEFAULT_MASS             = 1000.0f;
static constexpr float DEFAULT_EVENT_HORIZON    = 8.0f;
static constexpr float DEFAULT_INFLUENCE_RADIUS = 150.0f;
static constexpr int   MAX_BLACK_HOLES          = 16;
static constexpr float MIN_BLACK_HOLE_MASS      = 100.0f;
static constexpr float MAX_BLACK_HOLE_MASS      = 10000.0f;
static constexpr float GRAVITATIONAL_CONSTANT   = 6.674f;   // Scaled for simulation
static constexpr float INVERSE_SQUARE_DAMPING   = 0.5f;     // Prevents extreme near-field force
```

### 4.3 Attraction Force Calculation

```cpp
float BlackHoleEngine::calculateAttraction(float cell_x, float cell_y,
                                           float& out_dx, float& out_dy) const {
    float total_force_x = 0.0f, total_force_y = 0.0f, max_force = 0.0f;

    for (const BlackHole& bh : black_holes) {
        if (!bh.active) continue;

        float dx = bh.x - cell_x;
        float dy = bh.y - cell_y;
        float distance = std::sqrt(dx * dx + dy * dy);

        if (distance > bh.influence_radius || distance < 0.01f) continue;

        float dir_x = dx / distance;
        float dir_y = dy / distance;

        // Inverse-square with close-range damping
        float effective_distance = distance + INVERSE_SQUARE_DAMPING;
        float force = (GRAVITATIONAL_CONSTANT * bh.mass)
                    / (effective_distance * effective_distance);

        force = std::min(force, 2.0f); // Hard cap

        total_force_x += dir_x * force;
        total_force_y += dir_y * force;
        max_force = std::max(max_force, force);
    }

    // Normalize combined direction, scale by clamped magnitude
    float total = std::sqrt(total_force_x * total_force_x +
                             total_force_y * total_force_y);
    if (total > 0.001f) {
        float scale = std::min(total, 1.0f) / total;
        out_dx = total_force_x * scale;
        out_dy = total_force_y * scale;
    } else {
        out_dx = out_dy = 0.0f;
    }

    return std::min(max_force, 1.0f);
}
```

### 4.4 Event Horizon Consumption

```cpp
int BlackHoleEngine::checkEventHorizon(float x, float y) const {
    for (int i = 0; i < (int)black_holes.size(); i++) {
        const BlackHole& bh = black_holes[i];
        if (!bh.active) continue;
        float dx = bh.x - x, dy = bh.y - y;
        if (std::sqrt(dx*dx + dy*dy) < bh.event_horizon)
            return i;   // consumed by black hole at index i
    }
    return -1;          // not consumed
}
```

### 4.5 GDScript Black Hole API

```gdscript
# Add a black hole, returns management index (0–15)
func add_black_hole(x: float, y: float,
                    mass: float = 1000.0,
                    event_horizon: float = 8.0) -> int

func remove_black_hole(index: int) -> void
func set_black_hole_position(index: int, x: float, y: float) -> void
func set_black_hole_mass(index: int, mass: float) -> void
func get_black_hole_count() -> int
func get_black_hole_info(index: int) -> Dictionary
    # Returns: { x, y, mass, event_horizon, active }
func clear_all_black_holes() -> void
func set_black_holes_enabled(enabled: bool) -> void
```

---

## 5. FallingSandSimulation Node

The main Godot-facing class. Extends `Node2D`.

### 5.1 Exported Properties

| Property | Type | Default | Range | Notes |
|----------|------|---------|-------|-------|
| `grid_width` | int | 500 | 100–2000 | Emits property_changed |
| `grid_height` | int | 500 | 100–2000 | Emits property_changed |
| `cell_scale` | int | 4 | — | Pixel scale for world↔grid conversion |
| `simulation_paused` | bool | false | — | Pauses physics tick |
| `time_scale` | float | 1.0 | 0.1–10.0 | Multiplies simulation speed |
| `black_holes_enabled` | bool | true | — | Globally enables/disables gravity |

### 5.2 Element Manipulation

```gdscript
func spawn_element(x: int, y: int, type: int, radius: int = 1) -> void
func spawn_element_at_world(world_pos: Vector2, type: int, radius: int = 1) -> void
func erase_element(x: int, y: int, radius: int = 2) -> void
func erase_element_at_world(world_pos: Vector2, radius: int = 2) -> void
func get_element_at(x: int, y: int) -> int        # Returns EMPTY(0) if OOB
func get_element_at_world(world_pos: Vector2) -> int
func fill_rect(x: int, y: int, w: int, h: int, type: int) -> void
func fill_circle(cx: int, cy: int, radius: int, type: int) -> void
func clear_grid() -> void
func get_element_count(type: int) -> int
```

### 5.3 World Generation

```gdscript
func generate_planet(center_x: int, center_y: int, radius: int) -> void
    # Creates 3-layer planet: core (r/4), mantle (r/2), crust (r)
    # ~10% of crust cells replaced with STONE for variation

func generate_mining_world(seed: int = 12345) -> void
    # Layered world with ores at depth bands
```

### 5.4 Texture & Rendering

```gdscript
var grid_texture: ImageTexture        # Updated each simulation step
func get_grid_texture() -> ImageTexture
func is_texture_dirty() -> bool       # True after step; false after mark_texture_clean()
func mark_texture_clean() -> void
```

### 5.5 Signals

```gdscript
signal element_changed(x: int, y: int, old_type: int, new_type: int)
signal simulation_stepped(delta: float)
signal black_hole_consumed(x: int, y: int, element_type: int)
signal planet_destroyed(planet_id: int)
```

### 5.6 Statistics

```gdscript
var ups: float          # Updates per second
var frame_count: int    # Total simulation steps
func get_ups() -> float
func get_frame_count() -> int
```

---

## 6. Rendering Architecture

### 6.1 Pipeline (500×500 — ImageTexture path)

```
Grid (C++)  →  Image (Godot RGBA8)  →  ImageTexture  →  Sprite2D
```

Each simulation step, `update_grid_image()` iterates the grid and calls `grid_image->set_pixel()` with color from the element's color table. Dynamic elements (FIRE, SMOKE) use per-cell variation functions. After the loop, `texture_dirty = true` and the Godot side calls `grid_texture->update(grid_image)`.

### 6.2 GPU Shader Path (1000×1000+)

```
Grid (C++)  →  PackedByteArray (element IDs)  →  RenderingServer canvas_item
                                                 + grid_render.gdshader
```

```glsl
// shaders/grid_render.gdshader
shader_type canvas_item;

uniform sampler2D grid_data    : filter_nearest; // packed element IDs (R channel)
uniform sampler2D color_palette: filter_nearest; // 256-entry palette
uniform vec2 grid_size;

void fragment() {
    vec2 pixel_pos = floor(UV * grid_size);
    float element_id = texture(grid_data, pixel_pos / grid_size).r;
    vec4 color = texture(color_palette, vec2((element_id + 0.5) / 256.0, 0.5));
    COLOR = color;
}
```

---

## 7. Planet Destruction System

### 7.1 Planet Struct

```cpp
struct Planet {
    int center_x, center_y, radius;
    int core_radius;    // radius / 4
    int mantle_radius;  // radius / 2
    int id;
    bool destroyed;
    std::vector<float> stress_map; // per-cell stress accumulation
};
```

### 7.2 Layer Generation

```cpp
void FallingSandSimulation::generate_planet(int cx, int cy, int radius) {
    int core_radius   = radius / 4;
    int mantle_radius = radius / 2;

    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int dist = (int)std::sqrt(dx*dx + dy*dy);
            if (dist > radius) continue;

            ElementType type;
            if      (dist <= core_radius)   type = ElementType::PLANET_CORE;
            else if (dist <= mantle_radius) type = ElementType::PLANET_MANTLE;
            else                            type = ElementType::PLANET_CRUST;

            // ~10% crust variation
            if (type == ElementType::PLANET_CRUST && (rand() % 10) == 0)
                type = ElementType::STONE;

            grid->setCell(cx + dx, cy + dy, type);
        }
    }
}
```

### 7.3 Stress-Based Destruction

Each planet cell accumulates stress from gravitational force applied by nearby black holes each simulation tick. When accumulated stress exceeds a layer's threshold, the cell becomes detachable and starts moving toward the black hole independently.

| Layer | Stress Threshold | Behavior when broken |
|-------|-----------------|----------------------|
| PLANET_CRUST | 0.3 | First to fracture; debris drifts rapidly |
| PLANET_MANTLE | 0.6 | Breaks after crust is stripped |
| PLANET_CORE | 1.0 | Requires sustained gravity to destroy |

Breaking cells spawn smaller debris chunks in the particle grid (typically converting to STONE or SAND), which are then subject to normal falling sand physics AND black hole gravity. This creates the characteristic peeling/spaghettification visual.

When `get_element_count(PLANET_CORE) + get_element_count(PLANET_MANTLE) + get_element_count(PLANET_CRUST) == 0` for a tracked planet, `planet_destroyed(planet_id)` fires.

### 7.4 Hawking Ejection (Game Mechanic)

When a particle is consumed at the event horizon (`black_hole_consumed` signal fires), the Godot game layer spawns a physical material pickup on the **opposite side** of the black hole from the consumed particle. These pickups drift outward through space and must be collected by the player's ship before escaping the collection zone. This is the core material acquisition loop.

---

## 8. Game Design — The Black Hole

The black hole is a static anchor at the center of each level. All gameplay geometry radiates from it.

### 8.1 Zone Definitions

| Zone | Behavior | Player risk |
|------|----------|-------------|
| **Outer influence** | Objects drift slightly inward; player can reposition freely | None |
| **Tidal zone** | Falling sand engine activates on objects; surface particles strip; score multiplier begins | Low — brief entry safe |
| **Accretion disk** | Heavy particle traffic; material orbits before infall; visually chaotic | High — hull damage over time |
| **Event horizon** | Particles consumed; Hawking ejection fires on far side | Fatal |

Zone radii are parameterized per-level and driven by the `BlackHole` struct's `event_horizon` and `influence_radius` values plus two derived thresholds stored in the level data.

### 8.2 Visual Design

- Gravitational lensing distortion shader warps the grid texture around the event horizon radius.
- Accretion disk rendered as a separate particle effect layer over the simulation texture.
- Zone boundaries are **not** shown explicitly — players learn them through visual particle behavior and audio intensity.

### 8.3 Late-Game: Feeding the Black Hole

Certain materials (specifically exotic matter and tech components) can be fed directly to the black hole to temporarily expand its tidal zone radius or increase its output rate multiplier. This creates a meaningful "invest now vs spend on ship" decision.

---

## 9. Game Design — The Player Ship

### 9.1 Gravity Beam Tool

The ship's primary tool. Generates an artificial gravity gradient between the ship and a target object.

| Parameter | Description | Upgradeable |
|-----------|-------------|-------------|
| Range | Maximum distance to target an object | Yes — unlocks distant objects |
| Strength | Acceleration applied to target | Yes — needed for large objects |
| Width | Narrow (precise point) vs wide (distributed force) | Yes — wide beam unlocked mid-game |
| Mode | Pull (default) or push (toggle) | Push mode is a Tier 3 upgrade |

**Narrow beam** is optimal for peeling specific layers off a partially destroyed body. **Wide beam** is needed to steer large objects without inducing uncontrollable spin. **Push mode** enables repositioning objects mid-transit or creating multi-body chain setups (object A knocks object B into tidal range).

### 9.2 Ship Systems

| System | Description | Depletion / Replenishment |
|--------|-------------|--------------------------|
| Hull integrity | Reduced by accretion disk, particle impacts, gravitational stress | Repaired at orbital station using Iron/Silicate |
| Fuel | Consumed by beam and thrusters | Replenished by collecting Ice/Hydrogen materials |
| Collector bay | Finite hold for harvested materials | Cashed in at orbital station or upgrade point |
| Shield | Brief charge deflecting incoming particles | Recharges over time; capacity upgradeable |

---

## 10. Game Design — Celestial Objects

### 10.1 Object Type Table

| Object | Base Score | Primary Materials | Difficulty | Notes |
|--------|-----------|-------------------|------------|-------|
| Asteroid (small) | Low | Iron, Silicon | Easy | Tutorial objects; plentiful |
| Asteroid (large) | Medium | Iron, Nickel, rare veins | Medium | May tumble unexpectedly |
| Comet | Medium | Ice, Carbon, trace organics | Medium | Fast-moving; harder to intercept |
| Rocky moon | High | Silicate, Iron core | Hard | Large; long destruction window |
| Ice moon | High | Ice, Ammonia, Water | Hard | Liquid layers stream dramatically |
| Gas giant fragment | Very High | Hydrogen, Helium, exotic gases | Very Hard | Partially gaseous; hard to steer |
| Neutron star fragment | Extreme | Dense matter, rare exotics | Extreme | Very high structural integrity |
| Derelict station | Variable | Mixed + tech components | Variable | Contains loot; may explode mid-destruction |

### 10.2 Pixel Material Composition

Each object's particle grid is layered concentrically, mirroring the `generate_planet()` logic but with object-type-specific material assignments:

- **Surface / atmosphere** — dust, regolith, ice caps, gas envelope. Strips first; low yield.
- **Crust** — silicate rock, light metal ores. Primary bulk material.
- **Mantle** — semi-liquid or compressed materials. Flows differently from solid layers.
- **Core** — densest, highest structural integrity. Highest per-particle value.

Revealing the core requires sustained time in the tidal zone. The player must weigh "stay longer for core material" against "accretion disk damage and fuel cost."

---

## 11. Game Design — Material Economy

### 11.1 Material Types

| Material | Primary Source | Primary Use |
|----------|---------------|-------------|
| Iron | Asteroids, rocky moons | Hull plating upgrades |
| Silicate | Asteroid/moon crust | Structural upgrades |
| Ice / Water | Comets, ice moons | Fuel synthesis |
| Carbon | Comets | Beam efficiency upgrades |
| Nickel | Asteroid cores | Collector bay upgrades |
| Hydrogen | Gas giants | Fuel (large quantities) |
| Exotic matter | Neutron fragments, deep cores | Rare Tier 3+ upgrades only |
| Tech components | Derelict stations | Unique / one-of-a-kind unlocks |

### 11.2 Collection Flow

1. `black_hole_consumed(x, y, element_type)` signal fires in Godot.
2. Godot maps `element_type` → material category using a lookup table.
3. Godot spawns a physical pickup node at the mirror position on the far side of the black hole.
4. Pickup drifts outward with a short collection window before escaping.
5. Player flies to collect. Pickups enter collector bay (finite capacity).
6. Player returns to orbital station to bank materials and spend on upgrades.

**Magnet upgrade** (mid-game): auto-collects pickups within a radius, eliminating the collection chase. High-priority quality-of-life unlock that the player will naturally gravitate toward once the pickup chase starts feeling tedious.

---

## 12. Game Design — Scoring System

| Component | Formula | Notes |
|-----------|---------|-------|
| Base score | object type multiplier × mass destroyed | Scales with object rarity |
| Depth bonus | multiplier for tidal zone depth at moment of full consumption | Reward for aggressive play |
| Layer bonus | +score per distinct material layer exposed and destroyed | Reward for full destruction |
| Speed bonus | bonus for time-to-destroy under a per-object target window | Reward clean, precise play |
| Combo | multiplier that stacks across consecutive objects without retreating to safe zone | Resets on zone exit |
| No-damage bonus | bonus for full object destruction with zero hull damage | High-skill bonus |

---

## 13. Game Design — Progression

### 13.1 Run Structure

Each run is a procedurally seeded "system" — a set of objects at varying distances and approach angles around the black hole. A run timer (the black hole is "unstable") creates urgency. Between runs, banked materials are spent on permanent upgrades.

### 13.2 Upgrade Tree

**Tier 1** — Iron + Silicate  
Beam range +1 · Hull plating +1 · Collector bay size +1

**Tier 2** — Nickel + Carbon  
Beam width unlock · Fuel capacity + · Shield charge unlock

**Tier 3** — Exotic matter  
Beam push mode · Dual-beam (grab two objects simultaneously) · Black hole feeding mechanic

**Tier 4** — Tech components (unique unlocks, limited supply)  
Gravity lens (bends beam around obstacles) · Resonance pulse (instant structural integrity collapse) · Tidal extender (expands black hole tidal zone for one object)

---

## 14. Performance Targets & Memory Budget

### 14.1 Frame Rate Targets

| Grid Size | Target | Rendering Method |
|-----------|--------|-----------------|
| 500×500 | 60+ FPS with ≤16 black holes | ImageTexture |
| 1000×1000 | 60+ FPS | GPU buffer + shader |

### 14.2 Memory Budget

| Component | 500×500 | 1000×1000 |
|-----------|---------|-----------|
| Grid buffers (×2) | 500 KB | 2 MB |
| Color table | 88 B | 88 B |
| Image texture (RGBA8) | 1 MB | 4 MB |
| **Total** | ~2.5 MB (limit: 3 MB) | ~10 MB (limit: 12 MB) |

### 14.3 Simulation Step Timing

| Requirement | Target |
|-------------|--------|
| 500×500 simulation step | < 8 ms |
| ImageTexture upload | < 4 ms |

---

## 15. Build System

### 15.1 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.15)
project(stardust_falling_sand)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GODOT_INCLUDE_DIRS "$ENV{GODOT_INCLUDE_DIRS}" CACHE PATH "")
set(GODOT_LIBRARIES    "$ENV{GODOT_LIBRARIES}"    CACHE FILEPATH "")

set(SOURCES
    src/FallingSandEngine.cpp
    src/BlackHoleEngine.cpp
    src/godot_extension/register_types.cpp
    src/godot_extension/FallingSandSimulation.cpp
    src/godot_extension/GridRenderer.cpp
)

add_library(godot_falling_sand SHARED ${SOURCES})

target_include_directories(godot_falling_sand PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${GODOT_INCLUDE_DIRS}
)

target_link_libraries(godot_falling_sand PRIVATE ${GODOT_LIBRARIES})

if(APPLE)
    set_target_properties(godot_falling_sand PROPERTIES
        FRAMEWORK TRUE
        MACOSX_FRAMEWORK_IDENTIFIER "com.stardust.falling-sand"
    )
endif()
```

### 15.2 Extension Manifest (`extension.gdextension`)

```json
{
    "entry_point": "godot_falling_sand_gdextension_init",
    "toolbox": false,
    "rdp": false,
    "script_language": "gdscript",
    "dependencies": [],
    "product_name": "Falling Sand Simulation",
    "product_version": "1.0.0",
    "supported_api_versions": ["4.3", "4.2"]
}
```

### 15.3 Build Commands

```bash
# Debug
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)

# Release
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

---

## 16. Acceptance Criteria

### Phase 1 — Core Engine (11 tests)

| ID | Requirement | Criteria |
|----|-------------|----------|
| T1.1.1 | ElementType enum | All 22 types present with correct ordinal values |
| T1.1.2 | Element colors | Each type has distinct RGBA in color table |
| T1.1.3 | Element properties | Density, flammability, conductivity defined per type |
| T1.2.1 | Double buffering | Two buffers exist; swap occurs each step |
| T1.2.2 | Memory layout | 500×500 uses ≤ 500 KB for two buffers |
| T1.2.3 | Row-major order | Cells accessed row-major for cache efficiency |
| T1.2.4 | Bounds checking | All get/set validate coordinates |
| T1.3.1–T1.3.8 | Physics per element | See element behavior table in §3.3 |

### Phase 2 — Black Hole Engine (15 tests)

| ID | Requirement | Criteria |
|----|-------------|----------|
| T2.1.1 | BlackHole struct fields | x, y, mass, event_horizon, influence_radius, active all present |
| T2.1.2 | Default constants | DEFAULT_MASS=1000, DEFAULT_EVENT_HORIZON=8, DEFAULT_INFLUENCE_RADIUS=150 |
| T2.1.3 | Mass bounds | MIN=100, MAX=10000 enforced |
| T2.1.4 | Max simultaneous | ≥ 16 black holes supported |
| T2.2.1 | Inverse-square law | Force = (6.674 × mass) / (dist² + 0.5) |
| T2.2.2 | Direction | Force vector points toward black hole center |
| T2.2.3 | Distance falloff | Force decreases with squared distance |
| T2.2.4 | Force clamping | Max magnitude capped at 2.0 |
| T2.2.5 | Influence cutoff | No force beyond influence_radius |
| T2.2.6 | Multiple black holes | Forces summed vectorially |
| T2.3.1 | Event horizon consumption | Elements within event_horizon set to EMPTY |
| T2.3.2 | Multiple overlapping horizons | Returns index of consuming black hole |
| T2.3.3 | Out of range | Returns -1 |
| T2.4.1–T2.4.8 | GDScript API methods | See §4.5; each method behaves per spec |

### Phase 3 — FallingSandSimulation Node (14 tests)

| ID | Requirement | Criteria |
|----|-------------|----------|
| T3.1.1–T3.1.5 | Properties | grid_width, grid_height, cell_scale, simulation_paused, time_scale per spec |
| T3.2.1–T3.2.8 | Element manipulation | Each method operates correctly at grid and world coordinates |
| T3.3.1–T3.3.3 | World generation | clear_grid, generate_planet, generate_mining_world work as specified |
| T3.4.1–T3.4.3 | Texture & rendering | grid_texture valid ImageTexture; dirty flag cycles correctly |
| T3.5.1–T3.5.4 | Signals | All four signals fire at correct events |
| T3.6.1–T3.6.3 | Statistics | ups, frame_count, get_element_count all accurate |

### Phase 4 — Planet Destruction (5 tests)

| ID | Requirement | Criteria |
|----|-------------|----------|
| T4.1.1 | Layered generation | generate_planet creates 3 distinct radius-based layers |
| T4.1.2–T4.1.4 | Layer boundaries | core ≤ r/4; mantle ≤ r/2; crust ≤ r |
| T4.2.1 | Stress accumulation | Each planet cell accumulates stress from gravitational forces |
| T4.2.2–T4.2.4 | Break thresholds | Crust=0.3, Mantle=0.6, Core=1.0 |
| T4.2.5 | Debris spawning | Breaking cells produce drifting chunks |

### Phase 5 — Performance (5 tests)

| ID | Requirement | Criteria |
|----|-------------|----------|
| T5.1.1 | 500×500 frame rate | 60+ FPS with ≤16 black holes |
| T5.1.2 | 1000×1000 frame rate | 60+ FPS with GPU shader rendering |
| T5.2.1 | 500×500 memory | ≤ 3 MB total |
| T5.2.2 | 1000×1000 memory | ≤ 12 MB total |
| T5.3.1–T5.3.2 | Timing | Simulation step < 8 ms; texture upload < 4 ms |

### Phase 6 — Build & Integration (5 tests)

| ID | Requirement | Criteria |
|----|-------------|----------|
| T6.1.1–T6.1.3 | CMake builds | Debug and Release builds produce shared library |
| T6.2.1–T6.2.3 | Extension manifest | Entry point, API version, product info per spec |
| T6.3.1–T6.3.4 | Godot integration | Node registered; properties visible in inspector; signals connectable; renders in scene |

**Total: 55 tests across 6 phases.**

---

## 17. Implementation Phases

| Week | Phase | Deliverables |
|------|-------|--------------|
| 1 | Core Port | GDExtension scaffold · FallingSandEngine ported · Basic GDScript bindings · ImageTexture rendering · Element spawning and physics verified |
| 2 | Rendering Optimization | GPU shader path for large grids · Dirty-region tracking · Simulation loop profiling |
| 3 | Black Hole Mechanics | BlackHoleEngine class · Full GDScript API · Attraction forces integrated · Event horizon consumption + Hawking ejection signal |
| 4 | Planet Destruction | PLANET_* elements · Stress map system · Debris spawning · generate_planet() + planet_destroyed signal |
| 5 | Polish & Testing | Performance profiling · Memory leak audit · Edge case hardening · All acceptance criteria green · Documentation |

---

## 18. Open Questions & Risks

**Simulation scale ceiling.** The chunk-streaming/dirty-region approach is essential. Define the maximum active simulation area (recommended: ~300×300 around the black hole) in Week 1 and tune everything around it. Do not commit to 500×500 active until profiling confirms headroom.

**Steering large objects.** The gravity beam on a gas giant needs careful force tuning: apply force to center of mass, let rotation develop naturally from asymmetric beam application. A "stabilizer" upgrade (reduces induced rotation) is a natural Tier 2 addition.

**Material output pacing.** If Hawking ejection spawns too many simultaneous pickups the collection phase becomes stressful and unrewarding. Consider a collection window timer visible to the player, and make the Magnet upgrade (auto-collect) available early at Tier 2.

**Spaghettification readability.** The layered tear-apart must be visually legible without the player reading particle colors. Consider: distinct color per planet layer (core = deep red, mantle = orange, crust = gray), plus audio intensity cues tied to `black_hole_consumed` event rate, plus a "structural integrity" radial bar around the object during active tidal destruction.

**Procedural run variety.** Object placement is straightforward to proceduralize. Object composition (material layer thicknesses, vein distributions) should also be seeded per run so the same object type feels different across runs. This is a data problem — define a `PlanetConfig` struct with seeded parameters in Week 4.

**Godot 4.6 vs spec API version.** The extension manifest lists 4.2 and 4.3 as supported versions. Update to include 4.6 once GDExtension API compatibility is confirmed. If the game targets 4.6 exclusively, simplify the manifest accordingly.
