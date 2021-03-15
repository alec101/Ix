#version 330 core

// output data
out vec3 UV;

// circle positions
uniform vec3 u_pos;     // quad start position
uniform vec2 u_delta;   // quad dx and dy

uniform vec3 u_tex0;    // tex coord start position    tex0.z= depth
uniform vec2 u_texE;    // tex coord end position

// camera matrix
uniform mat4 u_camera;

void main() {
  int vertID= (gl_VertexID)% 4;  /// vertex ID (0-3)

  vec4 vertPos= vec4(u_pos, 1);
  UV= u_tex0;

  if(vertID== 0)
    vertPos.y+= u_delta.y,
    UV.y= u_texE.y;

  else if(vertID== 1)
    vertPos.x+= u_delta.x,
    vertPos.y+= u_delta.y,
    UV.x= u_texE.x,
    UV.y= u_texE.y;

  else if(vertID== 3)
    vertPos.x+= u_delta.x,
    UV.x= u_texE.x;

  // output position of the vertex, in clip space
  gl_Position= u_camera* vertPos;
}
