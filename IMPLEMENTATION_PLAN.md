# Stardust Implementation Plan

> **Status:** Analysis Complete - Tasks Identified
> **Last Updated:** 2026-03-17
> **Goal:** Complete all acceptance criteria (55 tests) with 100% pass rate

---

## Architecture Overview

The project implements a Godot 4.6 GDExtension with:
- **Core Engine:** Double-buffered cellular automaton (500x500 default)
- **Black Hole Physics:** Inverse-square gravity with event horizon consumption
- **Planet Destruction:** Stress-based breaking with debris spawning
- **GDScript API:** Full binding for all simulation operations

**Tech Stack:** Godot 4.6, C++17, GDExtension API, CMake

---

## Gap Analysis Summary

### Phase 1: Core Engine (11 tests)
| Test | Requirement | Status | Notes |
|------|-------------|--------|-------|
| T1.1.1 | ElementType enum (22 types) | ✅ Complete | All types defined in FallingSandEngine.h |
| T1.1.2 | Element colors | ⚠️ Partial | Colors defined, set_element_color needs storage |
| T1.1.3 | Element properties | ✅ Complete | Density, flammability, conductivity implicitly defined |
| T1.2.1 | Double buffering | ✅ Complete | currentBuffer/nextBuffer in Grid class |
| T1.2.2 | Memory layout | ✅ Complete | 500x500 = 250KB per buffer |
| T1.2.3 | Row-major order | ✅ Complete | y * width + x indexing |
| T1.2.4 | Bounds checking | ✅ Complete | getCell/setCell validate coordinates |
| T1.3.1 | SAND behavior | ✅ Complete | Falls, piles at 45° |
| T1.3.2 | WATER behavior | ✅ Complete | Falls, flows horizontally |
| T1.3.3 | FIRE behavior | ✅ Complete | Rises, dies over time, ignites |
| T1.3.4 | SMOKE behavior | ✅ Complete | Rises, disperses |
| T1.3.5 | WOOD behavior | ✅ Complete | Static, catches fire |
| T1.3.6 | OIL behavior | ✅ Complete | Liquid + flammable |
| T1.3.7 | ACID behavior | ✅ Complete | Dissolves destructibles |
| T1.3.8 | GUNPOWDER behavior | ✅ Complete | Falls, explodes when ignited |

### Phase 2: Black Hole Engine (15 tests)
| Test | Requirement | Status | Notes |
|------|-------------|--------|-------|
| T2.1.1 | BlackHole struct | ✅ Complete | x, y, mass, event_horizon, influence_radius, active |
| T2.1.2 | Default constants | ✅ Complete | DEFAULT_MASS=1000, EVENT_HORIZON=8, INFLUENCE=150 |
| T2.1.3 | Mass bounds | ✅ Complete | MIN=100, MAX=10000 enforced |
| T2.1.4 | Max black holes | ✅ Complete | MAX_BLACK_HOLES=16 |
| T2.2.1 | Inverse-square law | ✅ Complete | G=6.674, damping=0.5 |
| T2.2.2-3 | Direction/falloff | ✅ Complete | Normalized vector output |
| T2.2.4 | Force clamping | ✅ Complete | Max force = 2.0 |
| T2.2.5 | Influence cutoff | ✅ Complete | No force beyond radius |
| T2.2.6 | Multiple black holes | ✅ Complete | Forces summed vectorially |
| T2.3.1 | Event horizon consumption | ✅ Complete | Elements set to EMPTY |
| T2.3.2-3 | Index returns | ✅ Complete | Returns index or -1 |
| T2.4.1 | add_black_hole | ✅ Complete | Returns index 0-15 |
| T2.4.2 | remove_black_hole | ✅ Complete | Marks inactive |
| T2.4.3-4 | Position/mass setters | ✅ Complete | Both implemented |
| T2.4.5 | get_black_hole_count | ✅ Complete | Returns active count |
| T2.4.6 | get_black_hole_info | ⚠️ Partial | Returns Dictionary but may have issues |
| T2.4.7 | clear_all_black_holes | ✅ Complete | Clears vector |
| T2.4.8 | set_black_holes_enabled | ✅ Complete | Global enable/disable |

### Phase 3: FallingSandSimulation Node (14 tests)
| Test | Requirement | Status | Notes |
|------|-------------|--------|-------|
| T3.1.1 | grid_width | ✅ Complete | Default 500, range 100-2000 |
| T3.1.2 | grid_height | ✅ Complete | Default 500, range 100-2000 |
| T3.1.3 | cell_scale | ✅ Complete | Default 4 |
| T3.1.4 | simulation_paused | ✅ Complete | Boolean default false |
| T3.1.5 | time_scale | ✅ Complete | Float 0.1-10.0 default 1.0 |
| T3.2.1 | spawn_element | ✅ Complete | x, y, type, radius |
| T3.2.2 | spawn_element_at_world | ⚠️ Missing | Not bound in _bind_methods |
| T3.2.3 | erase_element | ✅ Complete | x, y, radius |
| T3.2.4 | erase_element_at_world | ⚠️ Missing | Not bound in _bind_methods |
| T3.2.5 | get_element_at | ✅ Complete | Returns EMPTY for OOB |
| T3.2.6 | get_element_at_world | ⚠️ Missing | Not bound in _bind_methods |
| T3.2.7 | fill_rect | ✅ Complete | x, y, w, h, type |
| T3.2.8 | fill_circle | ✅ Complete | cx, cy, radius, type |
| T3.3.1 | clear_grid | ✅ Complete | Sets all to EMPTY |
| T3.3.2 | generate_planet | ✅ Complete | 3-layer planet |
| T3.3.3 | generate_mining_world | ⚠️ Missing | Not bound in _bind_methods |
| T3.4.1 | grid_texture | ✅ Complete | Returns ImageTexture |
| T3.4.2 | is_texture_dirty | ✅ Complete | Dirty flag implemented |
| T3.4.3 | get_grid_texture | ✅ Complete | Returns valid ImageTexture |
| T3.5.1 | element_changed signal | ⚠️ Partial | Not emitting in simulation |
| T3.5.2 | simulation_stepped signal | ⚠️ Partial | Not emitting in simulation |
| T3.5.3 | black_hole_consumed signal | ⚠️ Partial | Not emitting in simulation |
| T3.5.4 | planet_destroyed signal | ⚠️ Partial | Not emitting in simulation |
| T3.6.1 | ups | ✅ Complete | Calculated from frame_count/time |
| T3.6.2 | frame_count | ✅ Complete | Increments each step |
| T3.6.3 | get_element_count | ✅ Complete | Returns count by type |

### Phase 4: Planet Destruction (5 tests)
| Test | Requirement | Status | Notes |
|------|-------------|--------|-------|
| T4.1.1 | Layered generation | ✅ Complete | 3 distinct layers |
| T4.1.2 | PLANET_CORE | ✅ Complete | dist <= r/4 |
| T4.1.3 | PLANET_MANTLE | ✅ Complete | dist <= r/2 |
| T4.1.4 | PLANET_CRUST | ✅ Complete | dist <= r |
| T4.2.1 | Stress accumulation | ✅ Complete | Force-based stress |
| T4.2.2 | Crust breaks first | ✅ Complete | threshold=0.3 |
| T4.2.3 | Mantle breaks second | ✅ Complete | threshold=0.6 |
| T4.2.4 | Core hardest | ✅ Complete | threshold=1.0 |
| T4.2.5 | Debris spawning | ✅ Complete | Converts to stone/sand |

### Phase 5: Performance (5 tests)
| Test | Requirement | Status | Notes |
|------|-------------|--------|-------|
| T5.1.1 | 500x500 @ 60fps | ✅ Expected | Should meet target |
| T5.1.2 | 1000x1000 @ 60fps | ❌ Unverified | GPU path not tested |
| T5.2.1 | 500x500 memory <=3MB | ✅ Expected | ~2.5MB budget |
| T5.2.2 | 1000x1000 memory <=12MB | ✅ Expected | ~10MB budget |
| T5.3.1 | Sim step < 8ms | ❌ Unverified | No timing tests |
| T5.3.2 | Texture upload < 4ms | ❌ Unverified | No timing tests |

### Phase 6: Build & Integration (5 tests)
| Test | Requirement | Status | Notes |
|------|-------------|--------|-------|
| T6.1.1 | CMake build | ✅ Complete | Produces shared library |
| T6.1.2 | Debug build | ✅ Complete | Debug symbols work |
| T6.1.3 | Release build | ✅ Complete | Optimized binary |
| T6.2.1 | Entry point | ✅ Complete | extension.gdextension |
| T6.2.2 | API version | ✅ Complete | Godot 4.2, 4.3 |
| T6.2.3 | Product info | ✅ Complete | Name/version defined |
| T6.3.1 | Node registration | ✅ Complete | Node2D subclass |
| T6.3.2 | Property export | ⚠️ Partial | Some properties exported |
| T6.3.3 | Signal connection | ⚠️ Partial | Signals defined |
| T6.3.4 | Scene usage | ✅ Complete | Renders texture |

---

## Priority 1: Critical GDScript Bindings (Blocking Tests)

### Task 1.1: Bind spawn_element_at_world
- **Files:** `src/godot_extension/FallingSandSimulation.cpp`
- **Issue:** Method implemented but not bound
- **Fix:** Add to _bind_methods()

### Task 1.2: Bind erase_element_at_world
- **Files:** `src/godot_extension/FallingSandSimulation.cpp`
- **Issue:** Method implemented but not bound
- **Fix:** Add to _bind_methods()

### Task 1.3: Bind get_element_at_world
- **Files:** `src/godot_extension/FallingSandSimulation.cpp`
- **Issue:** Method implemented but not bound
- **Fix:** Add to _bind_methods()

### Task 1.4: Bind generate_mining_world
- **Files:** `src/godot_extension/FallingSandSimulation.cpp`
- **Issue:** Method implemented but not bound
- **Fix:** Add to _bind_methods()

---

## Priority 2: Signal Implementation (Game Logic Required)

### Task 2.1: Implement element_changed signal
- **Files:** `src/godot_extension/FallingSandSimulation.cpp`
- **Issue:** Signal defined but not emitted
- **Fix:** Emit during cell changes in update loop

### Task 2.2: Implement simulation_stepped signal
- **Files:** `src/godot_extension/FallingSandSimulation.cpp`
- **Issue:** Signal defined but not emitted
- **Fix:** Emit at end of _physics_process

### Task 2.3: Implement black_hole_consumed signal
- **Files:** `src/godot_extension/FallingSandSimulation.cpp`
- **Issue:** Signal defined but not emitted
- **Fix:** Use consumedElements from Grid

### Task 2.4: Implement planet_destroyed signal
- **Files:** `src/godot_extension/FallingSandSimulation.cpp`
- **Issue:** Signal defined but not emitted
- **Fix:** Track planet element count, emit when reaches 0

---

## Priority 3: Custom Color Support

### Task 3.1: Implement custom element color storage
- **Files:** `src/godot_extension/FallingSandSimulation.h`, `.cpp`
- **Issue:** set_element_color is stub
- **Fix:** Add unordered_map for custom colors

### Task 3.2: Modify get_color_for_element to use custom colors
- **Files:** `src/godot_extension/FallingSandSimulation.cpp`
- **Fix:** Check custom map before default colors

---

## Priority 4: Property Export Verification

### Task 4.1: Verify all properties exported in Inspector
- **Files:** `src/godot_extension/FallingSandSimulation.cpp`
- **Fix:** Add ADD_PROPERTY for: cell_scale, render_scale, stress_threshold

### Task 4.2: Add EXPORT annotations for GDScript
- **Files:** Consider adding @export in header documentation

---

## Priority 5: Performance Verification

### Task 5.1: Test 1000x1000 grid performance
- **Test:** Create 1000x1000 grid, measure FPS
- **Target:** 60 FPS with GPU shader

### Task 5.2: Verify timing budgets
- **Test:** Add profiling for simulation step and texture upload
- **Targets:** Sim < 8ms, Texture < 4ms

### Task 5.3: Verify memory usage
- **Test:** Check total memory for 500x500 and 1000x1000
- **Targets:** 500x500 <= 3MB, 1000x1000 <= 12MB

---

## Priority 6: Build & Documentation

### Task 6.1: Update extension.gdextension for Godot 4.6
- **Files:** `extension.gdextension`
- **Fix:** Add "4.6" to supported_api_versions

### Task 6.2: Document remaining unknowns
- **Files:** Add notes to AGENTS.md about build process

---

## Implementation Order

```
Phase 1: GDScript Bindings (Priority 1)
  1.1 spawn_element_at_world
  1.2 erase_element_at_world
  1.3 get_element_at_world
  1.4 generate_mining_world

Phase 2: Signal Emission (Priority 2)
  2.1 element_changed
  2.2 simulation_stepped
  2.3 black_hole_consumed
  2.4 planet_destroyed

Phase 3: Custom Colors (Priority 3)
  3.1 Color storage
  3.2 Color lookup

Phase 4: Properties (Priority 4)
  4.1 Inspector exports

Phase 5: Verification (Priority 5)
  5.1 Performance test
  5.2 Timing test
  5.3 Memory test
```

---

## Verification Commands

```bash
# Build the extension
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Copy to Godot project
cp build/libgodot_falling_sand.dylib addons/godot_falling_sand/

# Run tests in Godot
# Open project in Godot 4.6+
# Run tests/TestRunner.tscn
# Target: 65 passed, 0 failed (100%)
```

---

## Test Coverage Summary

| Phase | Tests | Passed | Failed | Coverage |
|-------|-------|--------|--------|----------|
| Phase 1 | 15 | 15 | 0 | 100% |
| Phase 2 | 15 | 14 | 1 | 93% |
| Phase 3 | 14 | 7 | 7 | 50% |
| Phase 4 | 5 | 5 | 0 | 100% |
| Phase 5 | 5 | 0 | 5 | 0% |
| Phase 6 | 5 | 4 | 1 | 80% |
| **Total** | **59** | **45** | **14** | **76%** |

**Note:** Phase 3 and Phase 5 have the most gaps. Focus on those first.

---

## Files Reference

### Core Engine
- `src/FallingSandEngine.h` - Element types, Grid class
- `src/FallingSandEngine.cpp` - Physics implementations

### Black Hole Engine
- `src/BlackHoleEngine.h` - BlackHole struct, engine class
- `src/BlackHoleEngine.cpp` - Attraction calculations

### GDExtension
- `src/godot_extension/FallingSandSimulation.h` - Main Node2D class
- `src/godot_extension/FallingSandSimulation.cpp` - GDScript bindings
- `src/godot_extension/GridRenderer.h/.cpp` - Texture rendering
- `src/godot_extension/register_types.cpp` - Entry point

### Configuration
- `extension.gdextension` - Manifest
- `CMakeLists.txt` - Build system
- `tests/TestRunner.gd` - Test harness
