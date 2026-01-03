#version 440

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D spectrumData;
layout(binding = 2) uniform sampler2D colorLut;  // Spectrum color LUT

// Uniform buffer: 80 bytes, std140 layout
layout(std140, binding = 0) uniform buf {
    vec4 fillBaseColor;     // offset 0:  (unused)
    vec4 fillPeakColor;     // offset 16: (unused)
    vec4 glowColor;         // offset 32: cyan glow color
    float glowIntensity;    // offset 48: glow strength (0.8)
    float glowWidth;        // offset 52: glow falloff width (0.04)
    float spectrumHeight;   // offset 56: spectrum area height in pixels
    float padding1;         // offset 60: padding for alignment
    vec2 viewportSize;      // offset 64: viewport dimensions
    vec2 padding2;          // offset 72: padding for 16-byte alignment
};  // Total: 80 bytes

void main() {
    // Sample spectrum value at this X position (normalized 0-1)
    float spectrumValue = texture(spectrumData, vec2(fragTexCoord.x, 0.5)).r;

    // spectrumValue is 0.0 (no signal) to ~1.0 (max signal)
    // Convert to Y threshold: peakY where 0.0 = top, 1.0 = bottom (in texCoord space)
    float peakY = 1.0 - spectrumValue;
    float currentY = fragTexCoord.y;

    // Calculate distance from peak
    float distFromPeak = currentY - peakY;

    // Discard pixels above the peak (allow region for glow)
    float glowRegion = glowWidth * 2.0;
    if (distFromPeak < -glowRegion) {
        discard;
    }

    // === dBm-to-Color LUT Mapping ===
    // currentY: 0.0 = top (strong signal), 1.0 = bottom (weak signal)
    // LUT index: 0 = weak, 1 = strong (invert Y)
    float lutIndex = 1.0 - currentY;
    lutIndex = clamp(lutIndex, 0.0, 1.0);

    // Sample color from LUT
    vec3 mappedColor = texture(colorLut, vec2(lutIndex, 0.5)).rgb;

    // Fill mask: only show below the peak
    float fillMask = smoothstep(-0.002, 0.002, distFromPeak);

    // === Glow effect - exponential falloff from peak ===
    float glowDist = abs(distFromPeak);
    float glow = exp(-glowDist / glowWidth) * glowIntensity;

    // Extra peak glow - tighter, brighter at the edge
    float peakGlow = exp(-glowDist / (glowWidth * 0.3)) * glowIntensity * 0.5;
    glow = max(glow, peakGlow);

    // === FINAL COMPOSITE ===
    vec3 finalColor = mappedColor * fillMask;
    // Add glow (additive blending for bright halo effect)
    finalColor += glowColor.rgb * glow;

    // Alpha: fill alpha where filled, glow alpha where glowing
    float finalAlpha = max(0.9 * fillMask, glow * glowColor.a);

    // Bottom fade - gentle transition at noise floor
    float bottomFade = 1.0 - smoothstep(0.88, 0.98, currentY);
    finalAlpha *= max(bottomFade, 0.1);

    // Clamp to prevent over-saturation
    finalColor = clamp(finalColor, 0.0, 1.0);

    outColor = vec4(finalColor, finalAlpha);
}
