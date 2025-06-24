#version 100

uniform sampler2D texture;
precision lowp float;
varying vec2 var_uv;
varying vec4 var_color;

void main(void) {
    gl_FragColor = var_color * texture2D(texture, var_uv).aaaa;
}
