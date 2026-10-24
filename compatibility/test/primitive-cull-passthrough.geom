#version 450
layout(triangles) in;
layout(triangle_strip,max_vertices=3) out;
layout(location=1) in vec4 positionIn[];
layout(location=0) in vec4 colorIn[];
layout(location=0) out vec4 color;
void main(){
#ifdef MAVK_CULL_CONSTANT_POSITION
    vec2 positions[3]=vec2[](vec2(-0.75,-0.75),vec2(0.75,-0.75),vec2(0,0.75));
#endif
    for(int i=0;i<3;i++){
#ifdef MAVK_CULL_CONSTANT_POSITION
        gl_Position=vec4(positions[i],0.5,1);
        color=vec4(1,0,0,1);
#else
        gl_Position=positionIn[i];
        color=colorIn[i];
#endif
        EmitVertex();
    }
    EndPrimitive();
}
