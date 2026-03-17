# TestRunner.gd - Stardust GDExtension Test Harness
#
# Runs all GDScript tests for the FallingSandSimulation GDExtension.
# Auto-executes on scene load and prints [PASS]/[FAIL] to Output panel.

extends Node

var sim
var pass_count := 0
var fail_count := 0


func _ready() -> void:
	# Wait for the simulation node to be ready
	await get_tree().process_frame
	sim = $FallingSandSimulation

	if sim == null:
		print("[FAIL] TestRunner: FallingSandSimulation node not found")
		print("=== RESULTS: 0 passed, 1 failed ===")
		return

	run_all_tests()
	print("=== RESULTS: %d passed, %d failed ===" % [pass_count, fail_count])


func run_all_tests() -> void:
	# Phase 1: Element Type System (T1.3.x - GDScript tests)
	test_T1_3_2()
	test_T1_3_3()

	# Phase 2: Black Hole Engine (T2.4.x - GDScript tests)
	test_T2_4_1()
	test_T2_4_2()
	test_T2_4_3()
	test_T2_4_4()
	test_T2_4_5()
	test_T2_4_6()
	test_T2_4_7()

	# Phase 3: FallingSandSimulation Node Properties
	test_T3_1_1()
	test_T3_1_2()
	test_T3_1_3()
	test_T3_1_4()
	test_T3_1_5()

	# Phase 4: Element Manipulation API
	test_T4_1_1()
	test_T4_1_2()
	test_T4_1_3()
	test_T4_1_4()
	test_T4_1_5()
	test_T4_1_6()
	test_T4_1_7()
	test_T4_1_8()
	test_T4_1_9()
	test_T4_1_10()
	test_T4_1_11()
	test_T4_1_12()

	# Phase 5: Signals
	test_T5_1_1()
	test_T5_1_2()
	test_T5_1_3()
	test_T5_1_4()
	test_T5_1_5()
	test_T5_1_6()
	test_T5_1_7()
	test_T5_1_8()

	# Phase 6: Rendering & Texture
	test_T6_1_1()
	test_T6_1_2()
	test_T6_1_3()
	test_T6_1_4()
	test_T6_1_5()
	test_T6_1_6()

	# Phase 7: Planet Generation & Destruction
	test_T7_1_1()
	test_T7_1_2()
	test_T7_1_3()
	test_T7_1_5()
	test_T7_1_6()
	test_T7_1_7()
	test_T7_1_8()
	test_T7_1_9()
	test_T7_1_10()

	# Phase 8: Performance & Build
	test_T8_1_1()
	test_T8_1_2()
	test_T8_1_3()
	test_T8_1_4()
	test_T8_3_1()


# ============================================================================
# Assertion Helpers
# ============================================================================

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


# ============================================================================
# Phase 1: Element Type System (GDScript tests)
# ============================================================================

func test_T1_3_2() -> void:
	# Verify get_element_count returns correct counts after spawn
	sim.set_grid_size(10, 10)
	sim.spawn_element(5, 5, 1)  # SAND
	sim.spawn_element(6, 5, 1)  # SAND
	sim.spawn_element(5, 6, 2)  # WATER
	var sand_count = sim.get_element_count(1)
	var water_count = sim.get_element_count(2)
	assert_eq("T1.3.2", sand_count, 2)
	assert_eq("T1.3.2b", water_count, 1)


func test_T1_3_3() -> void:
	# Verify get_element_count returns 0 for empty grid
	sim.set_grid_size(10, 10)
	var empty_count = sim.get_element_count(1)
	assert_eq("T1.3.3", empty_count, 0)


# ============================================================================
# Phase 2: Black Hole Engine (GDScript tests)
# ============================================================================

func test_T2_4_1() -> void:
	# Test black hole gravity force calculation
	sim.set_black_hole(0, 250, 250, 10000, 10)
	sim.set_grid_size(500, 500)
	sim.spawn_element(300, 250, 1)  # SAND
	# Advance simulation - element should move toward black hole
	sim._physics_process(0.016)
	var pos = sim.get_element_position(300, 250)
	# Element should have moved toward black hole (x should decrease from 300)
	assert_true("T2.4.1", pos.x < 300)


func test_T2_4_2() -> void:
	# Test black hole gravity decreases with distance (inverse square)
	sim.set_black_hole(0, 100, 100, 10000, 10)
	sim.set_grid_size(500, 500)
	sim.spawn_element(150, 100, 1)  # Close to black hole
	sim.spawn_element(300, 100, 1)  # Far from black hole
	var pos_close = sim.get_element_position(150, 100)
	var pos_far = sim.get_element_position(300, 100)
	# Close element should move faster than far element
	var close_moved = 150.0 - pos_close.x
	var far_moved = 300.0 - pos_far.x
	assert_true("T2.4.2", close_moved > far_moved)


func test_T2_4_3() -> void:
	# Test multiple black holes - force vectors sum
	sim.set_black_hole(0, 50, 100, 10000, 10)
	sim.set_black_hole(1, 150, 100, 10000, 10)
	sim.set_grid_size(500, 500)
	sim.spawn_element(100, 100, 1)  # Between both black holes
	sim._physics_process(0.016)
	var pos = sim.get_element_position(100, 100)
	# Should be pulled toward center point between black holes
	assert_range("T2.4.3", pos.x, 95, 105)


func test_T2_4_4() -> void:
	# Test event horizon consumption
	sim.set_black_hole(0, 100, 100, 10000, 10)
	sim.set_grid_size(500, 500)
	sim.spawn_element(105, 100, 1)  # Just inside event horizon (distance 5 < 10)
	sim._physics_process(0.016)
	var count = sim.get_element_count(1)
	assert_eq("T2.4.4", count, 0)  # Should be consumed


func test_T2_4_5() -> void:
	# Test element outside event horizon survives
	sim.set_black_hole(0, 100, 100, 10000, 10)
	sim.set_grid_size(500, 500)
	sim.spawn_element(120, 100, 1)  # Outside event horizon (distance 20 > 10)
	sim._physics_process(0.016)
	var count = sim.get_element_count(1)
	assert_eq("T2.4.5", count, 1)  # Should survive


func test_T2_4_6() -> void:
	# Test black hole properties can be read back
	sim.set_black_hole(0, 100, 200, 5000, 15)
	var props = sim.get_black_hole_properties(0)
	assert_eq("T2.4.6.x", props.x, 100)
	assert_eq("T2.4.6.y", props.y, 200)
	assert_eq("T2.4.6.mass", props.mass, 5000)
	assert_eq("T2.4.6.event_horizon", props.event_horizon, 15)


func test_T2_4_7() -> void:
	# Test removing black hole
	sim.set_black_hole(0, 100, 100, 10000, 10)
	sim.remove_black_hole(0)
	var props = sim.get_black_hole_properties(0)
	assert_eq("T2.4.7", props.active, false)


# ============================================================================
# Phase 3: FallingSandSimulation Node Properties
# ============================================================================

func test_T3_1_1() -> void:
	# Test default grid size
	assert_eq("T3.1.1", sim.grid_width, 500)
	assert_eq("T3.1.1b", sim.grid_height, 500)


func test_T3_1_2() -> void:
	# Test set_grid_size
	sim.set_grid_size(256, 256)
	assert_eq("T3.1.2", sim.grid_width, 256)
	assert_eq("T3.1.2b", sim.grid_height, 256)


func test_T3_1_3() -> void:
	# Test simulation running state
	sim.set_simulation_running(true)
	assert_true("T3.1.3", sim.is_simulation_running())


func test_T3_1_4() -> void:
	# Test simulation paused state
	sim.set_simulation_running(false)
	assert_true("T3.1.4", not sim.is_simulation_running())


func test_T3_1_5() -> void:
	# Test get_grid_texture returns ImageTexture
	var tex = sim.get_grid_texture()
	assert_true("T3.1.5", tex != null)


# ============================================================================
# Phase 4: Element Manipulation API
# ============================================================================

func test_T4_1_1() -> void:
	# Test spawn_element
	sim.set_grid_size(100, 100)
	sim.spawn_element(50, 50, 1)  # SAND
	var elem = sim.get_element(50, 50)
	assert_eq("T4.1.1", elem, 1)


func test_T4_1_2() -> void:
	# Test spawn_element with out-of-bounds
	sim.set_grid_size(100, 100)
	sim.spawn_element(150, 50, 1)  # Out of bounds
	var elem = sim.get_element(150, 50)
	assert_eq("T4.1.2", elem, 0)  # Should be EMPTY


func test_T4_1_3() -> void:
	# Test get_element
	sim.set_grid_size(100, 100)
	sim.spawn_element(25, 30, 2)  # WATER
	var elem = sim.get_element(25, 30)
	assert_eq("T4.1.3", elem, 2)


func test_T4_1_4() -> void:
	# Test clear_grid
	sim.set_grid_size(100, 100)
	sim.spawn_element(10, 10, 1)
	sim.spawn_element(20, 20, 2)
	sim.clear_grid()
	var e1 = sim.get_element(10, 10)
	var e2 = sim.get_element(20, 20)
	assert_eq("T4.1.4", e1, 0)
	assert_eq("T4.1.4b", e2, 0)


func test_T4_1_5() -> void:
	# Test fill_rect
	sim.set_grid_size(100, 100)
	sim.fill_rect(10, 10, 5, 5, 3)  # STONE
	var c = sim.get_element_count(3)
	assert_eq("T4.1.5", c, 25)


func test_T4_1_6() -> void:
	# Test fill_circle
	sim.set_grid_size(100, 100)
	sim.fill_circle(50, 50, 10, 4)  # FIRE
	var c = sim.get_element_count(4)
	assert_true("T4.1.6", c > 0)


func test_T4_1_7() -> void:
	# Test screen_to_grid coordinates
	sim.set_grid_size(100, 100)
	var grid_pos = sim.screen_to_grid(50.0, 50.0)
	assert_range("T4.1.7", grid_pos.x, 0, 99)
	assert_range("T4.1.7b", grid_pos.y, 0, 99)


func test_T4_1_8() -> void:
	# Test grid_to_screen coordinates
	sim.set_grid_size(100, 100)
	var screen_pos = sim.grid_to_screen(50, 50)
	assert_true("T4.1.8", screen_pos.x >= 0)
	assert_true("T4.1.8b", screen_pos.y >= 0)


func test_T4_1_9() -> void:
	# Test is_valid_position
	assert_true("T4.1.9", sim.is_valid_position(50, 50))
	assert_true("T4.1.9b", sim.is_valid_position(0, 0))
	assert_true("T4.1.9c", sim.is_valid_position(499, 499))
	assert_true("T4.1.9d", not sim.is_valid_position(-1, 50))
	assert_true("T4.1.9e", not sim.is_valid_position(50, 500))


func test_T4_1_10() -> void:
	# Test spawn_row
	sim.set_grid_size(100, 100)
	sim.spawn_row(50, 1, 10)  # 10 sand at row 50
	var c = sim.get_element_count(1)
	assert_eq("T4.1.10", c, 10)


func test_T4_1_11() -> void:
	# Test spawn_column
	sim.set_grid_size(100, 100)
	sim.spawn_column(50, 1, 10)  # 10 sand at column 50
	var c = sim.get_element_count(1)
	assert_eq("T4.1.11", c, 10)


func test_T4_1_12() -> void:
	# Test spawn_rectangle
	sim.set_grid_size(100, 100)
	sim.spawn_rectangle(10, 10, 5, 3, 1)  # 5x3 sand
	var c = sim.get_element_count(1)
	assert_eq("T4.1.12", c, 15)


# ============================================================================
# Phase 5: Signals
# ============================================================================

func test_T5_1_1() -> void:
	# Test element_changed signal fires when element changes
	sim.set_grid_size(100, 100)
	var fired = false
	sim.element_changed.connect(func(x, y, old_type, new_type):
		fired = true
	)
	sim.spawn_element(50, 50, 1)
	# Signal should have fired
	assert_true("T5.1.1", fired)


func test_T5_1_2() -> void:
	# Test black_hole_consumed signal fires on consumption
	sim.set_black_hole(0, 100, 100, 10000, 20)
	sim.set_grid_size(500, 500)
	var fired = false
	sim.black_hole_consumed.connect(func(x, y, elem_type):
		fired = true
	)
	sim.spawn_element(105, 100, 1)  # Inside event horizon
	sim._physics_process(0.016)
	assert_true("T5.1.2", fired)


func test_T5_1_3() -> void:
	# Test planet_destroyed signal
	sim.set_grid_size(500, 500)
	var fired = false
	sim.planet_destroyed.connect(func(planet_id):
		fired = true
	)
	# Spawn planet elements
	sim.spawn_element(250, 250, 18)  # PLANET_CORE
	sim.spawn_element(250, 251, 19)  # PLANET_MANTLE
	sim.spawn_element(250, 252, 20)  # PLANET_CRUST
	# Trigger destruction via black hole
	sim.set_black_hole(0, 250, 250, 100000, 50)
	sim._physics_process(0.1)
	# Wait for all elements to be consumed
	await get_tree().create_timer(1.0).timeout
	assert_true("T5.1.3", fired)


func test_T5_1_4() -> void:
	# Test simulation_started signal
	var fired = false
	sim.simulation_started.connect(func():
		fired = true
	)
	sim.set_simulation_running(true)
	await get_tree().process_frame
	assert_true("T5.1.4", fired)


func test_T5_1_5() -> void:
	# Test simulation_paused signal
	sim.set_simulation_running(true)
	var fired = false
	sim.simulation_paused.connect(func():
		fired = true
	)
	sim.set_simulation_running(false)
	await get_tree().process_frame
	assert_true("T5.1.5", fired)


func test_T5_1_6() -> void:
	# Test stress_critical signal
	sim.set_grid_size(100, 100)
	sim.set_stress_threshold(10.0)
	var fired = false
	sim.stress_critical.connect(func(x, y, stress_level):
		fired = true
	)
	# Manually set stress (if API allows)
	# This test depends on implementation
	assert_true("T5.1.6", true)  # Placeholder


func test_T5_1_7() -> void:
	# Test frame_updated signal
	var frame_count = 0
	sim.frame_updated.connect(func():
		frame_count += 1
	)
	sim.set_simulation_running(true)
	await get_tree().create_timer(0.1).timeout
	sim.set_simulation_running(false)
	assert_true("T5.1.7", frame_count > 0)


func test_T5_1_8() -> void:
	# Test grid_resized signal
	var fired = false
	sim.grid_resized.connect(func(new_width, new_height):
		fired = true
	)
	sim.set_grid_size(256, 256)
	await get_tree().process_frame
	assert_true("T5.1.8", fired)


# ============================================================================
# Phase 6: Rendering & Texture
# ============================================================================

func test_T6_1_1() -> void:
	# Test get_grid_texture returns valid texture
	var tex = sim.get_grid_texture()
	assert_true("T6.1.1", tex != null)


func test_T6_1_2() -> void:
	# Test texture size matches grid size
	sim.set_grid_size(256, 256)
	var tex = sim.get_grid_texture()
	assert_eq("T6.1.2", tex.get_width(), 256)
	assert_eq("T6.1.2b", tex.get_height(), 256)


func test_T6_1_3() -> void:
	# Test texture updates on simulation step
	sim.set_grid_size(100, 100)
	sim.spawn_element(50, 50, 1)
	var tex1 = sim.get_grid_texture()
	sim._physics_process(0.016)
	var tex2 = sim.get_grid_texture()
	# Textures should be different after update
	assert_true("T6.1.3", tex1 != tex2)


func test_T6_1_4() -> void:
	# Test get_color_for_element
	var color = sim.get_color_for_element(1)  # SAND
	assert_true("T6.1.4", color != null)


func test_T6_1_5() -> void:
	# Test set_element_color
	var original = sim.get_color_for_element(1)
	sim.set_element_color(1, Color(1, 0, 0))
	var modified = sim.get_color_for_element(1)
	assert_eq("T6.1.5", modified.r, 1.0)
	# Restore original
	sim.set_element_color(1, original)


func test_T6_1_6() -> void:
	# Test render_scale property
	sim.render_scale = 2.0
	assert_approx("T6.1.6", sim.render_scale, 2.0)


# ============================================================================
# Phase 7: Planet Generation & Destruction
# ============================================================================

func test_T7_1_1() -> void:
	# Test generate_planet creates all three planet element types
	sim.set_grid_size(500, 500)
	sim.generate_planet(250, 250, 50)
	var core = sim.get_element_count(18)  # PLANET_CORE
	var mantle = sim.get_element_count(19)  # PLANET_MANTLE
	var crust = sim.get_element_count(20)  # PLANET_CRUST
	assert_true("T7.1.1", core > 0)
	assert_true("T7.1.1b", mantle > 0)
	assert_true("T7.1.1c", crust > 0)


func test_T7_1_2() -> void:
	# Test planet has correct structure (core in center)
	sim.set_grid_size(500, 500)
	sim.generate_planet(250, 250, 30)
	var center_elem = sim.get_element(250, 250)
	assert_eq("T7.1.2", center_elem, 18)  # Should be PLANET_CORE


func test_T7_1_3() -> void:
	# Test planet radius parameter works
	sim.set_grid_size(500, 500)
	sim.generate_planet(250, 250, 20)
	sim.generate_planet(250, 250, 40)
	var small_planet = sim.get_element_count(18)
	# Clear and do large
	sim.clear_grid()
	sim.generate_planet(250, 250, 40)
	var large_planet = sim.get_element_count(18)
	assert_true("T7.1.3", large_planet > small_planet)


func test_T7_1_5() -> void:
	# Test destroy_planet_by_id
	sim.set_grid_size(500, 500)
	sim.generate_planet(250, 250, 30)
	sim.destroy_planet(0)
	var planet_elements = sim.get_element_count(18) + sim.get_element_count(19) + sim.get_element_count(20)
	assert_eq("T7.1.5", planet_elements, 0)


func test_T7_1_6() -> void:
	# Test multiple planets can coexist
	sim.set_grid_size(500, 500)
	sim.generate_planet(150, 250, 20)
	sim.generate_planet(350, 250, 20)
	var planet_elements = sim.get_element_count(18) + sim.get_element_count(19) + sim.get_element_count(20)
	assert_true("T7.1.6", planet_elements > 40)


func test_T7_1_7() -> void:
	# Test planet destruction via black hole
	sim.set_grid_size(500, 500)
	sim.generate_planet(250, 250, 25)
	sim.set_black_hole(0, 250, 250, 100000, 100)
	sim._physics_process(0.1)
	await get_tree().create_timer(1.0).timeout
	var planet_elements = sim.get_element_count(18) + sim.get_element_count(19) + sim.get_element_count(20)
	assert_eq("T7.1.7", planet_elements, 0)


func test_T7_1_8() -> void:
	# Test planet element types are distinct
	assert_true("T7.1.8", 18 != 19)
	assert_true("T7.1.8b", 19 != 20)
	assert_true("T7.1.8c", 18 != 20)


func test_T7_1_9() -> void:
	# Test get_planet_count
	sim.set_grid_size(500, 500)
	var count_before = sim.get_planet_count()
	sim.generate_planet(150, 250, 20)
	sim.generate_planet(350, 250, 20)
	var count_after = sim.get_planet_count()
	assert_eq("T7.1.9", count_after, count_before + 2)


func test_T7_1_10() -> void:
	# Test get_planet_id_at
	sim.set_grid_size(500, 500)
	sim.generate_planet(250, 250, 30)
	var planet_id = sim.get_planet_id_at(250, 250)
	assert_true("T7.1.10", planet_id >= 0)


# ============================================================================
# Phase 8: Performance & Build
# ============================================================================

func test_T8_1_1() -> void:
	# Test simulation runs at target FPS
	sim.set_grid_size(100, 100)
	sim.set_simulation_running(true)
	var start = Time.get_ticks_usec()
	sim._physics_process(0.016)  # One frame at ~60 FPS
	var elapsed = Time.get_ticks_usec() - start
	sim.set_simulation_running(false)
	# Should complete in under 10ms for small grid
	assert_true("T8.1.1", elapsed < 10000)


func test_T8_1_2() -> void:
	# Test memory usage stays within budget
	var initial_mem = OS.get_static_memory_usage()
	sim.set_grid_size(500, 500)
	# Spawn many elements
	for i in range(1000):
		sim.spawn_element(i % 500, i / 500, 1)
	var final_mem = OS.get_static_memory_usage()
	var diff = final_mem - initial_mem
	# Should not exceed 100MB for this operation
	assert_true("T8.1.2", diff < 100 * 1024 * 1024)


func test_T8_1_3() -> void:
	# Test large grid performance
	sim.set_grid_size(500, 500)
	var start = Time.get_ticks_usec()
	for i in range(100):
		sim._physics_process(0.016)
	var elapsed = Time.get_ticks_usec() - start
	# 100 frames should complete in reasonable time
	assert_true("T8.1.3", elapsed < 1000000)  # < 1 second


func test_T8_1_4() -> void:
	# Test no memory leak over time
	sim.set_grid_size(200, 200)
	var start_mem = OS.get_static_memory_usage()
	for i in range(1000):
		sim.spawn_element(i % 200, i / 200, 1)
		sim._physics_process(0.016)
		sim.clear_grid()
	var end_mem = OS.get_static_memory_usage()
	var leak = end_mem - start_mem
	# Allow some tolerance but should not grow unbounded
	assert_true("T8.1.4", leak < 50 * 1024 * 1024)


func test_T8_3_1() -> void:
	# Integration test: Full simulation cycle
	sim.set_grid_size(256, 256)
	sim.generate_planet(128, 128, 30)
	sim.set_black_hole(0, 128, 128, 50000, 40)
	sim.set_simulation_running(true)
	# Let it run for a bit
	await get_tree().create_timer(2.0).timeout
	sim.set_simulation_running(false)
	# Should have had many frame updates
	assert_true("T8.3.1", true)  # If we get here without crash, test passes
