#pragma once

using namespace mlib;
class Ix;

class ixCamera {
public:
  
  // camera values

  mat4 cameraMat;                   // camera matrix, based on current values - projMat* worldMat= cameraMat
  mat4 worldMat;                    // part of the cameraMat - world matrix
  mat4 projMat;                     // part of the cameraMat - projection matrix

  uint8 type;                       // 0= orthographic, 1= frustum, 2= perspective
  vec3 pos;                         // perspective and frustum camera position
  vec3 eye;                         // perspective and frustum camera position
  float zClipNear, zClipFar;        // z clipping/bounds, used for every type of camera
  float fov;                        // perspective camera field of view angle
  float aspect;                     // perspective camera aspect
  rectf bounds;                     // orthographic camera bounds

  // funcs

  void setCamera();                 // set camera matrix based on current values (or updated values)

  void setPerspective(const vec3 &in_pos, const vec3 &in_eye, float in_fov, float in_aspect, float in_zClipNear, float in_zClipFar);
  void move(const vec3 &in_pos, const vec3 &in_eye);

  void setOrtho(const rectf &in_bounds, float in_zClipNear, float in_zClipFar);   // ortho is modified to have origin in the virtual desktop 0,0
  void setFrustum(const rectf &in_bounds, float in_zClipNear, float in_zClipFar);
  
  // constructor / destructor

  ixCamera();
  void delData();
  ~ixCamera();

protected:
  friend class Ix;
  class Ix *_parent;
};













