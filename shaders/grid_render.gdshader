shader_type canvas_item;

uniform sampler2D grid_data : filter_nearest;
uniform sampler2D color_palette : filter_nearest;
uniform vec2 grid_size;

void fragment() {
    vec2 pixel_pos = floor(UV * grid_size);
    float element_id = texture(grid_data, pixel_pos / grid_size).r;
    vec4 color = texture(color_palette, vec2((element_id + 0.5) / 256.0, 0.5));
    COLOR = color;
}
