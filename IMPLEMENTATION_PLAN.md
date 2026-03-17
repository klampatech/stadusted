# Stardust Implementation Plan

## Project Overview

- **Project:** Stardust - Black Hole Falling Sand Game
- **Type:** Godot 4.x GDExtension (C++)
- **Status:** Core implementation complete, integration testing in progress
- **Target:** 500x500 grid at 60 FPS with 16 black holes

---

## Gap Analysis Summary

| Phase | Spec Tests | Implemented | Gap |
|-------|------------|-------------|-----|
| Phase 1: Core Engine | 11 | 10 | ⚠️ 1 gap: explicit element properties |
| Phase 2: Black Hole | 15 | 15 | ✅ Complete |
| Phase 3: Simulation Node | 14 | 13 | ⚠️ 1 gap: radius validation |
| Phase 4: Planet Destruction | 5 | 5 | ✅ Complete |
| Phase 5: Performance | 5 | 0 | ❌ Not benchmarked |
| Phase 6: Build & Integration | 5 | 4 | ⚠️ 1 gap: runtime verification |

---

## Implementation Status (Detailed)

### ✅ Phase 1: Core Engine - MOSTLY COMPLETE

| Component | Status | Files |
|-----------|--------|-------|
| ElementType enum (22 types) | ✅ | `src/FallingSandEngine.h:21-44` |
| Element colors | ✅ | `src/FallingSandEngine.cpp:9-38` |
| Double buffering | ✅ | `src/FallingSandEngine.h:84-85`, `src/FallingSandEngine.cpp:94-100` |
| Row-major layout | ✅ | `src/FallingSandEngine.cpp:83,90` |
| Bounds checking | ✅ | `src/FallingSandEngine.cpp:79-91` |
| Physics behaviors (SAND, WATER, FIRE, SMOKE, WOOD, OIL, ACID, GUNPOWDER) | ✅ | `src/FallingSandEngine.cpp:350-956` |
| Element properties (density, flammability, conductivity) | ⚠️ | Not explicit constants - implicit in physics |

**Gap (P1.1):** Element properties (density, flammability, conductivity) are not defined as explicit constants in the code. The spec requires these to be defined per element type. Currently the behaviors are implicitly handled in the update functions.

### ✅ Phase 2: Black Hole Engine - COMPLETE

| Component | Status | Files |
|-----------|--------|-------|
| BlackHole struct (x, y, mass, event_horizon, influence_radius, active) | ✅ | `src/BlackHoleEngine.h:13-23` |
| Constants (DEFAULT_MASS=1000, DEFAULT_EVENT_HORIZON=8, etc.) | ✅ | `src/BlackHoleEngine.h:34-42` |
| Attraction force (inverse-square law) | ✅ | `src/BlackHoleEngine.cpp:115-169` |
| Event horizon consumption | ✅ | `src/BlackHoleEngine.cpp:171-187` |
| GDScript API (add/remove/set black holes) | ✅ | `src/godot_extension/FallingSandSimulation.cpp:437-488` |

### ⚠️ Phase 3: Simulation Node - MOSTLY COMPLETE

| Component | Status | Files |
|-----------|--------|-------|
| Properties (grid_width, grid_height, cell_scale, simulation_paused, time_scale) | ✅ | `src/godot_extension/FallingSandSimulation.cpp:169-269` |
| Element manipulation methods | ⚠️ | `src/godot_extension/FallingSandSimulation.cpp:272-369` - Need radius validation |
| World generation (clear_grid, generate_planet, generate_mining_world) | ✅ | `src/godot_extension/FallingSandSimulation.cpp:372-393` |
| Texture rendering (grid_texture, is_texture_dirty, mark_texture_clean) | ✅ | `src/godot_extension/FallingSandSimulation.cpp:396-434` |
| Signals (element_changed, simulation_stepped, black_hole_consumed, planet_destroyed) | ✅ | `src/godot_extension/FallingSandSimulation.cpp:726-743` |
| Statistics (ups, frame_count, get_element_count) | ✅ | `src/godot_extension/FallingSandSimulation.cpp:491-502` |

**Gap (P3.1):** `spawn_element`, `erase_element`, `fill_circle` don't validate radius parameter bounds. Negative or extremely large radii could cause undefined behavior.

### ✅ Phase 4: Planet Destruction - COMPLETE

| Component | Status | Files |
|-----------|--------|-------|
| Layered planet generation (core, mantle, crust) | ✅ | `src/FallingSandEngine.cpp:1071-1105` |
| Stress accumulation from gravity | ✅ | `src/FallingSandEngine.cpp:286-303,1166-1184` |
| Break thresholds (CRUST=0.3, MANTLE=0.6, CORE=1.0) | ✅ | `src/FallingSandEngine.cpp:1186-1197` |
| Debris spawning on destruction | ✅ | `src/FallingSandEngine.cpp:1199-1212` |

### ❌ Phase 5: Performance - NOT TESTED

No benchmarking has been performed. Need to verify:
- 500x500 grid at 60+ FPS with ≤16 black holes
- 1000x1000 grid at 60+ FPS (requires GPU shader path)
- Memory budget compliance (≤3MB for 500x500)
- Update timing targets (<8ms simulation, <4ms texture)

### ⚠️ Phase 6: Build & Integration - PARTIAL

| Component | Status | Files |
|-----------|--------|-------|
| CMake build system | ✅ | `CMakeLists.txt` configured |
| Extension manifest | ✅ | `extension.gdextension` present |
| Node registration | ✅ | `src/godot_extension/register_types.cpp` |
| GridRenderer class | ✅ | `src/godot_extension/GridRenderer.h/cpp` skeleton exists |
| Shaders | ✅ | `shaders/grid_render.gdshader` exists but not integrated |
| GDScript bindings | ⚠️ | Need runtime verification in Godot |

---

## Priority Tasks

### P0: Critical Fixes

#### Task P0.1: Add explicit element properties
**Files:** `src/FallingSandEngine.h`, `src/FallingSandEngine.cpp`

**Issue:** Spec requires explicit element properties (density, flammability, conductivity) to be defined per element type. Currently these are implicit in physics functions.

**Fix:** Add property struct/constants:

```cpp
// In FallingSandEngine.h, after ElementType enum:
struct ElementProperties {
    float density;        // For gravity-based sorting
    float flammability;  // 0-1, chance to catch fire
    float conductivity;  // For heat/fire spread
};

ElementProperties getElementProperties(ElementType type);
```

#### Task P0.2: Validate brush radius in spawn/erase methods
**Files:** `src/godot_extension/FallingSandSimulation.cpp`

**Issue:** Methods like `spawn_element`, `erase_element`, `fill_circle` accept radius without bounds validation.

**Fix:** Add radius validation in all affected methods:

```cpp
// Add at start of each method:
radius = std::max(0, std::min(radius, 50));  // Clamp to reasonable max
```

**Affected methods:**
- `spawn_element` (line 272)
- `erase_element` (line 306)
- `fill_circle` (line 364)

---

### P1: Integration Testing

#### Task P1.1: Verify GDScript bindings work in Godot Editor
**Steps:**
1. Load extension in Godot 4.2+
2. Create test scene with FallingSandSimulation node
3. Verify all exported properties visible in Inspector
4. Test basic spawn_element call via GDScript

#### Task P1.2: Verify signal emissions
**Steps:**
1. Connect to `element_changed` signal - verify it fires on spawn
2. Connect to `simulation_stepped` signal - verify fires each frame
3. Connect to `black_hole_consumed` signal - verify fires near black hole
4. Connect to `planet_destroyed` signal - verify fires when planet consumed

#### Task P1.3: Verify black hole API
**Steps:**
1. Call `add_black_hole()` - verify returns valid index (0-15)
2. Call `get_black_hole_info()` - verify returns correct Dictionary with x, y, mass, event_horizon, active
3. Call `remove_black_hole()` - verify removal
4. Test with multiple black holes (up to 16)
5. Test boundary conditions (max mass 10000, min mass 100)

---

### P2: Performance Benchmarking

#### Task P2.1: Run FPS benchmark (500x500)
**Procedure:**
1. Create 500x500 grid
2. Spawn 16 black holes
3. Fill grid with ~50% elements
4. Run simulation for 60 seconds
5. Measure average FPS via Godot Performance monitor

**Pass criteria:** Average FPS ≥ 60

#### Task P2.2: Measure simulation step timing
**Procedure:**
1. Profile `Grid::updateWithBlackHoles()` execution time
2. Run 100 iterations, compute average

**Pass criteria:** Average < 8ms per step

#### Task P2.3: Measure texture upload time
**Procedure:**
1. Profile `get_grid_texture()` execution
2. Focus on the image pixel iteration loop

**Pass criteria:** < 4ms per update

#### Task P2.4: Verify memory budget
**Procedure:**
1. Create 500x500 grid
2. Measure total memory usage

**Pass criteria:** ≤ 3MB total

---

### P3: GPU Shader Path (Optional)

#### Task P3.1: Implement GPU shader rendering for large grids
**Files:** `shaders/grid_render.gdshader`, `src/godot_extension/FallingSandSimulation.cpp`

The spec defines a GPU shader path for 1000x1000+ grids. Currently only ImageTexture path is implemented.

**Required work:**
1. Create PackedByteArray from grid data in C++
2. Pass to shader via uniform
3. Implement color palette lookup in shader
4. Add conditional rendering path based on grid size

**This task is optional** - only needed if 500x500 performance is insufficient or to support larger grids.

---

### P4: Documentation & Cleanup

#### Task P4.1: Create/update README.md
Document how to build and use the extension.

#### Task P4.2: Verify all spec elements implemented
Review STARDUST_SPEC.md and ensure all 55 acceptance criteria are addressed.

---

## Dependency Graph

```
P0: Critical Fixes
├── P0.1: Element properties
└── P0.2: Radius validation
    │
    └── P1: Integration Testing
        ├── P1.1: Verify GDScript bindings
        ├── P1.2: Verify signal emissions
        └── P1.3: Verify black hole API
            │
            └── P2: Performance Benchmarking
                ├── P2.1: FPS benchmark
                ├── P2.2: Step timing
                ├── P2.3: Texture timing
                └── P2.4: Memory budget
                    │
                    └── P3: GPU Shader (optional)
                        │
                        └── P4: Documentation
```

---

## Acceptance Criteria Status

### Phase 1: Core Engine (11 tests) ⚠️
| ID | Test | Status |
|----|------|--------|
| T1.1.1 | ElementType enum (22 types) | ✅ |
| T1.1.2 | Element colors (distinct RGBA) | ✅ |
| T1.1.3 | Element properties (density, flammability, conductivity) | ⚠️ Not explicit |
| T1.2.1 | Double buffering | ✅ |
| T1.2.2 | Memory layout (≤500KB for 500x500) | ✅ |
| T1.2.3 | Row-major order | ✅ |
| T1.2.4 | Bounds checking | ✅ |
| T1.3.1 | SAND physics | ✅ |
| T1.3.2 | WATER physics | ✅ |
| T1.3.3 | FIRE physics | ✅ |
| T1.3.4-8 | SMOKE, WOOD, OIL, ACID, GUNPOWDER | ✅ |

### Phase 2: Black Hole Engine (15 tests) ✅
| ID | Test | Status |
|----|------|--------|
| T2.1.1 | BlackHole struct fields | ✅ |
| T2.1.2 | Default constants | ✅ |
| T2.1.3 | Mass bounds (MIN=100, MAX=10000) | ✅ |
| T2.1.4 | Max simultaneous (≥16) | ✅ |
| T2.2.1 | Inverse-square law | ✅ |
| T2.2.2-6 | Direction, falloff, clamping, influence cutoff, multiple | ✅ |
| T2.3.1-3 | Event horizon consumption | ✅ |
| T2.4.1-8 | GDScript API methods | ✅ |

### Phase 3: Simulation Node (14 tests) ⚠️
| ID | Test | Status |
|----|------|--------|
| T3.1.1-5 | Properties (grid_width, grid_height, cell_scale, simulation_paused, time_scale) | ✅ |
| T3.2.1-8 | Element methods | ⚠️ Need radius validation |
| T3.3.1-3 | World generation | ✅ |
| T3.4.1-3 | Texture rendering | ✅ |
| T3.5.1-4 | Signals | ✅ |
| T3.6.1-3 | Statistics | ✅ |

### Phase 4: Planet Destruction (5 tests) ✅
| ID | Test | Status |
|----|------|--------|
| T4.1.1-4 | Layer generation | ✅ |
| T4.2.1-5 | Stress & destruction | ✅ |

### Phase 5: Performance (5 tests) ❌
| ID | Test | Status |
|----|------|--------|
| T5.1.1 | 500x500 at 60+ FPS | ❌ Not tested |
| T5.1.2 | 1000x1000 at 60+ FPS | ❌ Not tested |
| T5.2.1-2 | Memory budget | ❌ Not tested |
| T5.3.1-2 | Update timing | ❌ Not tested |

### Phase 6: Build & Integration (5 tests) ⚠️
| ID | Test | Status |
|----|------|--------|
| T6.1.1-3 | CMake builds | ✅ |
| T6.2.1-3 | Extension manifest | ✅ |
| T6.3.1-4 | Godot integration | ❌ Not verified |

---

## Next Steps

1. **Immediately:** Fix P0.1 (element properties) and P0.2 (radius validation)
2. **Then:** Verify integration in Godot Editor (P1.x)
3. **Then:** Run performance benchmarks (P2.x)
4. **If needed:** Implement GPU shader path (P3.1)
5. **Finally:** Complete documentation (P4.x)

---

## Notes

- The existing implementation covers all major spec requirements
- Main gaps are in input validation (radius) and explicit element properties
- GPU shader path is optional - ImageTexture path should handle 500x500 at 60 FPS
- GridRenderer class is a skeleton - main rendering uses ImageTexture in FallingSandSimulation
- extension.gdextension points to macOS .dylib - needs paths for other platforms
