#version 100

uniform sampler2D texture;
uniform sampler2D palette;
uniform int palette_size;
uniform ivec2 world_size;
precision mediump float;
varying vec2 uv;

// FIXME: the following functions are duplicated and must be kept in sync with shader.frag
int getParticleMaterial(vec2 uv) {
    return int(texture2D(texture, uv).r * 255.0 + 0.5);
}

vec4 getMaterialColor(int mat) {
    return texture2D(palette, vec2((float(mat) + 0.5) / float(palette_size), 0.5));
}

// no built-in min/max for integers in GLSL ES 1.0 apparently
int int_max(int a, int b) {
    return a > b ? a : b;
}

int int_min(int a, int b) {
    return a < b ? a : b;
}

ivec2 ivec2_max(ivec2 a, ivec2 b) {
    return ivec2(int_max(a.x, b.x), int_max(a.y, b.y));
}

ivec2 ivec2_min(ivec2 a, ivec2 b) {
    return ivec2(int_min(a.x, b.x), int_min(a.y, b.y));
}

#define RANGE 5
#define MAX_ITERS (RANGE + 1 + RANGE)

void main() {
    vec2 world_coords = (uv * vec2(world_size)) - 0.5;

    vec3 light = vec3(0.0);

    ivec2 start = ivec2_max(ivec2(0), ivec2(world_coords) - RANGE);
    ivec2 end = ivec2_min(world_size - 1, ivec2(world_coords) + RANGE);

    // GLSL ES 1.0 loops are limited (see Appendix A of the spec), and WebGL is insanely pedantic about that
    int y = start.y;
    for (int iter = 0; iter < MAX_ITERS; ++iter) {
        if (!(y <= end.y)) {
            break;
        }

        int x = start.x;
        for (int iter = 0; iter < MAX_ITERS; ++iter) {
            if (!(x <= end.x)) {
                break;
            }

            vec2 light_uv = (vec2(x, y) + 0.5) / vec2(world_size);
            int mat = getParticleMaterial(light_uv);
            if ((mat >= 4 && mat <= 6) || mat == 1) {
                float r = 2.0;
                float a = world_coords.x - float(x);
                float b = world_coords.y - float(y);
                float dist = a*a + b*b;

                light += getMaterialColor(mat).rgb * 0.25 / max(r, dist - r);
            }

            ++x;
        }

        ++y;
    }

    gl_FragColor = vec4(light, 1.0);
}
