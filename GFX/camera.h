#pragma once

using namespace mlib;
class Ix;

class ixCamera {
public:
  

  vec3 pos;         // camera's position (aka eye position)
  vec3 target;      // camera's target position 
  vec3 front;       // normalized front facing vector

  vec3 angle;       // front angles

  // learnOGL.com names
  //vec3 direction;   // norm(pos- target), z must be negative
  //vec3 upTmp;       // [def: 0, -1, 0]                   points to top, y negative for Vulkan
  //vec3 right;       // [normalize(cross(up, direction))] right vector direction
  //vec3 up;          // [cross(direction, right)] y axis, -y points to top

  //vec3 xAxis;   // [normalize(cross(up, zAxis))] right vector direction; up is 0, -1, 0, negative y for Vulkan
  //vec3 yAxis;   // [norm(cross(zAxis, xAxis))]   -y points to top in Vulkan
  //vec3 zAxis;   // [norm(pos- target)]           z should be negative


  mat4 viewMat;     // view matrix
  mat4 projMat;     // projection matrix
  mat4 camMat;      // proj* view

  void computeOrbitFromAngles();
  void computeFrontFromAngles();
  inline void checkAnglesClamp(vec3 *in_ang) const { if(in_ang->x< 0.01f) in_ang->x=  0.01f;  if(in_ang->x>  179.09f) in_ang->x=  179.09f;
                                                     if(in_ang->y< 0.0f)  in_ang->y+= 360.0f; if(in_ang->y>= 360.0f)  in_ang->y-= 360.0f;
                                                     if(in_ang->z< 0.0f)  in_ang->z+= 360.0f; if(in_ang->z>= 360.0f)  in_ang->z-= 360.0f; }


  void computeViewMat();
  void computeProjMat();
  void computeCamMat();     // proj* view ATM

  inline void computeFront() { front= target- pos; front.normalize(); }
  void computeAngles();

  // camera values

//  mat4 cameraMat;                   // camera matrix, based on current values - projMat* worldMat= cameraMat
//  mat4 worldMat;                    // part of the cameraMat - world matrix
//  mat4 projMat;                     // part of the cameraMat - projection matrix

  uint8 type;                       // 0= orthographic, 1= frustum, 2= perspective


  float zClipNear, zClipFar;        // z clipping/bounds, used for every type of camera
  float fov;                        // perspective camera field of view angle
  float aspect;                     // perspective camera aspect
  rectf bounds;                     // orthographic camera bounds

  // funcs

  void setCamera();                 // set camera matrix based on current values (or updated values)

  void setPerspective(const vec3 &in_pos, const vec3 &in_target, float in_fov, float in_aspect, float in_zClipNear, float in_zClipFar);


  



  void setOrtho(const rectf &in_bounds, float in_zClipNear, float in_zClipFar);   // ortho is modified to have origin in the virtual desktop 0,0
  void setFrustum(const rectf &in_bounds, float in_zClipNear, float in_zClipFar);
  
  // camera movement/placement funcs

  void move(const vec3 &in_pos, const vec3 &in_target);   // moves position and target
  void movePos(const vec3 &in_pos);                       // moves only position
  void moveTarget(const vec3 &in_tgt);                    // moves only target

  void rotateTargetX(float in_angle);       // rotate target around pos (eye) around x axis
  void rotateTargetY(float in_angle);       // rotate target around pos (eye) around y axis
  void rotateTargetZ(float in_angle);       // rotate target around pos (eye) around z axis
  void rotateTarget(const vec3 &in_angles); // rotate target around pos (eye) around all axis

  void lookAtAngles(float pitch_x, float roll_y, float yaw_z);

  void orbitX(float in_angle);        // orbit around the target on the x axis
  void orbitY(float in_angle);        // orbit around the target on the y axis
  void orbitZ(float in_angle);        // orbit around the target on the z axis
  void orbit(const vec3 &in_angles);  // orbit around the target on on all axis

  // extracts viewMatrix's angles
  void extractAngles(float *out_pitchX, float *out_rollY, float *out_yawZ);
  // extracts viewMatrix's forward direction
  inline void extractForward(vec3 *out_frw)  { out_frw->set(-viewMat.v[2], -viewMat.v[6], -viewMat.v[10]); /*out_frw->normalize();*/ }

  // constructor / destructor

  ixCamera();
  void delData();
  ~ixCamera();

protected:
  friend class Ix;
  class Ix *_parent;
};













