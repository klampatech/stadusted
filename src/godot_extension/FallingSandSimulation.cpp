#include "FallingSandSimulation.h"

#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/time.hpp>

FallingSandSimulation::FallingSandSimulation()
    : gridWidth(DEFAULT_GRID_WIDTH)
    , gridHeight(DEFAULT_GRID_HEIGHT)
    , cellScale(DEFAULT_CELL_SCALE)
    , simulationPaused(false)
    , timeScale(1.0f)
    , blackHolesEnabled(true)
    , textureDirty(false)
    , frameCount(0)
    , updateAccumulator(0.0f)
    , updatesPerSecond(0.0f)
    , lastUpdateTime(0.0)
    , lastPlanetElementCount(0) {
    // Initialize the grid
    grid = new Grid(gridWidth, gridHeight);
    blackHoleEngine = new BlackHoleEngine();
}

FallingSandSimulation::~FallingSandSimulation() {
    if (grid) {
        delete grid;
        grid = nullptr;
    }
    if (blackHoleEngine) {
        delete blackHoleEngine;
        blackHoleEngine = nullptr;
    }
    // Ref<Image> and Ref<ImageTexture> are automatically cleaned up
}

void FallingSandSimulation::_ready() {
    // Create the grid image for rendering
    gridImage = Image::create(gridWidth, gridHeight, false, Image::FORMAT_RGBA8);
    gridTexture = ImageTexture::create_from_image(gridImage);

    // Generate a test pattern
    grid->fillTestPattern();
    textureDirty = true;
}

void FallingSandSimulation::_physics_process(double delta) {
    if (simulationPaused) return;

    float scaledDelta = delta * timeScale;
    updateAccumulator += scaledDelta;

    // Target 60 updates per second
    float targetUpdateTime = 1.0f / 60.0f;

    while (updateAccumulator >= targetUpdateTime) {
        updateAccumulator -= targetUpdateTime;

        // Update simulation with black holes
        if (blackHolesEnabled) {
            grid->updateWithBlackHoles(blackHoleEngine);
        } else {
            grid->update();
        }

        grid->swapBuffers();

        // Emit black_hole_consumed signals for elements consumed this frame
        auto consumed = grid->getConsumedElements();
        for (const auto& elem : consumed) {
            emit_signal("black_hole_consumed", elem.x, elem.y, static_cast<int>(elem.type));
        }
        grid->clearConsumedElements();

        // Check for planet destruction
        int currentPlanetCount =
            grid->getElementCount(ElementType::PLANET_CORE) +
            grid->getElementCount(ElementType::PLANET_MANTLE) +
            grid->getElementCount(ElementType::PLANET_CRUST);

        if (lastPlanetElementCount > 0 && currentPlanetCount == 0) {
            emit_signal("planet_destroyed", 0);
        }
        lastPlanetElementCount = currentPlanetCount;

        frameCount++;
        textureDirty = true;
    }

    // Calculate UPS
    double currentTime = Time::get_singleton()->get_ticks_msec() / 1000.0;
    if (currentTime - lastUpdateTime >= 1.0) {
        updatesPerSecond = frameCount / static_cast<float>(currentTime - lastUpdateTime);
        lastUpdateTime = currentTime;
    }

    // Emit stepped signal
    emit_signal("simulation_stepped", delta);
}

void FallingSandSimulation::_exit_tree() {
    // Cleanup handled in destructor
}

// Configuration properties
void FallingSandSimulation::set_grid_width(int width) {
    width = std::max(100, std::min(width, 2000));
    if (width != gridWidth) {
        gridWidth = width;
        // Note: This would require recreating the grid
        emit_signal("property_changed", "grid_width");
    }
}

int FallingSandSimulation::get_grid_width() const {
    return gridWidth;
}

void FallingSandSimulation::set_grid_height(int height) {
    height = std::max(100, std::min(height, 2000));
    if (height != gridHeight) {
        gridHeight = height;
        emit_signal("property_changed", "grid_height");
    }
}

int FallingSandSimulation::get_grid_height() const {
    return gridHeight;
}

void FallingSandSimulation::set_cell_scale(int scale) {
    cellScale = std::max(1, std::min(scale, 32));
}

int FallingSandSimulation::get_cell_scale() const {
    return cellScale;
}

void FallingSandSimulation::set_simulation_paused(bool paused) {
    simulationPaused = paused;
}

bool FallingSandSimulation::is_simulation_paused() const {
    return simulationPaused;
}

void FallingSandSimulation::set_time_scale(float scale) {
    timeScale = std::max(0.1f, std::min(scale, 10.0f));
}

float FallingSandSimulation::get_time_scale() const {
    return timeScale;
}

// Element manipulation
void FallingSandSimulation::spawn_element(int x, int y, int element_type, int radius) {
    if (element_type < 0 || element_type > 21) return;

    ElementType newType = static_cast<ElementType>(element_type);

    // Track changes for each cell in the brush radius
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy <= radius * radius) {
                int checkX = x + dx;
                int checkY = y + dy;
                ElementType oldType = grid->getCell(checkX, checkY);

                // Only emit signal if type actually changes
                if (oldType != newType) {
                    emit_signal("element_changed", checkX, checkY,
                                static_cast<int>(oldType),
                                static_cast<int>(newType));
                }
            }
        }
    }

    grid->spawnElement(x, y, newType, radius);
    textureDirty = true;
}

void FallingSandSimulation::spawn_element_at_world(Vector2 world_pos, int element_type, int radius) {
    int gridX = static_cast<int>(world_pos.x / cellScale);
    int gridY = static_cast<int>(world_pos.y / cellScale);
    spawn_element(gridX, gridY, element_type, radius);
}

void FallingSandSimulation::erase_element(int x, int y, int radius) {
    // Track changes for each cell in the brush radius before erasing
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy <= radius * radius) {
                int checkX = x + dx;
                int checkY = y + dy;
                ElementType oldType = grid->getCell(checkX, checkY);

                // Only emit signal if there was actually something to erase
                if (oldType != ElementType::EMPTY) {
                    emit_signal("element_changed", checkX, checkY,
                                static_cast<int>(oldType),
                                static_cast<int>(ElementType::EMPTY));
                }
            }
        }
    }

    grid->eraseElement(x, y, radius);
    textureDirty = true;
}

void FallingSandSimulation::erase_element_at_world(Vector2 world_pos, int radius) {
    int gridX = static_cast<int>(world_pos.x / cellScale);
    int gridY = static_cast<int>(world_pos.y / cellScale);
    erase_element(gridX, gridY, radius);
}

int FallingSandSimulation::get_element_at(int x, int y) const {
    return static_cast<int>(grid->getCell(x, y));
}

int FallingSandSimulation::get_element_at_world(Vector2 world_pos) const {
    int gridX = static_cast<int>(world_pos.x / cellScale);
    int gridY = static_cast<int>(world_pos.y / cellScale);
    return get_element_at(gridX, gridY);
}

void FallingSandSimulation::fill_rect(int x, int y, int w, int h, int element_type) {
    if (element_type < 0 || element_type > 21) return;
    grid->fillRect(x, y, w, h, static_cast<ElementType>(element_type));
    textureDirty = true;
}

void FallingSandSimulation::fill_circle(int cx, int cy, int radius, int element_type) {
    if (element_type < 0 || element_type > 21) return;
    grid->fillCircle(cx, cy, radius, static_cast<ElementType>(element_type));
    textureDirty = true;
}

// World generation
void FallingSandSimulation::clear_grid() {
    grid->clear();
    textureDirty = true;
}

void FallingSandSimulation::generate_planet(int center_x, int center_y, int radius) {
    grid->generatePlanet(center_x, center_y, radius);
    textureDirty = true;
}

void FallingSandSimulation::generate_mining_world(int seed) {
    grid->generateWorld(seed);
    textureDirty = true;
}

void FallingSandSimulation::fill_test_pattern() {
    grid->fillTestPattern();
    textureDirty = true;
}

// Texture access
Ref<ImageTexture> FallingSandSimulation::get_grid_texture() {
    if (textureDirty && gridImage.is_valid() && grid) {
        for (int y = 0; y < gridHeight; y++) {
            for (int x = 0; x < gridWidth; x++) {
                ElementType type = grid->getCell(x, y);
                ElementColor color = getElementColor(type);

                // Special handling for dynamic elements
                if (type == ElementType::FIRE) {
                    color = getFireColor(x, y);
                } else if (type == ElementType::SMOKE) {
                    color = getSmokeColor(x, y);
                }

                gridImage->set_pixel(x, y, Color(
                    color.r / 255.0f,
                    color.g / 255.0f,
                    color.b / 255.0f,
                    color.a / 255.0f
                ));
            }
        }

        if (gridTexture.is_valid()) {
            gridTexture->update(gridImage);
        }
        textureDirty = false;
    }

    return gridTexture;
}

bool FallingSandSimulation::is_texture_dirty() const {
    return textureDirty;
}

void FallingSandSimulation::mark_texture_clean() {
    textureDirty = false;
}

// Black hole management
int FallingSandSimulation::add_black_hole(float x, float y, float mass, float event_horizon) {
    return blackHoleEngine->addBlackHole(x, y, mass, event_horizon);
}

void FallingSandSimulation::remove_black_hole(int index) {
    blackHoleEngine->removeBlackHole(index);
}

void FallingSandSimulation::set_black_hole_position(int index, float x, float y) {
    blackHoleEngine->setPosition(index, x, y);
}

void FallingSandSimulation::set_black_hole_mass(int index, float mass) {
    blackHoleEngine->setMass(index, mass);
}

int FallingSandSimulation::get_black_hole_count() const {
    return blackHoleEngine->getCount();
}

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

void FallingSandSimulation::clear_all_black_holes() {
    blackHoleEngine->clearAll();
}

void FallingSandSimulation::set_black_holes_enabled(bool enabled) {
    blackHolesEnabled = enabled;
    blackHoleEngine->setEnabled(enabled);
}

bool FallingSandSimulation::are_black_holes_enabled() const {
    return blackHolesEnabled;
}

// Statistics
float FallingSandSimulation::get_ups() const {
    return updatesPerSecond;
}

uint64_t FallingSandSimulation::get_frame_count() const {
    return frameCount;
}

int FallingSandSimulation::get_element_count(int element_type) const {
    if (element_type < 0 || element_type > 21) return 0;
    return grid->getElementCount(static_cast<ElementType>(element_type));
}

// GDScript bindings
void FallingSandSimulation::_bind_methods() {
    // Configuration
    ClassDB::bind_method(D_METHOD("set_grid_width", "width"), &FallingSandSimulation::set_grid_width);
    ClassDB::bind_method(D_METHOD("get_grid_width"), &FallingSandSimulation::get_grid_width);
    ClassDB::bind_method(D_METHOD("set_grid_height", "height"), &FallingSandSimulation::set_grid_height);
    ClassDB::bind_method(D_METHOD("get_grid_height"), &FallingSandSimulation::get_grid_height);
    ClassDB::bind_method(D_METHOD("set_simulation_paused", "paused"), &FallingSandSimulation::set_simulation_paused);
    ClassDB::bind_method(D_METHOD("is_simulation_paused"), &FallingSandSimulation::is_simulation_paused);
    ClassDB::bind_method(D_METHOD("set_time_scale", "scale"), &FallingSandSimulation::set_time_scale);
    ClassDB::bind_method(D_METHOD("get_time_scale"), &FallingSandSimulation::get_time_scale);

    // Element operations
    ClassDB::bind_method(D_METHOD("spawn_element", "x", "y", "element_type", "radius"),
                         &FallingSandSimulation::spawn_element, DEFVAL(1));
    ClassDB::bind_method(D_METHOD("spawn_element_at_world", "world_pos", "element_type", "radius"),
                         &FallingSandSimulation::spawn_element_at_world, DEFVAL(1));
    ClassDB::bind_method(D_METHOD("erase_element", "x", "y", "radius"),
                         &FallingSandSimulation::erase_element, DEFVAL(2));
    ClassDB::bind_method(D_METHOD("erase_element_at_world", "world_pos", "radius"),
                         &FallingSandSimulation::erase_element_at_world, DEFVAL(2));
    ClassDB::bind_method(D_METHOD("get_element_at", "x", "y"), &FallingSandSimulation::get_element_at);
    ClassDB::bind_method(D_METHOD("get_element_at_world", "world_pos"), &FallingSandSimulation::get_element_at_world);
    ClassDB::bind_method(D_METHOD("fill_rect", "x", "y", "w", "h", "element_type"),
                         &FallingSandSimulation::fill_rect);
    ClassDB::bind_method(D_METHOD("fill_circle", "cx", "cy", "radius", "element_type"),
                         &FallingSandSimulation::fill_circle);

    // Texture
    ClassDB::bind_method(D_METHOD("get_grid_texture"), &FallingSandSimulation::get_grid_texture);
    ClassDB::bind_method(D_METHOD("is_texture_dirty"), &FallingSandSimulation::is_texture_dirty);
    ClassDB::bind_method(D_METHOD("mark_texture_clean"), &FallingSandSimulation::mark_texture_clean);

    // Black holes
    ClassDB::bind_method(D_METHOD("add_black_hole", "x", "y", "mass", "event_horizon"),
                         &FallingSandSimulation::add_black_hole, DEFVAL(1000.0f), DEFVAL(8.0f));
    ClassDB::bind_method(D_METHOD("remove_black_hole", "index"), &FallingSandSimulation::remove_black_hole);
    ClassDB::bind_method(D_METHOD("set_black_hole_position", "index", "x", "y"),
                         &FallingSandSimulation::set_black_hole_position);
    ClassDB::bind_method(D_METHOD("set_black_hole_mass", "index", "mass"),
                         &FallingSandSimulation::set_black_hole_mass);
    ClassDB::bind_method(D_METHOD("get_black_hole_count"), &FallingSandSimulation::get_black_hole_count);
    ClassDB::bind_method(D_METHOD("get_black_hole_info", "index"), &FallingSandSimulation::get_black_hole_info);
    ClassDB::bind_method(D_METHOD("clear_all_black_holes"), &FallingSandSimulation::clear_all_black_holes);
    ClassDB::bind_method(D_METHOD("set_black_holes_enabled", "enabled"), &FallingSandSimulation::set_black_holes_enabled);
    ClassDB::bind_method(D_METHOD("are_black_holes_enabled"), &FallingSandSimulation::are_black_holes_enabled);

    // World generation
    ClassDB::bind_method(D_METHOD("clear_grid"), &FallingSandSimulation::clear_grid);
    ClassDB::bind_method(D_METHOD("generate_planet", "center_x", "center_y", "radius"),
                         &FallingSandSimulation::generate_planet);
    ClassDB::bind_method(D_METHOD("generate_mining_world", "seed"),
                         &FallingSandSimulation::generate_mining_world, DEFVAL(12345));
    ClassDB::bind_method(D_METHOD("fill_test_pattern"), &FallingSandSimulation::fill_test_pattern);

    // Statistics
    ClassDB::bind_method(D_METHOD("get_ups"), &FallingSandSimulation::get_ups);
    ClassDB::bind_method(D_METHOD("get_frame_count"), &FallingSandSimulation::get_frame_count);
    ClassDB::bind_method(D_METHOD("get_element_count", "element_type"), &FallingSandSimulation::get_element_count);

    // Properties
    ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_width"), "set_grid_width", "get_grid_width");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_height"), "set_grid_height", "get_grid_height");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "cell_scale"), "set_cell_scale", "get_cell_scale");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "simulation_paused"), "set_simulation_paused", "is_simulation_paused");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "time_scale"), "set_time_scale", "get_time_scale");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "black_holes_enabled"), "set_black_holes_enabled", "are_black_holes_enabled");

    // Signals
    ADD_SIGNAL(MethodInfo("element_changed", PropertyInfo(Variant::INT, "x"),
                         PropertyInfo(Variant::INT, "y"),
                         PropertyInfo(Variant::INT, "old_type"),
                         PropertyInfo(Variant::INT, "new_type")));
    ADD_SIGNAL(MethodInfo("simulation_stepped", PropertyInfo(Variant::FLOAT, "delta")));
    ADD_SIGNAL(MethodInfo("black_hole_consumed", PropertyInfo(Variant::INT, "x"),
                         PropertyInfo(Variant::INT, "y"),
                         PropertyInfo(Variant::INT, "element_type")));
    ADD_SIGNAL(MethodInfo("planet_destroyed", PropertyInfo(Variant::INT, "planet_id")));
    ADD_SIGNAL(MethodInfo("property_changed", PropertyInfo(Variant::STRING, "property_name")));
}
