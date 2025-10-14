#type vertex
#version 450

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} camera;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 4) in mat4 instanceModel;
layout(location = 8) in vec3 instanceColor;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragNormal;

void main() {
    gl_Position = camera.proj * camera.view * instanceModel * vec4(inPosition, 1.0);

    fragTexCoord = inTexCoord;
    fragColor = inColor * instanceColor;
    fragNormal = normalize(mat3(transpose(inverse(instanceModel))) * inNormal);
}
#type fragment
#version 450

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform LightUBO {
    vec3 lightDir;
    vec3 lightColor;
} light;

void main() {
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(-light.lightDir);

    float diff = max(dot(norm, lightDir), 0.0);

    float ambient = 0.2;

    vec3 lighting = (ambient + diff) * light.lightColor;

    lighting = pow(lighting, vec3(1.0 / 2.2));

    vec4 tex = texture(texSampler, fragTexCoord);
    vec3 finalColor = tex.rgb * fragColor * lighting;

    outColor = vec4(finalColor, 1.0);
}