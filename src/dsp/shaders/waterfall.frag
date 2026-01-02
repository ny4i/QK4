#version 440

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D waterfallTex;
layout(binding = 2) uniform sampler2D colorLutTex;

void main() {
    float dbValue = texture(waterfallTex, fragTexCoord).r;
    outColor = texture(colorLutTex, vec2(dbValue, 0.5));
}
