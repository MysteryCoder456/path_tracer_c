#version 410

layout(location = 0) in vec2 screen_coords;
out vec2 coords;

void main() {
    gl_Position = vec4(screen_coords, 0.0, 1.0);
    coords = screen_coords;
}
