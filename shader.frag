#version 100

uniform sampler2D texture;
precision lowp float;

vec3 color_map[4];

void main(void) {
    color_map[0] = vec3(0.0);
    color_map[1] = vec3(1.0, 0.0, 1.0);
    color_map[2] = vec3(1.0, 1.0, 0.0);
    color_map[3] = vec3(0.0, 0.0, 1.0);

    vec2 uv = gl_FragCoord.xy/vec2(1024.0, 512.0);

    int mat = int(texture2D(texture, uv).r * 255.0 + 0.5);
    vec3 color = color_map[mat];

    gl_FragColor = vec4(color, 1.0);
}
