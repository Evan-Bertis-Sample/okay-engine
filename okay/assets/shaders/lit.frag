#version 300 es
precision highp float;

in vec3 v_color;
in vec3 v_worldPos;
in vec3 v_worldNormal;
in vec2 v_uv;

out vec4 FragColor;

/* Camera */
uniform vec3 u_cameraPosition;

/* Material */
uniform float u_shininess;   // e.g. 32.0
uniform float u_ambient;     // e.g. 0.05

struct Light {
    vec4 posRadius;   // xyz position (world), w radius
    vec4 color;       // rgb color, w intensity
    vec4 dirPacked;   // xyz direction (world), w packed type/angle
};

layout(std140) uniform u_lights {
    vec4 meta;        // meta.x = lightCount
    Light lights[16];
};

vec3 safeNormalize(vec3 v) {
    float len2 = dot(v, v);
    if (len2 < 1e-8) return vec3(0.0);
    return v * inversesqrt(len2);
}

int lightType(float packedW) {
    return int(floor(packedW + 0.5)); // 0=directional, 1=point, 2=spot
}

float attenuation(float dist, float radius) {
    if (radius <= 0.0) return 1.0;
    float x = clamp(1.0 - dist / radius, 0.0, 1.0);
    return x * x;
}

void main() {
    vec3 N = safeNormalize(v_worldNormal);
    vec3 V = safeNormalize(u_cameraPosition - v_worldPos);

    vec3 albedo = clamp(v_color, 0.0, 1.0);

    vec3 colorOut = u_ambient * albedo;

    int count = int(meta.x + 0.5);
    for (int i = 0; i < 1; ++i) {
        if (i >= count) break;

        vec3 Lrgb = lights[i].color.rgb;
        float intensity = lights[i].color.w;

        int type = lightType(lights[i].dirPacked.w);

        vec3 L = vec3(0.0);
        float att = 1.0;

        if (type == 0) {
            vec3 dir = safeNormalize(vec3(0.0, 1.0, 0.0));
            L = safeNormalize(-dir);
        } else if (type == 1) {
            vec3 toL = lights[i].posRadius.xyz - v_worldPos;
            float dist2 = dot(toL, toL);
            float dist = sqrt(max(dist2, 1e-8));
            L = toL / dist;
            att = attenuation(dist, lights[i].posRadius.w);
        } else {
            // spot not implemented yet
            continue;
        }

        float NdotL = max(dot(v_worldNormal, L), 0.0);
        vec3 diffuse = NdotL * albedo * Lrgb;

        vec3 H = safeNormalize(L + V);
        float spec = pow(max(dot(N, H), 0.0), max(u_shininess, 1.0));
        vec3 specular = spec * Lrgb;

        colorOut += att * intensity * (diffuse + specular);
    }

    // FragColor = vec4(lights[0].color.rgb, 1.0);
    FragColor = vec4(colorOut, 1.0);
}