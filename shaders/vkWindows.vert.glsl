#version 450

// in/out

layout(location= 0) in vec3 in_vertPos;
layout(location= 1) in vec2 in_vertUV;
layout(location= 0) out vec2 out_UV;

// sets

layout(set= 0, binding= 0) uniform GlobalUniforms {
  mat4 cameraPersp;         // perspective camera matrix
  mat4 cameraOrtho;         // orthographic camera matrix
  vec2 vp;                  // viewport position on the virtual desktop
} glb;

// push constants

layout(push_constant) uniform PConsts {
  vec4 color;               // color
  vec4 pos;                 // position/origin
  //float x0, y0, xe, ye;     // custom position
  //float tx0, ty0;           // custom tex coords start
  //float txe, tye;           // custom tex coords end
  int flags;                // each byte meaning: 1= persp camera, 2= ortho camera, 3= enable texture, 4= disabled TO DEL/*, 5= customPosition, 6= customTex*/
} p;

// ************************************************************************************

void main() {

  gl_Position= glb.cameraOrtho* (p.pos+ vec4(in_vertPos, 1));
  if((p.flags& 0x0004)> 0)
    out_UV= in_vertUV;


  /* SCREW CUSTOM COORDS+POS, CAN BE DONE WITH QUAD
  int vertID= (gl_VertexIndex)% 4;// vertex ID (0-3)

  // position
  if((p.flags& 0x0010)> 0) {        // custom position
    if(vertID== 2)      gl_Position= vec4(p.x0, p.y0, 0, 1);
    else if(vertID== 0) gl_Position= vec4(p.x0, p.ye, 0, 1);
    else if(vertID== 1) gl_Position= vec4(p.xe, p.ye, 0, 1);
    else                gl_Position= vec4(p.xe, p.y0, 0, 1);
    
    gl_Position= glb.cameraOrtho* gl_Position;
  
  } else                            // position from vertex buffer
    gl_Position= glb.cameraOrtho* (p.pos+ vec4(in_vertPos, 1));  // <<< ONLY THIS

  // use texture
  if((p.flags& 0x0004)> 0) {
    if((p.flags& 0x0020)> 0) {    // custom tex coords
      if(vertID== 2)      out_UV= vec2(p.tx0, p.ty0);
      else if(vertID== 0) out_UV= vec2(p.tx0, p.tye);
      else if(vertID== 1) out_UV= vec2(p.txe, p.tye);
      else                out_UV= vec2(p.txe, p.ty0);
    } else                        // tex coords from vertex buffer
      out_UV= in_vertUV;    // <<< ONLY THIS
  }
  */
}
