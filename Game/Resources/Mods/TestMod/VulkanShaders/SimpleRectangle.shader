#type vertex
#version 450

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} camera;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 3) in mat4 instanceModel;
layout(location = 7) in vec3 instanceColor;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragColor;

void main() {
    gl_Position = camera.proj * camera.view * instanceModel * vec4(inPosition, 1.0);

    fragTexCoord = inTexCoord;
    fragColor = inColor * instanceColor;
}
#type fragment
#version 450

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 tex = texture(texSampler, fragTexCoord);
    outColor = tex * vec4(fragColor, 1.0);
}
