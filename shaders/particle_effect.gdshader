shader_type canvas_item;

uniform vec4 tint : source_color = vec4(1.0);
uniform float emission_strength : hint_range(0.0, 5.0) = 1.0;

void fragment() {
    vec4 color = texture(TEXTURE, UV);
    color.rgb *= emission_strength;
    color *= tint;
    COLOR = color;
}
