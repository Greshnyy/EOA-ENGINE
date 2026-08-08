#version 460

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D gAlbedo;
layout(set = 0, binding = 1) uniform sampler2D gNormal;
layout(set = 0, binding = 2) uniform sampler2D gORM;
layout(set = 0, binding = 3) uniform sampler2D gDepth;

layout(set = 0, binding = 4) uniform LightData {
    vec4 camPos;             // camera world position (.xyz)
    vec4 ambientColor;       // ambient term (.rgb)
    uint lightCount;
    uint _pad0, _pad1, _pad2;
} lightInfo;

struct Light {
    vec4 color_intensity;    // .rgb = color, .a = intensity
    vec4 pos_range;          // .xyz = position, .w = range
    vec4 dir_type;           // .xyz = direction, .w = type (0=directional,1=point,2=spot)
    vec4 cone_params;        // .x = innerCone, .y = outerCone, .z/.w = pad
};

layout(set = 0, binding = 5) uniform LightArray {
    Light lights[64];
} lightArray;

// PBR functions
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return nom / (3.14159265 * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    return GeometrySchlickGGX(max(dot(N, V), 0.0), roughness) *
           GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec4 albedoSample = texture(gAlbedo, inUV);
    vec4 normalSample = texture(gNormal, inUV);
    vec4 ormSample = texture(gORM, inUV);

    vec3 albedo = albedoSample.rgb;
    vec3 N = normalize(normalSample.rgb * 2.0 - 1.0);
    float roughness = ormSample.r;
    float metallic = ormSample.g;

    vec3 camPos = lightInfo.camPos.xyz;
    vec3 V = normalize(camPos - vec3(0.0)); // world pos not available in quad; use camera look dir
    // Actually we need world pos from depth — simplified: use view direction from UV

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 Lo = vec3(0.0);

    for (uint i = 0u; i < lightInfo.lightCount && i < 64u; i++) {
        Light L = lightArray.lights[i];
        float lType = L.dir_type.w;
        vec3 lightColor = L.color_intensity.rgb * L.color_intensity.a;

        vec3 Li = vec3(0.0);

        if (lType < 0.5) {
            // Directional
            vec3 lightDir = normalize(-L.dir_type.xyz);
            vec3 H = normalize(V + lightDir);
            float NDF = DistributionGGX(N, H, roughness);
            float G = GeometrySmith(N, V, lightDir, roughness);
            vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

            vec3 spec = (NDF * G * F) / (4.0 * max(dot(N, V), 0.0) * max(dot(N, lightDir), 0.0) + 0.0001);
            vec3 kD = (1.0 - F) * (1.0 - metallic);
            float NdotL = max(dot(N, lightDir), 0.0);
            Li = (kD * albedo / 3.14159265 + spec) * lightColor * NdotL;
        } else {
            // Point light (simplified)
            vec3 lightPos = L.pos_range.xyz;
            vec3 worldPos = camPos; // approximate (full deferred would reconstruct from depth)
            vec3 lightVec = lightPos - worldPos;
            float dist = length(lightVec);
            vec3 lightDir = lightVec / dist;

            float attenuation = 1.0 / (1.0 + dist * dist / (L.pos_range.w * L.pos_range.w));
            if (attenuation < 0.001) continue;

            vec3 H = normalize(V + lightDir);
            float NDF = DistributionGGX(N, H, roughness);
            float G = GeometrySmith(N, V, lightDir, roughness);
            vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

            vec3 spec = (NDF * G * F) / (4.0 * max(dot(N, V), 0.0) * max(dot(N, lightDir), 0.0) + 0.0001);
            vec3 kD = (1.0 - F) * (1.0 - metallic);
            float NdotL = max(dot(N, lightDir), 0.0);
            Li = (kD * albedo / 3.14159265 + spec) * lightColor * NdotL * attenuation;
        }

        Lo += Li;
    }

    vec3 ambient = lightInfo.ambientColor.rgb * albedo;
    vec3 color = ambient + Lo;

    // Tone mapping (simple Reinhard)
    color = color / (color + vec3(1.0));
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, 1.0);
}
