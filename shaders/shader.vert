#version 450

layout(set=0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec3 fragNormal;

layout(push_constant) uniform PushConstant{
    mat4 model;
}pc;

void main() {
    fragPos = ubo.proj * ubo.view * pc.model *  vec4(inPosition,  1.0);
    gl_Position = fragPos;
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragNormal = inNormal;
}