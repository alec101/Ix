 #version 330 core

struct CharData {
  int chDx;
  float x0, y0, xe, ye; 
};

uniform U_data {
  int chDy;
  CharData data[1024];  // unbound data size data[] is ogl4.3, data sizes are fixed otherwise
};

// Output data ; will be interpolated for each fragment.
out vec2 UV;

// uniforms
uniform mat4 u_camera;
uniform int u_chDy;
uniform vec4 u_pos;     // changed frequently

void main() {
  int chID= (gl_VertexID)/ 4;    /// character ID
  int vertID= (gl_VertexID)% 4;  /// vertex ID (0-3)
  
  vec4 vertPos= u_pos;

  // GL_QUADS: V1, V2, V3, V4
  // GL_TRIANGLE_STRIP V2, V3, V1, V4
  if(vertID== 2) {
    UV.x= data[chID].x0;
    UV.y= data[chID].y0;
    
  } else if(vertID== 0) {
    vertPos.y+= chDy;
    UV.x= data[chID].x0;
    UV.y= data[chID].ye;

  } else if(vertID== 1) {
    vertPos.y+= chDy;
    vertPos.x+= data[chID].chDx;
    UV.x= data[chID].xe;
    UV.y= data[chID].ye;

  } else {
    vertPos.x+= data[chID].chDx;
    UV.x= data[chID].xe;
    UV.y= data[chID].y0;
  }
  
  // Output position of the vertex, in clip space : MVP * position
  gl_Position= u_camera* vertPos;
}
