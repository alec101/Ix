#version 450
#extension GL_ARB_separate_shader_objects : enable

// in / out

layout(location= 0) out vec2 out_UV;

// sets

layout(set= 0, binding= 0) uniform GlobalUniforms {
  mat4 cameraPersp;         // perspective camera matrix
  mat4 cameraOrtho;         // orthographic camera matrix
  vec2 vp;                  // viewport position on the virtual desktop
} glb;

layout(set= 1, binding= 0) readonly buffer Storage_fontData {
  vec4 tex[1024];
  int chDx[1024];
  int chDy;                 // font character height
} d;

// push constants

layout(push_constant) uniform PConsts {
  vec4 pos;               // changed frequently
  vec4 color1;            // main color
  vec4 color2;            // outline color
  int flags;              // each byte meaning: 0= persp camera, 1= ortho camera
  int outline;            // 0= no outline, [1-5]= outline distance
} p;


// ######################################################
void main() {
  int chID= (gl_VertexIndex)/ 4;    /// character ID
  int vertID= (gl_VertexIndex)% 4;  /// vertex ID (0-3)

  if(vertID== 2) {         // x0, y0
    gl_Position= p.pos,
    out_UV=      vec2(d.tex[chID].x, d.tex[chID].y);

  } else if(vertID== 0) {  // x0, ye
    gl_Position= vec4(p.pos.x,               p.pos.y+ d.chDy, p.pos.z, p.pos.w),
    out_UV=      vec2(d.tex[chID].x,         d.tex[chID].w);

  } else if(vertID== 1) {  // xe, ye
    gl_Position= vec4(p.pos.x+ d.chDx[chID], p.pos.y+ d.chDy, p.pos.z, p.pos.w),
    out_UV=      vec2(d.tex[chID].z,         d.tex[chID].w);

  } else if(vertID== 3) {  // xe, y0
    gl_Position= vec4(p.pos.x+ d.chDx[chID], p.pos.y,         p.pos.z, p.pos.w),
    out_UV=      vec2(d.tex[chID].z,         d.tex[chID].y);
  }

  out_UV.y-= 1.0f;    // y coordinate change in vulkan....

  // output position
  if((p.flags& 0x0001)> 0)
    gl_Position= glb.cameraPersp* gl_Position;
  else
    gl_Position= glb.cameraOrtho* gl_Position;
}
