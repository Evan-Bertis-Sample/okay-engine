#version 300 es
precision highp float;
precision highp int;

in vec3 v_color;
in vec3 v_worldPos;
in vec3 v_worldNormal;
in vec2 v_uv;
in vec3 v_tangent;
in vec3 v_bitangent;
in mat3 v_worldToTangent;

out vec4 FragColor;

/* Camera */
uniform vec3 u_cameraPosition;

/* Material */
uniform float u_ambient;     // e.g. 0.05
uniform sampler2D u_albedo;  // optional, can be ignored if not used

// Disney BRDF
uniform float u_subsurface; // controls diffuse shape with subsurface approx
uniform float u_metallic; // 0 = dielectric, 1 = metallic
uniform float u_specular; // incident specular amount (0 = matte, 1 = mirror-like)
uniform float u_specularTint; // tints incident specular towards base color
uniform float u_roughness; // surface roughness
uniform float u_anisotropic; // controls the aspect ratio of the specular highlight (0 = isotropic, 1 = max anisotropic)
uniform float u_sheen; // additional grazing component primary used for cloth
uniform float u_sheenTint; // amount of tint sheen towards base color
uniform float u_clearcoat; // another specular lobe
uniform float u_clearcoatGloss; // controls clearcoat glossiness (0 = satin, 1 = gloss)

// Disney BSDF
uniform float u_specularTrans;
uniform float u_flatness;

const float PI = 3.1415926535897932384626433832795;


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

float square(float a) { return a * a; }

float SchlickFresnel(float u)
{
    float m = clamp(1.0f - u, 0.0f, 1.0f);
    float m2 = m*m;
    return m2*m2*m; // pow(m,5)
}

float GTR1(float NdotH, float a)
{
    if (a >= 1.0f) return 1.0f / PI;
    float a2 = a*a;
    float t = 1.0f + (a2 - 1.0f) * NdotH * NdotH;
    return (a2 - 1.0f) / (PI * log(a2) * t);
}

float GTR2(float NdotH, float a)
{
    float a2 = a*a;
    float t = 1.0f + (a2 - 1.0f) * NdotH * NdotH;
    return a2 / (PI * t * t);
}

float GTR2_aniso(float NdotH, float HdotX, float HdotY, float ax, float ay)
{
    return 1.0f / (PI * ax * ay * square(square(HdotX / ax) + square(HdotY / ay) + NdotH * NdotH ));
}

float smithG_GGX(float NdotV, float alphaG)
{
    float a = alphaG * alphaG;
    float b = NdotV * NdotV;
    return 1.0f / (NdotV + sqrt(a + b - a * b));
}

float smithG_GGX_aniso(float NdotV, float VdotX, float VdotY, float ax, float ay)
{
    return 1.0f / (NdotV + sqrt( square(VdotX * ax) + square(VdotY * ay) + square(NdotV) ));
}

vec3 mon2lin(vec3 x)
{
    return vec3(pow(x[0], 2.2), pow(x[1], 2.2), pow(x[2], 2.2));
}


vec3 BRDF( vec3 L, vec3 V, vec3 N, vec3 X, vec3 Y )
{
    float NdotL = dot(N,L);
    float NdotV = dot(N,V);
    if (NdotL < 0.0f || NdotV < 0.0f) return vec3(0.0f);

    vec3 H = safeNormalize(L + V);
    float NdotH = dot(N,H);
    float LdotH = dot(L,H);

    vec3 Cdlin = mon2lin(v_color);
    float Cdlum = .3*Cdlin[0] + .6*Cdlin[1]  + .1*Cdlin[2]; // luminance approx.

    vec3 Ctint = Cdlum > 0.0f ? Cdlin/Cdlum : vec3(1.0f); // normalize lum. to isolate hue+sat
    vec3 Cspec0 = mix(u_specular * .08f * mix(vec3(1.0f), Ctint, u_specularTint), Cdlin, u_metallic);
    vec3 Csheen = mix(vec3(1.0f), Ctint, u_sheenTint);

    // Diffuse fresnel - go from 1 at normal incidence to .5 at grazing
    // and mix in diffuse retro-reflection based on roughness
    float FL = SchlickFresnel(NdotL), FV = SchlickFresnel(NdotV);
    float Fd90 = 0.5f + 2.0f * LdotH * LdotH * u_roughness;
    float Fd = mix(1.0f, Fd90, FL) * mix(1.0f, Fd90, FV);

    // Based on Hanrahan-Krueger brdf approximation of isotropic bssrdf
    // 1.25 scale is used to (roughly) preserve albedo
    // Fss90 used to "flatten" retroreflection based on roughness
    float Fss90 = LdotH * LdotH * u_roughness;
    float Fss = mix(1.0f, Fss90, FL) * mix(1.0f, Fss90, FV);
    float ss = 1.25f * (Fss * (1.0f / (NdotL + NdotV) - 0.5f) + 0.5f);

    // specular
    float aspect = sqrt(1.0f - u_anisotropic * 0.9f);
    float ax = max(0.001f, square(u_roughness) / aspect);
    float ay = max(0.001f, square(u_roughness) * aspect);
    float Ds = GTR2_aniso(NdotH, dot(H, X), dot(H, Y), ax, ay);
    float FH = SchlickFresnel(LdotH);
    vec3 Fs = mix(Cspec0, vec3(1), FH);
    float Gs;
    Gs  = smithG_GGX_aniso(NdotL, dot(L, X), dot(L, Y), ax, ay);
    Gs *= smithG_GGX_aniso(NdotV, dot(V, X), dot(V, Y), ax, ay);

    // sheen
    vec3 Fsheen = FH * u_sheen * Csheen;

    // clearcoat (ior = 1.5 -> F0 = 0.04)
    float Dr = GTR1(NdotH, mix(0.1f, 0.001f, u_clearcoatGloss));
    float Fr = mix(0.04f, 1.0f, FH);
    float Gr = smithG_GGX(NdotL, 0.25f) * smithG_GGX(NdotV, 0.25f);

    return ((1.0f / PI) * mix(Fd, ss, u_subsurface) * Cdlin + Fsheen)
        * (1.0f - u_metallic)
        + Gs*Fs*Ds + 0.25f * u_clearcoat *Gr*Fr*Dr;
}

void main() {
    vec3 N = safeNormalize(v_worldNormal); // normal vector
    vec3 V = safeNormalize(u_cameraPosition - v_worldPos); // view vector

    vec4 texAlbedo = texture(u_albedo, v_uv);
    vec3 albedo = clamp(texAlbedo.rgb * v_color, vec3(0.0), vec3(1.0));
    // vec3 colorOut = u_ambient * albedo;
    vec3 colorOut;

    int count = int(meta.x + 0.5);
    int maxLights = min(count, 16);

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

        float NdotL = max(dot(N, L), 0.0);
        colorOut += BRDF(L, V, N, v_tangent, v_bitangent) * NdotL * Lrgb * intensity * att;
    }

    FragColor = vec4(colorOut, texAlbedo.a);
}
