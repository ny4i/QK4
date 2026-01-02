#version 440

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D spectrumData;

layout(std140, binding = 0) uniform buf {
    float spectrumHeight;
    float frostHeight;
    float padding1;
    float padding2;
    vec4 peakColor;
    vec4 baseColor;
};

void main() {
    // Sample spectrum value at this X position (normalized 0-1)
    float spectrumValue = texture(spectrumData, vec2(fragTexCoord.x, 0.5)).r;

    // spectrumValue is 0.0 (no signal) to ~1.0 (max signal)
    // Convert to Y threshold: 1.0 = bottom, 0.0 = top (in texCoord space)
    float peakY = 1.0 - spectrumValue;
    float currentY = fragTexCoord.y;

    // Discard pixels ABOVE the peak
    if (currentY < peakY) {
        discard;
    }

    // Color based on ABSOLUTE Y position in the spectrum area
    // currentY: 0.0 = top of spectrum (strongest), 1.0 = bottom (weakest)
    // Invert so higher signals = brighter colors
    float brightness = 1.0 - currentY;

    // Reach peak color earlier - signals at 40% height already get near-full brightness
    brightness = smoothstep(0.0, 0.4, brightness);

    // Interpolate from base color (bottom/dim) to peak color (top/bright)
    vec4 color = mix(baseColor, peakColor, brightness);

    // Gentle fade at bottom edge - start earlier for smoother transition
    float bottomFade = 1.0 - smoothstep(0.88, 0.98, currentY);
    color.a *= max(bottomFade, 0.15);  // Keep minimum 15% visibility at very bottom

    outColor = color;
}
