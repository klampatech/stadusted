# Test-Driven Acceptance Criteria: Stardust GDExtension

## Context

The Stardust project is a Godot 4.6 GDExtension that ports an existing SDL2/C++ Falling Sand Engine with black hole attraction physics. The project is in its initial state - only the technical specification (`GDEXTENSION_SPEC.md`) exists with no implementation code.

The goal is to create detailed, test-driven acceptance criteria that validate all implementation requirements from the spec.

---

## Phase 1: Core Engine Acceptance Criteria

### 1.1 Element Types

| Test ID | Requirement | Acceptance Criteria |
|---------|------------|---------------------|
| T1.1.1 | ElementType enum | Must include all 22 types: EMPTY(0), SAND, WATER, STONE, FIRE, SMOKE, WOOD, OIL, ACID, GUNPOWDER, COAL, COPPER, GOLD, DIAMOND, DIRT, PICKUP_PARTICLES, PICKUP_POINTS, GOLD_CRACKED, PLANET_CORE(18), PLANET_MANTLE(19), PLANET_CRUST(20), BLACK_HOLE(21) |
| T1.1.2 | Element colors | Each element type must have a distinct RGBA color defined in color table |
| T1.1.3 | Element properties | Each element must have defined properties: density, flammability, conductivity |

### 1.2 Grid System

| Test ID | Requirement | Acceptance Criteria |
|---------|------------|---------------------|
| T1.2.1 | Double buffering | Grid must use two buffers (currentBuffer, nextBuffer) for simulation |
| T1.2.2 | Memory layout | 500x500 grid uses <= 500KB for two buffers (250KB each) |
| T1.2.3 | Row-major order | Grid cells accessed in row-major order for cache efficiency |
| T1.2.4 | Bounds checking | All get/set operations validate coordinates are within grid bounds |

### 1.3 Physics Behaviors

| Test ID | Element | Acceptance Criteria |
|---------|---------|---------------------|
| T1.3.1 | SAND | Falls down, piles at 45° angle, does not fall through WATER |
| T1.3.2 | WATER | Falls down, flows horizontally when blocked, displaced by SAND |
| T1.3.3 | FIRE | Rises, dies over time (lifespan 60-180 frames), ignites WOOD/OIL/GUNPOWDER |
| T1.3.4 | SMOKE | Rises, disperses (probability-based removal), rises through most elements |
| T1.3.5 | WOOD | Static, catches fire from adjacent FIRE (probability-based) |
| T1.3.6 | OIL | Liquid behavior + flammable (ignites from FIRE, burns faster than WOOD) |
| T1.3.7 | ACID | Dissolves adjacent destructibles (SAND, WOOD, etc.) except STONE, DIAMOND |
| T1.3.8 | GUNPOWDER | Falls like SAND, explodes when ignited (expands to FIRE/SMOKE in radius) |

---

## Phase 2: Black Hole Engine Acceptance Criteria

### 2.1 Black Hole Structure

| Test ID | Requirement | Acceptance Criteria |
|---------|------------|---------------------|
| T2.1.1 | Structure fields | BlackHole struct has: x, y, mass, event_horizon, influence_radius, active |
| T2.1.2 | Default constants | DEFAULT_MASS=1000, DEFAULT_EVENT_HORIZON=8, DEFAULT_INFLUENCE_RADIUS=150 |
| T2.1.3 | Mass bounds | MIN_BLACK_HOLE_MASS=100, MAX_BLACK_HOLE_MASS=10000 |
| T2.1.4 | Max black holes | System supports at least 16 simultaneous black holes |

### 2.2 Attraction Force Calculation

| Test ID | Requirement | Acceptance Criteria |
|---------|------------|---------------------|
| T2.2.1 | Inverse-square law | Force magnitude = (G * mass) / (distance² + damping), G=6.674 |
| T2.2.2 | Direction | Force vector points toward black hole center |
| T2.2.3 | Distance falloff | Force decreases with squared distance, damped at close range |
| T2.2.4 | Force clamping | Maximum force magnitude capped at 2.0 |
| T2.2.5 | Influence cutoff | No force applied beyond influence_radius |
| T2.2.6 | Multiple black holes | Forces from all active black holes are summed vectorially |

### 2.3 Event Horizon

| Test ID | Requirement | Acceptance Criteria |
|---------|------------|---------------------|
| T2.3.1 | Consumption | Elements within event_horizon radius are removed (set to EMPTY) |
| T2.3.2 | Multiple black holes | Returns index of consuming black hole when multiple overlap |
| T2.3.3 | Out of range | Returns -1 when position not in any event horizon |

### 2.4 Black Hole GDScript API

| Test ID | Method | Acceptance Criteria |
|---------|--------|---------------------|
| T2.4.1 | add_black_hole(x, y, mass, event_horizon) | Returns index (0-15), black hole active after call |
| T2.4.2 | remove_black_hole(index) | Removes black hole, decrements active count |
| T2.4.3 | set_black_hole_position(index, x, y) | Updates position, works with animation |
| T2.4.4 | set_black_hole_mass(index, mass) | Updates mass, affects attraction immediately |
| T2.4.5 | get_black_hole_count() | Returns count of active black holes |
| T2.4.6 | get_black_hole_info(index) | Returns Dictionary with: x, y, mass, event_horizon, active |
| T2.4.7 | clear_all_black_holes() | Removes all black holes, count returns to 0 |
| T2.4.8 | set_black_holes_enabled(bool) | Globally enables/disables attraction physics |

---

## Phase 3: FallingSandSimulation Node Acceptance Criteria

### 3.1 Node Properties

| Test ID | Property | Acceptance Criteria |
|---------|----------|---------------------|
| T3.1.1 | grid_width | Default 500, settable 100-2000, emits property_changed signal |
| T3.1.2 | grid_height | Default 500, settable 100-2000, emits property_changed signal |
| T3.1.3 | cell_scale | Default 4, affects world position conversion |
| T3.1.4 | simulation_paused | Boolean, default false, pauses physics when true |
| T3.1.5 | time_scale | Float 0.1-10.0, default 1.0, multiplies simulation speed |

### 3.2 Element Manipulation Methods

| Test ID | Method | Acceptance Criteria |
|---------|--------|---------------------|
| T3.2.1 | spawn_element(x, y, type, radius) | Fills circle radius at x,y with element type |
| T3.2.2 | spawn_element_at_world(world_pos, type, radius) | Converts world pos to grid, spawns element |
| T3.2.3 | erase_element(x, y, radius) | Sets cells to EMPTY within radius |
| T3.2.4 | erase_element_at_world(world_pos, radius) | Converts world pos to grid, erases |
| T3.2.5 | get_element_at(x, y) | Returns element type, returns EMPTY(0) for out-of-bounds |
| T3.2.6 | get_element_at_world(world_pos) | Returns element type at world position |
| T3.2.7 | fill_rect(x, y, w, h, type) | Fills rectangular area with element type |
| T3.2.8 | fill_circle(cx, cy, radius, type) | Fills circular area with element type |

### 3.3 World Generation Methods

| Test ID | Method | Acceptance Criteria |
|---------|--------|---------------------|
| T3.3.1 | clear_grid() | Sets all cells to EMPTY |
| T3.3.2 | generate_planet(cx, cy, radius) | Creates layered planet: core(r/4), mantle(r/2), crust |
| T3.3.3 | generate_mining_world(seed) | Creates layered world with ores at specific depths |

### 3.4 Texture & Rendering

| Test ID | Requirement | Acceptance Criteria |
|---------|------------|---------------------|
| T3.4.1 | grid_texture | Returns ImageTexture, updated each simulation step |
| T3.4.2 | is_texture_dirty() | Returns true after simulation step, false after mark_texture_clean |
| T3.4.3 | get_grid_texture() | Returns valid ImageTexture usable in Sprite2D |

### 3.5 Signals

| Test ID | Signal | Acceptance Criteria |
|---------|--------|---------------------|
| T3.5.1 | element_changed(x, y, old_type, new_type) | Emitted when cell changes during simulation |
| T3.5.2 | simulation_stepped(delta) | Emitted after each physics update |
| T3.5.3 | black_hole_consumed(x, y, element_type) | Emitted when element enters event horizon |
| T3.5.4 | planet_destroyed(planet_id) | Emitted when planet is fully destroyed |

### 3.6 Statistics

| Test ID | Property | Acceptance Criteria |
|---------|----------|---------------------|
| T3.6.1 | ups | Updates per second, calculated from frame_count/time |
| T3.6.2 | frame_count | Increments by 1 each simulation step |
| T3.6.3 | get_element_count(type) | Returns total count of specific element in grid |

---

## Phase 4: Planet Destruction Acceptance Criteria

### 4.1 Planet Structure

| Test ID | Requirement | Acceptance Criteria |
|---------|------------|---------------------|
| T4.1.1 | Layered generation | generate_planet creates 3 distinct layers based on distance from center |
| T4.1.2 | PLANET_CORE | Created at dist <= radius/4, highest density |
| T4.1.3 | PLANET_MANTLE | Created at radius/4 < dist <= radius/2 |
| T4.1.4 | PLANET_CRUST | Created at radius/2 < dist <= radius |

### 4.2 Destruction Mechanics

| Test ID | Requirement | Acceptance Criteria |
|---------|------------|---------------------|
| T4.2.1 | Stress accumulation | Each planet cell accumulates stress from gravitational forces |
| T4.2.2 | Crust breaks first | PLANET_CRUST has lowest stress threshold, breaks at stress=0.3 |
| T4.2.3 | Mantle breaks second | PLANET_MANTLE has medium threshold, breaks at stress=0.6 |
| T4.2.4 | Core hardest to destroy | PLANET_CORE has highest threshold, breaks at stress=1.0 |
| T4.2.5 | Debris spawning | Breaking cells spawn smaller chunks that can drift |

---

## Phase 5: Performance Acceptance Criteria

### 5.1 Frame Rate Targets

| Test ID | Grid Size | Acceptance Criteria |
|---------|-----------|---------------------|
| T5.1.1 | 500x500 | Maintains 60+ FPS with up to 16 black holes |
| T5.1.2 | 1000x1000 | Maintains 60+ FPS with GPU shader rendering |

### 5.2 Memory Budget

| Test ID | Grid Size | Acceptance Criteria |
|---------|-----------|---------------------|
| T5.2.1 | 500x500 | Total memory usage <= 3MB |
| T5.2.2 | 1000x1000 | Total memory usage <= 12MB |

### 5.3 Update Performance

| Test ID | Requirement | Acceptance Criteria |
|---------|------------|---------------------|
| T5.3.1 | Simulation step | 500x500 grid completes in < 8ms per frame |
| T5.3.2 | Texture update | ImageTexture upload completes in < 4ms |

---

## Phase 6: Integration & Build Acceptance Criteria

### 6.1 Build System

| Test ID | Requirement | Acceptance Criteria |
|---------|------------|---------------------|
| T6.1.1 | CMake build | Project builds with cmake, produces shared library |
| T6.1.2 | Debug build | cmake -DCMAKE_BUILD_TYPE=Debug produces symbols |
| T6.1.3 | Release build | cmake -DCMAKE_BUILD_TYPE=Release produces optimized binary |

### 6.2 GDExtension Manifest

| Test ID | Requirement | Acceptance Criteria |
|---------|------------|---------------------|
| T6.2.1 | Entry point | extension.gdextension defines "godot_falling_sand_gdextension_init" |
| T6.2.2 | API version | Supports Godot 4.2 and 4.3 |
| T6.2.3 | Product info | Defines product_name="Falling Sand Simulation", product_version="1.0.0" |

### 6.3 Godot Integration

| Test ID | Requirement | Acceptance Criteria |
|---------|------------|---------------------|
| T6.3.1 | Node registration | FallingSandSimulation registered as Node2D subclass |
| T6.3.2 | Property export | All properties visible in Godot inspector |
| T6.3.3 | Signal connection | All signals connectable in Godot editor |
| T6.3.4 | Scene usage | Node can be added to scene, renders grid texture |

---

## Test Execution Order

1. **Unit Tests First** (Phase 1-4): Test each component in isolation
   - ElementType enum and colors
   - Grid buffer operations
   - Physics behaviors
   - Black hole calculations

2. **Integration Tests Second** (Phase 3): Test component interactions
   - Black hole + grid interaction
   - Signal emissions
   - Texture updates

3. **Performance Tests Third** (Phase 5): Verify benchmarks
   - Frame rate
   - Memory usage
   - Update timing

4. **Build & Integration Tests Last** (Phase 6): Verify deployment
   - Compilation
   - Godot loading
   - Scene integration

---

## Verification Commands

```bash
# Build verification
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)

# Runtime verification (in Godot editor)
# 1. Create test scene with FallingSandSimulation node
# 2. Run GDScript tests for each API method
# 3. Verify signals fire correctly
# 4. Measure FPS with Performance monitor

# Performance verification
# 1. Set grid to 500x500
# 2. Spawn 16 black holes
# 3. Monitor FPS - should stay >= 60
```

---

## Summary

Total acceptance criteria: **55 tests** across 6 phases

- Phase 1 (Core Engine): 11 tests
- Phase 2 (Black Hole): 15 tests
- Phase 3 (Simulation Node): 14 tests
- Phase 4 (Planet Destruction): 5 tests
- Phase 5 (Performance): 5 tests
- Phase 6 (Build & Integration): 5 tests
