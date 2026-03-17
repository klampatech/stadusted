# Stardust Implementation Plan

## Project Overview
- **Project:** Stardust - Black Hole Falling Sand Game
- **Type:** Godot 4.x GDExtension (C++)
- **Status:** Core engine implemented, GDScript integration in progress
- **Target:** 500x500 grid at 60 FPS with 16 black holes

---

## Gap Analysis Summary

| Phase | Spec Tests | Implemented | Gap |
|-------|------------|-------------|-----|
| Phase 1: Core Engine | 11 | 11 | ✅ Complete |
| Phase 2: Black Hole | 15 | 15 | ✅ Complete |
| Phase 3: Simulation Node | 14 | ~10 | ⚠️ Missing signals, world coords |
| Phase 4: Planet Destruction | 5 | 5 | ✅ Complete |
| Phase 5: Performance | 5 | 0 | ❌ Not tested |
| Phase 6: Build & Integration | 5 | 4 | ⚠️ Manifest update needed |

---

## Priority 1: GDScript Integration (Blocking)

### Task 1.1: Complete FallingSandSimulation._bind_methods()
**Files:** `src/godot_extension/FallingSandSimulation.cpp`

The header declares all methods, but bindings need to be implemented for:
- All property getters/setters (grid_width, grid_height, cell_scale, time_scale, simulation_paused)
- All element manipulation methods
- All black hole management methods
- All world generation methods
- Statistics methods
- Signal registrations

**Verification needed:** Check that all methods in the header have corresponding `ClassDB::bind_method()` calls.

### Task 1.2: Implement Signal Emissions
**Files:** `src/godot_extension/FallingSandSimulation.cpp`

The spec requires these signals:
- `element_changed(x, y, old_type, new_type)` - emit when cell changes during simulation
- `simulation_stepped(delta)` - emit after each physics update
- `black_hole_consumed(x, y, element_type)` - emit when element enters event horizon
- `planet_destroyed(planet_id)` - emit when planet is fully destroyed

### Task 1.3: World Position Conversion Methods
**Files:** `src/godot_extension/FallingSandSimulation.cpp`

Implement these methods that accept Vector2 world positions:
- `spawn_element_at_world(Vector2 world_pos, int type, int radius)`
- `erase_element_at_world(Vector2 world_pos, int radius)`
- `get_element_at_world(Vector2 world_pos)`
- `screen_to_grid(Vector2)` and `grid_to_screen(Vector2)`

Formula: `grid_pos = world_pos / cell_scale`

---

## Priority 2: Configuration & Manifest

### Task 2.1: Update extension.gdextension for 4.3 Support
**File:** `extension.gdextension`

Current: `compatibility_minimum = "4.2"`
Spec requires: Support Godot 4.2 and 4.3

Update manifest to include 4.3 compatibility:
```json
[configuration]
compatibility_minimum = "4.2"
```

Note: Godot 4.3 is backward compatible with 4.2 extensions, so minimum 4.2 covers both.

---

## Priority 3: Testing & Verification

### Task 3.1: Run Existing Test Suite
**Files:** `tests/TestRunner.gd`, `tests/TestRunner.tscn`

The test runner exists but needs verification:
- Run in Godot editor
- Verify all Phase 1-4 tests pass
- Document any failures

### Task 3.2: Performance Benchmarking
**Targets from spec:**
- 500x500 grid: 60+ FPS with ≤16 black holes
- 1000x1000 grid: 60+ FPS with GPU shader rendering
- Simulation step: <8ms for 500x500
- Texture upload: <4ms

**Tests needed:**
- T5.1.1: 500x500 FPS test
- T5.1.2: 1000x1000 FPS test (requires GPU shader path)
- T5.2.1: Memory budget check (≤3MB for 500x500)
- T5.2.2: Memory budget check (≤12MB for 1000x1000)
- T5.3.1: Simulation step timing
- T5.3.2: Texture update timing

### Task 3.3: GPU Shader Path Implementation
**Files:** `shaders/grid_render.gdshader`

The shader exists but needs integration with the main simulation for 1000x1000 grids:
- Pass grid data as PackedByteArray
- Use palette lookup for colors
- Verify 60 FPS target is met

---

## Priority 4: Polish & Edge Cases

### Task 4.1: Implement generate_mining_world
**Files:** `src/FallingSandEngine.cpp`, `src/godot_extension/FallingSandSimulation.cpp`

Method declared in header but may need verification:
- `generate_mining_world(int seed = 12345)`
- Creates layered world with ores at specific depths

### Task 4.2: Property Validation
**Files:** `src/godot_extension/FallingSandSimulation.cpp`

Per spec:
- `grid_width`: Default 500, range 100-2000
- `grid_height`: Default 500, range 100-2000
- `cell_scale`: Default 4
- `time_scale`: Default 1.0, range 0.1-10.0
- `simulation_paused`: Default false

Add validation in setters and emit property_changed signals.

### Task 4.3: Grid Resize Handling
**Files:** `src/godot_extension/FallingSandSimulation.cpp`

When grid dimensions change:
- Reallocate buffers
- Recreate Image/ImageTexture
- Reset statistics

---

## Dependency Graph

```
Priority 1 (Blocking)
├── Task 1.1: _bind_methods()
├── Task 1.2: Signal Emissions
└── Task 1.3: World Position Methods
    │
    └── Task 2.1: Manifest Update

Priority 2
└── Task 3.1: Run Test Suite
    │
    ├── Task 3.2: Performance Tests
    │   │
    │   └── Task 3.3: GPU Shader Path
    │
    └── Task 4.1: generate_mining_world
        │
        ├── Task 4.2: Property Validation
        └── Task 4.3: Grid Resize Handling
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
| T2.4.1-8 | GDScript API | ⚠️ Need verification |

### Phase 3: Simulation Node (14 tests) ⚠️
| ID | Test | Status |
|----|------|--------|
| T3.1.1-5 | Properties | ⚠️ Need _bind_methods |
| T3.2.1-8 | Element methods | ⚠️ Need world pos methods |
| T3.3.1-3 | World generation | ⚠️ Need verification |
| T3.4.1-3 | Texture rendering | ⚠️ Need verification |
| T3.5.1-4 | Signals | ❌ Not implemented |
| T3.6.1-3 | Statistics | ⚠️ Need verification |

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
| T6.2.1-3 | Extension manifest | ⚠️ 4.3 compatibility |
| T6.3.1-4 | Godot integration | ⚠️ Need verification |

---

## Next Steps

1. **Immediate:** Complete Task 1.1-1.3 to enable basic GDScript usage
2. **After Phase 3:** Run test suite to verify all functionality
3. **Performance:** Benchmark and optimize if needed
4. **GPU Shader:** Implement for large grid support if performance targets not met
