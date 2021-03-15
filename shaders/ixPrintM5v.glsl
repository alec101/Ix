#version 330 core

// layout(location= 0) - buffer id | in - input | vec3 vertPos_modelSpace - input data from buffer for each vert will be filled in this var
layout(location= 0) in int dataDx;    // M5 normal buffers
layout(location= 1) in vec2 dataUV;   // M5 normal buffers

/* UNIFORM ONLY DRAWING- not working, but this is good research - DO NOT DEL ***
struct CharData {
  int chDx;
  float x0, y0, xe, ye; 
};

uniform U_data {
  int chDy;
  CharData data[1024];  // unbound data size data[] is ogl4.3, data sizes are fixed otherwise
};
*/

// Output data ; will be interpolated for each fragment.
out vec2 UV;

// uniforms
uniform mat4 u_camera;
uniform int u_chDy;
uniform vec4 u_pos;     // changed frequently

void main() {
  //int chID= (gl_VertexID)/ 4;    /// character ID
  int vertID= (gl_VertexID)% 4;  /// vertex ID (0-3)
  
  vec4 vertPos= u_pos;

  vertPos.x+= dataDx;

  if(vertID== 0) {
    //m5 UBO buffers - not working, but good for research
    //UV.x= data[chID].x0;
    //UV.y= data[chID].y0;
    
  } else if(vertID== 1) {
    vertPos.y+= u_chDy;   // m5 normal buffers

    // m5 UBO buffers
    //vertPos+= chDy;
    //UV.x= data[chID].x0;
    //UV.y= data[chID].ye;

  } else if(vertID== 2) {
    vertPos.y+= u_chDy; // m5 normal buffers

    // m5 UBO buffers
    //vertPos+= chDy;
    //vertPos+= data[chID].chDx;
    //UV.x= data[chID].xe;
    //UV.y= data[chID].ye;

  } else {
//    vertPos.x+= dataDx; // m5 normal buffers

    // m5 UBO buffers
    //vertPos+= data[chID].chDx;
    //UV.x= data[chID].xe;
    //UV.y= data[chID].y0;
  }

  /// advance character position - NOTHING LIKE THIS IS POSSIBLE, AS FAR AS I CAN SEE ATM
  //if(vertID== 3)
  //  xpos+= vertData.z;

  // Output position of the vertex, in clip space : MVP * position
  gl_Position= u_camera* vertPos;

  // UV of the vertex
  UV= dataUV; // m5 normal buffers
}
