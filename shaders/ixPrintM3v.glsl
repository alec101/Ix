#version 330 core


method 3 needs heavy redoing - just update some uniforms, no need for any buffer for 4 vertices - at this state, it's extremly slow
===================================================================================================================================


// layout(location= 0) - buffer id | in - input | vec3 vertPos_modelSpace - input data from buffer for each vert will be filled in this var
layout(location= 0) in vec3 vertPos;
layout(location= 1) in vec2 vertUV;

// Output data ; will be interpolated for each fragment.
out vec2 UV;

// Values that stay constant for the whole mesh.
//uniform mat4 MVP;
uniform mat4 u_camera;
//uniform mat4 model

void main() {
  
  // Output position of the vertex, in clip space : MVP * position
  gl_Position= u_camera* vec4(vertPos, 1.0);

  // UV of the vertex. No special space for this one.
  UV= vertUV;
}
