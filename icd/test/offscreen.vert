#version 450
layout(push_constant) uniform Parameters { vec4 state; } parameters;
vec2 vertices[3]=vec2[](vec2(-.75,-.75),vec2(.75,-.75),vec2(0,.75));
void main(){gl_Position=vec4(vertices[gl_VertexIndex]+parameters.state.xy,0,1);}
