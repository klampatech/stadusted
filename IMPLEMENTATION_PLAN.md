# Stardust Implementation Plan

> **Status:** Implementation Near Complete
> **Last Updated:** 2026-03-17
> **Goal:** Complete all acceptance criteria with 100% pass rate

---

## Architecture Overview

The project implements a Godot 4.6 GDExtension with:
- **Core Engine:** Double-buffered cellular automaton (500x500 default)
- **Black Hole Physics:** Inverse-square gravity with event horizon consumption
- **Planet Destruction:** Stress-based breaking with debris spawning
- **GDScript API:** Full binding for all simulation operations

**Tech Stack:** Godot 4.6, C++17, GDExtension API, CMake

---

## Implementation Status Summary

| Phase | Tests | Complete | Notes |
|-------|-------|----------|-------|
| Phase 1: Core Engine | 15 | ✅ | All element types and physics behaviors implemented |
| Phase 2: Black Hole | 17 | ✅ | Physics, API methods all complete |
| Phase 3: Simulation Node | 14 | ✅ | Properties, methods, signals bound |
| Phase 4: Planet Destruction | 10 | ✅ | Generation, stress, debris complete |
| Phase 5: Performance | 4 | ⚠️ | Needs runtime verification |
| Phase 6: Build & Integration | 4 | ⚠️ | Missing product info in manifest |

**Total: 82 tests across 6 phases**

---

## Remaining Tasks (Prioritized)

### Task 1: Fix Extension Manifest (Blocking)
- **Files:** `extension.gdextension`
- **Issue:** Missing product_name, product_version, and incomplete API version
- **Fix:** Update to:
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

### Task 2: Implement Custom Element Color Storage
- **Files:** `src/godot_extension/FallingSandSimulation.h`, `.cpp`
- **Issue:** `set_element_color` is stub - needs storage
- **Fix:** Add `std::unordered_map<int, Color>` for custom colors

### Task 3: Run Performance Verification
- **Test 3.1:** 500x500 grid + 16 black holes, verify FPS ≥ 60
- **Test 3.2:** Measure simulation step time, target < 8ms
- **Test 3.3:** Measure texture upload time, target < 4ms
- **Test 3.4:** Verify memory: 500x500 ≤ 3MB, 1000x1000 ≤ 12MB

### Task 4: Run GDScript Tests in Godot
- Open `tests/TestRunner.tscn` in Godot 4.6+
- Target: 82 passed, 0 failed

---

## Detailed Gap Analysis

### Phase 1: Core Engine (Complete ✅)
- ElementType enum (22 types) - All defined in `FallingSandEngine.h`
- Element colors - Defined in `getElementColor()`
- Double buffering - `currentBuffer`/`nextBuffer` implemented
- All physics behaviors (SAND, WATER, FIRE, SMOKE, WOOD, OIL, ACID, GUNPOWDER)

### Phase 2: Black Hole Engine (Complete ✅)
- BlackHole struct with all fields
- Inverse-square force calculation
- Event horizon consumption
- Full GDScript API (add, remove, position, mass, count, info)

### Phase 3: FallingSandSimulation Node (Complete ✅)
- All exported properties (grid_width, grid_height, cell_scale, time_scale, etc.)
- All element manipulation methods
- All world generation methods (clear_grid, generate_planet, generate_mining_world)
- Texture access and dirty flag
- All signals (element_changed, simulation_stepped, black_hole_consumed, planet_destroyed)
- Statistics (ups, frame_count, get_element_count)

### Phase 4: Planet Destruction (Complete ✅)
- Three-layer planet generation (core, mantle, crust)
- Stress accumulation system
- Break thresholds (crust=0.3, mantle=0.6, core=1.0)
- Debris spawning on destruction

### Phase 5: Performance (Needs Verification ⚠️)
| Test | Requirement | Status |
|------|-------------|--------|
| T5.1.1 | 500x500 @ 60fps with 16 black holes | ⚠️ Unverified |
| T5.1.2 | 1000x1000 @ 60fps | ⚠️ Unverified |
| T5.2.1 | 500x500 memory ≤3MB | ⚠️ Unverified |
| T5.2.2 | 1000x1000 memory ≤12MB | ⚠️ Unverified |
| T5.3.1 | Simulation step < 8ms | ⚠️ Unverified |
| T5.3.2 | Texture upload < 4ms | ⚠️ Unverified |

### Phase 6: Build & Integration (Almost Complete ⚠️)
| Test | Requirement | Status |
|------|-------------|--------|
| T6.1.1 | CMake debug build | ✅ Complete |
| T6.1.2 | CMake release build | ✅ Complete |
| T6.2.1 | Entry point correct | ✅ Complete |
| T6.2.2 | Product info in manifest | ❌ Missing |
| T6.3.1 | Node registered as Node2D | ✅ Complete |
| T6.3.2 | Properties visible in inspector | ⚠️ Needs verification |
| T6.3.3 | Signals connectable | ⚠️ Needs verification |
| T6.3.4 | Renders in scene | ⚠️ Needs verification |

---

## Implementation Order

```
1. Fix extension.gdextension manifest (Task 1)
2. Implement custom element colors (Task 2)
3. Run in Godot and verify tests pass (Task 4)
4. Run performance benchmarks (Task 3)
```

---

## Build & Test Commands

```bash
# Build the extension
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Copy to Godot project
cp build/libgodot_falling_sand.dylib addons/godot_falling_sand/

# Run tests
# Open project in Godot 4.6+
# Run tests/TestRunner.tscn
# Target: 82 passed, 0 failed
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
- `extension.gdextension` - Manifest (needs Task 1 fix)
- `CMakeLists.txt` - Build system
- `tests/TestRunner.gd` - Test harness

### Shaders
- `shaders/grid_render.gdshader` - GPU rendering (exists, needs testing)
- `shaders/particle_effect.gdshader` - Visual effects
