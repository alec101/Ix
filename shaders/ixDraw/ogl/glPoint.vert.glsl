#version 330 core

// output data
out vec3 UV;


// camera matrix
uniform mat4 u_camera;

void main() {
  int vertID= (gl_VertexID)% 4;  /// vertex ID (0-3)

  vec4 vertPos= makeme;

    

  // Output position of the vertex, in clip space : MVP * position
  gl_Position= u_camera* vertPos;

}
