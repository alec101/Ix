#version 450

// in/out
layout(location= 0) out vec3 out_UV;

// global uniform buffer
layout(set= 0, binding= 0) uniform GlobalUniforms {
  mat4 cameraPersp;         // perspective camera matrix
  mat4 cameraOrtho;         // orthographic camera matrix
  vec3 cameraPos;
  vec2 vp;                  // viewport position on the virtual desktop
  vec2 vs;                  // viewport size
} glb;

// push constants - to force more compact data, vectors are not used, as any vector ocupies 16 bytes (even vec2 or vec3)
layout(push_constant) uniform PConsts {
  vec4 color;                   // color
  float x, y, z, radius;        // position (centre), radius
  //float dx, dy;                 // dx/dy
  float tx0, ty0, tDepth;       // tex coords start (z= depth)
  float txe, tye;               // tex coords end
  float hollow;                 // negative= no hollowing;
  int flags;                    // each byte meaning: 1= persp camera, 2= ortho camera, 3= enable texture
} p;


// ************************************************************************************
void main() {
  int vertID=  gl_VertexIndex% 4;  /// vertex ID (0-3)

  if(vertID== 0)
    gl_Position= vec4(p.x- p.radius, p.y+ p.radius, p.z, 1),
    out_UV=      vec3(p.tx0,         p.ty0,         p.tDepth);

  else if(vertID== 1)
    gl_Position= vec4(p.x+ p.radius, p.y+ p.radius, p.z, 1),
    out_UV=      vec3(p.txe,         p.ty0,         p.tDepth);

  else if(vertID== 2)
    gl_Position= vec4(p.x- p.radius, p.y- p.radius, p.z, 1),
    out_UV=      vec3(p.tx0,         p.tye,         p.tDepth);

  else if(vertID== 3)
    gl_Position= vec4(p.x+ p.radius, p.y- p.radius, p.z, 1),
    out_UV=      vec3(p.txe,         p.tye,         p.tDepth);
  

  if((p.flags& 0x0001)> 0)
    gl_Position= glb.cameraPersp* gl_Position;
  else
    gl_Position= glb.cameraOrtho* gl_Position;
}
