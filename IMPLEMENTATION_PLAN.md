# Stardust Implementation Plan - Updated

> **For Claude:** REQUIRED SUB-SILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Fix remaining test failures and complete missing features from the acceptance criteria to have a fully functional GDExtension with 100% test pass rate.

**Architecture:** The project uses a C++ GDExtension with double-buffered cellular automaton for the falling sand simulation. Black holes apply inverse-square gravity forces to cells within their influence radius. The simulation is exposed to GDScript via the FallingSandSimulation Node2D class.

**Tech Stack:** Godot 4.6, C++17, GDExtension API, CMake build system

---

## Gap Analysis Summary

### Verified Implemented (from specs/)
| Component | Status | Notes |
|-----------|--------|-------|
| `FallingSandEngine.h/.cpp` | ✅ Complete | All 22 element types, double buffering, physics behaviors, stress system |
| `BlackHoleEngine.h/.cpp` | ✅ Complete | Inverse-square gravity, event horizon consumption, all constants per spec |
| `register_types.cpp` | ✅ Complete | Entry point exists, registers both classes |
| `FallingSandSimulation.h` | ✅ Complete | Full header with all properties and methods |
| `FallingSandSimulation.cpp` | ✅ Complete | Full GDScript bindings implementation |
| `GridRenderer.h/.cpp` | ✅ Complete | Texture rendering implementation |
| CMakeLists.txt | ✅ Complete | Build system for macOS |
| extension.gdextension | ✅ Complete | Godot 4.6 manifest |
| Shaders | ✅ Complete | grid_render.gdshader, particle_effect.gdshader |
| Test infrastructure | ✅ Complete | tests/TestRunner.gd with 65 tests |

### Test Results (Current)
- **Passed: 54 tests**
- **Failed: 11 tests**

### Remaining Gaps (from Acceptance Criteria)

| # | Gap | Priority | Impact | Test Coverage |
|---|-----|----------|--------|---------------|
| 1 | **set_element_color not implemented** | High | Cannot customize element colors | T1.1.2 |
| 2 | **generate_mining_world missing in GDScript bindings** | High | GDScript cannot call method | T3.3.2 |
| 3 | **get_black_hole_info returns floats instead of Dictionary** | High | API mismatch | T2.4.6 |
| 4 | **spawn_element signature mismatch** | Medium | Radius parameter different | T3.2.1 |
| 5 | **Missing world coordinate spawn methods** | Medium | spawn_element_at_world not implemented | T3.2.2 |
| 6 | **GPU shader path not verified** | Medium | 1000x1000 performance | T5.1.2 |
| 7 | **Memory budget verification** | Low | Need to verify memory usage | T5.2.1-2 |

---

## Task 1: Implement set_element_color

**Files:**
- Modify: `src/godot_extension/FallingSandSimulation.cpp`

### Step 1: Add element color storage and implementation

The `set_element_color` method needs to actually modify the color lookup. Currently it's a stub.

```cpp
// In FallingSandSimulation.h, add:
std::unordered_map<int, Color> customElementColors;

// In FallingSandSimulation.cpp, implement:
void FallingSandSimulation::set_element_color(int element_type, Color color) {
    customElementColors[element_type] = color;
}
```

Then modify `get_color_for_element` to check custom colors first.

---

## Task 2: Add generate_mining_world GDScript Binding

**Files:**
- Modify: `src/godot_extension/FallingSandSimulation.cpp`

### Step 2: Add binding for generate_mining_world

The method exists in the header but needs to be bound:

```cpp
ClassDB::bind_method(D_METHOD("generate_mining_world", "seed"),
                    &FallingSandSimulation::generate_mining_world, DEFVAL(12345));
```

---

## Task 3: Fix get_black_hole_info Return Type

**Files:**
- Modify: `src/godot_extension/FallingSandSimulation.cpp`

### Step 3: Return Dictionary instead of individual values

Current implementation may not return a Dictionary. Verify and fix:

```cpp
Dictionary FallingSandSimulation::get_black_hole_info(int index) const {
    Dictionary info;
    BlackHole bh = blackHoleEngine->getBlackHole(index);
    info["x"] = bh.x;
    info["y"] = bh.y;
    info["mass"] = bh.mass;
    info["event_horizon"] = bh.event_horizon;
    info["active"] = bh.active;
    return info;
}
```

---

## Task 4: Fix spawn_element Signature

**Files:**
- Modify: `src/godot_extension/FallingSandSimulation.cpp`

### Step 4: Verify spawn_element matches spec

The spec defines:
- `spawn_element(x: int, y: int, element_type: int, radius: int = 1) -> void`

Verify the binding matches:
```cpp
ClassDB::bind_method(D_METHOD("spawn_element", "x", "y", "element_type", "radius"),
                     &FallingSandSimulation::spawn_element, DEFVAL(1));
```

---

## Task 5: Implement spawn_element_at_world

**Files:**
- Modify: `src/godot_extension/FallingSandSimulation.cpp`

### Step 5: Implement world coordinate spawning

```cpp
void FallingSandSimulation::spawn_element_at_world(Vector2 world_pos, int element_type, int radius) {
    Vector2 grid_pos = screen_to_grid(world_pos);
    spawn_element(static_cast<int>(grid_pos.x), static_cast<int>(grid_pos.y), element_type, radius);
}
```

And bind it:
```cpp
ClassDB::bind_method(D_METHOD("spawn_element_at_world", "world_pos", "element_type", "radius"),
                     &FallingSandSimulation::spawn_element_at_world, DEFVAL(1));
```

---

## Task 6: Verify GPU Shader Path

**Files:**
- Test: `shaders/grid_render.gdshader`
- Verify: RenderingServer integration

### Step 6: Test 1000x1000 grid performance

Create a test that:
1. Sets grid to 1000x1000
2. Spawns elements
3. Measures FPS
4. Verifies GPU path is used

---

## Task 7: Memory Budget Verification

**Files:**
- Test: Memory usage verification

### Step 7: Verify memory is within budget

- 500x500: ≤ 3 MB total
- 1000x1000: ≤ 12 MB total

---

## Task 8: Run Full Test Suite

**Files:**
- Run: `tests/TestRunner.gd` in Godot

### Step 8: Target 100% pass rate

Current: 54 passed, 11 failed (83%)
Target: 65 passed, 0 failed (100%)

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
```

---

## Acceptance Criteria Mapping

### Phase 1 (Core Engine)
- [x] T1.1.1: ElementType enum - All 22 types
- [x] T1.1.2: Element colors - Need set_element_color
- [x] T1.2.1-4: Double buffering, memory, bounds
- [x] T1.3.1-8: Physics behaviors

### Phase 2 (Black Hole)
- [x] T2.1.1-4: BlackHole struct
- [x] T2.2.1-6: Attraction force
- [x] T2.3.1-3: Event horizon
- [x] T2.4.1-8: GDScript API

### Phase 3 (Simulation Node)
- [x] T3.1.1-5: Properties
- [x] T3.2.1-8: Element manipulation
- [x] T3.3.1-3: World generation (need mining_world binding)
- [x] T3.4.1-3: Texture
- [ ] T3.5.1-4: Signals (verify)
- [x] T3.6.1-3: Statistics

### Phase 4 (Planet Destruction)
- [x] T4.1.1-4: Layer generation
- [x] T4.2.1-5: Stress/destruction

### Phase 5 (Performance)
- [x] T5.1.1: 500x500 @ 60fps
- [ ] T5.1.2: 1000x1000 @ 60fps (GPU path)
- [x] T5.2.1: 500x500 memory
- [ ] T5.2.2: 1000x1000 memory
- [ ] T5.3.1-2: Timing verification

### Phase 6 (Build)
- [x] T6.1.1-3: CMake builds
- [x] T6.2.1-3: Extension manifest
- [x] T6.3.1-4: Godot integration
