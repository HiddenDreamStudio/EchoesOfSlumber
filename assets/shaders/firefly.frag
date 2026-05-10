#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;

// Firefly data
struct Firefly {
    vec2 position; // Screen space 0-1
    vec3 color;
    float intensity;
};

layout(std140, binding = 1) uniform FireflyBuffer {
    Firefly fireflies[32];
    int count;
    float time;
    vec2 resolution;
} ubo;

void main() {
    vec4 baseColor = texture(texSampler, fragTexCoord);
    vec3 finalColor = baseColor.rgb;

    // Apply simple vignette/darkness
    float distToCenter = distance(fragTexCoord, vec2(0.5));
    finalColor *= (1.0 - distToCenter * 0.5);

    // Calculate Firefly Glow
    for (int i = 0; i < ubo.count; ++i) {
        // Simple procedural movement based on time
        vec2 pos = ubo.fireflies[i].position;
        pos.x += sin(ubo.time * 0.5 + float(i)) * 0.02;
        pos.y += cos(ubo.time * 0.3 + float(i)) * 0.02;

        float d = distance(fragTexCoord * (ubo.resolution / ubo.resolution.y), pos * (ubo.resolution / ubo.resolution.y));
        float glow = exp(-d * 20.0) * ubo.fireflies[i].intensity;
        
        finalColor += ubo.fireflies[i].color * glow;
    }

    outColor = vec4(finalColor, baseColor.a);
}
