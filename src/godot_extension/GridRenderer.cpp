#include "GridRenderer.h"

#include <godot_cpp/godot.hpp>

GridRenderer::GridRenderer()
    : width(0)
    , height(0)
    , dirty(false) {
}

void GridRenderer::initialize(int w, int h) {
    width = w;
    height = h;
    gridImage = Image::create(width, height, false, Image::FORMAT_RGBA8);
    gridTexture = ImageTexture::create_from_image(gridImage);
    dirty = false;
}

void GridRenderer::shutdown() {
    width = 0;
    height = 0;
    gridImage.unref();
    gridTexture.unref();
}

void GridRenderer::updateTexture(const void* gridData, int elementSize) {
    if (!gridImage.is_valid()) return;

    const uint8_t* data = static_cast<const uint8_t*>(gridData);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            uint8_t elementType = data[idx * elementSize];

            // Get color from element type (simplified - would use getElementColor in real implementation)
            Color color = Color(0.1f, 0.1f, 0.15f, 1.0f); // Default empty color
            gridImage->set_pixel(x, y, color);
        }
    }

    if (gridTexture.is_valid()) {
        gridTexture->update(gridImage);
    }
    dirty = true;
}

void GridRenderer::_bind_methods() {
    // No methods needed for now - will be expanded for GPU rendering
}
