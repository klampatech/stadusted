#ifndef GRID_RENDERER_H
#define GRID_RENDERER_H

#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>

using namespace godot;

/**
 * GridRenderer - Handles rendering the simulation grid to Godot textures
 */
class GridRenderer : public Node {
    GDCLASS(GridRenderer, Node)

private:
    Ref<Image> gridImage;
    Ref<ImageTexture> gridTexture;
    int width;
    int height;
    bool dirty;

public:
    GridRenderer();
    ~GridRenderer() override = default;

    void initialize(int w, int h);
    void shutdown();

    void updateTexture(const void* gridData, int elementSize);
    Ref<ImageTexture> getTexture() const { return gridTexture; }
    bool isDirty() const { return dirty; }
    void markClean() { dirty = false; }

protected:
    static void _bind_methods();
};

#endif // GRID_RENDERER_H
