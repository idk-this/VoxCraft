#type vertex
#version 450

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} camera;

layout(location = 0) in vec3 inPosition;       // Vertex position
layout(location = 1) in vec3 inColor;          // Vertex color

layout(location = 2) in mat4 instanceModel;    // Instance transform
layout(location = 6) in vec3 instanceColor;    // Instance color

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = camera.proj * camera.view * instanceModel * vec4(inPosition, 1.0);
    fragColor = inColor * instanceColor; // комбинируем цвет вершины и экземпляра
}

#type fragment
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
