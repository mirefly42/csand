#version 100

uniform sampler2D texture;
uniform sampler2D palette;
uniform sampler2D glow;
uniform int palette_size;
uniform int glow_mode;
precision mediump float;
varying vec2 uv;

// FIXME: the following functions are duplicated and must be kept in sync with glow.frag
int getParticleMaterial(vec2 uv) {
    return int(texture2D(texture, uv).r * 255.0 + 0.5);
}

vec4 getMaterialColor(int mat) {
    return texture2D(palette, vec2((float(mat) + 0.5) / float(palette_size), 0.5));
}

vec4 getParticleColor(vec2 uv) {
    return getMaterialColor(getParticleMaterial(uv));
}

void main(void) {
    vec4 color = getParticleColor(uv);
    vec3 glow_color = texture2D(glow, uv).rgb;
    gl_FragColor = vec4(mix(glow_color, color.rgb * (1.0 + glow_color), color.a), 1.0);
}
