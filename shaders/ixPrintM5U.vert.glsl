#version 450

layout(location= 0) out vec2 out_UV;

// global uniform buffer
layout(set= 0, binding= 0) uniform UniformBufferObject {
  mat4 cameraPersp;       // perspective camera matrix
  mat4 cameraOrtho;       // orthographic camera matrix
  int vpx, vpy;           // viewport position on the virtual desktop
} glb;

// push constants
layout(push_constant) uniform PConsts {
  vec4 pos;               // changed frequently
  vec4 color;
  vec2 clip0;             // clipping: 0= start / x0, y0
  vec2 clipE;             // clipping: e= end / xe, ye
  int flags;              // each byte meaning: 0= persp camera, 1= ortho camera, 2= clip enable
} p;

// buffer with the font page data
layout(set= 1, binding= 0) uniform UBO_fontData {
  int chDy;               // font character height
  struct CharData {
    int chDx;             // each character width
    float x0, y0, xe, ye; // each character texture coords
  } data[1024];
  //CharData data[1024];    // unbound data size data[] is ogl4.3, data sizes are fixed otherwise
} u;



void main() {
  int chID= (gl_VertexID)/ 4;    /// character ID
  int vertID= (gl_VertexID)% 4;  /// vertex ID (0-3)

  // GL_QUADS: V1, V2, V3, V4
  // GL_TRIANGLE_STRIP V2, V3, V1, V4
  if(vertID== 2) {
    out_UV.x= u.data[chID].x0;
    out_UV.y= u.data[chID].y0;
    
  } else if(vertID== 0) {
    p.pos.y+= u.chDy;
    out_UV.x= u.data[chID].x0;
    out_UV.y= u.data[chID].ye;

  } else if(vertID== 1) {
    p.pos.y+= u.chDy;
    p.pos.x+= u.data[chID].chDx;
    out_UV.x= u.data[chID].xe;
    out_UV.y= u.data[chID].ye;

  } else {
    p.pos.x+= u.data[chID].chDx;
    out_UV.x= u.data[chID].xe;
    out_UV.y= u.data[chID].y0;
  }
  
  // output position
  if(p.flags& 0x0001)
    gl_Position= glb.cameraPersp* p.pos;
  else
    gl_Position= glb.cameraOrtho* p.pos;
}


/*
#version 450,

layout(location= 0) out vec2 out_UV;

struct CharData {
  int chDx;
  float x0, y0, xe, ye; 
};

uniform U_data {
  int chDy;
  CharData data[1024];  // unbound data size data[] is ogl4.3, data sizes are fixed otherwise
};

i think just start translating from ogl...
probly let the old code in here, so i don't ahve to go back to the backup
uniforms should be push constants i think, also, they're small enough

there's no easy way, just start translating ... everthing

// uniforms
uniform vertUni {
  mat4 camera;
  int chDy;
  vec4 pos;     // changed frequently
} u;


void main() {
  int chID= (gl_VertexID)/ 4;    /// character ID
  int vertID= (gl_VertexID)% 4;  /// vertex ID (0-3)
  
  vec4 vertPos= u_pos;

  // GL_QUADS: V1, V2, V3, V4
  // GL_TRIANGLE_STRIP V2, V3, V1, V4
  if(vertID== 2) {
    out_UV.x= data[chID].x0;
    out_UV.y= data[chID].y0;
    
  } else if(vertID== 0) {
    vertPos.y+= chDy;
    out_UV.x= data[chID].x0;
    out_UV.y= data[chID].ye;

  } else if(vertID== 1) {
    vertPos.y+= chDy;
    vertPos.x+= data[chID].chDx;
    out_UV.x= data[chID].xe;
    out_UV.y= data[chID].ye;

  } else {
    vertPos.x+= data[chID].chDx;
    out_UV.x= data[chID].xe;
    out_UV.y= data[chID].y0;
  }
  
  // Output position of the vertex, in clip space : MVP * position
  gl_Position= u_camera* vertPos;
}
*/