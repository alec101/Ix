#version 330 core
// #version 120 mac... lol

// output data
out vec3 UV;

// uniforms that generate the triangle
uniform mat4 u_camera;    // camera matrix
uniform vec3 u_vert[3];   // the 3 vertices
uniform vec3 u_tex[3];    // texture coordinates for the 3 vertices

void main() {
  //int vertID= (gl_VertexID)% 4;  /// vertex ID (0-3)
  UV= u_tex[gl_VertexID];
  gl_Position= u_camera* vec4(u_vert[gl_VertexID], 1);
}
