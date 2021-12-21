#include "ix/ix.h"

using namespace mlib;
/*
- perspective/ortho/rotations/translations matrix operations from teh vulkan cookbook
  https://github.com/PacktPublishing/Vulkan-Cookbook/blob/master/Library/Source%20Files/10%20Helper%20Recipes/04%20Preparing%20a%20perspective%20projection%20matrix.cpp


  TODO:
  - camera roll is still a thing to do, but i don't see the need for it atm
  - still not mastering all the rotations

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
  
  pos.set(0.0f, 0.0f, 3.0f);
  target.set(0.0f, 0.0f, 0.0f);
  angle.set(179.9f, 0.0f, 180.0f);      // default angle, top, looking towards bottom
  computeFront();

  zClipNear= .1f;
  zClipFar= 100.5f;
  fov= 80;
  aspect= 2.33333f;
  bounds.set(0, 1024, 0, 768);

  //vec3 up(0.0f, -1.0f, 0.0f);
  //zAxis= normalize(pos- target);          // z should be negative
  //xAxis= normalize(cross(up, zAxis));     // right vector direction; up is 0, -1, 0, negative y for Vulkan
  //yAxis= normalize(cross(zAxis, xAxis));  // -y points to top in Vulkan

  camMat.identity();                      // camera matrix, based on current values - projMat* viewMat= cameraMat
  viewMat.identity();                     /// part of the cameraMat - view matrix
  projMat.identity();                     /// part of the cameraMat - projection matrix
}





void ixCamera::computeViewMat() {

  if(type== 2)
    viewMat.vkLookAt(pos, target, vec3(0.0f, 0.0f, 1.0f));
}


void ixCamera::computeProjMat() {
  if(type== 2)
    projMat.vkPerspective(fov, aspect, zClipNear, zClipFar);
}


void ixCamera::computeCamMat() {
  camMat= projMat* viewMat;
}




void ixCamera::setCamera() {
  if(type== 0)              // orthographic camera
    setOrtho(bounds, zClipNear, zClipFar);

  else if(type== 1)         // frustum camera
    setFrustum(bounds, zClipNear, zClipFar);

  else if(type== 2)         // perspective camera
    setPerspective(pos, target, fov, aspect, zClipNear, zClipFar);
}


void ixCamera::setPerspective(const vec3 &in_pos, const vec3 &in_target, float in_fov, float in_aspect, float in_zClipNear, float in_zClipFar) {
  type= 2;                  // set camera as a perspective camera
  pos= in_pos;
  target= in_target;
  fov= in_fov;
  aspect= in_aspect;
  zClipNear= in_zClipNear;
  zClipFar= in_zClipFar;

  projMat.glPerspective(in_fov, in_aspect, in_zClipNear, in_zClipFar);
  viewMat.vkLookAt(in_pos, in_target, vec3(0.0f, 0.0f, 1.0f));
  camMat= projMat* viewMat;
}


void ixCamera::setOrtho(const rectf &in_bounds, float in_zn, float in_zf) {
  type= 0;                // set camera as a ortographic camera
  zClipFar= in_zf;
  zClipNear= in_zn;
  bounds= in_bounds;

  projMat.vkOrtho(bounds.l, bounds.r, bounds.b, bounds.t, zClipNear, zClipFar);
  viewMat.identity();
  camMat= projMat; //* viewMat;
}


void ixCamera::setFrustum(const rectf &in_bounds, float in_zn, float in_zf) {
  type= 1;                // set camera as a frustum camera
  bounds= in_bounds;
  zClipNear= in_zn;
  zClipFar= in_zf;

  // NO VULKAN VARIANT
  projMat.glFrustrum(bounds.l, bounds.r, bounds.b, bounds.t, zClipNear, zClipFar);
  //worldMat.lookAt(*pos, *eye, vec3(0.0f, 1.0f, 0.0f));
  viewMat.identity();
  camMat= projMat; //* worldMat;
}
  



void ixCamera::computeAngles() {
  //mlib::asinf(x);
  //do i need the angles?
}


// moves position and target
void ixCamera::move(const vec3 &in_pos, const vec3 &in_target) {
  pos= in_pos;
  target= in_target;
  computeFront();

  //setCamera();
}

// moves only position
void ixCamera::movePos(const vec3 &in_pos) {
  pos= in_pos;
  computeFront();
}

// moves only target
void ixCamera::moveTarget(const vec3 &in_tgt) {
  target= in_tgt;
  computeFront();
}




void ixCamera::extractAngles(float *out_pitchX, float *out_rollY, float *out_yawZ) {
  if(out_pitchX) *out_pitchX= atan2f(viewMat.v[6], mlib::sqrtf( (viewMat.v[10]* viewMat.v[10])+ (viewMat.v[2]* viewMat.v[2]) ) ); // sCam.look.z^ 2 + sCam.look.x^ 2 );
  if(out_rollY)  *out_rollY=  atan2f(viewMat.v[5], viewMat.v[4])- PI_HALF;                                                        // sCam.up.y, sCam.right.y ) - D3DX_PI/2;
  if(out_yawZ)   *out_yawZ=   atan2f(viewMat.v[2], viewMat.v[10]);                                                                // sCam.look.x, sCam.look.z );
}








// rotate target around pos (eye) around x axis
void ixCamera::rotateTargetX(float in_angle) {
  angle.x+= in_angle;
  checkAnglesClamp(&angle);
  computeFrontFromAngles();
}

// rotate target around pos (eye) around y axis
void ixCamera::rotateTargetY(float in_angle) {
  angle.y+= in_angle;
  checkAnglesClamp(&angle);
  computeFrontFromAngles();}

// rotate target around pos (eye) around z axis
void ixCamera::rotateTargetZ(float in_angle) {
  angle.z+= in_angle;
  checkAnglesClamp(&angle);
  computeFrontFromAngles();
}

// rotate target around pos (eye) around all axis
void ixCamera::rotateTarget(const vec3 &in_angles) {
  angle+= in_angles;
  checkAnglesClamp(&angle);
  computeFrontFromAngles();
}


void ixCamera::computeFrontFromAngles() {
  // THIS SHOULD BE IT, BUT IT DON'T WORK.
  // EACH ROTATION, IF ONLY ONE WAS DONE:
  // in theory, this is kinda it, but it just don't work right, there must be issues i am not knowing, i cannot master this
  // rotation around z axis - yaw
  //tgt0.x= mlib::cosf(angle.z);
  //tgt0.y= mlib::sinf(angle.z);
  // rotation around x axis - pitch
  //tgt0.y= mlib::cosf(angle.x);
  //tgt0.z= mlib::sinf(angle.x);
  // rotation around y axis - roll
  //tgt0.x= mlib::cosf(angle.y);
  //tgt0.z= mlib::sinf(angle.y);

  // THEREFORE:
  //front.x= mlib::cosf(angleRads.z)* mlib::cosf(angleRads.y);
  //front.y= mlib::sinf(angleRads.z)* mlib::cosf(angleRads.x);
  //front.z= mlib::sinf(angleRads.x)* mlib::sinf(angleRads.y);

  vec3 angleRads(radians(angle.x), radians(angle.y), radians(angle.z));
  float dist= (target-pos).length();

  mat4 rot= mat4().vkRotateZ(angle.z)* mat4().vkRotateX(angle.x); // ROLL IS F-ING THINGS UP* mat4().vkRotateY(angle.y);
  // the initial vector is important, it should represent what the initial pose - should be the top up pose
  front= rot* vec3(0, 0, 1); // the initial vector does it :| if you put 1 on the x, adios.
  //front= rot* vec3(0, 0, 0).normalize(); // the initial vector does it :| if you put 1 on the x, adios.
  front.normalize();
  
  target= (front* dist)+ pos;
}


void ixCamera::computeOrbitFromAngles() {
  vec3 angleRads(radians(angle.x), radians(angle.y), radians(angle.z));
  float dist= (target- pos).length();
  mat4 rot= mat4().vkRotateZ(angle.z)* mat4().vkRotateX(angle.x); // ROLL IS F-ING THINGS UP* mat4().vkRotateY(angle.y);
  // the initial vector is important, it should represent what the initial pose - should be the top up pose
  front= rot* vec3(0, 0, 1);    // the initial vector does it :| if you put 1 on the x, adios.
  front.normalize();
  pos= target- (front* dist);
}

/* THIS COULD BE THE REAL THING, I MIGHT HAVE BUGGED IT ABIT, BUT I DID IT FROM MY HEAD
void ixCamera::lookAtAngles(float pitch_x, float roll_y, float yaw_z) {
  float dist= mlib::absf((pos- target).length());
  front.x= mlib::cosf(radians(yaw_z))* mlib::cosf(radians(pitch_x));
  front.y= mlib::sinf(radians(pitch_x));
  front.z= mlib::sinf(radians(yaw_z))* mlib::cosf(radians(pitch_x));
  front.normalize();
  target= pos+ (front* dist);

  computeViewMat();
}
*/

// orbit around the target on the x axis
void ixCamera::orbitX(float in_angle) {
  angle.x+= in_angle;
  checkAnglesClamp(&angle);
  computeOrbitFromAngles();
}

// orbit around the target on the y axis
void ixCamera::orbitY(float in_angle) {
  angle.y+= in_angle;
  checkAnglesClamp(&angle);
  computeOrbitFromAngles();
}

// orbit around the target on the z axis
void ixCamera::orbitZ(float in_angle) {
  angle.z+= in_angle;
  checkAnglesClamp(&angle);
  computeOrbitFromAngles();
}

// orbit around the target on on all axis
void ixCamera::orbit(const vec3 &in_angles) {
  angle+= in_angles;
  checkAnglesClamp(&angle);
  computeOrbitFromAngles();
}











