# Stardust Implementation Plan

> **Status:** Implementation Complete - Verification Phase
> **Last Updated:** 2026-03-17
> **Goal:** Complete all acceptance criteria with 100% pass rate

---

## Executive Summary

The Stardust project implements a **Godot 4.6 GDExtension** for a falling sand cellular automaton simulation with black hole physics. The core C++ engine, black hole physics, planet destruction mechanics, and GDScript bindings are **fully implemented**. Remaining work focuses on manifest configuration and runtime verification.

**Tech Stack:** Godot 4.6, C++17, GDExtension API, CMake

---

## Implementation Status Summary

| Phase | Tests | Complete | Notes |
|-------|-------|----------|-------|
| Phase 1: Core Engine | 15 | ✅ | All element types and physics behaviors implemented |
| Phase 2: Black Hole | 21 | ✅ | Physics, API methods all complete |
| Phase 3: Simulation Node | 27 | ✅ | Properties, methods, signals bound |
| Phase 4: Planet Destruction | 9 | ✅ | Generation, stress, debris complete |
| Phase 5: Performance | 6 | ⚠️ | Needs runtime verification in Godot |
| Phase 6: Build & Integration | 9 | ⚠️ | Missing product info in manifest |

**Total: 55 acceptance tests across 6 phases + 54 GDScript runtime tests**

---

## Gap Analysis Results

### Phase 1: Core Engine - COMPLETE ✅

| Test ID | Requirement | Status |
|---------|-------------|--------|
| T1.1.1 | ElementType enum (22 types) | ✅ Complete |
| T1.1.2 | Element colors | ✅ Complete |
| T1.1.3 | Element properties | ✅ Complete |
| T1.2.1 | Double buffering | ✅ Complete |
| T1.2.2 | Memory layout (500KB) | ✅ Complete |
| T1.2.3 | Row-major order | ✅ Complete |
| T1.2.4 | Bounds checking | ✅ Complete |
| T1.3.1 | SAND physics | ✅ Complete |
| T1.3.2 | WATER physics | ✅ Complete |
| T1.3.3 | FIRE physics | ✅ Complete |
| T1.3.4 | SMOKE physics | ✅ Complete |
| T1.3.5 | WOOD physics | ✅ Complete |
| T1.3.6 | OIL physics | ✅ Complete |
| T1.3.7 | ACID physics | ✅ Complete |
| T1.3.8 | GUNPOWDER physics | ✅ Complete |

### Phase 2: Black Hole Engine - COMPLETE ✅

| Test ID | Requirement | Status |
|---------|-------------|--------|
| T2.1.1 | BlackHole struct fields | ✅ Complete |
| T2.1.2 | Default constants | ✅ Complete |
| T2.1.3 | Mass bounds | ✅ Complete |
| T2.1.4 | Max 16 black holes | ✅ Complete |
| T2.2.1 | Inverse-square law | ✅ Complete |
| T2.2.2 | Direction | ✅ Complete |
| T2.2.3 | Distance falloff | ✅ Complete |
| T2.2.4 | Force clamping | ✅ Complete |
| T2.2.5 | Influence cutoff | ✅ Complete |
| T2.2.6 | Multiple black holes | ✅ Complete |
| T2.3.1 | Event horizon consumption | ✅ Complete |
| T2.3.2 | Multiple overlapping | ✅ Complete |
| T2.3.3 | Out of range | ✅ Complete |
| T2.4.1 | add_black_hole | ✅ Complete |
| T2.4.2 | remove_black_hole | ✅ Complete |
| T2.4.3 | set_black_hole_position | ✅ Complete |
| T2.4.4 | set_black_hole_mass | ✅ Complete |
| T2.4.5 | get_black_hole_count | ✅ Complete |
| T2.4.6 | get_black_hole_info | ✅ Complete |
| T2.4.7 | clear_all_black_holes | ✅ Complete |
| T2.4.8 | set_black_holes_enabled | ✅ Complete |

### Phase 3: FallingSandSimulation Node - COMPLETE ✅

| Test ID | Requirement | Status |
|---------|-------------|--------|
| T3.1.1 | grid_width | ✅ Complete |
| T3.1.2 | grid_height | ✅ Complete |
| T3.1.3 | cell_scale | ✅ Complete |
| T3.1.4 | simulation_paused | ✅ Complete |
| T3.1.5 | time_scale | ✅ Complete |
| T3.2.1 | spawn_element | ✅ Complete |
| T3.2.2 | spawn_element_at_world | ✅ Complete |
| T3.2.3 | erase_element | ✅ Complete |
| T3.2.4 | erase_element_at_world | ✅ Complete |
| T3.2.5 | get_element_at | ✅ Complete |
| T3.2.6 | get_element_at_world | ✅ Complete |
| T3.2.7 | fill_rect | ✅ Complete |
| T3.2.8 | fill_circle | ✅ Complete |
| T3.3.1 | clear_grid | ✅ Complete |
| T3.3.2 | generate_planet | ✅ Complete |
| T3.3.3 | generate_mining_world | ✅ Complete |
| T3.4.1 | grid_texture | ✅ Complete |
| T3.4.2 | is_texture_dirty | ✅ Complete |
| T3.4.3 | get_grid_texture | ✅ Complete |
| T3.5.1 | element_changed signal | ✅ Complete |
| T3.5.2 | simulation_stepped signal | ✅ Complete |
| T3.5.3 | black_hole_consumed signal | ✅ Complete |
| T3.5.4 | planet_destroyed signal | ✅ Complete |
| T3.6.1 | ups | ✅ Complete |
| T3.6.2 | frame_count | ✅ Complete |
| T3.6.3 | get_element_count | ✅ Complete |

### Phase 4: Planet Destruction - COMPLETE ✅

| Test ID | Requirement | Status |
|---------|-------------|--------|
| T4.1.1 | Layered generation | ✅ Complete |
| T4.1.2 | PLANET_CORE at r/4 | ✅ Complete |
| T4.1.3 | PLANET_MANTLE at r/2 | ✅ Complete |
| T4.1.4 | PLANET_CRUST at r | ✅ Complete |
| T4.2.1 | Stress accumulation | ✅ Complete |
| T4.2.2 | Crust breaks at 0.3 | ✅ Complete |
| T4.2.3 | Mantle breaks at 0.6 | ✅ Complete |
| T4.2.4 | Core breaks at 1.0 | ✅ Complete |
| T4.2.5 | Debris spawning | ✅ Complete |

### Phase 5: Performance - NEEDS VERIFICATION ⚠️

| Test ID | Requirement | Status |
|---------|-------------|--------|
| T5.1.1 | 500x500 @ 60fps with 16 black holes | ⚠️ Unverified |
| T5.1.2 | 1000x1000 @ 60fps | ⚠️ Unverified |
| T5.2.1 | 500x500 memory ≤3MB | ⚠️ Unverified |
| T5.2.2 | 1000x1000 memory ≤12MB | ⚠️ Unverified |
| T5.3.1 | Simulation step < 8ms | ⚠️ Unverified |
| T5.3.2 | Texture upload < 4ms | ⚠️ Unverified |

### Phase 6: Build & Integration - NEEDS COMPLETION ⚠️

| Test ID | Requirement | Status |
|---------|-------------|--------|
| T6.1.1 | CMake debug build | ✅ Complete |
| T6.1.2 | CMake release build | ✅ Complete |
| T6.2.1 | Entry point correct | ✅ Complete |
| T6.2.2 | Product info in manifest | ⚠️ Missing |
| T6.2.3 | API version (4.2-4.6) | ⚠️ Partial |
| T6.3.1 | Node registered as Node2D | ✅ Complete |
| T6.3.2 | Properties visible in inspector | ⚠️ Needs verification |
| T6.3.3 | Signals connectable | ⚠️ Needs verification |
| T6.3.4 | Renders in scene | ⚠️ Needs verification |

---

## Remaining Tasks (Prioritized)

### Task 1: Fix Extension Manifest (Required)
- **Files:** `extension.gdextension`
- **Current State:** Missing `compatibility_maximum` and `[project]` section
- **Current Content:**
```ini
[configuration]
entry_symbol = "godot_falling_sand_gdextension_init"
compatibility_minimum = "4.2"

[libraries]
macos.debug = "res://addons/godot_falling_sand/libgodot_falling_sand.dylib"
macos.release = "res://addons/godot_falling_sand/libgodot_falling_sand.dylib"
```
- **Missing per GDEXTENSION_SPEC.md:**
  - `compatibility_maximum = "4.6"` in [configuration]
  - `[project]` section with `product_name = "Falling Sand Simulation"` and `product_version = "1.0.0"`
- **Impact:** Medium - extension works but Godot may show warnings
- **Fix:** Add missing lines to extension.gdextension:
```ini
[configuration]
entry_symbol = "godot_falling_sand_gdextension_init"
compatibility_minimum = "4.2"
compatibility_maximum = "4.6"

[project]
product_name = "Falling Sand Simulation"
product_version = "1.0.0"

[libraries]
macos.debug = "res://addons/godot_falling_sand/libgodot_falling_sand.dylib"
macos.release = "res://addons/godot_falling_sand/libgodot_falling_sand.dylib"
```
- **Status:** PENDING - Needs to be applied

### Task 2: Implement Custom Element Color Storage (Nice to Have)
- **Files:** `src/godot_extension/FallingSandSimulation.h`, `.cpp`
- **Issue:** `set_element_color` is stub - needs storage
- **Fix:** Add `std::unordered_map<int, Color>` for custom colors in class

### Task 3: Run GDScript Tests in Godot (High Priority)
- **Action:** Open `tests/TestRunner.tscn` in Godot 4.6+
- **Test File:** `tests/TestRunner.gd` - 54 test cases defined
- **Target:** All tests pass

### Task 4: Run Performance Verification (Medium Priority)
- **Test 4.1:** 500x500 grid + 16 black holes, verify FPS ≥ 60
- **Test 4.2:** Measure simulation step time, target < 8ms
- **Test 4.3:** Measure texture upload time, target < 4ms
- **Test 4.4:** Verify memory: 500x500 ≤ 3MB, 1000x1000 ≤ 12MB

---

## Implementation Order

```
1. Fix extension manifest (Task 1) - Required for proper Godot integration
2. Run GDScript tests in Godot (Task 3) - Verify all 54 tests pass
3. Run performance benchmarks (Task 4)
```

---

## Files Reference

### Core Engine
- `src/FallingSandEngine.h` - Element types, Grid class, stress thresholds
- `src/FallingSandEngine.cpp` - Physics implementations, double buffering
- `src/BlackHoleEngine.h` - BlackHole struct, constants
- `src/BlackHoleEngine.cpp` - Attraction calculations, event horizon

### GDExtension
- `src/godot_extension/FallingSandSimulation.h` - Main Node2D class
- `src/godot_extension/FallingSandSimulation.cpp` - GDScript bindings
- `src/godot_extension/GridRenderer.h/.cpp` - Texture rendering
- `src/godot_extension/register_types.cpp` - Entry point

### Configuration
- `extension.gdextension` - Manifest
- `CMakeLists.txt` - Build system
- `tests/TestRunner.gd` - Test harness

### Shaders
- `shaders/grid_render.gdshader` - GPU rendering
- `shaders/particle_effect.gdshader` - Visual effects

---

## Build & Test Commands

```bash
# Build the extension (Debug)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)

# Build the extension (Release)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Copy to Godot project
cp build/libgodot_falling_sand.dylib addons/godot_falling_sand/

# Run tests in Godot
# 1. Open project in Godot 4.6+
# 2. Run tests/TestRunner.tscn
# 3. Target: All tests pass

# Performance verification in Godot
# 1. Create scene with FallingSandSimulation node
# 2. Set grid to 500x500, add 16 black holes
# 3. Monitor FPS (target >= 60)
# 4. Use Performance monitor for memory/timing
```

---

## Spec Reference

- **GDEXTENSION_SPEC.md** - Technical specification for C++ GDExtension
- **STARDUST_SPEC.md** - Game design and architecture
- **STARDUST_ACCEPTANCE_CRITERIA.md** - Test-driven acceptance criteria (97 tests)
- **ACCEPTANCE_CRITERIA.md** - Additional acceptance criteria
