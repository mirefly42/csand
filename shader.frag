#version 100

uniform sampler2D texture;
precision lowp float;

void main(void) {
    vec2 uv = gl_FragCoord.xy/vec2(1024.0, 512.0);

    int mat = int(texture2D(texture, uv).r * 255.0 + 0.5);

    vec3 color = mat == 0 ? vec3(0.0)
               : mat == 1 ? vec3(1.0, 0.0, 1.0)
               : mat == 2 ? vec3(1.0, 1.0, 0.0)
               : mat == 3 ? vec3(0.0, 0.0, 1.0)
               : vec3(1.0);

    gl_FragColor = vec4(color, 1.0);
}
