#version 300 es
precision highp float;
precision highp int;

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
uniform sampler2D u_albedo;  // optional, can be ignored if not used

struct Light {
    vec4 posType;    // xyz = position (POINT/SPOT), w = type (0 dir, 1 point, 2 spot)
    vec4 color;      // rgb = color, w = intensity
    vec4 direction;  // xyz = direction (DIR/SPOT), w unused
    vec4 extra;      // point: x = radius, spot: x = radius, y = angleRad
};

layout(std140) uniform u_lights {
    vec4 meta;       // meta.x = lightCount
    Light lights[16];
};

vec3 safeNormalize(vec3 v) {
    float len2 = dot(v, v);
    if (len2 < 1e-8) return vec3(0.0);
    return v * inversesqrt(len2);
}

// Smooth radius falloff: 1 at dist=0, 0 at dist>=radius
float radiusAttenuation(float dist, float radius) {
    if (radius <= 0.0) return 1.0;
    float x = clamp(1.0 - dist / radius, 0.0, 1.0);
    return x * x;
}

// Returns [0..1] spotlight cone factor (smooth edge)
// L = direction from fragment -> light
// spotDir = direction the light shines (light forward), world-space
float spotConeFactor(vec3 L, vec3 spotDir, float angleRad) {
    // Incoming light direction at fragment is from light -> fragment, which is -L.
    vec3 fromLightToFrag = safeNormalize(-L);
    vec3 d = safeNormalize(spotDir);

    float cosTheta = dot(fromLightToFrag, d);
    float cosCutoff = cos(angleRad);

    // Soft edge band. If you want a fixed softness, change this constant.
    float softBand = max(0.001, (1.0 - cosCutoff) * 0.10);
    return smoothstep(cosCutoff, cosCutoff + softBand, cosTheta);
}

void main() {
    vec3 N = safeNormalize(v_worldNormal);
    vec3 V = safeNormalize(u_cameraPosition - v_worldPos);

    vec4 texAlbedo = texture(u_albedo, v_uv);
    vec3 albedo = clamp(texAlbedo.rgb * v_color, vec3(0.0), vec3(1.0));
    vec3 colorOut = u_ambient * albedo;

    int count = int(meta.x + 0.5);
    int maxLights = min(count, 16);

    float shininess = max(u_shininess, 1.0);

    for (int i = 0; i < 16; ++i) {
        if (i >= maxLights) break;

        vec3 Lrgb = lights[i].color.rgb;
        float intensity = lights[i].color.w;

        int type = int(floor(lights[i].posType.w + 0.5)); // 0=dir, 1=point, 2=spot

        vec3 L = vec3(0.0); // fragment -> light
        float att = 1.0;

        if (type == 0) {
            // Directional: direction.xyz is the direction light shines (world)
            vec3 dir = safeNormalize(lights[i].direction.xyz);
            // Fragment -> light is opposite incoming direction
            L = safeNormalize(-dir);
        } else if (type == 1) {
            // Point
            vec3 toL = lights[i].posType.xyz - v_worldPos;
            float dist2 = dot(toL, toL);
            float dist = sqrt(max(dist2, 1e-8));
            L = toL / dist;

            float radius = lights[i].extra.x;
            att = radiusAttenuation(dist, radius);
        } else if (type == 2) {
            // Spot
            vec3 toL = lights[i].posType.xyz - v_worldPos;
            float dist2 = dot(toL, toL);
            float dist = sqrt(max(dist2, 1e-8));
            L = toL / dist;

            float radius = lights[i].extra.x;
            float angleRad = lights[i].extra.y;

            float cone = spotConeFactor(L, lights[i].direction.xyz, angleRad);
            att = radiusAttenuation(dist, radius) * cone;

            // If cone factor is 0, skip work
            if (att <= 0.0) continue;
        } else {
            continue;
        }

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL <= 0.0) continue;

        // Diffuse
        vec3 diffuse = NdotL * albedo * Lrgb;

        // specular, phong model
        vec3 R = reflect(-L, N);
        float RdotV = max(dot(R, V), 0.0);
        vec3 specular = pow(RdotV, shininess) * Lrgb;
        specular *= att;

        colorOut += att * intensity * (diffuse + specular);
    }

    float viewDep = pow(1.0 - max(dot(N, V), 0.0), 5.0);
    colorOut += viewDep * 0.25;

    FragColor = vec4(colorOut, texAlbedo.a);
}