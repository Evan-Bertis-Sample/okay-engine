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
uniform int u_thin;

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

vec3 calcTint() {
    float luminance = dot(vec3(0.2126f, 0.7152f, 0.0722f), v_color); // 0.2126f, 0.7152f, 0.0722f
    return (luminance > 0.0) ? v_color * (1.0 / luminance) : vec3(-1.0);
}

// Fresnel equations
float schlickWeight(float u) {
    float m = clamp(1.0f - u, 0.0f, 1.0f);
    float m2 = m * m;
    return m * m2 * m2;
}

float schlickFresnel(float r0, float radians) {
    return mix(1.0f, schlickWeight(radians), r0);
}

vec3 schlickFresnel(vec3 r0, float radians) {
    return r0 + (vec3(1.0f) - r0) * pow(1.0f - radians, 5.0f);
}

float schlickR0FromRelativeIOR(float eta) {
    return pow(eta - 1.0f, 2.0f) / pow(eta + 1.0f, 2.0f);
}

float schlickDielectric(float cosThetaI, float relativeIor) {
    float r0 = schlickR0FromRelativeIOR(relativeIor);
    return r0 + (1.0f - r0) * schlickWeight(cosThetaI);
}

float dielectric(float cosThetaI, float etaI, float etaT) {
    cosThetaI = clamp(cosThetaI, -1.0f, 1.0f);

    if (cosThetaI <= 0.0f) {
        float temp = etaI;
        etaI = etaT;
        etaT = temp;
        cosThetaI = abs(cosThetaI);
    }

    float sinThetaI = sqrt(max(0.0f, 1.0f - cosThetaI * cosThetaI));
    float sinThetaT = etaI / etaT * sinThetaI;

    if (sinThetaT >= 1.0f) return 1.0f;
    float cosThetaT = sqrt(max(0.0f, 1.0f - sinThetaT * sinThetaT));
    float rParallel =       ((etaT * cosThetaI) - (etaI * cosThetaT)) /
                            ((etaT * cosThetaI) + (etaI * cosThetaT));
    float rPerpendicular =  ((etaI * cosThetaI) - (etaT * cosThetaT)) /
                            ((etaI * cosThetaI) + (etaT * cosThetaT));

    return (rParallel * rParallel + rPerpendicular * rPerpendicular) / 2.0f;
    
}

vec3 disneyFresnel(vec3 N, vec3 L, vec3 H, vec3 V) {
    float dotHV = abs(dot(H, V));

    vec3 tint = calcTint();
    float ior = (2.0f / (1.0f - sqrt(0.08f * u_specular))) - 1.0f;
    float relativeIOR = dot(N, L) > 0.0f ? ior : 1.0f / ior;
    
    vec3 R0 = schlickR0FromRelativeIOR(relativeIOR) * mix(vec3(1.0f), tint, u_specularTint);
    R0 = mix(R0, v_color, u_metallic);

    float dielectricFresnel = dielectric(dotHV, 1.0f, ior);
    vec3 metallicFresnel = schlickFresnel(R0, dotHV);

    return mix(vec3(dielectricFresnel), metallicFresnel, u_metallic);
}

void calcAnisotropicParams(float roughness, float anisotropic, out float ax, out float ay) {
    float aspect = sqrt(1.0f - 0.9f * anisotropic);
    ax = max(0.001f, roughness * roughness / aspect);
    ay = max(0.001f, roughness * roughness * aspect);
}

float GTR1(float absDotHL, float a) {
    if (a >= 1.0) return 1.0f / PI;

    float a2 = a * a;
    return (a2 - 1.0) / (PI * log2(a2) * (1.0 + (a2 - 1.0) * absDotHL * absDotHL));
}

float GTR2(vec3 N, vec3 H, float ax, float ay) {
    vec3 tangent = v_tangent;
    vec3 bitangent = v_bitangent;
    float dotHX2 = dot(H, tangent) * dot(H, tangent);
    float dotHY2 = dot(H, bitangent) * dot(H, bitangent);
    float cos2Theta = pow(max(dot(N, H), 0.0f), 2.0f);
    float ax2 = ax * ax;
    float ay2 = ay * ay;

    return 1.0f / (PI * ax * ay * pow(dotHX2 / ax2 + dotHY2 / ay2 + cos2Theta, 2.0f));
}

float GGXG1(vec3 N, vec3 W, float a) {
    float a2 = a * a;
    float absDotNW = abs(dot(N, W));

    return 2.0f / (1.0f + sqrt(a2 + (1.0 - a2) * absDotNW * absDotNW));
}

float GGXG1(vec3 W, vec3 H, vec3 N, float ax, float ay) {
    float cosTheta = dot(W, N);
    float sinTheta = sqrt(max(0.0f, 1.0f - cosTheta * cosTheta));
    float absTanTheta = abs(sinTheta / cosTheta);
    if (isinf(absTanTheta)) return 0.0f;

    vec3 tangent = v_tangent;
    vec3 bitangent = v_bitangent;

    float denom = max(1e-8f, 1.0f - dot(W, N) * dot(W, N));

    float cos2Phi = dot(W, tangent) * dot(W, tangent) / denom;
    float sin2Phi = dot(W, bitangent) * dot(W, bitangent) / denom;
    
    float a = sqrt(cos2Phi * ax * ax + sin2Phi * ay * ay);
    float a2Tan2Theta = pow(a * absTanTheta, 2.0f);

    float lambda = 0.5f * (-1.0f + sqrt(1.0 + a2Tan2Theta));
    return 1.0f / (1.0f + lambda);
}

// sheen

vec3 calcSheen(vec3 L, vec3 H) {
    if (u_sheen <= 0.0f) return vec3(0.0f);

    float dotHL = dot(H, L);
    vec3 tint = calcTint();
    vec3 sheenColor = mix(vec3(1.0), tint, u_sheenTint);
    return u_sheen * sheenColor * schlickWeight(dotHL);
}

// clearcoat

float disneyClearcoat(vec3 N, vec3 V, vec3 H, vec3 L) {
    if (u_clearcoat <= 0.0f) return 0.0f;

    float absDotNH = abs(dot(N, H));
    float dotHV = dot(H, V);

    float clearcoatGloss = u_clearcoatGloss;

    float d = GTR1(absDotNH, mix(0.1, 0.001, clearcoatGloss));
    float f = schlickFresnel(0.04f, dotHV);
    float gl = GGXG1(N, L, 0.25f);
    float gv = GGXG1(N, V, 0.25f);

    float absDotNL = abs(dot(N, L));
    float absDotNV = abs(dot(N, V));

    return 0.25f * u_clearcoat * d * f * gl * gv;
}


// Specular BRDF



vec3 calcDisneySpecTransmission(vec3 N, vec3 L, vec3 V, vec3 H, float ax, float ay, bool thin);

vec3 calcDisneyBRDF(vec3 N, vec3 V, vec3 H, vec3 L) {
    float dotNL = dot(N, L);
    float dotNV = dot(N, V);
    if (dotNL <= 0.0f || dotNV <= 0.0f) return vec3(0.0f);

    float ax, ay;
    calcAnisotropicParams(u_roughness, u_anisotropic, ax, ay);

    float d = GTR2(N, H, ax, ay);
    float gl = GGXG1(L, H, N, ax, ay);
    float gv = GGXG1(V, H, N, ax, ay);

    vec3 f = disneyFresnel(N, L, H, V);

    return d * gl * gv * f / (4.0f * dotNL * dotNV);
}

// Specular BSDF

float thinTransmissionRoughness(float ior) {
    return clamp((0.65f * ior - 0.35f) * u_roughness, 0.0f, 1.0f);
}

vec3 calcDisneySpecTransmission(vec3 N, vec3 L, vec3 V, vec3 H, float ax, float ay, int thin) {    
    float ior = (2.0f / (1.0f - sqrt(0.08f * u_specular))) - 1.0f;

    float relativeIOR = dot(N, L) > 0.0f ? ior : 1.0f / ior;
    float n2 = relativeIOR * relativeIOR;

    float absDotNL = abs(dot(N, L));
    float absDotNV = abs(dot(N, V));
    float dotHL = dot(H, L);
    float dotHV = dot(H, V);
    float absDotHL = abs(dotHL);
    float absDotHV = abs(dotHV);

    float d = GTR2(N, H, ax, ay);
    float gl = GGXG1(L, H, N, ax, ay);
    float gv = GGXG1(V, H, N, ax, ay);

    float f = dielectric(dotHV, 1.0f, 1.0f / relativeIOR);

    vec3 color = thin > 0 ? sqrt(v_color) : v_color;
    float c = (absDotHL * absDotHV) / max(1e-8f, absDotNL * absDotNV);

    float t = n2 / pow(dotHL + relativeIOR * dotHV, 2.0f);

    return color * c * t * (1.0f - f) * gl * gv * d;

}

float retroDiffuse(vec3 N, vec3 L, vec3 H, vec3 V) {
    float dotNL = abs(dot(N, L));
    float dotNV = abs(dot(N, V));

    float roughness = u_roughness * u_roughness;

    float rr = 0.5f + 2.0f * dotNL * dotNL * roughness;
    float fl = schlickWeight(dotNL);
    float fv = schlickWeight(dotNV);
    return rr * (fl + fv + fl * fv * (rr - 1.0f));
}

float calcDisneyDiffuse(vec3 N, vec3 L, vec3 H, vec3 V, int thin) {
    float dotNL = abs(dot(N, L));
    float dotNV = abs(dot(N, V));

    float fl = schlickWeight(dotNL);
    float fv = schlickWeight(dotNV);

    float hanrahanKrueger = 0.0f;
    
    if (thin > 0 && u_flatness > 0.0f) {
        float roughness = u_roughness * u_roughness;

        float dotHL = dot(H, L);
        float fss90 = dotHL * dotHL * roughness;
        float fss = mix(1.0f, fss90, fl) * mix(1.0f, fss90, fv);

        float ss = 1.25f * (fss * (1.0f / (dotNL + dotNV) - 0.5f) + 0.5f);
        hanrahanKrueger = ss;
    }

    float lambert = 1.0f;
    float retro = retroDiffuse(N, L, H, V);
    float subsurfaceApprox = mix(lambert, hanrahanKrueger, thin > 0 ? u_flatness : 0.0f);
    return 1.0f / PI * (retro + subsurfaceApprox * (1.0f - 0.5f * fl) * (1.0f - 0.5f * fv));

}

vec3 evaluateDisney(vec3 N, vec3 L, vec3 H, vec3 V, int thin) {
    vec3 wo = normalize(V * v_worldToTangent);
    vec3 wi = normalize(L * v_worldToTangent);
    vec3 wm = normalize(wo + wi);
    vec3 n = normalize(N * v_worldToTangent);
    
    float dotNV = dot(N, V);
    float dotNL = dot(N, L);

    vec3 reflectance = vec3(0.0f);

    float metallic = u_metallic;
    float specularTrans = u_specularTrans;

    float ax, ay;
    calcAnisotropicParams(u_roughness, u_anisotropic, ax, ay);

    float diffuseWeight = (1.0f - metallic) * (1.0f - specularTrans);
    float transWeight = (1.0f - metallic) * specularTrans;

    // clearcoat
    bool upperHemisphere = dotNL > 0.0f && dotNV > 0.0f;
    if (upperHemisphere && u_clearcoat > 0.0f) {
        float forwardClearcoatPdfW;
        float reverseClearcoatPdfW;

        float clearcoat = disneyClearcoat(N, V, H, L);
        reflectance += vec3(clearcoat);
    }


    // diffuse
    if (upperHemisphere && diffuseWeight > 0.0f) {
        float diffuse = calcDisneyDiffuse(N, L, H, V, thin);
        vec3 sheen = calcSheen(L, H);

        reflectance += diffuseWeight * (diffuse * v_color + sheen);
    }

    // transmission
    if (transWeight > 0.0f) {
        float ior = (2.0f / (1.0f - sqrt(0.08f * u_specular))) - 1.0f;
        float rscaled = thin > 0 ? thinTransmissionRoughness(ior) : u_roughness;
        float tax, tay;
        calcAnisotropicParams(rscaled, u_anisotropic, tax, tay);

        vec3 transmission = calcDisneySpecTransmission(N, L, V, H, tax, tay, thin);
        reflectance += transWeight * transmission;
    }

    // specular
    if (upperHemisphere) {
        vec3 specular = calcDisneyBRDF(N, V, H, L);
        reflectance += specular;
    }

    reflectance = reflectance * abs(dotNL);

    return reflectance;
}


void main() {
    vec3 N = safeNormalize(v_worldNormal); // normal vector
    vec3 V = safeNormalize(u_cameraPosition - v_worldPos); // view vector

    vec4 texAlbedo = texture(u_albedo, v_uv);
    vec3 albedo = clamp(texAlbedo.rgb * v_color, vec3(0.0), vec3(1.0));
    vec3 colorOut = u_ambient * albedo;
    // vec3 colorOut;

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

        colorOut += evaluateDisney(N, L, H, V, u_thin) * Lrgb * intensity * att;
    }

    FragColor = vec4(colorOut, texAlbedo.a);
}
