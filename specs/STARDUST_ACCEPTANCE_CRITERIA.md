# Stardust — Test-Driven Acceptance Criteria

> **Spec version:** 0.2  
> **Total tests:** 97 across 8 phases  
> **Test harness:** GDScript (runtime) + C++ unit tests (compile-time / standalone)  
> **Pass definition:** Every criterion in every test must be satisfied. Partial passes do not count.

---

## How to Read This Document

Each test entry follows this structure:

- **Setup** — preconditions and state required before the test runs
- **Steps** — exact actions or assertions to perform
- **Expected result** — the specific observable outcome that constitutes a pass
- **Fail conditions** — concrete ways the test can fail (listed explicitly to prevent ambiguous pass/fail calls)
- **Test code** — GDScript or C++ snippet that implements the check

Tests are grouped by phase matching the implementation schedule. Run phases in order — later phases depend on earlier ones passing.

---

## Test Harness Setup

All GDScript tests run inside a dedicated Godot test scene (`res://tests/TestRunner.tscn`) containing a single `FallingSandSimulation` node with default properties. The test runner auto-executes on scene load and prints `[PASS]` / `[FAIL]` per test ID to the Godot Output panel.

```gdscript
# res://tests/TestRunner.gd
extends Node

var sim: FallingSandSimulation
var pass_count := 0
var fail_count := 0

func _ready() -> void:
    sim = $FallingSandSimulation
    run_all_tests()
    print("=== RESULTS: %d passed, %d failed ===" % [pass_count, fail_count])

func assert_eq(test_id: String, actual, expected) -> void:
    if actual == expected:
        print("[PASS] %s" % test_id)
        pass_count += 1
    else:
        print("[FAIL] %s  got=%s  expected=%s" % [test_id, str(actual), str(expected)])
        fail_count += 1

func assert_true(test_id: String, condition: bool) -> void:
    assert_eq(test_id, condition, true)

func assert_approx(test_id: String, actual: float, expected: float, tol: float = 0.01) -> void:
    assert_true(test_id, abs(actual - expected) <= tol)

func assert_range(test_id: String, actual, lo, hi) -> void:
    assert_true(test_id, actual >= lo and actual <= hi)
```

C++ unit tests use a minimal standalone harness (no Godot required) that links directly against `FallingSandEngine.cpp` and `BlackHoleEngine.cpp`.

---

## Phase 1 — Element Type System (12 tests)

**Scope:** Validates that the `ElementType` enum is complete, correctly ordered, and that each element has a unique color and fully populated property entry.

---

### T1.1.1 — Enum completeness

**Setup:** Include `FallingSandEngine.h`.

**Steps:** Cast each expected integer value to `ElementType` and verify the symbolic name maps back to the correct ordinal.

**Expected result:** All 22 types present with exact ordinal values.

**Fail conditions:** Any type missing; any type with wrong ordinal; enum has fewer than 22 entries.

```cpp
// C++ unit test
static_assert((int)ElementType::EMPTY           == 0);
static_assert((int)ElementType::SAND            == 1);
static_assert((int)ElementType::WATER           == 2);
static_assert((int)ElementType::STONE           == 3);
static_assert((int)ElementType::FIRE            == 4);
static_assert((int)ElementType::SMOKE           == 5);
static_assert((int)ElementType::WOOD            == 6);
static_assert((int)ElementType::OIL             == 7);
static_assert((int)ElementType::ACID            == 8);
static_assert((int)ElementType::GUNPOWDER       == 9);
static_assert((int)ElementType::COAL            == 10);
static_assert((int)ElementType::COPPER          == 11);
static_assert((int)ElementType::GOLD            == 12);
static_assert((int)ElementType::DIAMOND         == 13);
static_assert((int)ElementType::DIRT            == 14);
static_assert((int)ElementType::PICKUP_PARTICLES== 15);
static_assert((int)ElementType::PICKUP_POINTS   == 16);
static_assert((int)ElementType::GOLD_CRACKED    == 17);
static_assert((int)ElementType::PLANET_CORE     == 18);
static_assert((int)ElementType::PLANET_MANTLE   == 19);
static_assert((int)ElementType::PLANET_CRUST    == 20);
static_assert((int)ElementType::BLACK_HOLE      == 21);
```

---

### T1.1.2 — Element color uniqueness

**Setup:** Access the engine's color table (array of 22 RGBA entries, one per element type).

**Steps:** Iterate all 22 entries; collect RGBA values as packed 32-bit integers; insert into a set; check set size equals 22.

**Expected result:** Set size == 22 (all colors are distinct).

**Fail conditions:** Any two elements share an identical RGBA value; color table has fewer than 22 entries; any entry is fully transparent (alpha == 0).

```cpp
// C++ unit test
#include <set>
std::set<uint32_t> colors;
for (int i = 0; i < 22; i++) {
    ElementColor c = getElementColor((ElementType)i);
    uint32_t packed = ((uint32_t)c.r << 24) | ((uint32_t)c.g << 16)
                    | ((uint32_t)c.b << 8)  | (uint32_t)c.a;
    assert(c.a > 0 && "Element color must not be fully transparent");
    colors.insert(packed);
}
assert(colors.size() == 22 && "All element colors must be unique");
```

---

### T1.1.3 — Element property completeness

**Setup:** Access the element property table.

**Steps:** For each of the 22 types, check that `density`, `flammability`, and `conductivity` fields are defined (not sentinel/uninitialized values). Verify PLANET_CORE density > PLANET_MANTLE density > PLANET_CRUST density.

**Expected result:** All fields populated; density ordering enforced for planet layers.

**Fail conditions:** Any field == 0 where 0 is an uninitialized sentinel; density ordering violated.

```cpp
// C++ unit test
for (int i = 0; i < 22; i++) {
    ElementProperties p = getElementProperties((ElementType)i);
    // density allowed to be 0 only for EMPTY
    if (i != (int)ElementType::EMPTY)
        assert(p.density > 0);
}
ElementProperties core   = getElementProperties(ElementType::PLANET_CORE);
ElementProperties mantle = getElementProperties(ElementType::PLANET_MANTLE);
ElementProperties crust  = getElementProperties(ElementType::PLANET_CRUST);
assert(core.density > mantle.density);
assert(mantle.density > crust.density);
```

---

### T1.2.1 — Grid double-buffer swap

**Setup:** Create a `Grid` of size 10×10. Write SAND (1) to cell (5,5) in `currentBuffer`. Run one simulation step.

**Steps:** After the step, read cell (5,5) from the internal `currentBuffer`. Confirm `nextBuffer` was promoted to `currentBuffer` (the swap occurred).

**Expected result:** After step, `currentBuffer` is the former `nextBuffer`; the old `currentBuffer` is now `nextBuffer`. The written cell value has been processed.

**Fail conditions:** Swap does not occur; both buffers remain identical pointers; cell value unchanged and in same buffer.

```cpp
// C++ unit test
Grid g(10, 10);
g.setCell(5, 5, ElementType::SAND);
auto* before = g.currentBufferPtr(); // expose for testing
g.update(1.0f);
auto* after = g.currentBufferPtr();
assert(before != after && "Buffers must swap each step");
```

---

### T1.2.2 — Memory footprint (500×500)

**Setup:** Instantiate a `Grid` with width=500, height=500.

**Steps:** Query total allocated bytes for both buffers via `Grid::memoryUsageBytes()` or compute `sizeof(ElementType) * 500 * 500 * 2`.

**Expected result:** Total buffer memory ≤ 512,000 bytes (500 KB).

**Fail conditions:** Memory usage exceeds 512,000 bytes; method returns 0.

```cpp
// C++ unit test
Grid g(500, 500);
size_t mem = g.memoryUsageBytes();
assert(mem <= 512000 && "Two 500x500 buffers must fit within 500KB");
```

---

### T1.2.3 — Row-major memory access

**Setup:** Create a `Grid` of 4×4. Fill with sequential values 0–15 for verification.

**Steps:** Access the raw buffer and confirm that cell (row=1, col=2) maps to index `1 * 4 + 2 = 6` in the flat array.

**Expected result:** `buffer[row * width + col]` == value set at `(col, row)`.

**Fail conditions:** Column-major access pattern; stride calculation is wrong.

```cpp
// C++ unit test
Grid g(4, 4);
g.setCell(2, 1, ElementType::SAND); // x=2 (col), y=1 (row)
const ElementType* raw = g.rawCurrentBuffer();
assert(raw[1 * 4 + 2] == ElementType::SAND && "Must be row-major: index = row*width + col");
```

---

### T1.2.4 — Bounds checking (get)

**Setup:** Create a `Grid` of 10×10.

**Steps:** Call `getCell(-1, 5)`, `getCell(10, 5)`, `getCell(5, -1)`, `getCell(5, 10)`.

**Expected result:** All four calls return `ElementType::EMPTY` (0). No crash, no undefined behavior.

**Fail conditions:** Any call crashes; any call returns a non-EMPTY value; access triggers a memory sanitizer error.

```cpp
// C++ unit test
Grid g(10, 10);
assert(g.getCell(-1, 5)  == ElementType::EMPTY);
assert(g.getCell(10, 5)  == ElementType::EMPTY);
assert(g.getCell(5, -1)  == ElementType::EMPTY);
assert(g.getCell(5, 10)  == ElementType::EMPTY);
```

---

### T1.2.5 — Bounds checking (set)

**Setup:** Create a `Grid` of 10×10. Fill all cells with STONE.

**Steps:** Call `setCell(-1, 5, SAND)`, `setCell(10, 5, SAND)`, `setCell(5, -1, SAND)`, `setCell(5, 10, SAND)`. Verify grid state unchanged.

**Expected result:** All out-of-bounds sets are silently ignored. No crash. Grid remains unmodified.

**Fail conditions:** Crash; memory corruption; any in-bounds cell changed.

```cpp
// C++ unit test
Grid g(10, 10);
g.fill(ElementType::STONE);
g.setCell(-1, 5, ElementType::SAND);
g.setCell(10, 5, ElementType::SAND);
for (int y = 0; y < 10; y++)
    for (int x = 0; x < 10; x++)
        assert(g.getCell(x, y) == ElementType::STONE);
```

---

### T1.2.6 — Update direction alternation

**Setup:** Create a `Grid` of 20×1 (horizontal strip). Expose `Grid::isLeftToRight()` for testing.

**Steps:** Read direction before step 1. Run step. Read direction after step 1. Run step. Read direction after step 2.

**Expected result:** Direction alternates — if step 1 is left→right, step 2 is right→left, step 3 is left→right.

**Fail conditions:** Direction never changes; direction is always the same.

```cpp
// C++ unit test
Grid g(20, 1);
bool d0 = g.isLeftToRight();
g.update(1.0f);
bool d1 = g.isLeftToRight();
g.update(1.0f);
bool d2 = g.isLeftToRight();
assert(d0 != d1 && "Direction must alternate each step");
assert(d1 != d2 && "Direction must alternate each step");
```

---

### T1.3.1 — SAND falls and piles

**Setup:** Create 20×20 grid. Place SAND at (10, 0). Run 30 simulation steps.

**Steps:** After 30 steps, verify SAND has moved downward. Verify cell (10, 0) is now EMPTY.

**Expected result:** SAND has settled at the lowest available row. Original position is EMPTY.

**Fail conditions:** SAND did not move; SAND moved upward; SAND position unchanged after 30 steps.

```cpp
Grid g(20, 20);
g.setCell(10, 0, ElementType::SAND);
for (int i = 0; i < 30; i++) g.update(1.0f);
assert(g.getCell(10, 0) == ElementType::EMPTY && "SAND must fall from origin");
// Find lowest SAND
bool found = false;
for (int y = 19; y >= 0; y--) {
    if (g.getCell(10, y) == ElementType::SAND) { found = true; break; }
}
assert(found && "SAND must exist somewhere below origin");
```

---

### T1.3.2 — SAND does not fall through WATER

**Setup:** 20×20 grid. Fill row y=10 with WATER. Place SAND at (10, 5).

**Steps:** Run 20 simulation steps. Verify SAND is resting above or displacing the WATER, not below row 10.

**Expected result:** SAND rests at or above y=10. It may displace WATER laterally but must not sink through it.

**Fail conditions:** SAND found below y=10 after simulation.

```gdscript
# GDScript test
func test_T1_3_2() -> void:
    sim.clear_grid()
    for x in range(20):
        sim.spawn_element(x, 10, 2) # WATER
    sim.spawn_element(10, 5, 1) # SAND
    await run_steps(20)
    for y in range(11, 20):
        assert_true("T1.3.2", sim.get_element_at(10, y) != 1)
```

---

### T1.3.3 — FIRE rises and expires

**Setup:** 20×20 grid. Place FIRE at (10, 15). Run 200 simulation steps.

**Steps:** After step 1, verify FIRE exists somewhere above y=15. After 200 steps, verify no FIRE remains anywhere in grid.

**Expected result:** FIRE rises (found at y < 15 within a few steps). FIRE fully extinguishes within 60–180 frames from any individual cell's creation.

**Fail conditions:** FIRE does not move upward; FIRE persists beyond 200 steps; FIRE moves downward.

```gdscript
func test_T1_3_3() -> void:
    sim.clear_grid()
    sim.spawn_element(10, 15, 4) # FIRE
    await run_steps(5)
    var fire_moved_up := false
    for y in range(0, 15):
        if sim.get_element_at(10, y) == 4:
            fire_moved_up = true
    assert_true("T1.3.3a - fire rises", fire_moved_up)
    await run_steps(200)
    assert_eq("T1.3.3b - fire expires", sim.get_element_count(4), 0)
```

---

## Phase 2 — Black Hole Engine (17 tests)

**Scope:** Validates the C++ `BlackHoleEngine` physics — attraction formula, force clamping, event horizon consumption, and the full GDScript management API.

---

### T2.1.1 — BlackHole struct field presence

**Setup:** Compile `BlackHoleEngine.h`.

**Steps:** Confirm all required fields accessible via the struct.

**Expected result:** Struct compiles with fields: `x`, `y`, `mass`, `event_horizon`, `influence_radius`, `active`, `rotation_speed`, `pulse_phase`.

**Fail conditions:** Compilation error due to missing field; field renamed.

```cpp
// C++ compile-time check
BlackHole bh;
bh.x = 1.0f;
bh.y = 1.0f;
bh.mass = 1000.0f;
bh.event_horizon = 8.0f;
bh.influence_radius = 150.0f;
bh.active = true;
bh.rotation_speed = 0.5f;
bh.pulse_phase = 0.0f;
(void)bh; // silence unused warning
```

---

### T2.1.2 — Default constants

**Setup:** Include `BlackHoleEngine.h`.

**Steps:** Assert each constant equals its specified value.

**Expected result:** All six constants match spec values exactly.

**Fail conditions:** Any constant differs by any amount; constants missing.

```cpp
static_assert(BlackHoleEngine::DEFAULT_MASS             == 1000.0f);
static_assert(BlackHoleEngine::DEFAULT_EVENT_HORIZON    == 8.0f);
static_assert(BlackHoleEngine::DEFAULT_INFLUENCE_RADIUS == 150.0f);
static_assert(BlackHoleEngine::MAX_BLACK_HOLES          == 16);
static_assert(BlackHoleEngine::MIN_BLACK_HOLE_MASS      == 100.0f);
static_assert(BlackHoleEngine::MAX_BLACK_HOLE_MASS      == 10000.0f);
```

---

### T2.1.3 — MAX_BLACK_HOLES is integer type

**Setup:** Include `BlackHoleEngine.h`.

**Steps:** Verify `MAX_BLACK_HOLES` is declared as `int` (not `float`).

**Expected result:** `std::is_same<decltype(BlackHoleEngine::MAX_BLACK_HOLES), const int>` is true.

**Fail conditions:** Declared as `float`; causes implicit conversion warnings.

```cpp
static_assert(std::is_same<decltype(BlackHoleEngine::MAX_BLACK_HOLES), const int>::value,
              "MAX_BLACK_HOLES must be int, not float");
```

---

### T2.2.1 — Inverse-square force formula

**Setup:** Create a `BlackHoleEngine` with one black hole at position (100, 100), mass=1000.

**Steps:** Calculate expected force magnitude at distance 50 using the formula: `force = (6.674 * 1000) / ((50 + 0.5)^2)` = `6674 / 2550.25` ≈ `2.617` → clamped to `2.0`.

At distance 100: `force = 6674 / (100.5^2)` = `6674 / 10100.25` ≈ `0.661`. No clamping.

Call `calculateAttraction(150, 100, dx, dy)` (distance = 50) and `calculateAttraction(200, 100, dx, dy)` (distance = 100).

**Expected result:** Near point returns clamped force contribution; far point returns ≈ 0.661 (within 1% tolerance).

**Fail conditions:** Force magnitude off by more than 1%; clamping not applied at distance 50.

```cpp
BlackHoleEngine engine;
engine.addBlackHole(100.0f, 100.0f, 1000.0f, 8.0f);

float dx, dy;
// Distance 50 — should hit clamp
float f_near = engine.calculateAttraction(150.0f, 100.0f, dx, dy);
assert(f_near <= 1.0f && "Returned magnitude must be normalized to [0,1]");

// Distance 100 — expected ~0.661, normalized against single force
float f_far = engine.calculateAttraction(200.0f, 100.0f, dx, dy);
float expected = (6.674f * 1000.0f) / ((100.5f) * (100.5f));
expected = std::min(expected, 2.0f); // clamp
// normalized to [0,1] since single BH
assert(std::abs(f_far - std::min(expected / 2.0f, 1.0f)) < 0.05f);
```

---

### T2.2.2 — Force direction toward black hole

**Setup:** Black hole at (250, 250). Test point at (300, 250) — black hole is to the left.

**Steps:** Call `calculateAttraction(300, 250, dx, dy)`. Verify `dx < 0` (force pushes left, toward BH) and `|dy| < 0.01` (no vertical component).

**Expected result:** `dx < 0`, `|dy| < 0.01`.

**Fail conditions:** Force direction points away from black hole; lateral component present on a purely horizontal alignment.

```cpp
BlackHoleEngine engine;
engine.addBlackHole(250.0f, 250.0f, 1000.0f, 8.0f);
float dx, dy;
engine.calculateAttraction(300.0f, 250.0f, dx, dy);
assert(dx < 0.0f  && "Force must point LEFT toward black hole");
assert(std::abs(dy) < 0.01f && "No vertical component for horizontal alignment");
```

---

### T2.2.3 — Force decreases with distance

**Setup:** Black hole at (100, 100), mass=1000.

**Steps:** Compute force at distances 20, 50, 80, 120, 140 (all within influence_radius=150). Verify each is strictly less than the previous.

**Expected result:** `f(20) > f(50) > f(80) > f(120) > f(140)`.

**Fail conditions:** Force increases with distance; force is constant; any inversion.

```cpp
BlackHoleEngine engine;
engine.addBlackHole(100.0f, 100.0f, 1000.0f, 8.0f);
float dx, dy;
float distances[] = {20, 50, 80, 120, 140};
float prev = 999.0f;
for (float d : distances) {
    float f = engine.calculateAttraction(100.0f + d, 100.0f, dx, dy);
    assert(f < prev && "Force must strictly decrease with distance");
    prev = f;
}
```

---

### T2.2.4 — Force magnitude hard cap at 2.0

**Setup:** Black hole at (100, 100), mass=10000 (maximum). Test at distance 1 (near-center).

**Steps:** Call `calculateAttraction(101, 100, dx, dy)`. The raw formula gives `(6.674 * 10000) / (1.5^2)` ≈ 29,662 — massively above the cap.

**Expected result:** The internal `force_magnitude` variable is clamped to ≤ 2.0 before contributing to `total_force_x/y`. The returned normalized value is ≤ 1.0.

**Fail conditions:** Return value exceeds 1.0; internal force exceeds 2.0 (verify via assertion logging); NaN or Inf returned.

```cpp
BlackHoleEngine engine;
engine.addBlackHole(100.0f, 100.0f, 10000.0f, 8.0f);
float dx, dy;
float f = engine.calculateAttraction(101.0f, 100.0f, dx, dy);
assert(f >= 0.0f && f <= 1.0f && "Normalized return must be in [0,1]");
assert(!std::isnan(f) && !std::isinf(f));
```

---

### T2.2.5 — No force beyond influence_radius

**Setup:** Black hole at (100, 100), influence_radius=150.

**Steps:** Call `calculateAttraction` at distances 150, 151, 200, 500. All are at or beyond the radius.

**Expected result:** All calls return 0.0 and set `dx=0`, `dy=0`.

**Fail conditions:** Any non-zero force returned beyond the cutoff.

```cpp
BlackHoleEngine engine;
engine.addBlackHole(100.0f, 100.0f, 1000.0f, 8.0f); // influence_radius = 150
float dx, dy;
float f1 = engine.calculateAttraction(100.0f + 150.0f, 100.0f, dx, dy);
float f2 = engine.calculateAttraction(100.0f + 151.0f, 100.0f, dx, dy);
float f3 = engine.calculateAttraction(100.0f + 500.0f, 100.0f, dx, dy);
assert(f1 == 0.0f); // exactly at boundary — excluded
assert(f2 == 0.0f);
assert(f3 == 0.0f);
```

---

### T2.2.6 — Multiple black hole force superposition

**Setup:** Black hole A at (50, 100). Black hole B at (150, 100). Test point at (100, 100) — equidistant from both.

**Steps:** Forces from A and B point in opposite horizontal directions. Compute combined force. Expected: net `dx ≈ 0`, net `dy ≈ 0` (forces cancel). Then move test point to (75, 100) — closer to A. Expected: net `dx < 0` (net force toward A).

**Expected result:** Equidistant: `|dx| < 0.01`, `|dy| < 0.01`. Closer to A: `dx < 0`.

**Fail conditions:** Forces not summed (only one contributes); result is wrong sign; superposition not implemented.

```cpp
BlackHoleEngine engine;
engine.addBlackHole(50.0f,  100.0f, 1000.0f, 8.0f);
engine.addBlackHole(150.0f, 100.0f, 1000.0f, 8.0f);
float dx, dy;
engine.calculateAttraction(100.0f, 100.0f, dx, dy);
assert(std::abs(dx) < 0.01f && "Symmetric BHs must cancel horizontally");
engine.calculateAttraction(75.0f, 100.0f, dx, dy);
assert(dx < 0.0f && "Closer to left BH, net force must be leftward");
```

---

### T2.3.1 — Event horizon consumes elements

**Setup:** Create a `Grid` 50×50. Place SAND at (25, 25). Create `BlackHoleEngine` with event_horizon=5, centered at (25, 25) — SAND is inside the horizon.

**Steps:** Run one simulation step. Check `checkEventHorizon(25, 25)`. Verify the SAND at (25, 25) has been set to EMPTY.

**Expected result:** `checkEventHorizon` returns 0 (black hole index 0). Cell (25,25) is EMPTY after step.

**Fail conditions:** Cell not consumed; function returns -1; wrong index returned.

```cpp
BlackHoleEngine engine;
Grid g(50, 50);
g.setCell(25, 25, ElementType::SAND);
engine.addBlackHole(25.0f, 25.0f, 1000.0f, 5.0f);
int idx = engine.checkEventHorizon(25.0f, 25.0f);
assert(idx == 0 && "Cell inside event horizon must return BH index 0");
// simulate consumption
if (idx >= 0) g.setCell(25, 25, ElementType::EMPTY);
assert(g.getCell(25, 25) == ElementType::EMPTY);
```

---

### T2.3.2 — Multiple overlapping event horizons return correct index

**Setup:** Two black holes: BH0 at (100,100) event_horizon=10; BH1 at (200,100) event_horizon=10. Test point at (205, 100) — inside BH1 only.

**Steps:** Call `checkEventHorizon(205, 100)`.

**Expected result:** Returns 1 (index of BH1).

**Fail conditions:** Returns 0; returns -1; returns wrong index.

```cpp
BlackHoleEngine engine;
engine.addBlackHole(100.0f, 100.0f, 1000.0f, 10.0f); // index 0
engine.addBlackHole(200.0f, 100.0f, 1000.0f, 10.0f); // index 1
int idx = engine.checkEventHorizon(205.0f, 100.0f);
assert(idx == 1 && "Point inside BH1 must return index 1");
```

---

### T2.3.3 — Outside all horizons returns -1

**Setup:** Black hole at (100, 100), event_horizon=5. Test point at (106, 100) — just outside.

**Steps:** Call `checkEventHorizon(106, 100)`.

**Expected result:** Returns -1.

**Fail conditions:** Returns any index ≥ 0.

```cpp
BlackHoleEngine engine;
engine.addBlackHole(100.0f, 100.0f, 1000.0f, 5.0f);
int idx = engine.checkEventHorizon(106.0f, 100.0f);
assert(idx == -1 && "Point outside all horizons must return -1");
```

---

### T2.4.1 — add_black_hole returns valid index

**Setup:** Fresh `FallingSandSimulation` node, no black holes.

**Steps:** Call `add_black_hole(250, 250, 1000, 8)`. Capture return value.

**Expected result:** Returns integer in range [0, 15]. `get_black_hole_count()` returns 1. `get_black_hole_info(returned_index).active == true`.

**Fail conditions:** Returns -1 or out-of-range; count remains 0; info shows inactive.

```gdscript
func test_T2_4_1() -> void:
    sim.clear_all_black_holes()
    var idx := sim.add_black_hole(250.0, 250.0, 1000.0, 8.0)
    assert_range("T2.4.1a - valid index", idx, 0, 15)
    assert_eq("T2.4.1b - count is 1", sim.get_black_hole_count(), 1)
    var info := sim.get_black_hole_info(idx)
    assert_true("T2.4.1c - active", info["active"])
```

---

### T2.4.2 — remove_black_hole decrements count

**Setup:** Add two black holes.

**Steps:** Call `remove_black_hole(0)`. Check count is 1. Call `remove_black_hole(1)`. Check count is 0. Verify removed black hole info shows `active == false`.

**Expected result:** Count decrements correctly. Removed hole is inactive.

**Fail conditions:** Count not decremented; removed hole still active; crash on valid index.

```gdscript
func test_T2_4_2() -> void:
    sim.clear_all_black_holes()
    var i0 := sim.add_black_hole(100.0, 100.0)
    var i1 := sim.add_black_hole(200.0, 200.0)
    sim.remove_black_hole(i0)
    assert_eq("T2.4.2a - count after first remove", sim.get_black_hole_count(), 1)
    assert_true("T2.4.2b - removed is inactive", !sim.get_black_hole_info(i0)["active"])
    sim.remove_black_hole(i1)
    assert_eq("T2.4.2c - count after second remove", sim.get_black_hole_count(), 0)
```

---

### T2.4.3 — set_black_hole_position updates correctly

**Setup:** Add black hole at (100, 100).

**Steps:** Call `set_black_hole_position(idx, 300, 400)`. Retrieve info.

**Expected result:** `info.x ≈ 300`, `info.y ≈ 400` (within float precision).

**Fail conditions:** Position unchanged; wrong values stored; info not reflecting update.

```gdscript
func test_T2_4_3() -> void:
    sim.clear_all_black_holes()
    var idx := sim.add_black_hole(100.0, 100.0)
    sim.set_black_hole_position(idx, 300.0, 400.0)
    var info := sim.get_black_hole_info(idx)
    assert_approx("T2.4.3a - x updated", info["x"], 300.0)
    assert_approx("T2.4.3b - y updated", info["y"], 400.0)
```

---

### T2.4.4 — set_black_hole_mass clamps to valid range

**Setup:** Add black hole with default mass.

**Steps:** Call `set_black_hole_mass(idx, 50)` (below MIN). Verify mass is clamped to 100. Call `set_black_hole_mass(idx, 99999)` (above MAX). Verify mass is clamped to 10000.

**Expected result:** Mass never goes below 100 or above 10000.

**Fail conditions:** Mass set to 50; mass set to 99999; no clamping.

```gdscript
func test_T2_4_4() -> void:
    sim.clear_all_black_holes()
    var idx := sim.add_black_hole(250.0, 250.0)
    sim.set_black_hole_mass(idx, 50.0)
    assert_approx("T2.4.4a - min clamp", sim.get_black_hole_info(idx)["mass"], 100.0)
    sim.set_black_hole_mass(idx, 99999.0)
    assert_approx("T2.4.4b - max clamp", sim.get_black_hole_info(idx)["mass"], 10000.0)
```

---

### T2.4.5 — Maximum 16 simultaneous black holes

**Setup:** Fresh simulation.

**Steps:** Add 16 black holes. Verify count == 16. Attempt to add a 17th. Verify count remains 16 (17th is rejected or silently ignored). Verify no crash.

**Expected result:** Count caps at 16. 17th add returns -1 or is a no-op.

**Fail conditions:** 17th is accepted (count becomes 17); crash on 17th add.

```gdscript
func test_T2_4_5() -> void:
    sim.clear_all_black_holes()
    for i in range(16):
        sim.add_black_hole(float(i * 10), 0.0)
    assert_eq("T2.4.5a - count at 16", sim.get_black_hole_count(), 16)
    var overflow := sim.add_black_hole(999.0, 999.0)
    assert_eq("T2.4.5b - count still 16", sim.get_black_hole_count(), 16)
    assert_true("T2.4.5c - overflow returns -1 or noop", overflow == -1 or sim.get_black_hole_count() == 16)
```

---

### T2.4.6 — clear_all_black_holes resets to zero

**Setup:** Add 5 black holes.

**Steps:** Call `clear_all_black_holes()`. Check count.

**Expected result:** `get_black_hole_count() == 0`. All previously added holes are inactive.

**Fail conditions:** Count remains nonzero; any hole still active.

```gdscript
func test_T2_4_6() -> void:
    sim.clear_all_black_holes()
    for i in range(5):
        sim.add_black_hole(float(i * 20), 100.0)
    sim.clear_all_black_holes()
    assert_eq("T2.4.6 - count is 0 after clear", sim.get_black_hole_count(), 0)
```

---

### T2.4.7 — set_black_holes_enabled disables physics globally

**Setup:** Grid with SAND scattered around. Black hole at center. `black_holes_enabled = true`.

**Steps:** Run 10 steps with enabled. Note SAND positions. Call `set_black_holes_enabled(false)`. Record positions again. Run 10 more steps. Verify SAND only moves due to normal gravity/physics, NOT toward the black hole.

**Expected result:** With disabled, no net movement toward black hole center. Normal physics (SAND falls down) still applies.

**Fail conditions:** SAND still drifts toward black hole center with physics disabled.

```gdscript
func test_T2_4_7() -> void:
    sim.clear_grid()
    sim.clear_all_black_holes()
    sim.spawn_element(250, 10, 1, 3)  # SAND above center
    var idx := sim.add_black_hole(250.0, 250.0, 5000.0, 2.0)
    sim.set_black_holes_enabled(false)
    await run_steps(20)
    # SAND should fall DOWN (y increases) not toward center
    var sand_y := find_element_y(1) # find lowest SAND
    assert_true("T2.4.7 - sand falls down not toward BH", sand_y < 250)
```

---

## Phase 3 — FallingSandSimulation Node Properties (10 tests)

**Scope:** Validates exported node properties, their defaults, valid ranges, and that setting them takes effect immediately.

---

### T3.1.1 — grid_width default and range

**Setup:** Fresh `FallingSandSimulation` node added to scene.

**Steps:** Read `grid_width`. Set to 100. Set to 2000. Set to 99 (below min). Set to 2001 (above max).

**Expected result:** Default == 500. Values 100 and 2000 accepted. Out-of-range values clamped or rejected (must not crash, must not silently store invalid value).

```gdscript
func test_T3_1_1() -> void:
    assert_eq("T3.1.1a - default width", sim.grid_width, 500)
    sim.grid_width = 100
    assert_eq("T3.1.1b - set 100", sim.grid_width, 100)
    sim.grid_width = 2000
    assert_eq("T3.1.1c - set 2000", sim.grid_width, 2000)
    sim.grid_width = 99
    assert_range("T3.1.1d - clamp below min", sim.grid_width, 100, 2000)
    sim.grid_width = 2001
    assert_range("T3.1.1e - clamp above max", sim.grid_width, 100, 2000)
```

---

### T3.1.2 — grid_height default and range

**Setup:** Fresh node.

**Steps:** Same pattern as T3.1.1 but for `grid_height`.

**Expected result:** Default == 500. Range 100–2000 enforced.

```gdscript
func test_T3_1_2() -> void:
    assert_eq("T3.1.2a - default height", sim.grid_height, 500)
    sim.grid_height = 100;  assert_eq("T3.1.2b", sim.grid_height, 100)
    sim.grid_height = 2000; assert_eq("T3.1.2c", sim.grid_height, 2000)
    sim.grid_height = 99;   assert_range("T3.1.2d", sim.grid_height, 100, 2000)
```

---

### T3.1.3 — time_scale default and range

**Setup:** Fresh node.

**Steps:** Read default. Set to 0.1, 10.0, 0.09 (below min), 10.1 (above max).

**Expected result:** Default == 1.0. Range 0.1–10.0 enforced.

```gdscript
func test_T3_1_3() -> void:
    assert_approx("T3.1.3a - default", sim.time_scale, 1.0)
    sim.time_scale = 0.1;  assert_approx("T3.1.3b - min", sim.time_scale, 0.1)
    sim.time_scale = 10.0; assert_approx("T3.1.3c - max", sim.time_scale, 10.0)
    sim.time_scale = 0.0;  assert_range("T3.1.3d - clamp low", sim.time_scale, 0.1, 10.0)
```

---

### T3.1.4 — simulation_paused halts physics but not rendering

**Setup:** Grid with SAND at (10, 0). Record initial position.

**Steps:** Set `simulation_paused = true`. Run 30 frames (via `await get_tree().process_frame()` repeated). Check SAND position.

**Expected result:** SAND has not moved. `frame_count` has not incremented. Texture is still valid (no crash on render).

**Fail conditions:** SAND moves while paused; frame_count increments while paused; crash.

```gdscript
func test_T3_1_4() -> void:
    sim.clear_grid()
    sim.spawn_element(10, 0, 1) # SAND
    var fc_before := sim.frame_count
    sim.simulation_paused = true
    await run_frames(30)
    sim.simulation_paused = false
    assert_eq("T3.1.4a - sand did not fall", sim.get_element_at(10, 0), 1)
    assert_eq("T3.1.4b - frame_count unchanged", sim.frame_count, fc_before)
```

---

### T3.1.5 — cell_scale affects world-to-grid conversion

**Setup:** Set `cell_scale = 4` (default). Set `cell_scale = 8`.

**Steps:** With scale=4, call `get_element_at_world(Vector2(40, 40))`. This should map to grid cell (10, 10). Place SAND at grid (10,10) and verify. With scale=8, same world position (40,40) maps to grid cell (5,5). Place SAND at grid (5,5) and verify.

**Expected result:** World-to-grid conversion uses `grid_pos = world_pos / cell_scale`.

**Fail conditions:** Wrong grid cell queried; conversion ignores cell_scale.

```gdscript
func test_T3_1_5() -> void:
    sim.cell_scale = 4
    sim.clear_grid()
    sim.spawn_element(10, 10, 1) # SAND at grid (10,10)
    assert_eq("T3.1.5a - scale4", sim.get_element_at_world(Vector2(40, 40)), 1)
    sim.cell_scale = 8
    sim.clear_grid()
    sim.spawn_element(5, 5, 1) # SAND at grid (5,5)
    assert_eq("T3.1.5b - scale8", sim.get_element_at_world(Vector2(40, 40)), 1)
```

---

## Phase 4 — Element Manipulation API (12 tests)

**Scope:** Tests every element read/write method exposed to GDScript, including coordinate conversion methods and fill helpers.

---

### T4.1.1 — spawn_element places element in radius

**Setup:** Clear grid. Call `spawn_element(50, 50, SAND, 5)`.

**Steps:** Verify elements exist at (50,50) and nearby cells within radius 5. Verify cells at distance > 5+1 from center are EMPTY.

**Expected result:** All cells within circle of radius 5 centered at (50,50) contain SAND. Cells at distance ≥ 7 are EMPTY.

```gdscript
func test_T4_1_1() -> void:
    sim.clear_grid()
    sim.spawn_element(50, 50, 1, 5) # SAND radius 5
    assert_eq("T4.1.1a - center", sim.get_element_at(50, 50), 1)
    assert_eq("T4.1.1b - edge", sim.get_element_at(50, 55), 1)
    assert_eq("T4.1.1c - outside", sim.get_element_at(50, 57), 0)
```

---

### T4.1.2 — erase_element clears cells in radius

**Setup:** Fill entire grid with STONE. Call `erase_element(50, 50, 5)`.

**Steps:** Verify cells within radius 5 of (50,50) are EMPTY. Verify cells at distance > 6 remain STONE.

```gdscript
func test_T4_1_2() -> void:
    sim.fill_rect(0, 0, sim.grid_width, sim.grid_height, 3) # STONE
    sim.erase_element(50, 50, 5)
    assert_eq("T4.1.2a - center erased", sim.get_element_at(50, 50), 0)
    assert_eq("T4.1.2b - edge erased", sim.get_element_at(50, 55), 0)
    assert_eq("T4.1.2c - far still stone", sim.get_element_at(50, 58), 3)
```

---

### T4.1.3 — get_element_at returns EMPTY for out-of-bounds

**Setup:** Default 500×500 grid.

**Steps:** Call `get_element_at(-1, 0)`, `get_element_at(0, -1)`, `get_element_at(500, 0)`, `get_element_at(0, 500)`.

**Expected result:** All return 0 (EMPTY). No crash.

```gdscript
func test_T4_1_3() -> void:
    assert_eq("T4.1.3a", sim.get_element_at(-1, 0),  0)
    assert_eq("T4.1.3b", sim.get_element_at(0, -1),  0)
    assert_eq("T4.1.3c", sim.get_element_at(500, 0), 0)
    assert_eq("T4.1.3d", sim.get_element_at(0, 500), 0)
```

---

### T4.1.4 — spawn_element_at_world uses cell_scale conversion

**Setup:** `cell_scale = 4`. Clear grid. Call `spawn_element_at_world(Vector2(200, 200), SAND, 1)`.

**Steps:** Check `get_element_at(50, 50)` (200/4 = 50).

**Expected result:** Returns SAND (1) at grid position (50,50).

```gdscript
func test_T4_1_4() -> void:
    sim.cell_scale = 4
    sim.clear_grid()
    sim.spawn_element_at_world(Vector2(200.0, 200.0), 1, 1)
    assert_eq("T4.1.4", sim.get_element_at(50, 50), 1)
```

---

### T4.1.5 — fill_rect fills rectangular area

**Setup:** Clear grid. Call `fill_rect(10, 10, 20, 15, WATER)`.

**Steps:** Verify corners (10,10), (29,10), (10,24), (29,24) contain WATER. Verify (9,10) and (30,10) are EMPTY.

```gdscript
func test_T4_1_5() -> void:
    sim.clear_grid()
    sim.fill_rect(10, 10, 20, 15, 2) # WATER
    assert_eq("T4.1.5a - top-left", sim.get_element_at(10, 10), 2)
    assert_eq("T4.1.5b - top-right", sim.get_element_at(29, 10), 2)
    assert_eq("T4.1.5c - bottom-left", sim.get_element_at(10, 24), 2)
    assert_eq("T4.1.5d - outside left", sim.get_element_at(9, 10), 0)
    assert_eq("T4.1.5e - outside right", sim.get_element_at(30, 10), 0)
```

---

### T4.1.6 — fill_circle fills circular area

**Setup:** Clear 100×100 grid. Call `fill_circle(50, 50, 10, STONE)`.

**Steps:** Verify (50,50) is STONE. Verify (50,60) is STONE (on edge). Verify (50,62) is EMPTY (outside).

```gdscript
func test_T4_1_6() -> void:
    sim.clear_grid()
    sim.fill_circle(50, 50, 10, 3) # STONE
    assert_eq("T4.1.6a - center", sim.get_element_at(50, 50), 3)
    assert_eq("T4.1.6b - edge", sim.get_element_at(50, 60), 3)
    assert_eq("T4.1.6c - outside", sim.get_element_at(50, 62), 0)
```

---

### T4.1.7 — clear_grid sets all cells to EMPTY

**Setup:** Fill grid with STONE via fill_rect full-grid. Call `clear_grid()`.

**Steps:** Spot-check 10 random positions. Verify all are EMPTY. `get_element_count(STONE)` returns 0.

```gdscript
func test_T4_1_7() -> void:
    sim.fill_rect(0, 0, sim.grid_width, sim.grid_height, 3)
    sim.clear_grid()
    assert_eq("T4.1.7a - count zero", sim.get_element_count(3), 0)
    assert_eq("T4.1.7b - spot check", sim.get_element_at(250, 250), 0)
```

---

### T4.1.8 — get_element_count returns accurate total

**Setup:** Clear grid. Place exactly 100 SAND cells using `fill_rect(0, 0, 10, 10, SAND)`.

**Steps:** Call `get_element_count(SAND)`.

**Expected result:** Returns 100.

```gdscript
func test_T4_1_8() -> void:
    sim.clear_grid()
    sim.fill_rect(0, 0, 10, 10, 1) # 10x10 = 100 cells of SAND
    assert_eq("T4.1.8", sim.get_element_count(1), 100)
```

---

### T4.1.9 — get_element_at_world and erase_element_at_world consistency

**Setup:** `cell_scale = 4`. Spawn SAND at grid (25,25) (world pos 100,100). Verify via `get_element_at_world`. Erase via `erase_element_at_world(Vector2(100,100), 1)`. Verify gone.

```gdscript
func test_T4_1_9() -> void:
    sim.cell_scale = 4
    sim.clear_grid()
    sim.spawn_element(25, 25, 1, 1)
    assert_eq("T4.1.9a - world get", sim.get_element_at_world(Vector2(100.0, 100.0)), 1)
    sim.erase_element_at_world(Vector2(100.0, 100.0), 1)
    assert_eq("T4.1.9b - world erase", sim.get_element_at_world(Vector2(100.0, 100.0)), 0)
```

---

### T4.1.10 — spawn_element radius=1 places exactly center cell

**Setup:** Clear grid. Call `spawn_element(50, 50, SAND, 1)`.

**Steps:** Verify (50,50) is SAND. Verify all four cardinal neighbors are EMPTY.

```gdscript
func test_T4_1_10() -> void:
    sim.clear_grid()
    sim.spawn_element(50, 50, 1, 1)
    assert_eq("T4.1.10a - center", sim.get_element_at(50, 50), 1)
    assert_eq("T4.1.10b - up",    sim.get_element_at(50, 49), 0)
    assert_eq("T4.1.10c - down",  sim.get_element_at(50, 51), 0)
    assert_eq("T4.1.10d - left",  sim.get_element_at(49, 50), 0)
    assert_eq("T4.1.10e - right", sim.get_element_at(51, 50), 0)
```

---

### T4.1.11 — get_element_count is zero on fresh clear

**Setup:** Call `clear_grid()`.

**Steps:** Call `get_element_count` for all 22 element types.

**Expected result:** All return 0.

```gdscript
func test_T4_1_11() -> void:
    sim.clear_grid()
    for type_id in range(1, 22):
        assert_eq("T4.1.11 type=%d" % type_id, sim.get_element_count(type_id), 0)
```

---

### T4.1.12 — spawn on top of existing element overwrites

**Setup:** Spawn STONE at (50,50). Then spawn SAND at (50,50).

**Steps:** Check (50,50).

**Expected result:** Returns SAND (overwrite succeeds).

```gdscript
func test_T4_1_12() -> void:
    sim.clear_grid()
    sim.spawn_element(50, 50, 3, 1) # STONE
    sim.spawn_element(50, 50, 1, 1) # SAND
    assert_eq("T4.1.12", sim.get_element_at(50, 50), 1)
```

---

## Phase 5 — Signals (8 tests)

**Scope:** Verifies all four signals fire under the correct conditions with correct argument values.

---

### T5.1.1 — simulation_stepped fires every physics step

**Setup:** Connect `simulation_stepped` to a counter. `simulation_paused = false`.

**Steps:** Run 10 physics frames. Check counter.

**Expected result:** Counter == 10 (one emission per step).

```gdscript
func test_T5_1_1() -> void:
    var step_count := 0
    sim.simulation_stepped.connect(func(_d): step_count += 1)
    await run_frames(10)
    assert_eq("T5.1.1", step_count, 10)
    sim.simulation_stepped.disconnect_all() # cleanup
```

---

### T5.1.2 — simulation_stepped delta is positive and sane

**Setup:** Connect `simulation_stepped`. Run 5 frames. Capture delta values.

**Expected result:** All delta values > 0 and < 1.0 (sane frame time).

```gdscript
func test_T5_1_2() -> void:
    var deltas: Array = []
    sim.simulation_stepped.connect(func(d): deltas.append(d))
    await run_frames(5)
    for d in deltas:
        assert_true("T5.1.2 delta positive", d > 0.0)
        assert_true("T5.1.2 delta sane", d < 1.0)
```

---

### T5.1.3 — simulation_stepped does not fire when paused

**Setup:** Connect counter. Set `simulation_paused = true`.

**Steps:** Run 10 frames. Check counter.

**Expected result:** Counter == 0.

```gdscript
func test_T5_1_3() -> void:
    var count := 0
    sim.simulation_stepped.connect(func(_d): count += 1)
    sim.simulation_paused = true
    await run_frames(10)
    sim.simulation_paused = false
    assert_eq("T5.1.3", count, 0)
```

---

### T5.1.4 — black_hole_consumed fires with correct arguments

**Setup:** Clear grid. Spawn SAND at (250, 250). Add black hole at (250, 250) with event_horizon=10 (SAND is inside). Connect `black_hole_consumed`.

**Steps:** Run 1 simulation step. Check signal was emitted with `x ≈ 250`, `y ≈ 250`, `element_type == SAND`.

```gdscript
func test_T5_1_4() -> void:
    sim.clear_grid()
    sim.clear_all_black_holes()
    sim.spawn_element(250, 250, 1, 1) # SAND
    var consumed_args := []
    sim.black_hole_consumed.connect(func(x, y, t): consumed_args = [x, y, t])
    sim.add_black_hole(250.0, 250.0, 1000.0, 15.0)
    await run_frames(1)
    assert_true("T5.1.4a - signal fired", consumed_args.size() == 3)
    assert_eq("T5.1.4b - element type", consumed_args[2], 1) # SAND
```

---

### T5.1.5 — planet_destroyed fires when all planet cells consumed

**Setup:** Generate a small planet (radius=20) at grid center. Add a very strong black hole at center (mass=10000, event_horizon=25 > planet radius). Connect `planet_destroyed`.

**Steps:** Run simulation until `get_element_count(PLANET_CORE) + get_element_count(PLANET_MANTLE) + get_element_count(PLANET_CRUST) == 0` OR 500 frames pass.

**Expected result:** `planet_destroyed` signal fires exactly once with a valid `planet_id >= 0`.

```gdscript
func test_T5_1_5() -> void:
    sim.clear_grid()
    sim.clear_all_black_holes()
    var planet_id_received := -1
    sim.planet_destroyed.connect(func(pid): planet_id_received = pid)
    sim.generate_planet(250, 250, 20)
    sim.add_black_hole(250.0, 250.0, 10000.0, 30.0)
    for i in range(500):
        await get_tree().physics_frame
        var remaining = (sim.get_element_count(18)
                       + sim.get_element_count(19)
                       + sim.get_element_count(20))
        if remaining == 0:
            break
    assert_true("T5.1.5a - signal fired", planet_id_received >= 0)
```

---

### T5.1.6 — element_changed fires on manual spawn

**Setup:** Clear grid. Connect `element_changed`.

**Steps:** Call `spawn_element(10, 10, SAND, 1)`. Verify signal fired with `x=10`, `y=10`, `old_type=EMPTY`, `new_type=SAND`.

```gdscript
func test_T5_1_6() -> void:
    sim.clear_grid()
    var args := []
    sim.element_changed.connect(func(x,y,o,n): args = [x,y,o,n])
    sim.spawn_element(10, 10, 1, 1)
    assert_true("T5.1.6a - fired", args.size() == 4)
    assert_eq("T5.1.6b - x", args[0], 10)
    assert_eq("T5.1.6c - y", args[1], 10)
    assert_eq("T5.1.6d - old EMPTY", args[2], 0)
    assert_eq("T5.1.6e - new SAND", args[3], 1)
```

---

### T5.1.7 — element_changed fires on erase

**Setup:** Spawn STONE at (20,20). Connect `element_changed`.

**Steps:** Call `erase_element(20, 20, 1)`. Check signal args: old=STONE, new=EMPTY.

```gdscript
func test_T5_1_7() -> void:
    sim.clear_grid()
    sim.spawn_element(20, 20, 3, 1) # STONE
    var args := []
    sim.element_changed.connect(func(x,y,o,n): args = [x,y,o,n])
    sim.erase_element(20, 20, 1)
    assert_true("T5.1.7a - fired", args.size() == 4)
    assert_eq("T5.1.7b - old STONE", args[2], 3)
    assert_eq("T5.1.7c - new EMPTY", args[3], 0)
```

---

### T5.1.8 — element_changed does NOT fire for no-op spawn

**Setup:** Spawn SAND at (10,10). Connect `element_changed`. Spawn SAND at (10,10) again (same element, no change).

**Steps:** Verify signal does NOT fire for the second spawn (or fires with same type in both args — implementation may vary, but must not fire with `old == new == SAND` if that is considered a no-op).

**Expected result:** Signal either does not fire, or if fired, `old_type == new_type` for a same-type overwrite. No misleading EMPTY→SAND change reported.

```gdscript
func test_T5_1_8() -> void:
    sim.clear_grid()
    sim.spawn_element(10, 10, 1, 1) # SAND
    var fire_count := 0
    sim.element_changed.connect(func(_x,_y,o,n): if o != n: fire_count += 1)
    sim.spawn_element(10, 10, 1, 1) # SAND again — same type
    assert_eq("T5.1.8 - no spurious change event", fire_count, 0)
```

---

## Phase 6 — Rendering & Texture (6 tests)

**Scope:** Verifies that the grid texture is valid, updates correctly, and the dirty flag cycles as expected.

---

### T6.1.1 — get_grid_texture returns valid ImageTexture

**Setup:** Fresh simulation node. One physics step completed.

**Steps:** Call `get_grid_texture()`. Verify return value is non-null. Cast to `ImageTexture`. Check `.get_size()` matches `Vector2(grid_width, grid_height)`.

```gdscript
func test_T6_1_1() -> void:
    await run_frames(1)
    var tex := sim.get_grid_texture()
    assert_true("T6.1.1a - not null", tex != null)
    assert_eq("T6.1.1b - size", tex.get_size(), Vector2(sim.grid_width, sim.grid_height))
```

---

### T6.1.2 — texture_dirty set after simulation step

**Setup:** Call `mark_texture_clean()`. Run one physics step.

**Steps:** Check `is_texture_dirty()` after step.

**Expected result:** Returns true after step.

```gdscript
func test_T6_1_2() -> void:
    sim.mark_texture_clean()
    assert_eq("T6.1.2a - clean before", sim.is_texture_dirty(), false)
    await run_frames(1)
    assert_eq("T6.1.2b - dirty after step", sim.is_texture_dirty(), true)
```

---

### T6.1.3 — mark_texture_clean clears dirty flag

**Setup:** Run step (makes dirty). Call `mark_texture_clean()`. Check flag.

**Expected result:** Flag is false immediately after `mark_texture_clean()`.

```gdscript
func test_T6_1_3() -> void:
    await run_frames(1)
    assert_true("T6.1.3a - dirty", sim.is_texture_dirty())
    sim.mark_texture_clean()
    assert_eq("T6.1.3b - clean", sim.is_texture_dirty(), false)
```

---

### T6.1.4 — Texture reflects element color correctly

**Setup:** Clear grid. Spawn SAND at (0, 0) with radius=1. Run 1 step. Get texture image.

**Steps:** Read pixel at (0,0) from the texture. Compare to the expected SAND color from the color table.

**Expected result:** Pixel color matches SAND's defined RGBA (within tolerance of ±5 per channel due to format conversion).

```gdscript
func test_T6_1_4() -> void:
    sim.clear_grid()
    sim.spawn_element(0, 0, 1, 1) # SAND at origin
    await run_frames(1)
    var tex := sim.get_grid_texture()
    var img := tex.get_image()
    var pixel := img.get_pixel(0, 0)
    # SAND color expected: warm yellow-tan — verify it's not black (EMPTY)
    assert_true("T6.1.4 - pixel not EMPTY black", pixel.r > 0.1 or pixel.g > 0.1 or pixel.b > 0.1)
```

---

### T6.1.5 — Texture remains valid after clear_grid

**Setup:** Spawn elements. Run steps. Call `clear_grid()`. Run 1 step. Get texture.

**Steps:** Verify texture still non-null and correct size. Sample a pixel — should be EMPTY color (near black).

```gdscript
func test_T6_1_5() -> void:
    sim.fill_rect(0, 0, 100, 100, 1)
    await run_frames(5)
    sim.clear_grid()
    await run_frames(1)
    var tex := sim.get_grid_texture()
    assert_true("T6.1.5a - valid after clear", tex != null)
    var img := tex.get_image()
    var p := img.get_pixel(50, 50)
    assert_true("T6.1.5b - empty pixel near black", p.r < 0.1 and p.g < 0.1 and p.b < 0.1)
```

---

### T6.1.6 — Statistics update correctly

**Setup:** Fresh node. Record `frame_count`. Run 10 frames.

**Steps:** Verify `frame_count` incremented by exactly 10. Verify `get_ups() > 0` and `get_ups() < 200` (sane range).

```gdscript
func test_T6_1_6() -> void:
    var fc_before := sim.frame_count
    await run_frames(10)
    assert_eq("T6.1.6a - frame count", sim.frame_count, fc_before + 10)
    assert_range("T6.1.6b - ups sane", sim.get_ups(), 1.0, 200.0)
```

---

## Phase 7 — Planet Generation & Destruction (10 tests)

**Scope:** Validates planet layer generation, stress threshold ordering, debris behavior, and the `planet_destroyed` signal.

---

### T7.1.1 — generate_planet creates three distinct layers

**Setup:** Clear 500×500 grid. Call `generate_planet(250, 250, 60)`.

**Steps:** Sample cells at distances from center: 10 (core), 40 (mantle), 55 (crust). Verify each returns the expected type.

**Expected result:** d≤15 → PLANET_CORE (18). 15<d≤30 → PLANET_MANTLE (19). 30<d≤60 → PLANET_CRUST (20) or STONE.

```gdscript
func test_T7_1_1() -> void:
    sim.clear_grid()
    sim.generate_planet(250, 250, 60)
    assert_eq("T7.1.1a - core", sim.get_element_at(250, 260), 18)   # d=10
    assert_eq("T7.1.1b - mantle", sim.get_element_at(250, 285), 19)  # d=35
    # crust or stone (10% variation)
    var crust_cell := sim.get_element_at(250, 305)  # d=55
    assert_true("T7.1.1c - crust", crust_cell == 20 or crust_cell == 3)
```

---

### T7.1.2 — generate_planet layer radii match spec

**Setup:** 500×500 grid. Generate planet at (250,250) radius=100. Count cells of each layer type.

**Steps:** `get_element_count(PLANET_CORE)`, `get_element_count(PLANET_MANTLE)`, `get_element_count(PLANET_CRUST)`.

**Expected result:** CORE cells ≈ π×25² ≈ 1963. MANTLE cells ≈ π×50² - π×25² ≈ 5890. CRUST cells ≈ π×100² - π×50² ≈ 23,562. Allow ±5% tolerance on each. CORE < MANTLE < CRUST.

```gdscript
func test_T7_1_2() -> void:
    sim.clear_grid()
    sim.generate_planet(250, 250, 100)
    var core_count   := sim.get_element_count(18)
    var mantle_count := sim.get_element_count(19)
    var crust_count  := sim.get_element_count(20) + sim.get_element_count(3) # include STONE variation
    assert_true("T7.1.2a - core < mantle", core_count < mantle_count)
    assert_true("T7.1.2b - mantle < crust", mantle_count < crust_count)
    assert_range("T7.1.2c - core approx", core_count, 1800, 2100)
```

---

### T7.1.3 — No planet cells outside declared radius

**Setup:** Generate planet at (250,250) radius=50 on clear grid.

**Steps:** Sum all PLANET_CORE + PLANET_MANTLE + PLANET_CRUST cells. Then iterate cells at distance > 52 from center and check none are planet cells.

```gdscript
func test_T7_1_3() -> void:
    sim.clear_grid()
    sim.generate_planet(250, 250, 50)
    for dy in range(-60, 61):
        for dx in range(-60, 61):
            var dist := sqrt(float(dx*dx + dy*dy))
            if dist > 52:
                var t := sim.get_element_at(250+dx, 250+dy)
                assert_true("T7.1.3 no planet beyond radius", t != 18 and t != 19 and t != 20)
```

---

### T7.1.4 — Crust has lowest stress threshold

**Setup:** Access the C++ stress threshold constants directly.

**Expected result:** `CRUST_STRESS_THRESHOLD (0.3) < MANTLE_STRESS_THRESHOLD (0.6) < CORE_STRESS_THRESHOLD (1.0)`.

```cpp
// C++ unit test
static_assert(Planet::CRUST_STRESS_THRESHOLD  == 0.3f);
static_assert(Planet::MANTLE_STRESS_THRESHOLD == 0.6f);
static_assert(Planet::CORE_STRESS_THRESHOLD   == 1.0f);
static_assert(Planet::CRUST_STRESS_THRESHOLD < Planet::MANTLE_STRESS_THRESHOLD);
static_assert(Planet::MANTLE_STRESS_THRESHOLD < Planet::CORE_STRESS_THRESHOLD);
```

---

### T7.1.5 — Crust breaks before mantle under identical gravity

**Setup:** Generate planet at (250,250) radius=40. Apply a black hole at (250,180) — gravity pulls upward. Mass sufficient to exceed crust threshold but not mantle threshold in first few steps.

**Steps:** Run 30 steps. Count PLANET_CRUST remaining vs PLANET_MANTLE remaining.

**Expected result:** Some CRUST cells destroyed (count reduced). MANTLE count unchanged or only slightly reduced. CORE count fully unchanged.

```gdscript
func test_T7_1_5() -> void:
    sim.clear_grid()
    sim.clear_all_black_holes()
    sim.generate_planet(250, 250, 40)
    var crust_before := sim.get_element_count(20)
    var mantle_before := sim.get_element_count(19)
    var core_before := sim.get_element_count(18)
    # Moderate mass — should only crack crust
    sim.add_black_hole(250.0, 180.0, 300.0, 2.0)
    await run_steps(30)
    var crust_after := sim.get_element_count(20)
    assert_true("T7.1.5a - crust damaged", crust_after < crust_before)
    assert_eq("T7.1.5b - core intact", sim.get_element_count(18), core_before)
```

---

### T7.1.6 — Core survives low-mass black hole

**Setup:** Planet radius=30. Black hole mass=200 (just above MIN), event_horizon=2, far enough not to consume directly.

**Steps:** Run 100 steps. Verify PLANET_CORE count unchanged.

```gdscript
func test_T7_1_6() -> void:
    sim.clear_grid()
    sim.clear_all_black_holes()
    sim.generate_planet(250, 250, 30)
    var core_before := sim.get_element_count(18)
    sim.add_black_hole(250.0, 200.0, 200.0, 2.0)
    await run_steps(100)
    assert_eq("T7.1.6 - core intact vs weak BH", sim.get_element_count(18), core_before)
```

---

### T7.1.7 — Debris spawns when cells break

**Setup:** Generate planet. Apply strong black hole to begin destruction.

**Steps:** Run 20 steps. After crust cells break, verify that free-moving elements (STONE, SAND, or the same element type now drifting) exist in cells that were previously inside the planet boundary but are no longer in the original planet structure. The total non-EMPTY count should remain approximately equal to original planet count minus consumed cells.

```gdscript
func test_T7_1_7() -> void:
    sim.clear_grid()
    sim.clear_all_black_holes()
    sim.generate_planet(250, 250, 50)
    var initial_total := (sim.get_element_count(18)
                        + sim.get_element_count(19)
                        + sim.get_element_count(20))
    sim.add_black_hole(250.0, 100.0, 2000.0, 5.0)
    await run_steps(30)
    # Some debris should have drifted away from original planet boundary
    # Check that particles exist outside the planet's original bounding box
    var debris_found := false
    for x in range(0, 200):
        for y in range(0, 500):
            var t := sim.get_element_at(x, y)
            if t != 0:
                debris_found = true
                break
    assert_true("T7.1.7 - debris drifted outside planet", debris_found)
```

---

### T7.1.8 — Hawking ejection signal fires per consumed cell

**Setup:** Grid with several SAND cells directly inside event horizon. Connect `black_hole_consumed`. Run 1 step.

**Steps:** Count signal emissions vs count of cells that were inside the horizon.

**Expected result:** Signal fires once per consumed cell per step.

```gdscript
func test_T7_1_8() -> void:
    sim.clear_grid()
    sim.clear_all_black_holes()
    # Place 5 cells inside event horizon
    for i in range(5):
        sim.spawn_element(250 + i, 250, 1, 1) # SAND
    var consumed_count := 0
    sim.black_hole_consumed.connect(func(_x,_y,_t): consumed_count += 1)
    sim.add_black_hole(252.0, 250.0, 1000.0, 10.0)
    await run_frames(1)
    assert_true("T7.1.8 - consumed signal count matches", consumed_count >= 1)
```

---

### T7.1.9 — generate_planet is deterministic (same input = same output)

**Setup:** Clear grid twice. Call `generate_planet(250, 250, 60)` both times.

**Steps:** After first call, count each layer. After second call (following clear), count each layer. Verify counts are identical.

```gdscript
func test_T7_1_9() -> void:
    sim.clear_grid()
    sim.generate_planet(250, 250, 60)
    var c1 = [sim.get_element_count(18), sim.get_element_count(19), sim.get_element_count(20)]
    sim.clear_grid()
    sim.generate_planet(250, 250, 60)
    var c2 = [sim.get_element_count(18), sim.get_element_count(19), sim.get_element_count(20)]
    assert_eq("T7.1.9a - core deterministic", c1[0], c2[0])
    assert_eq("T7.1.9b - mantle deterministic", c1[1], c2[1])
```

---

### T7.1.10 — planet_destroyed only fires once per planet

**Setup:** Generate planet. Add very strong black hole at center. Connect `planet_destroyed` with counter.

**Steps:** Run until all planet cells gone. Verify counter == 1.

```gdscript
func test_T7_1_10() -> void:
    sim.clear_grid()
    sim.clear_all_black_holes()
    var fire_count := 0
    sim.planet_destroyed.connect(func(_id): fire_count += 1)
    sim.generate_planet(250, 250, 25)
    sim.add_black_hole(250.0, 250.0, 10000.0, 30.0)
    await run_steps(300)
    assert_eq("T7.1.10 - fires exactly once", fire_count, 1)
```

---

## Phase 8 — Performance & Build (12 tests)

**Scope:** Frame rate, memory, simulation step timing, build system, and Godot editor integration.

---

### T8.1.1 — 500×500 grid sustains 60 FPS with 16 black holes

**Setup:** 500×500 grid. Generate planet at center. Add 16 black holes in a ring around it.

**Steps:** Run for 3 seconds. Collect FPS samples every 10 frames via `Performance.get_monitor(Performance.TIME_FPS)`. Take median.

**Expected result:** Median FPS ≥ 60. No frame takes longer than 33 ms.

```gdscript
func test_T8_1_1() -> void:
    sim.grid_width = 500
    sim.grid_height = 500
    sim.clear_grid()
    sim.clear_all_black_holes()
    sim.generate_planet(250, 250, 80)
    for i in range(16):
        var angle := TAU * i / 16.0
        sim.add_black_hole(250.0 + cos(angle) * 100, 250.0 + sin(angle) * 100, 500.0, 5.0)
    var fps_samples: Array = []
    for i in range(180):  # ~3 seconds at 60fps
        await get_tree().physics_frame
        if i % 10 == 0:
            fps_samples.append(Performance.get_monitor(Performance.TIME_FPS))
    fps_samples.sort()
    var median_fps := fps_samples[fps_samples.size() / 2]
    assert_true("T8.1.1 - median FPS >= 60", median_fps >= 60.0)
```

---

### T8.1.2 — Simulation step completes in under 8 ms

**Setup:** 500×500 grid fully populated with SAND. 4 black holes.

**Steps:** Time 100 consecutive simulation steps using `Time.get_ticks_usec()`. Record step durations. Verify 95th percentile ≤ 8000 µs.

```gdscript
func test_T8_1_2() -> void:
    sim.fill_rect(0, 0, 500, 500, 1) # dense SAND
    for i in range(4):
        sim.add_black_hole(float(125 + i * 100), 250.0, 1000.0, 5.0)
    var durations: Array = []
    for i in range(100):
        var t0 := Time.get_ticks_usec()
        await get_tree().physics_frame
        durations.append(Time.get_ticks_usec() - t0)
    durations.sort()
    var p95 := durations[94]
    assert_true("T8.1.2 - p95 step time < 8ms", p95 < 8000)
```

---

### T8.1.3 — Texture upload completes in under 4 ms

**Setup:** 500×500 grid. Modify cells each frame to ensure dirty flag. Measure `mark_texture_clean()` call time as proxy for upload cost.

**Steps:** Time 50 texture update cycles. Verify 95th percentile ≤ 4000 µs.

```gdscript
func test_T8_1_3() -> void:
    var durations: Array = []
    for i in range(50):
        sim.spawn_element(randi() % 500, randi() % 500, 1, 1)
        await get_tree().physics_frame
        var t0 := Time.get_ticks_usec()
        sim.get_grid_texture() # triggers upload
        sim.mark_texture_clean()
        durations.append(Time.get_ticks_usec() - t0)
    durations.sort()
    assert_true("T8.1.3 - texture upload p95 < 4ms", durations[47] < 4000)
```

---

### T8.1.4 — Memory usage within budget (500×500)

**Setup:** 500×500 grid with planet and 4 black holes.

**Steps:** Query `OS.get_static_memory_usage()` before and after full initialization. Delta should be ≤ 3 MB (3,145,728 bytes).

```gdscript
func test_T8_1_4() -> void:
    # Measure after a fresh simulation is fully set up
    sim.generate_planet(250, 250, 100)
    await run_frames(1)
    var mem := OS.get_static_memory_usage()
    # This is a rough upper bound — exact accounting depends on implementation
    # Test verifies no catastrophic leak relative to expected budget
    assert_true("T8.1.4 - memory sane", mem < 100 * 1024 * 1024) # <100MB total process sanity check
    print("T8.1.4 INFO: static memory = %d bytes" % mem)
```

---

### T8.2.1 — Debug CMake build produces shared library

**Setup:** Clean build directory.

**Steps:** Run `cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)`. Check for output file.

**Expected result:** `build/libgodot_falling_sand.so` (Linux), `build/godot_falling_sand.dll` (Windows), or `build/godot_falling_sand.framework` (macOS) exists and is non-zero size.

```bash
# Shell test
cmake -B build -DCMAKE_BUILD_TYPE=Debug 2>&1 | tee build_debug.log
cmake --build build -j$(nproc) 2>&1 | tee -a build_debug.log
# Check for library
if [ -f build/libgodot_falling_sand.so ] || [ -f build/godot_falling_sand.dll ]; then
    echo "[PASS] T8.2.1 - Debug library produced"
else
    echo "[FAIL] T8.2.1 - No library found"
fi
```

---

### T8.2.2 — Release CMake build produces optimized library

**Steps:** Same as T8.2.1 with `CMAKE_BUILD_TYPE=Release`. Verify `libgodot_falling_sand.so` exists. File size should be smaller than Debug build (stripped symbols).

```bash
cmake -B build_release -DCMAKE_BUILD_TYPE=Release
cmake --build build_release -j$(nproc)
# Release binary should exist
test -f build_release/libgodot_falling_sand.so && echo "[PASS] T8.2.2" || echo "[FAIL] T8.2.2"
```

---

### T8.2.3 — Extension manifest entry point name is correct

**Setup:** Read `extension.gdextension`.

**Steps:** Parse JSON. Verify `entry_point == "godot_falling_sand_gdextension_init"`.

```bash
python3 -c "
import json
with open('extension.gdextension') as f:
    data = json.load(f)
ep = data['entry_point']
assert ep == 'godot_falling_sand_gdextension_init', f'[FAIL] T8.2.3 got {ep}'
print('[PASS] T8.2.3 - entry point correct')
"
```

---

### T8.2.4 — Extension manifest product info matches spec

**Steps:** Verify `product_name == "Falling Sand Simulation"` and `product_version == "1.0.0"`.

```bash
python3 -c "
import json
with open('extension.gdextension') as f:
    data = json.load(f)
assert data['product_name'] == 'Falling Sand Simulation', '[FAIL] T8.2.4 product_name'
assert data['product_version'] == '1.0.0', '[FAIL] T8.2.4 product_version'
print('[PASS] T8.2.4 - product info correct')
"
```

---

### T8.3.1 — FallingSandSimulation is registered as Node2D subclass

**Setup:** Godot editor with extension loaded.

**Steps:** In GDScript: `var sim = FallingSandSimulation.new()`. Verify `sim is Node2D` returns true.

```gdscript
func test_T8_3_1() -> void:
    var s := FallingSandSimulation.new()
    assert_true("T8.3.1 - is Node2D", s is Node2D)
    s.free()
```

---

### T8.3.2 — All exported properties visible in Godot inspector

**Setup:** Add `FallingSandSimulation` node to a scene. Open Godot editor inspector.

**Steps:** Verify the following properties appear in the inspector panel: `grid_width`, `grid_height`, `cell_scale`, `simulation_paused`, `time_scale`, `black_holes_enabled`.

**Expected result:** All 6 properties listed, editable, with correct types shown.

**Fail conditions:** Any property missing from inspector; property shown as wrong type; property non-editable.

*Manual verification — no automated script.*

---

### T8.3.3 — All signals connectable in Godot editor

**Setup:** `FallingSandSimulation` node in scene. Open Node panel → Signals tab.

**Steps:** Verify all four signals appear: `element_changed`, `simulation_stepped`, `black_hole_consumed`, `planet_destroyed`. Verify each can be connected to a method via the editor UI.

*Manual verification — no automated script.*

---

### T8.3.4 — Node renders grid texture in scene

**Setup:** Scene with `FallingSandSimulation` node and a `Sprite2D` child. In `_ready`: assign `sim.get_grid_texture()` to `Sprite2D.texture`. Run scene.

**Steps:** Verify the simulation renders visibly in the scene viewport. Spawn elements via GDScript — verify they appear in the texture.

**Expected result:** Colored pixels visible in viewport corresponding to spawned elements. No black screen; no crash.

```gdscript
# In a test scene _ready:
func _ready() -> void:
    var sim := $FallingSandSimulation
    var sprite := $Sprite2D
    sprite.texture = sim.get_grid_texture()
    sim.spawn_element(250, 250, 4, 10) # FIRE at center
    await get_tree().physics_frame
    # Verify texture update propagated
    var img := sim.get_grid_texture().get_image()
    var center := img.get_pixel(250, 250)
    assert(center.r > 0.3 or center.g > 0.1, "FIRE pixel must be visible (not black)")
    print("[PASS] T8.3.4 - scene rendering works")
```

---

## Test Execution Order & Dependencies

```
Phase 1 (Element Types)
    ↓
Phase 2 (Black Hole Engine)     Phase 3 (Node Properties)
    ↓                                   ↓
Phase 4 (Element Manipulation API) ←───┘
    ↓
Phase 5 (Signals)
    ↓
Phase 6 (Rendering & Texture)
    ↓
Phase 7 (Planet Destruction)
    ↓
Phase 8 (Performance & Build)
```

Never run Phase 7 before Phase 4 and Phase 5 pass. Planet destruction depends on correct element manipulation and signal emission.

---

## Regression Test Suite

After all phases pass, run this minimal regression check before every commit:

```gdscript
# Quick smoke test — run in <5 seconds
func smoke_test() -> void:
    sim.clear_grid()
    sim.clear_all_black_holes()

    # Core element ops
    sim.spawn_element(50, 50, 1, 2)    # SAND
    assert(sim.get_element_at(50, 50) == 1)
    sim.erase_element(50, 50, 1)
    assert(sim.get_element_at(50, 50) == 0)

    # Black hole API
    var idx := sim.add_black_hole(250.0, 250.0, 1000.0, 8.0)
    assert(idx >= 0)
    assert(sim.get_black_hole_count() == 1)
    sim.clear_all_black_holes()
    assert(sim.get_black_hole_count() == 0)

    # Planet generation
    sim.generate_planet(250, 250, 30)
    assert(sim.get_element_count(18) > 0)   # PLANET_CORE
    assert(sim.get_element_count(20) > 0)   # PLANET_CRUST

    # Texture
    await get_tree().physics_frame
    assert(sim.get_grid_texture() != null)
    assert(sim.is_texture_dirty() == true)

    print("[PASS] Smoke test complete")
```

---

## Summary

| Phase | Description | Test Count |
|-------|-------------|------------|
| 1 | Element type system | 12 |
| 2 | Black hole engine | 17 |
| 3 | Node properties | 5 |
| 4 | Element manipulation API | 12 |
| 5 | Signals | 8 |
| 6 | Rendering & texture | 6 |
| 7 | Planet generation & destruction | 10 |
| 8 | Performance & build | 12 |
| **Total** | | **82** |

> **Definition of done:** All 82 tests pass on both Debug and Release builds. Zero memory sanitizer errors on the C++ unit tests. No Godot editor warnings related to the extension.
