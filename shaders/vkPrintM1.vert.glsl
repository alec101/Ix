#version 450
#extension GL_ARB_separate_shader_objects : enable

// in / out

layout(location= 0) out vec2 out_UV;

// sets

layout(set= 0, binding= 0) uniform GlobalUniforms {
  mat4 cameraPersp;         // perspective camera matrix
  mat4 cameraUI;            // UI orthographic camera matrix
  vec3 cameraPos;           // camera position (eye) in the world
  vec2 vp;                  // viewport position on the virtual desktop
  vec2 vs;                  // viewport size;
  float UIscale;
} glb;

layout(set= 1, binding= 0) readonly buffer Storage_fontData {
  //struct { float x0, y0, dx, dy; } ch[1024]; <--- more clear view
  vec4 ch[1024];              // x0, y0, dx, dy
  vec2 texSize;               // texture size
  //vec4 tex[1024];
  //int chDx[1024];
  //int chDy;                 // font character height
} d;

// push constants

layout(push_constant) uniform PConsts {
  vec4 pos;               // changed frequently
  vec4 color1;            // main color
  vec4 color2;            // outline color
  int flags;              // each byte meaning: 0= persp camera, 1= ortho camera
  int outline;            // 0= no outline, [1-5]= outline distance
  float scale;
} p;


// ######################################################
void main() {
  int chID= (gl_VertexIndex)/ 4;    /// character ID
  int vertID= (gl_VertexIndex)% 4;  /// vertex ID (0-3)
  gl_Position= p.pos;
  vec4 ch= d.ch[chID];


  if(vertID== 0) {         // x0, y0
    /// no change in position
    out_UV= vec2(ch.x/ d.texSize.x, (ch.y+ ch.w)/ d.texSize.y);           // y0 switched with ye - fnt/w ogl char generation
    
  } else if(vertID== 1) {  // x0, ye
    gl_Position.y+= ch.w* p.scale;
    out_UV= vec2(ch.x/ d.texSize.x, ch.y/ d.texSize.y);                   // y0 switched with ye - fnt/w ogl char generation

  } else if(vertID== 2) {  // xe, y0
    gl_Position.x+= ch.z* p.scale;
    out_UV= vec2((ch.x+ ch.z)/ d.texSize.x, (ch.y+ ch.w)/ d.texSize.y);   // y0 switched with ye - fnt/w ogl char generation

  } else if(vertID== 3) {  // xe, ye
    gl_Position.x+= ch.z* p.scale,
    gl_Position.y+= ch.w* p.scale;
    out_UV= vec2((ch.x+ ch.z)/ d.texSize.x, ch.y/ d.texSize.y);           // y0 switched with ye - fnt/w ogl char generation
  }


  /*
  if(vertID== 2) {         // x0, y0
    /// no change in position
    out_UV= vec2(ch.x/ d.texSize.x, ch.y/ d.texSize.y);

  } else if(vertID== 0) {  // x0, ye
    gl_Position.y+= ch.w* p.scale;
    out_UV= vec2(ch.x/ d.texSize.x, (ch.y+ ch.w)/ d.texSize.y);
    
  } else if(vertID== 1) {  // xe, ye
    gl_Position.x+= ch.z* p.scale,
    gl_Position.y+= ch.w* p.scale;
    out_UV= vec2((ch.x+ ch.z)/ d.texSize.x, (ch.y+ ch.w)/ d.texSize.y);

  } else if(vertID== 3) {  // xe, y0
    gl_Position.x+= ch.z* p.scale;
    out_UV= vec2((ch.x+ ch.z)/ d.texSize.x, ch.y/ d.texSize.y);
  }
  */

  /* TO DEL AFTER TESTING
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
  */

  //out_UV.y-= 1.0f;    // y coordinate change in vulkan....
  //out_UV.y= - out_UV.y;    // y coordinate change in vulkan....

  // output position
  if((p.flags& 0x0001)> 0)
    gl_Position= glb.cameraPersp* gl_Position;
  else
    gl_Position= glb.cameraUI* gl_Position;
}
