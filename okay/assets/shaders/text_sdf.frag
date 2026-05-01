#version 300 es

precision highp float;

in vec2 v_uv;

out vec4 FragColor;

uniform sampler2D u_albedo;
uniform vec3 u_color;

void main() {
    float distance = texture(u_albedo, v_uv).a;
    float smoothing_factor = fwidth(distance);
    float alpha = smoothstep(0.5 - smoothing_factor, 0.5 + smoothing_factor, distance);

    if (alpha < 0.01) {
        discard;
    }
    
    FragColor = vec4(u_color, alpha);
}
