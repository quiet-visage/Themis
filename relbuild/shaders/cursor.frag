#version 430

in vec2 frag_tex_coord;

uniform sampler2D texture0;
out vec4 final_color;

void main() {
    final_color = vec4(0.0, 1.0, 0.0, 1.0);
}
