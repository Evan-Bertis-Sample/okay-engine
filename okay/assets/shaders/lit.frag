#version 300 es
precision highp float;
precision highp int;

in vec3 v_color;
in vec3 v_worldPos;
in vec3 v_worldNormal;
in vec2 v_uv;
in vec3 v_tangent;
in vec3 v_bitangent;

out vec4 FragColor;

/* Camera */
uniform vec3 u_cameraPosition;

/* Material */
uniform float u_shininess;   // e.g. 32.0
uniform float u_ambient;     // e.g. 0.05
uniform sampler2D u_albedo;  // optional, can be ignored if not used
uniform float u_antialiasing;
uniform float u_roughness;
uniform float u_sheen;
uniform float u_sheenTint;
uniform float u_anisotropic;
uniform float u_clearcoat;
uniform float u_clearcoatGloss;


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

vec3 celLighting(vec3 N, vec3 L, vec3 Lrgb, vec3 albedo, vec3 V, float shininess, float intensity, float att) {
    float NdotL = max(dot(N, L), 0.0);
    if (NdotL <= 0.0) return vec3(0.0);

    // Diffuse
    float antialiasing = u_antialiasing;
    float delta = fwidth(NdotL) * antialiasing;
    float diffuseSmooth = smoothstep(0.0f, delta, NdotL);
    vec3 diffuse = diffuseSmooth * albedo * Lrgb;

    // specular, phong model
    vec3 R = reflect(-L, N);
    float RdotV = max(dot(R, V), 0.0);

    float specularSmooth = pow(RdotV * diffuseSmooth, shininess);
    specularSmooth = smoothstep(0.0, 0.01 * antialiasing, specularSmooth);
    vec3 specular = specularSmooth * Lrgb * att;

    float rim = 1.0 - dot(N, V);
    rim = rim * NdotL;
    float fresnel = 0.5;
    float fresnelSize = 1.0 - fresnel;
    float rimSmooth = smoothstep(fresnelSize, fresnelSize * 1.1, rim);

    return att * intensity * (diffuse + specular + rimSmooth);
}

vec3 defaultLighting(vec3 N, vec3 L, vec3 Lrgb, vec3 albedo, vec3 V, float shininess, float intensity, float att) {
    float NdotL = max(dot(N, L), 0.0);
    if (NdotL <= 0.0) return vec3(0.0);

    // Diffuse
    vec3 diffuse = NdotL * albedo * Lrgb;

    // specular, phong model
    vec3 R = reflect(-L, N);
    float RdotV = max(dot(R, V), 0.0);
    vec3 specular = pow(RdotV, shininess) * Lrgb;
    specular *= att;

    return att * intensity * (diffuse + specular);
}

// sheen
vec3 calcTint() {
    vec3 baseColor = v_color;
    float luminance = dot(vec3(0.2126, 0.7152, 0.0722), baseColor);
    return (luminance > 0.0) ? baseColor * (1.0 / luminance) : vec3(-1.0);
}

vec3 calcSheen(vec3 N, vec3 V, vec3 L, vec3 H) {
    float NdotL = max(dot(N, L), 0.0);
    if (NdotL <= 0.0) return vec3(0.0);

    float cosTheta = max(dot(H, V), 0.0);
    float sheen = u_sheen;
    float sheenTint = u_sheenTint;
    vec3 tint = calcTint();
    vec3 sheenColor = mix(vec3(1.0), tint, sheenTint);

    return NdotL * sheen * sheenColor * pow(1.0 - cosTheta, 5.0);
}

// clearcoat
float GTR1(vec3 N, vec3 H, float a) {
    const float PI = 3.1415926535897932384626433832795;
    if (a >= 1.0) return 1.0/PI;

    float a2 = a * a;
    float absDotNH = abs(dot(N, H));
    return (a2 - 1.0) / (PI * log2(a2) * (1.0 + (a2 - 1.0) * absDotNH * absDotNH));
}

float schlickFresnel(vec3 V, vec3 H) {
    float F0 = 0.04f; // base reflectance
    float F90 = 1.0f; // max reflectance
    float cosTheta = max(dot(H, V), 0.0); // cosine of angle between view and half vector

    return mix(F0, F90, pow(1.0 - cosTheta, 5.0));
}

float GGXG1(vec3 V, vec3 N, float a) {
    float a2 = a * a;
    float absDotNV = abs(dot(N, V));

    return 2.0f / (1.0f + sqrt(a2 + (1.0 - a2) * absDotNV * absDotNV));
}

float disneyClearcoat(vec3 N, vec3 V, vec3 H, vec3 L) {
    float NdotL = max(dot(N, L), 0.0f);
    if (NdotL <= 0.0) return 0.0f;

    float clearcoat = u_clearcoat;
    if (clearcoat <= 0.0f) return 0.0f;

    float clearcoatGloss = u_clearcoatGloss;

    float d = GTR1(N, H, mix(0.1, 0.001, clearcoatGloss));
    float f = schlickFresnel(V, H);
    float gl = GGXG1(L, N, 0.25f);
    float gv = GGXG1(V, N, 0.25f);

    return NdotL * 0.25f * clearcoat * d * f * gl * gv;
}


// Specular BRDF

float GTR2(vec3 N, vec3 H, float ax, float ay) {
    const float PI = 3.1415926535897932384626433832795;
    vec3 tangent = v_tangent;
    vec3 bitangent = v_bitangent;
    float dotHX2 = dot(H, tangent) * dot(H, tangent);
    float dotHY2 = dot(H, bitangent) * dot(H, bitangent);
    float cos2Theta = pow(max(dot(N, H), 0.0f), 2.0f);
    float ax2 = ax * ax;
    float ay2 = ay * ay;

    return 1.0f / (PI * ax * ay * pow(dotHX2 / ax2 + dotHY2 / ay2 + cos2Theta, 2.0f));
}

float GGXG1(vec3 W, vec3 H, vec3 N, float ax, float ay) {
    float dotHW = dot(H, W);
    if (dotHW <= 0.0f) return 0.0f;

    float absTanTheta = abs(tan(dot(W, N)));
    if (isinf(absTanTheta)) return 0.0f;

    vec3 tangent = v_tangent;
    vec3 bitangent = v_bitangent;

    float cos2Phi = dot(W, tangent) * dot(W, tangent) / (1.0f - dot(W, N) * dot(W, N));
    float sin2Phi = dot(W, bitangent) * dot(W, bitangent) / (1.0f - dot(W, N) * dot(W, N));
    float a = sqrt(cos2Phi * ax * ax + sin2Phi * ay * ay);
    float a2Tan2Theta = pow(a * absTanTheta, 2.0f);

    float lambda = 0.5f * (-1.0f + sqrt(1.0 + a2Tan2Theta));
    return 1.0f / (1.0f + lambda);
}

vec3 calcDisneyBRDF(vec3 N, vec3 V, vec3 H, vec3 L) {
    float dotNL = dot(N, L);
    float dotNV = dot(N, V);
    if (dotNL <= 0.0f || dotNV <= 0.0f) return vec3(0.0f);

    float roughness = u_roughness;
    float anisotropic = u_anisotropic;
    float aspect = sqrt(1.0f - 0.9f * anisotropic);
    float ax = roughness * roughness / aspect;
    float ay = roughness * roughness * aspect;

    float d = GTR2(N, H, ax, ay);
    float gl = GGXG1(L, H, N, ax, ay);
    float gv = GGXG1(V, H, N, ax, ay);

    vec3 f = vec3(1.0); // to be replace with DisneyFresnel

    return dotNL * d * gl * gv * f / (4.0f * dotNL * dotNV);
}

void main() {
    vec3 N = safeNormalize(v_worldNormal); // normal vector
    vec3 V = safeNormalize(u_cameraPosition - v_worldPos); // view vector

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
        float att = 1.0; // attenuation

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
        vec3 H = safeNormalize(V + L); // half vector
        // default

        // diffuse

        // specular
        float NdotL = max(dot(N, L), 0.0);
        if (NdotL <= 0.0) continue;

        // Diffuse
        vec3 diffuse = NdotL * albedo * Lrgb;

        // vec3 R = reflect(-L, N);
        // float RdotV = max(dot(R, V), 0.0);
        // vec3 specular = pow(RdotV, shininess) * Lrgb;
        // specular *= att;

        // vec3 specular = calcDisneyBRDF(N, V, H, L);

        // colorOut += att * intensity * (diffuse + specular);

        colorOut += defaultLighting(N, L, Lrgb, albedo, V, shininess, intensity, att);
        colorOut += calcSheen(N, V, L, H);
        colorOut += disneyClearcoat(N, V, H, L);
        
        // cel lighting
        // colorOut += celLighting(N, L, Lrgb, albedo, V, shininess, intensity, att);
    }

    float viewDep = pow(1.0 - max(dot(N, V), 0.0), 5.0);
    colorOut += viewDep * 0.25;

    FragColor = vec4(colorOut, texAlbedo.a);
}
