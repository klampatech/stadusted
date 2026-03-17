#include "FallingSandEngine.h"
#include "BlackHoleEngine.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <iostream>

// Get color for each element type (as per spec)
ElementColor getElementColor(ElementType type) {
    switch (type) {
        case ElementType::EMPTY:       return {20, 20, 30, 255};     // Dark background
        case ElementType::SAND:      return {194, 178, 128, 255}; // Warm tan
        case ElementType::WATER:     return {30, 144, 255, 255};   // Blue
        case ElementType::STONE:     return {128, 128, 128, 255};  // Gray
        case ElementType::SMOKE:     return {80, 80, 80, 180};     // Gray with transparency
        case ElementType::WOOD:      return {139, 69, 19, 255};    // Brown
        case ElementType::OIL:       return {30, 30, 30, 255};     // Dark/black
        case ElementType::ACID:      return {50, 205, 50, 255};    // Green
        case ElementType::GUNPOWDER: return {60, 60, 60, 255};     // Dark gray
        case ElementType::FIRE:      return {255, 100, 0, 255};    // Orange-red fire
        // Ore colors
        case ElementType::COAL:      return {20, 20, 20, 255};     // Black
        case ElementType::COPPER:    return {184, 115, 51, 255};   // Copper/bronze
        case ElementType::GOLD:      return {255, 215, 0, 255};    // Gold
        case ElementType::DIAMOND:   return {0, 191, 255, 255};   // Cyan/diamond blue
        case ElementType::DIRT:      return {101, 67, 33, 255};    // Brown dirt
        case ElementType::GOLD_CRACKED: return {169, 169, 169, 255}; // Darker gray
        // Pickup colors
        case ElementType::PICKUP_PARTICLES: return {0, 255, 127, 255}; // Bright green
        case ElementType::PICKUP_POINTS: return {255, 215, 0, 255};   // Gold
        // Planet element colors
        case ElementType::PLANET_CORE:    return {255, 80, 30, 255};    // Hot orange-red
        case ElementType::PLANET_MANTLE: return {180, 60, 20, 255};   // Magma red
        case ElementType::PLANET_CRUST:  return {100, 80, 60, 255};   // Brown/rock
        case ElementType::BLACK_HOLE:    return {0, 0, 0, 255};       // Pure black
        default:                        return {20, 20, 30, 255};
    }
}

// Get fire color with gradient variation (orange/red/yellow)
ElementColor getFireColor(int x, int y) {
    int variation = (x * 7 + y * 13 + rand() % 30) % 100;

    if (variation < 33) {
        // Yellow (hottest)
        return {255, 255, 50, 255};
    } else if (variation < 66) {
        // Orange
        return {255, 140, 0, 255};
    } else {
        // Red-orange (cooler)
        return {255, 69, 0, 255};
    }
}

// Get smoke color with slight variation for texture
ElementColor getSmokeColor(int x, int y) {
    int variation = (x * 11 + y * 17) % 40;
    uint8_t gray = 80 + variation;
    return {gray, gray, gray, 150};
}

// Grid implementation
Grid::Grid(int w, int h) : width(w), height(h) {
    currentBuffer.resize(width * height, ElementType::EMPTY);
    nextBuffer.resize(width * height, ElementType::EMPTY);
    stressMap.resize(width * height, 0.0f);
    nextStressMap.resize(width * height, 0.0f);
    gravityDisplacementX.resize(width * height, 0.0f);
    gravityDisplacementY.resize(width * height, 0.0f);
    elementCounts.resize(22, 0); // 22 element types
}

Grid::~Grid() = default;

int Grid::getWidth() const { return width; }
int Grid::getHeight() const { return height; }

ElementType Grid::getCell(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return ElementType::EMPTY;
    }
    return currentBuffer[y * width + x];
}

void Grid::setCell(int x, int y, ElementType type) {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return;
    }
    currentBuffer[y * width + x] = type;
}

// Swap buffers for double-buffering
void Grid::swapBuffers() {
    currentBuffer.swap(nextBuffer);
    stressMap.swap(nextStressMap);
    std::fill(nextBuffer.begin(), nextBuffer.end(), ElementType::EMPTY);
    std::fill(nextStressMap.begin(), nextStressMap.end(), 0.0f);
    updateElementCounts();
}

// Get reference to next buffer for updates
ElementType& Grid::getNextCell(int x, int y) {
    return nextBuffer[y * width + x];
}

// Check if cell is empty in current buffer
bool Grid::isEmpty(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return false;
    }
    return currentBuffer[y * width + x] == ElementType::EMPTY;
}

// Check if cell contains sand in current buffer
bool Grid::isSand(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return false;
    }
    return currentBuffer[y * width + x] == ElementType::SAND;
}

// Check if cell contains water in current buffer
bool Grid::isWater(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return false;
    }
    return currentBuffer[y * width + x] == ElementType::WATER;
}

// Check if cell is empty or water (for water displacement)
bool Grid::isEmptyOrWater(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return false;
    }
    ElementType type = currentBuffer[y * width + x];
    return type == ElementType::EMPTY || type == ElementType::WATER;
}

// Check if cell contains a flammable element
bool Grid::isFlammable(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return false;
    }
    ElementType type = currentBuffer[y * width + x];
    return type == ElementType::WOOD || type == ElementType::OIL ||
           type == ElementType::GUNPOWDER || type == ElementType::SAND;
}

// Check if cell contains fire in current buffer
bool Grid::isFire(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return false;
    }
    return currentBuffer[y * width + x] == ElementType::FIRE;
}

// Check if cell contains wood in current buffer
bool Grid::isWood(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return false;
    }
    return currentBuffer[y * width + x] == ElementType::WOOD;
}

// Check if cell contains oil in current buffer
bool Grid::isOil(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return false;
    }
    return currentBuffer[y * width + x] == ElementType::OIL;
}

// Check if cell contains acid in current buffer
bool Grid::isAcid(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return false;
    }
    return currentBuffer[y * width + x] == ElementType::ACID;
}

// Check if cell contains gunpowder in current buffer
bool Grid::isGunpowder(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return false;
    }
    return currentBuffer[y * width + x] == ElementType::GUNPOWDER;
}

// Check if cell contains a destructible element (acid can destroy most things)
bool Grid::isDestructible(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return false;
    }
    ElementType type = currentBuffer[y * width + x];
    // Acid destroys everything except itself, fire, smoke, empty, and stone/diamond (indestructible)
    return type != ElementType::EMPTY &&
           type != ElementType::ACID &&
           type != ElementType::FIRE &&
           type != ElementType::SMOKE &&
           type != ElementType::STONE &&
           type != ElementType::DIAMOND;
}

// Check if cell contains burning wood (FIRE on top of WOOD in current buffer)
bool Grid::isBurning(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return false;
    }
    return currentBuffer[y * width + x] == ElementType::FIRE;
}

// Check if cell contains a planet element
bool Grid::isPlanetElement(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return false;
    }
    ElementType type = currentBuffer[y * width + x];
    return type == ElementType::PLANET_CORE ||
           type == ElementType::PLANET_MANTLE ||
           type == ElementType::PLANET_CRUST;
}

// Update physics for all elements
void Grid::update() {
    updateWithBlackHoles(nullptr);
}

void Grid::updateWithBlackHoles(BlackHoleEngine* blackHoleEngine) {
    // Alternate both vertical direction and horizontal direction each frame
    // to prevent visual bias and ensure even processing across the grid
    static bool bottomToTop = true;
    static bool leftToRight = true;

    // Calculate gravity displacement from black holes for all cells
    if (blackHoleEngine && blackHoleEngine->getCount() > 0) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                ElementType cellType = getCell(x, y);
                // Only apply gravity to movable non-planet elements
                if (cellType != ElementType::EMPTY && !isPlanetElement(x, y)) {
                    float dx, dy;
                    float force = blackHoleEngine->calculateAttraction(
                        static_cast<float>(x), static_cast<float>(y), dx, dy);
                    // Store displacement (capped to prevent extreme movement)
                    gravityDisplacementX[y * width + x] = std::max(-2.0f, std::min(2.0f, dx * force * 0.5f));
                    gravityDisplacementY[y * width + x] = std::max(-2.0f, std::min(2.0f, dy * force * 0.5f));
                } else {
                    gravityDisplacementX[y * width + x] = 0.0f;
                    gravityDisplacementY[y * width + x] = 0.0f;
                }
            }
        }
    } else {
        // No black holes - clear displacement
        std::fill(gravityDisplacementX.begin(), gravityDisplacementX.end(), 0.0f);
        std::fill(gravityDisplacementY.begin(), gravityDisplacementY.end(), 0.0f);
    }

    int yStart = bottomToTop ? height - 1 : 0;
    int yEnd = bottomToTop ? -1 : height;
    int yStep = bottomToTop ? -1 : 1;

    for (int y = yStart; y != yEnd; y += yStep) {
        int startX = leftToRight ? 0 : width - 1;
        int endX = leftToRight ? width : -1;
        int stepX = leftToRight ? 1 : -1;

        for (int x = startX; x != endX; x += stepX) {
            ElementType current = getCell(x, y);

            // Check event horizon consumption first
            if (blackHoleEngine) {
                int consumingIndex = blackHoleEngine->checkEventHorizon(x, y);
                if (consumingIndex >= 0) {
                    // Track consumed element for signal emission
                    ConsumedElement consumed = {x, y, current};
                    consumedElements.push_back(consumed);
                    // Element is consumed by black hole - set to empty
                    getNextCell(x, y) = ElementType::EMPTY;
                    // Reset stress when consumed
                    nextStressMap[y * width + x] = 0.0f;
                    continue;
                }

                // Accumulate stress for planet elements near black holes
                if (isPlanetElement(x, y)) {
                    float dx, dy;
                    float force = blackHoleEngine->calculateAttraction(
                        static_cast<float>(x), static_cast<float>(y), dx, dy);

                    // Only accumulate stress if there's meaningful force
                    if (force > 0.01f) {
                        float currentStress = getStress(x, y);
                        float stressIncrease = force * STRESS_ACCUMULATION_RATE;
                        float newStress = std::min(currentStress + stressIncrease, MAX_STRESS);
                        nextStressMap[y * width + x] = newStress;
                    } else {
                        // Decay stress slowly when not under gravity
                        float currentStress = getStress(x, y);
                        nextStressMap[y * width + x] = std::max(currentStress - 0.001f, 0.0f);
                    }
                }
            }

            // Process elements based on type
            switch (current) {
                case ElementType::SAND:
                    updateSand(x, y);
                    break;
                case ElementType::WATER:
                    updateWater(x, y);
                    break;
                case ElementType::FIRE:
                    updateFire(x, y);
                    break;
                case ElementType::SMOKE:
                    updateSmoke(x, y);
                    break;
                case ElementType::WOOD:
                    updateWood(x, y);
                    break;
                case ElementType::OIL:
                    updateOil(x, y);
                    break;
                case ElementType::ACID:
                    updateAcid(x, y);
                    break;
                case ElementType::GUNPOWDER:
                    updateGunpowder(x, y);
                    break;
                case ElementType::PLANET_CORE:
                case ElementType::PLANET_MANTLE:
                case ElementType::PLANET_CRUST:
                    updatePlanetElement(x, y);
                    break;
                default:
                    // Static elements - stay in place
                    getNextCell(x, y) = current;
                    break;
            }
        }
    }

    // Toggle both directions for next frame to prevent any persistent bias
    bottomToTop = !bottomToTop;
    leftToRight = !leftToRight;
}

void Grid::updateSand(int x, int y) {
    // Get gravity displacement from black holes
    float gravX = gravityDisplacementX[y * width + x];
    float gravY = gravityDisplacementY[y * width + x];

    // Try to move down
    if (isEmpty(x, y + 1)) {
        getNextCell(x, y + 1) = ElementType::SAND;
        return;
    }

    // Try to move down-left or down-right (45-degree pile angle)
    // Consider black hole gravity bias for horizontal movement
    bool preferLeft = gravX < -0.3f;
    bool preferRight = gravX > 0.3f;
    bool tryLeftFirst = (rand() % 2) == 0;

    // If gravity prefers a direction, try that first
    if (preferLeft) tryLeftFirst = true;
    else if (preferRight) tryLeftFirst = false;

    if (tryLeftFirst) {
        if (isEmpty(x - 1, y + 1)) {
            getNextCell(x - 1, y + 1) = ElementType::SAND;
            return;
        }
        if (isEmpty(x + 1, y + 1)) {
            getNextCell(x + 1, y + 1) = ElementType::SAND;
            return;
        }
    } else {
        if (isEmpty(x + 1, y + 1)) {
            getNextCell(x + 1, y + 1) = ElementType::SAND;
            return;
        }
        if (isEmpty(x - 1, y + 1)) {
            getNextCell(x - 1, y + 1) = ElementType::SAND;
            return;
        }
    }

    // Try pure horizontal movement if strong gravity pull
    if (preferLeft && isEmpty(x - 1, y)) {
        getNextCell(x - 1, y) = ElementType::SAND;
        return;
    }
    if (preferRight && isEmpty(x + 1, y)) {
        getNextCell(x + 1, y) = ElementType::SAND;
        return;
    }

    // Can't move - stay in place
    getNextCell(x, y) = ElementType::SAND;
}

void Grid::updateWater(int x, int y) {
    // Try to move down
    if (isEmpty(x, y + 1)) {
        getNextCell(x, y + 1) = ElementType::WATER;
        return;
    }

    // Try to move down-left or down-right
    bool tryLeftFirst = (rand() % 2) == 0;

    if (tryLeftFirst) {
        if (isEmpty(x - 1, y + 1)) {
            getNextCell(x - 1, y + 1) = ElementType::WATER;
            return;
        }
        if (isEmpty(x + 1, y + 1)) {
            getNextCell(x + 1, y + 1) = ElementType::WATER;
            return;
        }
    } else {
        if (isEmpty(x + 1, y + 1)) {
            getNextCell(x + 1, y + 1) = ElementType::WATER;
            return;
        }
        if (isEmpty(x - 1, y + 1)) {
            getNextCell(x - 1, y + 1) = ElementType::WATER;
            return;
        }
    }

    // Water spreads horizontally when blocked below
    bool tryLeft = (rand() % 2) == 0;

    if (tryLeft) {
        if (isEmpty(x - 1, y)) {
            getNextCell(x - 1, y) = ElementType::WATER;
            return;
        }
        if (isEmpty(x + 1, y)) {
            getNextCell(x + 1, y) = ElementType::WATER;
            return;
        }
    } else {
        if (isEmpty(x + 1, y)) {
            getNextCell(x + 1, y) = ElementType::WATER;
            return;
        }
        if (isEmpty(x - 1, y)) {
            getNextCell(x - 1, y) = ElementType::WATER;
            return;
        }
    }

    // Can't move - stay in place
    getNextCell(x, y) = ElementType::WATER;
}

// Fire lifespan configuration
static constexpr int FIRE_DEATH_CHANCE = 5;
static constexpr int IGNITE_CHANCE = 10;

void Grid::updateFire(int x, int y) {
    // Fire has limited lifespan - chance to die each frame
    if (rand() % 100 < FIRE_DEATH_CHANCE) {
        getNextCell(x, y) = ElementType::SMOKE;
        return;
    }

    // Try to move up (fire rises)
    if (isEmpty(x, y - 1)) {
        getNextCell(x, y - 1) = ElementType::FIRE;
        igniteNearbyFlammable(x, y - 1);
        return;
    }

    // Try to move up-left or up-right
    bool tryLeftFirst = (rand() % 2) == 0;

    if (tryLeftFirst) {
        if (isEmpty(x - 1, y - 1)) {
            getNextCell(x - 1, y - 1) = ElementType::FIRE;
            igniteNearbyFlammable(x - 1, y - 1);
            return;
        }
        if (isEmpty(x + 1, y - 1)) {
            getNextCell(x + 1, y - 1) = ElementType::FIRE;
            igniteNearbyFlammable(x + 1, y - 1);
            return;
        }
    } else {
        if (isEmpty(x + 1, y - 1)) {
            getNextCell(x + 1, y - 1) = ElementType::FIRE;
            igniteNearbyFlammable(x + 1, y - 1);
            return;
        }
        if (isEmpty(x - 1, y - 1)) {
            getNextCell(x - 1, y - 1) = ElementType::FIRE;
            igniteNearbyFlammable(x - 1, y - 1);
            return;
        }
    }

    // Try horizontal drift
    bool tryLeft = (rand() % 2) == 0;

    if (tryLeft) {
        if (isEmpty(x - 1, y)) {
            getNextCell(x - 1, y) = ElementType::FIRE;
            igniteNearbyFlammable(x - 1, y);
            return;
        }
        if (isEmpty(x + 1, y)) {
            getNextCell(x + 1, y) = ElementType::FIRE;
            igniteNearbyFlammable(x + 1, y);
            return;
        }
    } else {
        if (isEmpty(x + 1, y)) {
            getNextCell(x + 1, y) = ElementType::FIRE;
            igniteNearbyFlammable(x + 1, y);
            return;
        }
        if (isEmpty(x - 1, y)) {
            getNextCell(x - 1, y) = ElementType::FIRE;
            igniteNearbyFlammable(x - 1, y);
            return;
        }
    }

    // Can't move - stay in place but try to ignite
    igniteNearbyFlammable(x, y);
    getNextCell(x, y) = ElementType::FIRE;
}

// Try to ignite flammable elements adjacent to the given position
void Grid::igniteNearbyFlammable(int x, int y) {
    const int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    for (int i = 0; i < 8; i++) {
        int checkX = x + dx[i];
        int checkY = y + dy[i];

        if (isFlammable(checkX, checkY)) {
            if (rand() % 100 < IGNITE_CHANCE) {
                currentBuffer[checkY * width + checkX] = ElementType::FIRE;
            }
        }
    }
}

// Smoke configuration
static constexpr int SMOKE_DEATH_CHANCE = 3;

void Grid::updateSmoke(int x, int y) {
    // Smoke has limited lifespan - chance to disperse each frame
    if (rand() % 100 < SMOKE_DEATH_CHANCE) {
        getNextCell(x, y) = ElementType::EMPTY;
        return;
    }

    // Try to move up (smoke rises)
    if (isEmpty(x, y - 1)) {
        getNextCell(x, y - 1) = ElementType::SMOKE;
        return;
    }

    // Try to move up-left or up-right
    bool tryLeftFirst = (rand() % 2) == 0;

    if (tryLeftFirst) {
        if (isEmpty(x - 1, y - 1)) {
            getNextCell(x - 1, y - 1) = ElementType::SMOKE;
            return;
        }
        if (isEmpty(x + 1, y - 1)) {
            getNextCell(x + 1, y - 1) = ElementType::SMOKE;
            return;
        }
    } else {
        if (isEmpty(x + 1, y - 1)) {
            getNextCell(x + 1, y - 1) = ElementType::SMOKE;
            return;
        }
        if (isEmpty(x - 1, y - 1)) {
            getNextCell(x - 1, y - 1) = ElementType::SMOKE;
            return;
        }
    }

    // Spread horizontally
    bool tryLeft = (rand() % 2) == 0;

    if (tryLeft) {
        if (isEmpty(x - 1, y)) {
            getNextCell(x - 1, y) = ElementType::SMOKE;
            return;
        }
        if (isEmpty(x + 1, y)) {
            getNextCell(x + 1, y) = ElementType::SMOKE;
            return;
        }
    } else {
        if (isEmpty(x + 1, y)) {
            getNextCell(x + 1, y) = ElementType::SMOKE;
            return;
        }
        if (isEmpty(x - 1, y)) {
            getNextCell(x - 1, y) = ElementType::SMOKE;
            return;
        }
    }

    // Can't move - stay in place
    getNextCell(x, y) = ElementType::SMOKE;
}

// Wood burning configuration
static constexpr int BURN_CHANCE = 2;

void Grid::updateWood(int x, int y) {
    const int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    bool adjacentToFire = false;
    for (int i = 0; i < 8; i++) {
        int checkX = x + dx[i];
        int checkY = y + dy[i];
        if (isFire(checkX, checkY)) {
            adjacentToFire = true;
            break;
        }
    }

    if (adjacentToFire) {
        if (rand() % 100 < BURN_CHANCE) {
            getNextCell(x, y) = ElementType::FIRE;
            return;
        }
    }

    if (isFire(x, y)) {
        getNextCell(x, y) = ElementType::SMOKE;
        return;
    }

    // Wood is static
    getNextCell(x, y) = ElementType::WOOD;
}

// Oil configuration
static constexpr int OIL_IGNITE_CHANCE = 15;

void Grid::updateOil(int x, int y) {
    const int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    bool adjacentToFire = false;
    for (int i = 0; i < 8; i++) {
        int checkX = x + dx[i];
        int checkY = y + dy[i];
        if (isFire(checkX, checkY)) {
            adjacentToFire = true;
            break;
        }
    }

    if (adjacentToFire) {
        if (rand() % 100 < OIL_IGNITE_CHANCE) {
            getNextCell(x, y) = ElementType::FIRE;
            return;
        }
    }

    if (isFire(x, y)) {
        // Spread fire to adjacent oil cells
        for (int i = 0; i < 8; i++) {
            int spreadX = x + dx[i];
            int spreadY = y + dy[i];
            if (isOil(spreadX, spreadY) && rand() % 100 < 30) {
                currentBuffer[spreadY * width + spreadX] = ElementType::FIRE;
            }
        }
        getNextCell(x, y) = ElementType::FIRE;
        return;
    }

    // Oil flows like water
    if (isEmpty(x, y + 1)) {
        getNextCell(x, y + 1) = ElementType::OIL;
        return;
    }

    bool tryLeftFirst = (rand() % 2) == 0;

    if (tryLeftFirst) {
        if (isEmpty(x - 1, y + 1)) {
            getNextCell(x - 1, y + 1) = ElementType::OIL;
            return;
        }
        if (isEmpty(x + 1, y + 1)) {
            getNextCell(x + 1, y + 1) = ElementType::OIL;
            return;
        }
    } else {
        if (isEmpty(x + 1, y + 1)) {
            getNextCell(x + 1, y + 1) = ElementType::OIL;
            return;
        }
        if (isEmpty(x - 1, y + 1)) {
            getNextCell(x - 1, y + 1) = ElementType::OIL;
            return;
        }
    }

    // Spread horizontally
    bool tryLeft = (rand() % 2) == 0;

    if (tryLeft) {
        if (isEmpty(x - 1, y)) {
            getNextCell(x - 1, y) = ElementType::OIL;
            return;
        }
        if (isEmpty(x + 1, y)) {
            getNextCell(x + 1, y) = ElementType::OIL;
            return;
        }
    } else {
        if (isEmpty(x + 1, y)) {
            getNextCell(x + 1, y) = ElementType::OIL;
            return;
        }
        if (isEmpty(x - 1, y)) {
            getNextCell(x - 1, y) = ElementType::OIL;
            return;
        }
    }

    getNextCell(x, y) = ElementType::OIL;
}

// Acid configuration
static constexpr int DISSOLVE_CHANCE = 30;

void Grid::updateAcid(int x, int y) {
    dissolveNearby(x, y);

    // Acid flows like water
    if (isEmpty(x, y + 1)) {
        getNextCell(x, y + 1) = ElementType::ACID;
        return;
    }

    bool tryLeftFirst = (rand() % 2) == 0;

    if (tryLeftFirst) {
        if (isEmpty(x - 1, y + 1)) {
            getNextCell(x - 1, y + 1) = ElementType::ACID;
            return;
        }
        if (isEmpty(x + 1, y + 1)) {
            getNextCell(x + 1, y + 1) = ElementType::ACID;
            return;
        }
    } else {
        if (isEmpty(x + 1, y + 1)) {
            getNextCell(x + 1, y + 1) = ElementType::ACID;
            return;
        }
        if (isEmpty(x - 1, y + 1)) {
            getNextCell(x - 1, y + 1) = ElementType::ACID;
            return;
        }
    }

    bool tryLeft = (rand() % 2) == 0;

    if (tryLeft) {
        if (isEmpty(x - 1, y)) {
            getNextCell(x - 1, y) = ElementType::ACID;
            return;
        }
        if (isEmpty(x + 1, y)) {
            getNextCell(x + 1, y) = ElementType::ACID;
            return;
        }
    } else {
        if (isEmpty(x + 1, y)) {
            getNextCell(x + 1, y) = ElementType::ACID;
            return;
        }
        if (isEmpty(x - 1, y)) {
            getNextCell(x - 1, y) = ElementType::ACID;
            return;
        }
    }

    getNextCell(x, y) = ElementType::ACID;
}

void Grid::dissolveNearby(int x, int y) {
    const int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    for (int i = 0; i < 8; i++) {
        int checkX = x + dx[i];
        int checkY = y + dy[i];

        if (isDestructible(checkX, checkY)) {
            if (rand() % 100 < DISSOLVE_CHANCE) {
                currentBuffer[checkY * width + checkX] = ElementType::EMPTY;
            }
        }
    }
}

// Gunpowder configuration
static constexpr int GUNPOWDER_IGNITE_CHANCE = 20;
static constexpr int EXPLOSION_RADIUS = 4;

void Grid::createExplosion(int x, int y) {
    // Destroy elements in explosion radius
    for (int dy = -EXPLOSION_RADIUS; dy <= EXPLOSION_RADIUS; dy++) {
        for (int dx = -EXPLOSION_RADIUS; dx <= EXPLOSION_RADIUS; dx++) {
            int targetX = x + dx;
            int targetY = y + dy;

            if (targetX < 0 || targetX >= width || targetY < 0 || targetY >= height) {
                continue;
            }

            if (dx * dx + dy * dy <= EXPLOSION_RADIUS * EXPLOSION_RADIUS) {
                ElementType target = currentBuffer[targetY * width + targetX];

                if (target == ElementType::STONE) {
                    continue;
                }

                int distance = dx * dx + dy * dy;
                int destroyChance = 100 - (distance * 15);

                if (rand() % 100 < destroyChance) {
                    if (rand() % 2 == 0) {
                        currentBuffer[targetY * width + targetX] = ElementType::FIRE;
                    } else {
                        currentBuffer[targetY * width + targetX] = ElementType::SMOKE;
                    }
                }
            }
        }
    }

    // Push elements away from explosion center
    for (int dy = -EXPLOSION_RADIUS; dy <= EXPLOSION_RADIUS; dy++) {
        for (int dx = -EXPLOSION_RADIUS; dx <= EXPLOSION_RADIUS; dx++) {
            int targetX = x + dx;
            int targetY = y + dy;

            if (targetX < 0 || targetX >= width || targetY < 0 || targetY >= height) {
                continue;
            }
            if (dx == 0 && dy == 0) {
                continue;
            }

            if (dx * dx + dy * dy <= EXPLOSION_RADIUS * EXPLOSION_RADIUS) {
                ElementType target = currentBuffer[targetY * width + targetX];

                if (target == ElementType::SAND || target == ElementType::WATER ||
                    target == ElementType::OIL || target == ElementType::GUNPOWDER) {

                    int pushX = targetX;
                    int pushY = targetY;

                    if (dx < 0) pushX = targetX - 1;
                    else if (dx > 0) pushX = targetX + 1;

                    if (dy < 0) pushY = targetY - 1;
                    else if (dy > 0) pushY = targetY + 1;

                    if (isEmpty(pushX, pushY)) {
                        currentBuffer[pushY * width + pushX] = target;
                        currentBuffer[targetY * width + targetX] = ElementType::EMPTY;
                    }
                }
            }
        }
    }

    currentBuffer[y * width + x] = ElementType::FIRE;
}

void Grid::updateGunpowder(int x, int y) {
    const int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    bool adjacentToFire = false;
    for (int i = 0; i < 8; i++) {
        int checkX = x + dx[i];
        int checkY = y + dy[i];
        if (isFire(checkX, checkY)) {
            adjacentToFire = true;
            break;
        }
    }

    if (adjacentToFire) {
        if (rand() % 100 < GUNPOWDER_IGNITE_CHANCE) {
            getNextCell(x, y) = ElementType::FIRE;
            return;
        }
    }

    if (isFire(x, y)) {
        if (rand() % 100 < 25) {
            createExplosion(x, y);
            return;
        }
        getNextCell(x, y) = ElementType::FIRE;
        return;
    }

    // Falls like sand
    if (isEmpty(x, y + 1)) {
        getNextCell(x, y + 1) = ElementType::GUNPOWDER;
        return;
    }

    bool tryLeftFirst = (rand() % 2) == 0;

    if (tryLeftFirst) {
        if (isEmpty(x - 1, y + 1)) {
            getNextCell(x - 1, y + 1) = ElementType::GUNPOWDER;
            return;
        }
        if (isEmpty(x + 1, y + 1)) {
            getNextCell(x + 1, y + 1) = ElementType::GUNPOWDER;
            return;
        }
    } else {
        if (isEmpty(x + 1, y + 1)) {
            getNextCell(x + 1, y + 1) = ElementType::GUNPOWDER;
            return;
        }
        if (isEmpty(x - 1, y + 1)) {
            getNextCell(x - 1, y + 1) = ElementType::GUNPOWDER;
            return;
        }
    }

    getNextCell(x, y) = ElementType::GUNPOWDER;
}

// Planet element update - semi-static but can be affected by gravity
void Grid::updatePlanetElement(int x, int y) {
    ElementType current = getCell(x, y);

    // Get current stress level
    float stress = getStress(x, y);

    // Check if stress exceeds threshold for this element type
    if (shouldBreak(current, stress)) {
        // Cell breaks into debris
        ElementType debris = getDebrisType(current);
        getNextCell(x, y) = debris;
        // Reset stress for the new cell
        nextStressMap[y * width + x] = 0.0f;
    } else {
        // Planet elements are mostly static but may crumble under stress
        getNextCell(x, y) = current;
        // Preserve stress in next buffer
        nextStressMap[y * width + x] = stress;
    }
}

// World generation
void Grid::fillTestPattern() {
    for (int y = 10; y < 30; y++) {
        for (int x = 10; x < 30; x++) {
            setCell(x, y, ElementType::SAND);
        }
    }

    for (int y = 40; y < 60; y++) {
        for (int x = 20; x < 50; x++) {
            setCell(x, y, ElementType::WATER);
        }
    }

    for (int y = 100; y < 120; y++) {
        for (int x = 50; x < 150; x++) {
            setCell(x, y, ElementType::STONE);
        }
    }
}

void Grid::generateWorld(int worldSeed) {
    auto seededRand = [seed = worldSeed]() mutable {
        seed = (seed * 1103515245 + 12345) & 0x7fffffff;
        return seed;
    };

    const float SKY_END = 0.05f;
    const float SURFACE_END = 0.12f;
    const float DIRT_END = 0.25f;

    int skyY = static_cast<int>(height * SKY_END);
    int surfaceY = static_cast<int>(height * SURFACE_END);
    int dirtY = static_cast<int>(height * DIRT_END);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float depth = static_cast<float>(y) / height;
            int randVal = seededRand() % 100;

            if (y < skyY) {
                setCell(x, y, ElementType::EMPTY);
            } else if (y < surfaceY + (randVal % 3)) {
                setCell(x, y, ElementType::SAND);
            } else if (y < dirtY + (randVal % 5)) {
                if (randVal < 3) {
                    setCell(x, y, ElementType::SAND);
                } else {
                    setCell(x, y, ElementType::DIRT);
                }
            } else {
                float oreChance = seededRand() % 1000;

                if (oreChance < 8) {
                    setCell(x, y, ElementType::COAL);
                } else if (oreChance < 15 && depth > 0.30f) {
                    setCell(x, y, ElementType::COPPER);
                } else if (oreChance < 20 && depth > 0.50f) {
                    setCell(x, y, ElementType::GOLD);
                } else if (oreChance < 23 && depth > 0.70f) {
                    setCell(x, y, ElementType::DIAMOND);
                } else {
                    setCell(x, y, ElementType::STONE);
                }
            }
        }
    }

    // Add caves
    for (int i = 0; i < 15; i++) {
        int caveX = seededRand() % width;
        int caveY = dirtY + (seededRand() % (height - dirtY - 10));
        int caveRadius = 3 + (seededRand() % 5);

        for (int dy = -caveRadius; dy <= caveRadius; dy++) {
            for (int dx = -caveRadius; dx <= caveRadius; dx++) {
                if (dx * dx + dy * dy <= caveRadius * caveRadius) {
                    int cx = caveX + dx;
                    int cy = caveY + dy;
                    if (cx >= 0 && cx < width && cy >= 0 && cy < height) {
                        ElementType current = getCell(cx, cy);
                        if (current == ElementType::STONE) {
                            setCell(cx, cy, ElementType::EMPTY);
                        }
                    }
                }
            }
        }
    }
}

void Grid::generatePlanet(int centerX, int centerY, int radius) {
    // Reset stress map for new planet
    resetStressMap();

    int coreRadius = radius / 4;
    int mantleRadius = radius / 2;

    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int dist = static_cast<int>(std::sqrt(dx * dx + dy * dy));
            if (dist > radius) continue;

            int x = centerX + dx;
            int y = centerY + dy;

            ElementType type;
            if (dist <= coreRadius) {
                type = ElementType::PLANET_CORE;
            } else if (dist <= mantleRadius) {
                type = ElementType::PLANET_MANTLE;
            } else {
                type = ElementType::PLANET_CRUST;
            }

            // Add some variation
            if (type == ElementType::PLANET_CRUST && (rand() % 10) == 0) {
                type = ElementType::STONE;
            }

            if (x >= 0 && x < width && y >= 0 && y < height) {
                setCell(x, y, type);
            }
        }
    }
}

void Grid::spawnElement(int gridX, int gridY, ElementType type, int radius) {
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy <= radius * radius) {
                setCell(gridX + dx, gridY + dy, type);
            }
        }
    }
}

void Grid::eraseElement(int gridX, int gridY, int radius) {
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy <= radius * radius) {
                setCell(gridX + dx, gridY + dy, ElementType::EMPTY);
            }
        }
    }
}

void Grid::fillRect(int x, int y, int w, int h, ElementType type) {
    for (int py = y; py < y + h; py++) {
        for (int px = x; px < x + w; px++) {
            setCell(px, py, type);
        }
    }
}

void Grid::fillCircle(int cx, int cy, int radius, ElementType type) {
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy <= radius * radius) {
                setCell(cx + dx, cy + dy, type);
            }
        }
    }
}

void Grid::clear() {
    std::fill(currentBuffer.begin(), currentBuffer.end(), ElementType::EMPTY);
    std::fill(nextBuffer.begin(), nextBuffer.end(), ElementType::EMPTY);
    resetStressMap();
    updateElementCounts();
}

int Grid::getElementCount(ElementType type) const {
    return elementCounts[static_cast<int>(type)];
}

void Grid::updateElementCounts() {
    std::fill(elementCounts.begin(), elementCounts.end(), 0);
    for (size_t i = 0; i < currentBuffer.size(); i++) {
        int typeIndex = static_cast<int>(currentBuffer[i]);
        if (typeIndex >= 0 && typeIndex < 22) {
            elementCounts[typeIndex]++;
        }
    }
}

// Stress management implementation
float Grid::getStress(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return 0.0f;
    }
    return stressMap[y * width + x];
}

void Grid::setStress(int x, int y, float stress) {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return;
    }
    stressMap[y * width + x] = stress;
}

void Grid::resetStressMap() {
    std::fill(stressMap.begin(), stressMap.end(), 0.0f);
    std::fill(nextStressMap.begin(), nextStressMap.end(), 0.0f);
}

bool Grid::shouldBreak(ElementType type, float stress) const {
    switch (type) {
        case ElementType::PLANET_CRUST:
            return stress >= CRUST_STRESS_THRESHOLD;
        case ElementType::PLANET_MANTLE:
            return stress >= MANTLE_STRESS_THRESHOLD;
        case ElementType::PLANET_CORE:
            return stress >= CORE_STRESS_THRESHOLD;
        default:
            return false;
    }
}

ElementType Grid::getDebrisType(ElementType brokenType) const {
    // Planet elements break into stone/sand debris
    switch (brokenType) {
        case ElementType::PLANET_CRUST:
            return (rand() % 2 == 0) ? ElementType::STONE : ElementType::SAND;
        case ElementType::PLANET_MANTLE:
            return (rand() % 3 == 0) ? ElementType::SAND : ElementType::STONE;
        case ElementType::PLANET_CORE:
            // Core becomes lava-like (fire/sand)
            return (rand() % 2 == 0) ? ElementType::FIRE : ElementType::SAND;
        default:
            return ElementType::STONE;
    }
}

std::vector<ConsumedElement> Grid::getConsumedElements() const {
    return consumedElements;
}

void Grid::clearConsumedElements() {
    consumedElements.clear();
}
