#version 100

attribute vec2 position;
attribute vec2 uv;
attribute vec4 color;
varying vec2 var_uv;
varying vec4 var_color;
uniform vec2 framebuffer_size;

void main(void) {
    var_uv = uv;
    var_color = color;
    gl_Position = vec4(vec2(position.x, framebuffer_size.y - 1.0 - position.y) / framebuffer_size * 2.0 - 1.0, 0.0, 1.0);
}
