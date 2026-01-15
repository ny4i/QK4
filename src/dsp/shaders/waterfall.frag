#version 440

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D waterfallTex;
layout(binding = 2) uniform sampler2D colorLutTex;

layout(std140, binding = 0) uniform buf {
    float scrollOffset;   // Row scroll offset (used by vertex shader)
    float binCount;       // Actual spectrum bin count (e.g., 213 at 5kHz span)
    float textureWidth;   // Texture width for bin centering (1024)
    float padding;
};

// Lanczos kernel - sinc(x) * sinc(x/a) windowed
float lanczos(float x) {
    if (x == 0.0) return 1.0;
    if (abs(x) >= 3.0) return 0.0;
    float px = 3.14159265 * x;
    return (sin(px) / px) * (sin(px / 3.0) / (px / 3.0));
}

// Sample waterfall texture with Lanczos-3 interpolation
// Raw bins are centered in texture, this function handles the mapping
float sampleLanczos3(vec2 uv) {
    // Map display X [0,1] to bin index [0, binCount-1]
    float srcPos = uv.x * (binCount - 1.0);
    float texelSize = 1.0 / textureWidth;

    // Bins are centered in texture
    float binOffset = (textureWidth - binCount) / 2.0;

    int center = int(srcPos);
    float frac = srcPos - float(center);

    float sum = 0.0;
    float weightSum = 0.0;

    // 6-tap Lanczos kernel (a=3)
    for (int i = -2; i <= 3; i++) {
        float weight = lanczos(float(i) - frac);
        int idx = center + i;
        if (idx >= 0 && idx < int(binCount)) {
            // Map bin index to texture coordinate
            float texU = (binOffset + float(idx) + 0.5) * texelSize;
            sum += texture(waterfallTex, vec2(texU, uv.y)).r * weight;
            weightSum += weight;
        }
    }

    return (weightSum > 0.0) ? (sum / weightSum) : 0.0;
}

void main() {
    // Use Lanczos interpolation for high-quality waterfall display
    float dbValue = sampleLanczos3(fragTexCoord);
    outColor = texture(colorLutTex, vec2(dbValue, 0.5));
}
