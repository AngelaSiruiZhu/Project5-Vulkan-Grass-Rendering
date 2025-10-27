#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform CameraBufferObject {
    mat4 view;
    mat4 proj;
} camera;

// TODO: Declare fragment shader inputs
layout(location = 0) in float in_height;

layout(location = 0) out vec4 outColor;

void main() {
    // TODO: Compute fragment color
    vec3 baseColor = vec3(0.631, 0.361, 0.89);
    vec3 tipColor = vec3(1.0, 0.702, 0.976); 
    vec3 color = mix(baseColor, tipColor, in_height);
    
    outColor = vec4(color, 1.0);
}
