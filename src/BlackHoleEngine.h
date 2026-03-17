#ifndef BLACK_HOLE_ENGINE_H
#define BLACK_HOLE_ENGINE_H

#include <cstdint>
#include <vector>
#include <cmath>

/**
 * BlackHoleEngine - Handles black hole physics for the Stardust game.
 * Implements inverse-square gravity with event horizon consumption.
 */

struct BlackHole {
    float x;              // Position X (grid coordinates)
    float y;              // Position Y (grid coordinates)
    float mass;           // Gravitational mass (affects pull strength)
    float event_horizon;  // Radius where elements are consumed
    float influence_radius; // Maximum distance of gravitational effect
    bool active;          // Whether this black hole is currently active

    BlackHole() : x(0), y(0), mass(1000.0f), event_horizon(8.0f),
                  influence_radius(150.0f), active(true) {}
};

/**
 * BlackHoleEngine - Manages multiple black holes and their physics
 */
class BlackHoleEngine {
private:
    std::vector<BlackHole> blackHoles;
    bool enabled;

    // Constants as per spec
    static constexpr float DEFAULT_MASS = 1000.0f;
    static constexpr float DEFAULT_EVENT_HORIZON = 8.0f;
    static constexpr float DEFAULT_INFLUENCE_RADIUS = 150.0f;
    static constexpr int MAX_BLACK_HOLES = 16;
    static constexpr float MIN_BLACK_HOLE_MASS = 100.0f;
    static constexpr float MAX_BLACK_HOLE_MASS = 10000.0f;
    static constexpr float GRAVITATIONAL_CONSTANT = 6.674f;
    static constexpr float INVERSE_SQUARE_DAMPING = 0.5f;
    static constexpr float MAX_FORCE = 2.0f;

public:
    BlackHoleEngine();
    ~BlackHoleEngine() = default;

    /**
     * Add a black hole at position
     * @return Index of added black hole (0-15), or -1 if at capacity
     */
    int addBlackHole(float x, float y, float mass = DEFAULT_MASS,
                     float event_horizon = DEFAULT_EVENT_HORIZON);

    /**
     * Remove a black hole by index
     */
    void removeBlackHole(int index);

    /**
     * Update black hole position
     */
    void setPosition(int index, float x, float y);

    /**
     * Update black hole mass
     */
    void setMass(int index, float mass);

    /**
     * Set black hole properties at a specific index (creates if needed)
     */
    void setBlackHole(int index, float x, float y, float mass, float event_horizon);

    /**
     * Get number of active black holes
     */
    int getCount() const;

    /**
     * Get black hole info
     */
    BlackHole getBlackHole(int index) const;

    /**
     * Clear all black holes
     */
    void clearAll();

    /**
     * Enable/disable black hole physics
     */
    void setEnabled(bool enabled);

    /**
     * Check if black holes are enabled
     */
    bool isEnabled() const;

    /**
     * Calculate attraction force on a cell
     * @param cellX Cell X position
     * @param cellY Cell Y position
     * @param outDx Output: X component of force direction
     * @param outDy Output: Y component of force direction
     * @return Force magnitude (0-1 normalized)
     */
    float calculateAttraction(float cellX, float cellY, float& outDx, float& outDy) const;

    /**
     * Check if position is within any event horizon
     * @return Index of consuming black hole, or -1 if not consumed
     */
    int checkEventHorizon(float x, float y) const;

    /**
     * Check if position is within influence radius of any black hole
     */
    bool isInInfluenceRadius(float x, float y) const;
};

#endif // BLACK_HOLE_ENGINE_H
