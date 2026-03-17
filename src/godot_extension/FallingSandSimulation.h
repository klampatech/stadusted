#ifndef FALLING_SAND_SIMULATION_H
#define FALLING_SAND_SIMULATION_H

#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>

#include "../FallingSandEngine.h"
#include "../BlackHoleEngine.h"

using namespace godot;

/**
 * FallingSandSimulation - Main Godot node for the falling sand simulation
 * Extends Node2D and provides GDScript bindings for the simulation
 */
class FallingSandSimulation : public Node2D {
    GDCLASS(FallingSandSimulation, Node2D)

private:
    // Core simulation
    Grid* grid;
    BlackHoleEngine* blackHoleEngine;

    // Grid configuration
    int gridWidth;
    int gridHeight;
    int cellScale;

    // Simulation control
    bool simulationPaused;
    bool simulationRunning;
    float timeScale;
    float stressThreshold;
    bool blackHolesEnabled;
    float renderScale;

    // Rendering
    Ref<Image> gridImage;
    Ref<ImageTexture> gridTexture;
    bool textureDirty;

    // Statistics
    uint64_t frameCount;
    float updateAccumulator;
    float updatesPerSecond;
    float lastUpdateTime;

    // Planet destruction tracking
    int lastPlanetElementCount;

public:
    FallingSandSimulation();
    ~FallingSandSimulation() override;

    // Godot lifecycle
    void _ready() override;
    void _physics_process(double delta) override;
    void _exit_tree() override;

    // Configuration properties
    void set_grid_size(int width, int height);
    void set_grid_width(int width);
    int get_grid_width() const;
    void set_grid_height(int height);
    int get_grid_height() const;
    void set_cell_scale(int scale);
    int get_cell_scale() const;
    void set_simulation_paused(bool paused);
    bool is_simulation_paused() const;
    void set_simulation_running(bool running);
    bool is_simulation_running() const;
    void set_time_scale(float scale);
    float get_time_scale() const;
    void set_stress_threshold(float threshold);
    float get_stress_threshold() const;

    // Element manipulation
    void spawn_element(int x, int y, int element_type, int radius = 1);
    void spawn_element_at_world(Vector2 world_pos, int element_type, int radius = 1);
    void erase_element(int x, int y, int radius = 2);
    void erase_element_at_world(Vector2 world_pos, int radius = 2);
    int get_element_at(int x, int y) const;
    int get_element(int x, int y) const;
    int get_element_at_world(Vector2 world_pos) const;
    Vector2 get_element_position(int x, int y) const;
    bool is_valid_position(int x, int y) const;
    void fill_rect(int x, int y, int w, int h, int element_type);
    void fill_circle(int cx, int cy, int radius, int element_type);

    // World generation
    void clear_grid();
    void generate_planet(int center_x, int center_y, int radius);
    void generate_mining_world(int seed = 12345);
    void fill_test_pattern();

    // Texture access
    Ref<ImageTexture> get_grid_texture();
    bool is_texture_dirty() const;
    void mark_texture_clean();

    // Black hole management
    void set_black_hole(int index, float x, float y, float mass, float event_horizon);
    int add_black_hole(float x, float y, float mass = 1000.0f, float event_horizon = 8.0f);
    void remove_black_hole(int index);
    void set_black_hole_position(int index, float x, float y);
    void set_black_hole_mass(int index, float mass);
    int get_black_hole_count() const;
    Dictionary get_black_hole_info(int index) const;
    Dictionary get_black_hole_properties(int index) const;
    void clear_all_black_holes();
    void set_black_holes_enabled(bool enabled);
    bool are_black_holes_enabled() const;

    // Statistics
    float get_ups() const;
    uint64_t get_frame_count() const;
    int get_element_count(int element_type) const;

    // Color management
    Color get_color_for_element(int element_type) const;
    void set_element_color(int element_type, Color color);

    // Planet tracking
    int get_planet_count() const;
    int get_planet_id_at(int x, int y) const;
    void destroy_planet(int planet_id);

    // Coordinate conversion
    Vector2 screen_to_grid(Vector2 screen_pos) const;
    Vector2 grid_to_screen(Vector2 grid_pos) const;

    // Element spawning utilities
    void spawn_row(int y, int element_type, int count);
    void spawn_column(int x, int element_type, int count);
    void spawn_rectangle(int x, int y, int width, int height, int element_type);

    // Step method for testing
    void step(double delta);

    // Properties for GDScript binding
    bool get_simulation_started() const;
    bool get_frame_updated() const;
    bool get_grid_resized() const;
    bool get_stress_critical() const;
    float get_render_scale() const;
    void set_render_scale(float scale);

protected:
    static void _bind_methods();
};

#endif // FALLING_SAND_SIMULATION_H
