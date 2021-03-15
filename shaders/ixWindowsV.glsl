#version 330 core

// vertex data from normal buffer
layout(location= 0) in vec3 vertPos;
layout(location= 1) in vec2 vertUV;

// output to fragment
out vec2 UV;

// camera matrix
uniform mat4 u_camera;        // standard camera matrix - in this case it should be in ortho

// other uniforms
uniform vec3 u_origin;        // start point for the quad
uniform bool u_useTexture;    // enable / disable texture
uniform bool u_customPos;     // use custom supplied vertex positions
uniform bool u_customTex;     // use custom supplied texture coordonates
uniform vec2 u_quadPos0;      // custom vertex position x0, y0
uniform vec2 u_quadPosE;      // custom vertex position xe, ye
uniform vec2 u_quadTex0;      // custom vertex coords s0, t0
uniform vec2 u_quadTexE;      // custom vertex coords se, te


void main() {
  int vertID= (gl_VertexID)% 4;  /// vertex ID (0-3)

  // vertex position
  if(u_customPos) {
    
    vec4 pos= vec4(u_origin, 1);

    if(vertID== 0) {
      pos.x+= u_quadPos0.x;
      pos.y+= u_quadPosE.y;

    } else if(vertID== 1) {
      pos.x+= u_quadPosE.x;
      pos.y+= u_quadPosE.y;

    } else if(vertID== 2) {
      pos.x+= u_quadPos0.x;
      pos.y+= u_quadPos0.y;

    } else {
      pos.x+= u_quadPosE.x;
      pos.y+= u_quadPos0.y;

    }
    gl_Position= u_camera* pos;
    
  } else
    gl_Position= u_camera* vec4(u_origin+ vertPos, 1);

  // texture coords
  if(u_useTexture) {
    if(u_customTex) {
      if(vertID== 0) {
        UV.x= u_quadTex0.x;
        UV.y= u_quadTex0.y;

      } else if(vertID== 1) {
        UV.x= u_quadTex0.x;
        UV.y= u_quadTexE.y;

      } else if(vertID== 2) {
        UV.x= u_quadTexE.x;
        UV.y= u_quadTexE.y;

      } else {
        UV.x= u_quadTexE.x;
        UV.y= u_quadTex0.y;
      }

    } else 
      UV= vertUV;
  }
}











