#version 300 es
precision highp float;
precision highp int;

in vec3 v_color;
in vec3 v_worldPos;
in vec3 v_worldNormal;
in vec2 v_uv;
in mat3 v_worldToTangent;

out vec4 FragColor;

/* Camera */
uniform vec3 u_cameraPosition;

/* Material */
uniform float u_ambient;     // e.g. 0.05
uniform sampler2D u_albedo;  // optional, can be ignored if not used

/* Disney BSDF Parameters */
// All params are in [0, 1]

uniform float u_metallic;       // 0 = dielectric, 1 = metallic
uniform float u_specular;       // incident specular amount; remapped to IOR (0.5 -> IOR 1.5)
uniform float u_specularTint;   // tints the dielectric F0 toward baseColor's hue
uniform float u_roughness;      // surface roughness
uniform float u_anisotropic;    // highlight stretch along tangent axes (0 = isotropic)
uniform float u_sheen;          // grazing-angle component, primarily for cloth
uniform float u_sheenTint;      // blends sheen color from white (0) toward baseColor (1)
uniform float u_clearcoat;      // second specular layer (glossy, varnish look)
uniform float u_clearcoatGloss; // controls clearcoat glossiness (0 = satin, 1 = mirror)
uniform float u_specularTrans;  // transmission fraction (glass/translucency)
uniform float u_flatness;       // blends in Hanrahan-Krueger subsurface for thin surfaces
uniform int u_thin;             // 0 = solid, 1 = thin surface (leaves, paper)

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

/***************/
/** Utilities **/
/***************/

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

// Normalizes baseColor to unit luminance, gives hue without brightness
vec3 calcTint(vec3 baseColor) {
    // ITU-R BT.709 / sRGB luminance coefficients
    float luminance = dot(vec3(0.2126f, 0.7152f, 0.0722f), baseColor);
    return (luminance > 0.0) ? baseColor * (1.0 / luminance) : vec3(1.0);
}

// moniter (sRGB space) to linear light
vec3 mon2lin(vec3 x) {
    return vec3(pow(x.x, 2.2), pow(x.y, 2.2), pow(x.z, 2.2));
}


/*************/
/** Fresnel **/
/*************/

float schlickWeight(float u) {
    float m = clamp(1.0f - u, 0.0f, 1.0f);
    float m2 = m * m;
    return m * m2 * m2;
}

// Schlick's approximation (scalar): F0 + (1 - F0) * (1 - cosTheta)^5.
float schlickFresnel(float r0, float radians) {
    return mix(r0, 1.0f, schlickWeight(radians));
}

// Schlick's approximation (vec3): used for colored F0 (metals).
vec3 schlickFresnel(vec3 r0, float radians) {
    return r0 + (vec3(1.0f) - r0) * pow(1.0f - radians, 5.0f);
}

// F0 at normal incidence from an IOR ratio. IOR 1.5 -> F0 = 0.04.
float schlickR0FromRelativeIOR(float eta) {
    return pow(eta - 1.0f, 2.0f) / pow(eta + 1.0f, 2.0f);
}

// Full dielectric Fresnel equations. Handles total internal reflection and
// light coming from either side of the surface.
float dielectric(float cosThetaI, float etaI, float etaT) {
    cosThetaI = clamp(cosThetaI, -1.0f, 1.0f);

    // Light coming from the other side: swap IORs and use |cosThetaI|.
    if (cosThetaI <= 0.0f) {
        float temp = etaI;
        etaI = etaT;
        etaT = temp;
        cosThetaI = abs(cosThetaI);
    }

    // Snell's law: sin(thetaT) = (etaI / etaT) * sin(thetaI).
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

// dielectric Fresnel and colored metallic Fresnel based on u_metallic.
vec3 disneyFresnel(vec3 baseColor, vec3 N, vec3 wi, vec3 wm, vec3 wo) {
    float dotHV = abs(dot(wm, wo));

    vec3 tint = calcTint(baseColor);
    // IOR remap: u_specular = 0.5 -> IOR 1.5
    float ior = (2.0f / (1.0f - sqrt(0.08f * u_specular))) - 1.0f;
    float relativeIOR = dot(N, wi) > 0.0f ? ior : 1.0f / ior;
    
    vec3 R0 = schlickR0FromRelativeIOR(relativeIOR) * mix(vec3(1.0f), tint, u_specularTint);
    R0 = mix(R0, baseColor, u_metallic);

    float dielectricFresnel = dielectric(dotHV, 1.0f, ior);
    vec3 metallicFresnel = schlickFresnel(R0, dotHV);

    return mix(vec3(dielectricFresnel), metallicFresnel, u_metallic);
}

/***************************************************/
/** Microfacet distributions and masking-shadowing */
/***************************************************/

void calcAnisotropicParams(float roughness, float anisotropic, out float ax, out float ay) {
    float aspect = sqrt(1.0f - 0.9f * anisotropic);
    ax = max(0.001f, roughness * roughness / aspect);
    ay = max(0.001f, roughness * roughness * aspect);
}

// Generalized Trowbridge-Reitz
float GTR1(float absDotHL, float a) {
    if (a >= 1.0) return 1.0f / PI;

    float a2 = a * a;
    return (a2 - 1.0) / (PI * log2(a2) * (1.0 + (a2 - 1.0) * absDotHL * absDotHL));
}

// Anisotropic GGX (GTR2). Returns the
// density of microfacets oriented along wm.
float GTR2(vec3 N, vec3 wm, float ax, float ay) {
    float dotHX2 = wm.x * wm.x;
    float dotHY2 = wm.z * wm.z;
    float cos2Theta = pow(max(dot(N, wm), 0.0f), 2.0f);
    float ax2 = ax * ax;
    float ay2 = ay * ay;

    return 1.0f / (PI * ax * ay * pow(dotHX2 / ax2 + dotHY2 / ay2 + cos2Theta, 2.0f));
}

// Smith G1 for isotropic GGX. Returns [0, 1]: fraction of microfacets visible
// from direction w (masking-shadowing).
float GGXG1(vec3 w, float a) {
    float a2 = a * a;
    float absDotNW = w.y;

    return 2.0f / (1.0f + sqrt(a2 + (1.0 - a2) * absDotNW * absDotNW));
}

// Smith G1 for anisotropic GGX. Projects w onto tangent axes to get an
// effective roughness, then computes masking-shadowing.
float GGXG1(vec3 w, vec3 N, float ax, float ay) {
    float cosTheta = dot(w, N);
    float sinTheta = sqrt(max(0.0f, 1.0f - cosTheta * cosTheta));
    float absTanTheta = abs(sinTheta / cosTheta);
    if (isinf(absTanTheta)) return 0.0f;

    float denom = max(1e-8f, 1.0f - w.y * w.y);

    float cos2Phi = w.x * w.x / denom;
    float sin2Phi = w.z * w.z / denom;
    
    float a = sqrt(cos2Phi * ax * ax + sin2Phi * ay * ay);
    float a2Tan2Theta = pow(a * absTanTheta, 2.0f);

    float lambda = 0.5f * (-1.0f + sqrt(1.0 + a2Tan2Theta));
    return 1.0f / (1.0f + lambda);
}

/***********/
/** Sheen **/
/***********/

// Grazing-angle boost tinted toward baseColor.
vec3 calcSheen(vec3 baseColor, vec3 wi, vec3 wm) {
    if (u_sheen <= 0.0f) return vec3(0.0f);

    float dotHL = dot(wm, wi);
    vec3 tint = calcTint(baseColor);
    vec3 sheenColor = mix(vec3(1.0), tint, u_sheenTint);
    return u_sheen * sheenColor * schlickWeight(dotHL);
}

/*************/
/** Diffuse **/
/*************/

// Causes a bright silhouette outline on rough materials.
float retroDiffuse(vec3 N, vec3 wi, vec3 wm, vec3 wo) {
    float dotNL = abs(dot(N, wi));
    float dotNV = abs(dot(N, wo));
    float dotHL = abs(dot(wm, wi));

    float roughness = u_roughness * u_roughness;

    float rr = 0.5f + 2.0f * dotHL * dotHL * roughness;
    float fl = schlickWeight(dotNL);
    float fv = schlickWeight(dotNV);
    return rr * (fl + fv + fl * fv * (rr - 1.0f));
}

// Lambert + retro-reflection + optional Hanrahan-Krueger
// subsurface approximation for thin materials.
float calcDisneyDiffuse(vec3 N, vec3 wi, vec3 wm, vec3 wo) {
    float dotNL = abs(dot(N, wi));
    float dotNV = abs(dot(N, wo));

    float fl = schlickWeight(dotNL);
    float fv = schlickWeight(dotNV);

    float hanrahanKrueger = 0.0f;

    if (u_thin > 0 && u_flatness > 0.0f) {
        float roughness = u_roughness * u_roughness;

        float dotHL = dot(wm, wi);
        float fss90 = dotHL * dotHL * roughness;
        float fss = mix(1.0f, fss90, fl) * mix(1.0f, fss90, fv);

        float ss = 1.25f * (fss * (1.0f / (dotNL + dotNV) - 0.5f) + 0.5f);
        hanrahanKrueger = ss;
    }

    float lambert = 1.0f;
    float retro = retroDiffuse(N, wi, wm, wo);
    float subsurfaceApprox = mix(lambert, hanrahanKrueger, u_thin > 0 ? u_flatness : 0.0f);
    return 1.0f / PI * (retro + subsurfaceApprox * (1.0f - 0.5f * fl) * (1.0f - 0.5f * fv));

}

/********************************/
/** Specular BRDF (reflection) **/
/********************************/

// The main reflection lobe. Standard microfacet BRDF:
//      f_r = (D * G * F) / (4 * N.L * N.V)
// D = GGX distribution, G = Smith masking-shadowing, F = Disney Fresnel.
vec3 calcDisneyBRDF(vec3 baseColor, vec3 N, vec3 wo, vec3 wm, vec3 wi) {
    float dotNL = dot(N, wi);
    float dotNV = dot(N, wo);
    if (dotNL <= 0.0f || dotNV <= 0.0f) return vec3(0.0f);

    float ax, ay;
    calcAnisotropicParams(u_roughness, u_anisotropic, ax, ay);

    float d = GTR2(N, wm, ax, ay);
    float gl = GGXG1(wi, N, ax, ay);
    float gv = GGXG1(wo, N, ax, ay);

    vec3 f = disneyFresnel(baseColor, N, wi, wm, wo);

    return d * gl * gv * f / (4.0f * dotNL * dotNV);
}

/**********************************/
/** Specular BTDF (transmission) **/
/**********************************/

// Remaps roughness for thin-surface transmission based on IOR.
float thinTransmissionRoughness(float ior) {
    return clamp((0.65f * ior - 0.35f) * u_roughness, 0.0f, 1.0f);
}

// Specular transmission lobe
vec3 calcDisneySpecTransmission(vec3 baseColor, vec3 N, vec3 wi, vec3 wo, vec3 wm, float ax, float ay) {    
    float ior = (2.0f / (1.0f - sqrt(0.08f * u_specular))) - 1.0f;

    // Inverts ior based on the side it is coming from
    float relativeIOR = dot(N, wi) > 0.0f ? ior : 1.0f / ior;
    float n2 = relativeIOR * relativeIOR;

    float absDotNL = abs(dot(N, wi));
    float absDotNV = abs(dot(N, wo));
    float dotHL = dot(wm, wi);
    float dotHV = dot(wm, wo);
    float absDotHL = abs(dotHL);
    float absDotHV = abs(dotHV);

    float d = GTR2(N, wm, ax, ay);
    float gl = GGXG1(wi, N, ax, ay);
    float gv = GGXG1(wo, N, ax, ay);

    float f = dielectric(dotHV, 1.0f, 1.0f / relativeIOR);

    vec3 color = u_thin > 0 ? sqrt(baseColor) : baseColor;
    float c = (absDotHL * absDotHV) / max(1e-8f, absDotNL * absDotNV);

    float t = n2 / pow(dotHL + relativeIOR * dotHV, 2.0f);

    return color * c * t * (1.0f - f) * gl * gv * d;

}

/***************/
/** Clearcoat **/
/***************/

// Second specular layer on top of the base material
float disneyClearcoat(vec3 N, vec3 wo, vec3 wm, vec3 wi) {
    if (u_clearcoat <= 0.0f) return 0.0f;

    float absDotNH = abs(dot(N, wm));
    float dotHV = dot(wm, wo);

    float clearcoatGloss = u_clearcoatGloss;

    float d = GTR1(absDotNH, mix(0.1, 0.001, clearcoatGloss));
    float f = schlickFresnel(0.04f, dotHV);
    float gl = GGXG1(wi, 0.25f);
    float gv = GGXG1(wo, 0.25f);

    return 0.25f * u_clearcoat * d * f * gl * gv;
}

/*****************/
/** Disney BSDF **/
/*****************/

vec3 evaluateDisney(vec3 N, vec3 wi, vec3 wm, vec3 wo, vec3 baseColor) {    
    float dotNV = dot(N, wo);
    float dotNL = max(dot(N, wi), 0.0f);
    if (dotNL <= 0.0f) return vec3(0.0f);

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
        float clearcoat = disneyClearcoat(N, wo, wm, wi);
        reflectance += vec3(clearcoat);
    }

    // diffuse
    if (diffuseWeight > 0.0f) {
        float diffuse = calcDisneyDiffuse(N, wi, wm, wo);
        vec3 sheen = calcSheen(baseColor, wi, wm);

        reflectance += diffuseWeight * (diffuse * baseColor + sheen);
    }

    // transmission
    if (transWeight > 0.0f) {
        float ior = (2.0f / (1.0f - sqrt(0.08f * u_specular))) - 1.0f;
        float rscaled = u_thin > 0 ? thinTransmissionRoughness(ior) : u_roughness;
        float tax, tay;
        calcAnisotropicParams(rscaled, u_anisotropic, tax, tay);

        vec3 transmission = calcDisneySpecTransmission(baseColor, N, wi, wo, wm, tax, tay);
        reflectance += transWeight * transmission;
    }

    // specular
    if (upperHemisphere) {
        vec3 specular = calcDisneyBRDF(baseColor, N, wo, wm, wi);
        reflectance += specular;
    }

    reflectance = reflectance * abs(dotNL);

    return reflectance;
}


void main() {
    vec3 N = safeNormalize(v_worldNormal); // normal vector
    vec3 V = safeNormalize(u_cameraPosition - v_worldPos); // view vector

    
    vec4 texAlbedo = texture(u_albedo, v_uv);
    vec3 baseColor = mon2lin(texAlbedo.rgb) * v_color;
    vec3 colorOut = u_ambient * baseColor;

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

        vec3 nt = vec3(0.0f, 1.0f, 0.0f);
        vec3 wo = safeNormalize(v_worldToTangent * V);
        vec3 wi = safeNormalize(v_worldToTangent * L);
        vec3 wm = safeNormalize(wo + wi); // half vector

        colorOut += evaluateDisney(nt, wi, wm, wo, baseColor) * Lrgb * intensity * att;
    }

    FragColor = vec4(colorOut, texAlbedo.a);

}
