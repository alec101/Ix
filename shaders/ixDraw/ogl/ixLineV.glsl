#version 330 core

// output data
out vec3 UV;

// uniforms that generate the quad

// camera matrix
uniform mat4 u_camera;

void main() {
  int vertID= (gl_VertexID)% 4;  /// vertex ID (0-3)




  // MAKEME



  // Output position of the vertex, in clip space : MVP * position
  gl_Position= u_camera* vertPos;

}
