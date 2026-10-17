#version 450
layout(location=0) in vec2 position;
layout(set=0,binding=0) uniform Matrices {
    mat4 projectionMatrix;
    mat4 modelMatrix;
    mat4 viewMatrix;
} ubo;
void main(){gl_Position=ubo.projectionMatrix*ubo.viewMatrix*ubo.modelMatrix*vec4(position,0,1);}
