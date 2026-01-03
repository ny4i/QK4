#version 440

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D spectrumData;

// Uniform buffer: 80 bytes, std140 layout
layout(std140, binding = 0) uniform buf {
    vec4 fillBaseColor;     // offset 0:  dark navy (0.0, 0.08, 0.16, 0.85)
    vec4 fillPeakColor;     // offset 16: electric blue (0.0, 0.63, 1.0, 0.85)
    vec4 glowColor;         // offset 32: cyan glow (0.0, 0.83, 1.0, 1.0)
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

    // Calculate distance from peak for glow effect
    float distFromPeak = currentY - peakY;

    // Discard pixels ABOVE the peak (negative distance means above)
    // But allow small region above for glow effect
    float glowRegion = glowWidth * 2.0;
    if (distFromPeak < -glowRegion) {
        discard;
    }

    // Base fill color - exponential gradient from bottom to top
    // currentY: 0.0 = top of spectrum, 1.0 = bottom
    // pow(x, 3.0) creates knee at ~30% from top - bottom 70% stays nearly black
    float rawBrightness = 1.0 - currentY;
    float brightness = pow(rawBrightness, 3.0);

    // Nearly black base, bright blue peak (hardcoded for accurate reference look)
    vec4 darkBase = vec4(0.0, 0.01, 0.03, 0.9);
    vec4 brightPeak = vec4(0.0, 0.7, 1.0, 0.95);
    vec4 fillColor = mix(darkBase, brightPeak, brightness);

    // Only show fill below the peak
    float fillMask = smoothstep(-0.002, 0.002, distFromPeak);

    // Glow effect - exponential falloff from peak
    // Glow appears both slightly above and below the peak line
    float glowDist = abs(distFromPeak);
    float glow = exp(-glowDist / glowWidth) * glowIntensity;

    // Stronger glow right at the peak
    float peakGlow = exp(-glowDist / (glowWidth * 0.3)) * glowIntensity * 0.5;
    glow = max(glow, peakGlow);

    // Combine fill and glow
    vec3 finalColor = fillColor.rgb * fillMask;
    finalColor += glowColor.rgb * glow;

    // Alpha: fill alpha where filled, glow alpha where glowing
    float finalAlpha = max(fillColor.a * fillMask, glow * glowColor.a);

    // Bottom fade - gentle transition at noise floor
    float bottomFade = 1.0 - smoothstep(0.88, 0.98, currentY);
    finalAlpha *= max(bottomFade, 0.1);

    // Clamp final color to prevent over-saturation from glow
    finalColor = clamp(finalColor, 0.0, 1.0);

    outColor = vec4(finalColor, finalAlpha);
}
