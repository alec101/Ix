#include "ix/ix.h"

using namespace mlib;
/*
- perspective/ortho/rotations/translations matrix operations from teh vulkan cookbook
  https://github.com/PacktPublishing/Vulkan-Cookbook/blob/master/Library/Source%20Files/10%20Helper%20Recipes/04%20Preparing%20a%20perspective%20projection%20matrix.cpp


thoughts:
  


- ortho camera should be computed only once
- ortho camera should be updated only on resolution change
- perspective camera should be updated every frame
- frustum... must further check this mode, i think is some ortho/perspective hybrid


- maybe Camera should have a chainlist with all the shaders, and a func to update all the shaders' matrix
    could be called at the start of each frame, and update ortho on res change / program start (after the glsl programs are loaded)
    ^^^ this would be an elegant way to update camera matrices...

    more tests must be done before a definitive method can be choosen. dunno if there are shortcuts to matrix multiply, and stuff

*/

//ixCamera cameraPersp, cameraOrtho, *camera= &cameraPersp;



ixCamera::ixCamera() {
  delData();
}


ixCamera::~ixCamera() {
}


void ixCamera::delData() {
  type= 0;                       /// 0= orthographic, 1= frustum, 2= perspective
  
  pos= 0.0f;
  eye= vec3(0.0f, 0.0f, 1.0f);
  zClipNear= 100.5f;
  zClipFar= 100.5f;
  fov= 80;
  aspect= 2.33333f;
  bounds.set(0, 1024, 0, 768);

  cameraMat.identity();                    // camera matrix, based on current values - projMat* worldMat= cameraMat
  worldMat.identity();                    /// part of the cameraMat - world matrix
  projMat.identity();                     /// part of the cameraMat - projection matrix
}







void ixCamera::setCamera() {
  if(type== 0)              // orthographic camera
    setOrtho(bounds, zClipNear, zClipFar);

  else if(type== 1)         // frustum camera
    setFrustum(bounds, zClipNear, zClipFar);

  else if(type== 2)         // perspective camera
    setPerspective(pos, eye, fov, aspect, zClipNear, zClipFar);
}


void ixCamera::setPerspective(const vec3 &in_pos, const vec3 &in_eye, float in_fov, float in_aspect, float in_zClipNear, float in_zClipFar) {
  type= 2;                  // set camera as a perspective camera
  pos= in_pos;
  eye= in_eye;
  fov= in_fov;
  aspect= in_aspect;
  zClipNear= in_zClipNear;
  zClipFar= in_zClipFar;

  projMat.vkPerspective(in_fov, in_aspect, in_zClipNear, in_zClipFar);
  worldMat.vkLookAt(in_pos, in_eye, vec3(0.0f, 1.0f, 0.0f));
  cameraMat= projMat* worldMat;
}


void ixCamera::setOrtho(const rectf &in_bounds, float in_zn, float in_zf) {
  type= 0;                // set camera as a ortographic camera
  zClipFar= in_zf;
  zClipNear= in_zn;
  bounds= in_bounds;

  projMat.vkOrtho(bounds.l, bounds.r, bounds.b, bounds.t, zClipNear, zClipFar);
  worldMat.identity();
  cameraMat= projMat; //* worldMat;
}


void ixCamera::setFrustum(const rectf &in_bounds, float in_zn, float in_zf) {
  type= 1;                // set camera as a frustum camera
  bounds= in_bounds;
  zClipNear= in_zn;
  zClipFar= in_zf;

  // NO VULKAN VARIANT
  projMat.glFrustrum(bounds.l, bounds.r, bounds.b, bounds.t, zClipNear, zClipFar);
  //worldMat.lookAt(*pos, *eye, vec3(0.0f, 1.0f, 0.0f));
  worldMat.identity();
  cameraMat= projMat; //* worldMat;
}
  

void ixCamera::move(const vec3 &in_pos, const vec3 &in_eye) {
  pos= in_pos;
  eye= in_eye;
  setCamera();
}














