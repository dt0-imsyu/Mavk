#version 450
layout(triangles,equal_spacing,ccw) in;
in gl_PerVertex { vec4 gl_Position; } gl_in[];
#ifdef MAVK_CULL_REFERENCE
out gl_PerVertex { vec4 gl_Position; };
#else
out gl_PerVertex { vec4 gl_Position; float gl_CullDistance[2]; };
#endif
layout(location=0) out vec4 color;
layout(push_constant) uniform Params { vec4 distances0; vec4 distances1; } p;
#ifdef MAVK_CULL_DESCRIPTOR
layout(set=0,binding=0) uniform Tint { vec4 color; } tint;
#endif
void main(){
    gl_Position=gl_TessCoord.x*gl_in[0].gl_Position+gl_TessCoord.y*gl_in[1].gl_Position+gl_TessCoord.z*gl_in[2].gl_Position;
#ifndef MAVK_CULL_REFERENCE
    gl_CullDistance[0]=dot(gl_TessCoord,p.distances0.xyz);
    gl_CullDistance[1]=dot(gl_TessCoord,p.distances1.xyz);
#endif
    color=vec4(1,0,0,1);
#ifdef MAVK_CULL_DESCRIPTOR
    color=tint.color;
#endif
}
