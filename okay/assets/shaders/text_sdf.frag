#version 300 es
precision highp float;

in vec2 v_uv;
in vec2 v_clipSpaceUV;

layout(location = 0) out vec4 FragColor;

uniform sampler2D u_albedo;
uniform sampler2D u_clipMask;
uniform vec4 u_color;

void main() {
    float sd = texture(u_albedo, v_uv).a;
    float u_pxRange = 64.0;
    vec2 texSize = vec2(textureSize(u_albedo, 0));
    vec2 unitRange = vec2(u_pxRange) / texSize;
    float screenPxRange = max(0.5 * dot(unitRange, 1.0 / fwidth(v_uv)), 1.0);

    float alpha = clamp((sd - 0.5) * screenPxRange + 0.5, 0.0, 1.0);
    alpha *= u_color.a;

    if (alpha < 0.01) {
        discard;
    }

    FragColor = vec4(u_color.rgb, alpha);
}
