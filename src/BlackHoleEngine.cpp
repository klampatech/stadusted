#include "BlackHoleEngine.h"
#include <algorithm>

BlackHoleEngine::BlackHoleEngine() : enabled(true) {
    blackHoles.reserve(MAX_BLACK_HOLES);
}

int BlackHoleEngine::addBlackHole(float x, float y, float mass, float event_horizon) {
    if (blackHoles.size() >= MAX_BLACK_HOLES) {
        return -1; // At capacity
    }

    // Clamp mass to valid range
    mass = std::max(MIN_BLACK_HOLE_MASS, std::min(mass, MAX_BLACK_HOLE_MASS));

    BlackHole bh;
    bh.x = x;
    bh.y = y;
    bh.mass = mass;
    bh.event_horizon = event_horizon;
    bh.influence_radius = DEFAULT_INFLUENCE_RADIUS;
    bh.active = true;

    int index = static_cast<int>(blackHoles.size());
    blackHoles.push_back(bh);
    return index;
}

void BlackHoleEngine::removeBlackHole(int index) {
    if (index >= 0 && index < static_cast<int>(blackHoles.size())) {
        // Mark as inactive instead of erasing to keep indices stable
        blackHoles[index].active = false;
    }
}

void BlackHoleEngine::setPosition(int index, float x, float y) {
    if (index >= 0 && index < static_cast<int>(blackHoles.size())) {
        blackHoles[index].x = x;
        blackHoles[index].y = y;
    }
}

void BlackHoleEngine::setMass(int index, float mass) {
    if (index >= 0 && index < static_cast<int>(blackHoles.size())) {
        blackHoles[index].mass = std::max(MIN_BLACK_HOLE_MASS, std::min(mass, MAX_BLACK_HOLE_MASS));
    }
}

void BlackHoleEngine::setBlackHole(int index, float x, float y, float mass, float event_horizon) {
    // Clamp mass to valid range
    mass = std::max(MIN_BLACK_HOLE_MASS, std::min(mass, MAX_BLACK_HOLE_MASS));

    // If index is within current size, update existing
    if (index >= 0 && index < static_cast<int>(blackHoles.size())) {
        blackHoles[index].x = x;
        blackHoles[index].y = y;
        blackHoles[index].mass = mass;
        blackHoles[index].event_horizon = event_horizon;
        blackHoles[index].active = true;
    }
    // If index is at the end, add new
    else if (index == static_cast<int>(blackHoles.size()) && index < MAX_BLACK_HOLES) {
        BlackHole bh;
        bh.x = x;
        bh.y = y;
        bh.mass = mass;
        bh.event_horizon = event_horizon;
        bh.influence_radius = DEFAULT_INFLUENCE_RADIUS;
        bh.active = true;
        blackHoles.push_back(bh);
    }
    // If index is beyond current size, pad with inactive black holes
    else if (index > static_cast<int>(blackHoles.size()) && index < MAX_BLACK_HOLES) {
        // Add padding black holes
        while (blackHoles.size() < static_cast<size_t>(index)) {
            BlackHole bh;
            bh.active = false;
            blackHoles.push_back(bh);
        }
        // Add the actual black hole
        BlackHole bh;
        bh.x = x;
        bh.y = y;
        bh.mass = mass;
        bh.event_horizon = event_horizon;
        bh.influence_radius = DEFAULT_INFLUENCE_RADIUS;
        bh.active = true;
        blackHoles.push_back(bh);
    }
}

int BlackHoleEngine::getCount() const {
    return static_cast<int>(blackHoles.size());
}

BlackHole BlackHoleEngine::getBlackHole(int index) const {
    if (index >= 0 && index < static_cast<int>(blackHoles.size())) {
        return blackHoles[index];
    }
    return BlackHole();
}

void BlackHoleEngine::clearAll() {
    blackHoles.clear();
}

void BlackHoleEngine::setEnabled(bool enabled) {
    this->enabled = enabled;
}

bool BlackHoleEngine::isEnabled() const {
    return enabled;
}

float BlackHoleEngine::calculateAttraction(float cellX, float cellY, float& outDx, float& outDy) const {
    if (!enabled || blackHoles.empty()) {
        outDx = 0.0f;
        outDy = 0.0f;
        return 0.0f;
    }

    float totalForceX = 0.0f;
    float totalForceY = 0.0f;
    float maxForce = 0.0f;

    for (const BlackHole& bh : blackHoles) {
        if (!bh.active) continue;

        // Calculate distance to black hole
        float dx = bh.x - cellX;
        float dy = bh.y - cellY;
        float distanceSq = dx * dx + dy * dy;
        float distance = std::sqrt(distanceSq);

        // Skip if too far or at exact center
        if (distance > bh.influence_radius || distance < 0.01f) continue;

        // Normalized direction
        float dirX = dx / distance;
        float dirY = dy / distance;

        // Force calculation (inverse square with damping at close range)
        float effectiveDistance = distance + INVERSE_SQUARE_DAMPING;
        float forceMagnitude = (GRAVITATIONAL_CONSTANT * bh.mass) /
                               (effectiveDistance * effectiveDistance);

        // Clamp force to prevent extreme acceleration
        forceMagnitude = std::min(forceMagnitude, MAX_FORCE);

        totalForceX += dirX * forceMagnitude;
        totalForceY += dirY * forceMagnitude;

        maxForce = std::max(maxForce, forceMagnitude);
    }

    // Normalize output
    float totalForce = std::sqrt(totalForceX * totalForceX +
                                  totalForceY * totalForceY);
    if (totalForce > 0.001f) {
        float scale = std::min(totalForce, 1.0f) / totalForce;
        outDx = totalForceX * scale;
        outDy = totalForceY * scale;
    } else {
        outDx = 0.0f;
        outDy = 0.0f;
    }

    return std::min(maxForce, 1.0f);
}

int BlackHoleEngine::checkEventHorizon(float x, float y) const {
    if (!enabled) return -1;

    for (int i = 0; i < static_cast<int>(blackHoles.size()); i++) {
        const BlackHole& bh = blackHoles[i];
        if (!bh.active) continue;

        float dx = bh.x - x;
        float dy = bh.y - y;
        float distance = std::sqrt(dx * dx + dy * dy);

        if (distance < bh.event_horizon) {
            return i;
        }
    }
    return -1;
}

bool BlackHoleEngine::isInInfluenceRadius(float x, float y) const {
    if (!enabled) return false;

    for (const BlackHole& bh : blackHoles) {
        if (!bh.active) continue;

        float dx = bh.x - x;
        float dy = bh.y - y;
        float distance = std::sqrt(dx * dx + dy * dy);

        if (distance < bh.influence_radius) {
            return true;
        }
    }
    return false;
}
