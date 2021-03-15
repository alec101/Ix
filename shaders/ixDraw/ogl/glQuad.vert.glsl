#version 330 core

// output data
out vec3 UV;

// uniforms that generate the quad
uniform vec3 u_pos;   // quad start position
uniform vec2 u_delta;  // quad dx and dy

//uniform bool u_useTexture;
uniform vec3 u_tex0;   // texture coordonates start position
uniform vec2 u_texE;   // no need for the 3rd cooronate!! texture coordonates end position

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
  

  // Output position of the vertex, in clip space : MVP * position
  gl_Position= u_camera* vertPos;

}




