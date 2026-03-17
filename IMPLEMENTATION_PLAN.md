# Stardust Implementation Plan

## Project Overview
- **Project:** Stardust - Black Hole Falling Sand Game
- **Type:** Godot 4.x GDExtension (C++)
- **Status:** Core engine complete, integration testing in progress
- **Target:** 500x500 grid at 60 FPS with 16 black holes

---

## Gap Analysis Summary

| Phase | Spec Tests | Implemented | Gap |
|-------|------------|-------------|-----|
| Phase 1: Core Engine | 11 | 11 | ✅ Complete |
| Phase 2: Black Hole | 15 | 15 | ✅ Complete |
| Phase 3: Simulation Node | 14 | 13 | ⚠️ 1 gap: spawn_element radius validation |
| Phase 4: Planet Destruction | 5 | 5 | ✅ Complete |
| Phase 5: Performance | 5 | 0 | ❌ Not benchmarked |
| Phase 6: Build & Integration | 5 | 4 | ⚠️ Need Godot verification |

---

## Implementation Status (Detailed)

### ✅ Phase 1: Core Engine - COMPLETE

| Component | Status | Files |
|-----------|--------|-------|
| ElementType enum (22 types) | ✅ | `FallingSandEngine.h:21-44` |
| Element colors | ✅ | `FallingSandEngine.cpp:9-38` |
| Double buffering | ✅ | `FallingSandEngine.h:84-85`, `FallingSandEngine.cpp:94-100` |
| Row-major layout | ✅ | `FallingSandEngine.cpp:83,90` |
| Bounds checking | ✅ | `FallingSandEngine.cpp:79-91` |
| Physics behaviors | ✅ | `FallingSandEngine.cpp:350-956` |

### ✅ Phase 2: Black Hole Engine - COMPLETE

| Component | Status | Files |
|-----------|--------|-------|
| BlackHole struct | ✅ | `BlackHoleEngine.h:13-23` |
| Constants | ✅ | `BlackHoleEngine.h:34-42` |
| Attraction force (inverse-square) | ✅ | `BlackHoleEngine.cpp:115-169` |
| Event horizon | ✅ | `BlackHoleEngine.cpp:171-187` |
| GDScript API | ✅ | `FallingSandSimulation.cpp:437-488` |

### ⚠️ Phase 3: Simulation Node - MOSTLY COMPLETE

| Component | Status | Files |
|-----------|--------|-------|
| Properties (grid_width, grid_height, etc.) | ✅ | `FallingSandSimulation.cpp:169-269` |
| Element manipulation | ⚠️ | `FallingSandSimulation.cpp:272-369` - Need radius validation |
| World generation | ✅ | `FallingSandSimulation.cpp:372-393` |
| Texture rendering | ✅ | `FallingSandSimulation.cpp:396-434` |
| Signals | ✅ | `FallingSandSimulation.cpp:726-743` |
| Statistics | ✅ | `FallingSandSimulation.cpp:491-502` |

**Gap:** `spawn_element` and related methods don't validate radius parameter bounds - could cause buffer overflow.

### ✅ Phase 4: Planet Destruction - COMPLETE

| Component | Status | Files |
|-----------|--------|-------|
| Layered generation | ✅ | `FallingSandEngine.cpp:1071-1105` |
| Stress accumulation | ✅ | `FallingSandEngine.cpp:286-303,1166-1184` |
| Break thresholds | ✅ | `FallingSandEngine.cpp:1186-1197` |
| Debris spawning | ✅ | `FallingSandEngine.cpp:1199-1212` |

### ❌ Phase 5: Performance - NOT TESTED

No benchmarking has been performed. Need to verify:
- 500x500 grid at 60+ FPS with ≤16 black holes
- 1000x1000 grid at 60+ FPS (requires GPU shader)
- Memory budget compliance
- Update timing targets

### ⚠️ Phase 6: Build & Integration - NEEDS VERIFICATION

| Component | Status | Files |
|-----------|--------|-------|
| CMake build | ✅ | Configured in project |
| Extension manifest | ✅ | `extension.gdextension` |
| Godot integration | ⚠️ | Need runtime verification |

---

## Priority Tasks

### P0: Critical Fixes

#### Task P0.1: Validate brush radius in spawn/erase methods
**Files:** `src/godot_extension/FallingSandSimulation.cpp`, `src/FallingSandEngine.cpp`

Current issue: `spawn_element(int x, int y, int element_type, int radius = 1)` accepts any radius value without bounds checking.

```cpp
// Add validation in FallingSandSimulation::spawn_element:
if (radius < 0) radius = 0;
if (radius > 50) radius = 50;  // Max reasonable brush size

// Same for erase_element, fill_circle, etc.
```

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
1. Call `add_black_hole()` - verify returns valid index
2. Call `get_black_hole_info()` - verify returns correct Dictionary
3. Call `remove_black_hole()` - verify removal
4. Test with multiple black holes (up to 16)

---

### P2: Performance Benchmarking

#### Task P2.1: Run FPS benchmark (500x500)
**Procedure:**
1. Create 500x500 grid
2. Spawn 16 black holes
3. Run simulation for 60 seconds
4. Measure average FPS via Godot Performance monitor

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

**Pass criteria:** ≤ 3MB

---

### P3: GPU Shader Path (Optional)

#### Task P3.1: Integrate GPU shader for large grids
**Files:** `shaders/grid_render.gdshader`, `src/godot_extension/FallingSandSimulation.cpp`

The shader exists (`shaders/grid_render.gdshader`) but needs:
1. PackedByteArray data passing from C++ to shader
2. Color palette texture setup
3. Conditional rendering path for 1000x1000+ grids

**This task is optional** - only needed if 500x500 performance is insufficient or to support larger grids.

---

### P4: Documentation & Cleanup

#### Task P4.1: Update CHANGELOG.md
Record all implemented features.

#### Task P4.2: Add README.md if missing
Document how to build and use the extension.

---

## Dependency Graph

```
P0: Critical Fixes
└── P0.1: Radius validation
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

### Phase 1: Core Engine (11 tests) ✅
| ID | Test | Status |
|----|------|--------|
| T1.1.1 | ElementType enum | ✅ |
| T1.1.2 | Element colors | ✅ |
| T1.1.3 | Element properties | ✅ |
| T1.2.1 | Double buffering | ✅ |
| T1.2.2 | Memory layout | ✅ |
| T1.2.3 | Row-major order | ✅ |
| T1.2.4 | Bounds checking | ✅ |
| T1.3.1 | SAND physics | ✅ |
| T1.3.2 | WATER physics | ✅ |
| T1.3.3 | FIRE physics | ✅ |
| T1.3.4-8 | Other elements | ✅ |

### Phase 2: Black Hole Engine (15 tests) ✅
| ID | Test | Status |
|----|------|--------|
| T2.1.1-4 | Structure & constants | ✅ |
| T2.2.1-6 | Attraction force | ✅ |
| T2.3.1-3 | Event horizon | ✅ |
| T2.4.1-8 | GDScript API | ✅ |

### Phase 3: Simulation Node (14 tests) ⚠️
| ID | Test | Status |
|----|------|--------|
| T3.1.1-5 | Properties | ✅ |
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
| T5.1.1-2 | Frame rate targets | ❌ Not tested |
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

1. **Immediately:** Fix radius validation (P0.1)
2. **Then:** Verify integration in Godot Editor (P1.x)
3. **Then:** Run performance benchmarks (P2.x)
4. **If needed:** Implement GPU shader path (P3.1)
5. **Finally:** Complete documentation (P4.x)

---

## Notes

- The existing implementation is feature-complete per the spec
- Main gaps are in testing/verification rather than implementation
- GPU shader path is optional - ImageTexture path should handle 500x500 at 60 FPS
- The GridRenderer class is a skeleton - integration into main simulation is optional
