#version 450

// in/out
layout(location= 0) out vec3 out_UV;

// global uniform buffer
layout(set= 0, binding= 0) uniform GlobalUniforms {
  mat4 cameraPersp;         // perspective camera matrix
  mat4 cameraUI;            // UI orthographic camera matrix
  vec3 cameraPos;
  vec2 vp;                  // viewport position on the virtual desktop
  vec2 vs;                  // [scaled] viewport size
  float UIscale;            //
} glb;

// push constants
layout(push_constant) uniform PConsts {
  vec4 color;               // color
  vec4 vert[3];             // the 3 vertices (vec4 for padding)
  vec4 tex[3];              // texture coordinates for the 3 vertices (vec4 for padding)
  int flags;                // each byte meaning: 1= persp camera, 2= ortho camera, 3= enable texture
} p;


// ************************************************************************************
void main() {
  out_UV= p.tex[gl_VertexIndex].xyz;

  if((p.flags& 0x0001)> 0)
    gl_Position= glb.cameraPersp* p.vert[gl_VertexIndex];
  else
    gl_Position= glb.cameraUI* p.vert[gl_VertexIndex];
}
