#version 450
#ifndef MAVK_CULL_PLANES
#define MAVK_CULL_PLANES 2
#endif
#ifdef MAVK_CULL_REFERENCE
out gl_PerVertex { vec4 gl_Position; };
#else
out gl_PerVertex { vec4 gl_Position; float gl_CullDistance[MAVK_CULL_PLANES];
#ifdef MAVK_CULL_CLIP
float gl_ClipDistance[1];
#endif
};
#endif
#ifdef MAVK_CULL_USER_BLOCK
#ifdef MAVK_CULL_BLOCK_LOCATION
layout(location=4) out Payload { vec4 color; flat int tag; vec2 values[2]; } payload;
#else
out Payload { layout(location=0) vec4 color; layout(location=1) flat int tag; layout(location=2) vec2 values[2]; } payload;
#endif
#define cullColor payload.color
#else
layout(location=0) out vec4 color;
#define cullColor color
#endif
#ifdef MAVK_CULL_DESCRIPTOR
layout(set=0,binding=0) uniform Tint { vec4 color; } tint;
#endif
#ifdef MAVK_CULL_INTEGER
layout(location=4) flat out uint tag;
layout(location=5) flat out int signedTag;
#endif
layout(push_constant) uniform Params { vec4 distances0; vec4 distances1; } p;
void main(){
    vec2 positions[3]=vec2[](vec2(-0.75,-0.75),vec2(0.75,-0.75),vec2(0,0.75));
#ifdef MAVK_CULL_PRIMITIVE_ID
    int triangle=gl_VertexIndex/3;
    gl_Position=vec4(positions[gl_VertexIndex%3]*vec2(0.35,1)+vec2(float(triangle-1)*0.65,0),0.5,1);
#else
    gl_Position=vec4(positions[gl_VertexIndex],0.5,1);
#endif
#ifndef MAVK_CULL_REFERENCE
#ifdef MAVK_CULL_PRIMITIVE_ID
    gl_CullDistance[0]=p.distances0[triangle];
    gl_CullDistance[1]=p.distances1[gl_VertexIndex%3];
#else
    gl_CullDistance[0]=p.distances0[gl_VertexIndex];
    gl_CullDistance[1]=p.distances1[gl_VertexIndex];
#endif
#ifdef MAVK_CULL_CLIP
    gl_ClipDistance[0]=1;
#endif
#endif
    cullColor=vec4(1,0,0,1);
#ifdef MAVK_CULL_DESCRIPTOR
    cullColor=tint.color;
#endif
#ifdef MAVK_CULL_INTEGER
    tag=7; signedTag=-3;
#endif
#ifdef MAVK_CULL_USER_BLOCK
    payload.tag=7; payload.values[0]=vec2(2,3); payload.values[1]=vec2(4,5);
#endif
}
