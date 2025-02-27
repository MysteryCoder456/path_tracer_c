#version 410

layout(location = 0) in vec2 pos;
layout(location = 1) in vec3 color;
out vec3 vertColor;

void main() {
    gl_Position = vec4(pos, 0.0, 1.0);
    vertColor = color;
}
