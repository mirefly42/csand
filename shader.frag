#version 100

uniform sampler2D texture;
uniform sampler2D palette;
uniform int palette_size;
precision mediump float;

void main(void) {
    vec2 uv = gl_FragCoord.xy/vec2(1024.0, 512.0);

    int mat = int(texture2D(texture, uv).r * 255.0 + 0.5);

    vec4 color = texture2D(palette, vec2((float(mat) + 0.5) / float(palette_size), 0.5));

    gl_FragColor = color;
}
