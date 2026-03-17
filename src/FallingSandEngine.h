#ifndef FALLING_SAND_ENGINE_H
#define FALLING_SAND_ENGINE_H

#include <cstdint>
#include <vector>
#include <memory>

/**
 * FallingSandEngine - A cellular automaton-based physics simulation engine
 * for falling sand games. Supports multiple element types with unique behaviors.
 * Ported from Ralph with additions for black hole physics and planet destruction.
 */

// Forward declarations
class Grid;
class BlackHoleEngine;

/**
 * Element types for the simulation (22 total as per spec)
 */
enum class ElementType : uint8_t {
    EMPTY = 0,
    SAND = 1,
    WATER = 2,
    STONE = 3,
    FIRE = 4,
    SMOKE = 5,
    WOOD = 6,
    OIL = 7,
    ACID = 8,
    GUNPOWDER = 9,
    COAL = 10,
    COPPER = 11,
    GOLD = 12,
    DIAMOND = 13,
    DIRT = 14,
    PICKUP_PARTICLES = 15,
    PICKUP_POINTS = 16,
    GOLD_CRACKED = 17,
    PLANET_CORE = 18,
    PLANET_MANTLE = 19,
    PLANET_CRUST = 20,
    BLACK_HOLE = 21
};

/**
 * Color structure for elements
 */
struct ElementColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

/**
 * Get color for each element type
 */
ElementColor getElementColor(ElementType type);

/**
 * Get fire color with gradient variation (orange/red/yellow)
 */
ElementColor getFireColor(int x, int y);

/**
 * Get smoke color with slight variation for texture
 */
ElementColor getSmokeColor(int x, int y);

/**
 * Consumed element record for signal emission
 */
struct ConsumedElement {
    int x, y;
    ElementType type;
};

/**
 * Grid class with double buffering - the core of the falling sand simulation
 */
class Grid {
private:
    std::vector<ElementType> currentBuffer;
    std::vector<ElementType> nextBuffer;
    int width;
    int height;

    // Consumed elements tracking for signal emission
    std::vector<ConsumedElement> consumedElements;

    // Stress map for planet destruction tracking
    std::vector<float> stressMap;
    std::vector<float> nextStressMap;

    // Statistics
    std::vector<int> elementCounts;

public:
    /**
     * Construct a new Grid
     * @param w Width of the grid in cells
     * @param h Height of the grid in cells
     */
    Grid(int w, int h);
    ~Grid();

    /**
     * Get grid width
     */
    int getWidth() const;

    /**
     * Get grid height
     */
    int getHeight() const;

    /**
     * Get element at position
     * @param x X coordinate
     * @param y Y coordinate
     * @return ElementType at position, or EMPTY if out of bounds
     */
    ElementType getCell(int x, int y) const;

    /**
     * Set element at position
     * @param x X coordinate
     * @param y Y coordinate
     * @param type Element to place
     */
    void setCell(int x, int y, ElementType type);

    /**
     * Swap buffers for double-buffering and advance simulation
     */
    void swapBuffers();

    /**
     * Run one step of the physics simulation
     * Updates all elements according to their physics rules
     */
    void update();

    /**
     * Run one step of the physics simulation with black hole forces
     * @param blackHoleEngine Pointer to black hole engine for attraction forces
     */
    void updateWithBlackHoles(BlackHoleEngine* blackHoleEngine);

    /**
     * Fill grid with initial test pattern
     */
    void fillTestPattern();

    /**
     * Generate a layered world for mining game
     * @param worldSeed Random seed for world generation
     */
    void generateWorld(int worldSeed = 12345);

    /**
     * Generate a planet with core, mantle, and crust layers
     * @param centerX Center X coordinate
     * @param centerY Center Y coordinate
     * @param radius Planet radius
     */
    void generatePlanet(int centerX, int centerY, int radius);

    /**
     * Spawn element at position with brush radius
     * @param gridX X coordinate
     * @param gridY Y coordinate
     * @param type Element to spawn
     * @param radius Brush radius
     */
    void spawnElement(int gridX, int gridY, ElementType type, int radius = 1);

    /**
     * Erase elements at position with brush radius (set to EMPTY)
     * @param gridX X coordinate
     * @param gridY Y coordinate
     * @param radius Brush radius
     */
    void eraseElement(int gridX, int gridY, int radius = 2);

    /**
     * Fill rectangular area with element type
     */
    void fillRect(int x, int y, int w, int h, ElementType type);

    /**
     * Fill circular area with element type
     */
    void fillCircle(int cx, int cy, int radius, ElementType type);

    /**
     * Clear grid (set all to EMPTY)
     */
    void clear();

    /**
     * Get count of specific element type
     */
    int getElementCount(ElementType type) const;

    /**
     * Get total element count for all types
     */
    void updateElementCounts();

    // Stress management for planet destruction
    /**
     * Get stress level at position
     */
    float getStress(int x, int y) const;

    /**
     * Set stress level at position
     */
    void setStress(int x, int y, float stress);

    /**
     * Reset stress map (call when generating new planet)
     */
    void resetStressMap();

    /**
     * Check if stress exceeds threshold for element type
     */
    bool shouldBreak(ElementType type, float stress) const;

    /**
     * Get debris type when element breaks
     */
    ElementType getDebrisType(ElementType brokenType) const;

    // Consumed elements tracking for signal emission
    /**
     * Get list of elements consumed by black holes this frame
     */
    std::vector<ConsumedElement> getConsumedElements() const;

    /**
     * Clear consumed elements list (call after processing)
     */
    void clearConsumedElements();

private:
    // Helper methods for element updates
    void updateSand(int x, int y);
    void updateWater(int x, int y);
    void updateFire(int x, int y);
    void updateSmoke(int x, int y);
    void updateWood(int x, int y);
    void updateOil(int x, int y);
    void updateAcid(int x, int y);
    void updateGunpowder(int x, int y);

    // Planet element updates
    void updatePlanetElement(int x, int y);

    // Helper methods for checking cell properties
    bool isEmpty(int x, int y) const;
    bool isSand(int x, int y) const;
    bool isWater(int x, int y) const;
    bool isEmptyOrWater(int x, int y) const;
    bool isFlammable(int x, int y) const;
    bool isFire(int x, int y) const;
    bool isWood(int x, int y) const;
    bool isOil(int x, int y) const;
    bool isAcid(int x, int y) const;
    bool isGunpowder(int x, int y) const;
    bool isDestructible(int x, int y) const;
    bool isBurning(int x, int y) const;
    bool isPlanetElement(int x, int y) const;

    // Get reference to next buffer for updates
    ElementType& getNextCell(int x, int y);

    // Fire mechanics
    void igniteNearbyFlammable(int x, int y);
    void createExplosion(int x, int y);

    // Acid mechanics
    void dissolveNearby(int x, int y);
};

// Configuration constants for grid dimensions
constexpr int DEFAULT_GRID_WIDTH = 500;
constexpr int DEFAULT_GRID_HEIGHT = 500;
constexpr int DEFAULT_CELL_SCALE = 4;

// Planet destruction stress thresholds
constexpr float CRUST_STRESS_THRESHOLD = 0.3f;
constexpr float MANTLE_STRESS_THRESHOLD = 0.6f;
constexpr float CORE_STRESS_THRESHOLD = 1.0f;
constexpr float STRESS_ACCUMULATION_RATE = 0.01f;
constexpr float MAX_STRESS = 1.0f;

#endif // FALLING_SAND_ENGINE_H
