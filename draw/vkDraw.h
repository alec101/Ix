//#ifndef IX_LOAD_INLINES
//#ifndef IX_VK_DRAW  // <<< old C way, no pragma
//#define IX_VK_DRAW 1

#pragma once

class vkDraw;

// ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
// █ vkPoint class █████████████████████████████████████████████████████████████████████████
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀
class vkPoint {
public:
  struct PConsts {
    vec4 color;                     // color
    vec4 pos;                       // position
    float size;                     // point size
    // 0x0001= perspective camera
    // 0x0002= ortho camera
    // 0x0004= enable texture
    // 0x0008= disabled color (greyscale)
    int flags;
  } push;
  ixShader *sl;

  // funcs

  inline void flagPersp(bool in_enable)    { if(in_enable) push.flags|= 0x0001; else push.flags&= ~(0x0001); }
  inline void flagOrtho(bool in_enable)    { if(in_enable) push.flags|= 0x0002; else push.flags&= ~(0x0002); }
  inline void flagTexture(bool in_enable)  { if(in_enable) push.flags|= 0x0004; else push.flags&= ~(0x0004); }
  inline void flagDisabled(bool in_enable) { if(in_enable) push.flags|= 0x0008; else push.flags&= ~(0x0008); }

  inline void setPos(float x, float y) { push.pos.set(x, y, 0.0f, 1.0f); }

  inline void cmdPushColor(VkCommandBuffer in_c);
  inline void cmdPushPos(VkCommandBuffer in_c);
  inline void cmdPushSize(VkCommandBuffer in_c);
  inline void cmdPushFlags(VkCommandBuffer in_c);
  inline void cmdPushAll(VkCommandBuffer in_c);
  inline void cmdDraw(VkCommandBuffer in_c);          // 1 vertex

  // constructor / destructor

  vkPoint(Ix *in_ix);

private:
  void _init();
  Ix *_ix;
  vkDraw *parent;
  friend class vkDraw;
};



// ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
// █ vkLine class █████████████████████████████████████████████████████████████████████████
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀
class vkLine {
public:
  // https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
  // distance(p1, p2, (x0, y0))=  |(y2- y1)x0- (x2- x1)y0 + x2y1- y2x1|  /  sqrt((y2- y1)^2 + (x2- x1)^2)
  // for smoothing, gl_FragCoord's position vs distance from line is used to smooth the line
  // MUST set the (line width + 2) if smoothing is enabled, so the smoothed out pixels are actually drawn

  struct PConsts {
    vec4 color;               // color
    vec4 pos[2];              // line start and end point positions
    float size;               // point size
    // 0x0001= perspective camera
    // 0x0002= ortho camera
    // 0x0004= enable texture - NOT FOR LINE, PROBLY NEVER, BUT WHO KNOWS
    // 0x0008= disabled color (greyscale)
    // 0x0010= enable smoothing with alpha - MUST SET vkCmdSetLineWidth(width+ 2), if enabled
    int flags;
  } push;

  ixShader *sl;               // vulkan shader

  // funcs

  inline void flagPersp(bool in_enb)    { if(in_enb) push.flags|= 0x0001; else push.flags&= ~(0x0001); }
  inline void flagOrtho(bool in_enb)    { if(in_enb) push.flags|= 0x0002; else push.flags&= ~(0x0002); }
  inline void flagTexture(bool in_enb)  { if(in_enb) push.flags|= 0x0004; else push.flags&= ~(0x0004); }
  inline void flagDisabled(bool in_enb) { if(in_enb) push.flags|= 0x0008; else push.flags&= ~(0x0008); }
  inline void flagSmooth(bool in_enb)   { if(in_enb) push.flags|= 0x0010; else push.flags&= ~(0x0010); }
  
  inline void setPos(float x1, float y1, float x2, float y2) { push.pos[0].set(x1, y1, 0.0f, 1.0f), push.pos[1].set(x2, y2, 0.0f, 1.0f); }

  inline void cmdUpdateWidth(VkCommandBuffer in_c);  // if smoothing, adds +2 to width
  inline void cmdPushColor(VkCommandBuffer in_c);
  inline void cmdPushPos(VkCommandBuffer in_c);
  inline void cmdPushSize(VkCommandBuffer in_c);
  inline void cmdPushFlags(VkCommandBuffer in_c);
  inline void cmdPushAll(VkCommandBuffer in_c);
  inline void cmdDraw(VkCommandBuffer in_c);          // 2 vertices

  vkLine(Ix *in_ix);

private:
  void _init();

  Ix *_ix;
  vkDraw *parent;
  friend class vkDraw;
};



// ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
// █ vkTriangle class █████████████████████████████████████████████████████████████████████████
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀
class vkTriangle {
public:
  // push constants
  struct alignas(16) PConsts {
    vec4 color;               // color
    vec4 vert[3];             // the 3 vertices (vec4 for padding)
    vec4 tex[3];              // texture coordinates for the 3 vertices (vec4 for padding)
    int flags;                // each byte meaning: 1= persp camera, 2= ortho camera, 3= enable texture, 4= disabled / greyscale
  } push;
  
  ixShader *sl;

  // funcs
  
  inline void flagPersp(bool in_enable)    { if(in_enable) push.flags|= 0x0001; else push.flags&= ~(0x0001); }
  inline void flagOrtho(bool in_enable)    { if(in_enable) push.flags|= 0x0002; else push.flags&= ~(0x0002); }
  inline void flagTexture(bool in_enable)  { if(in_enable) push.flags|= 0x0004; else push.flags&= ~(0x0004); }
  inline void flagDisabled(bool in_enable) { if(in_enable) push.flags|= 0x0008; else push.flags&= ~(0x0008); }

  inline void setPos(uint i, float x, float y, float z= 0.0f, float w= 1.0f) { push.vert[i].set(x, y, z, w); }
  inline void setTex(uint i, float x, float y, float z= 0.0f)                { push.tex[i].set(x, y, z, 0.0f); }

  inline void cmdTexture(VkCommandBuffer in_cmd, ixTexture *in_t); // if null, will bind vki.noTexture; updates flag; NO push
  inline void cmdPushColor(VkCommandBuffer in_c);
  inline void cmdPushPos(VkCommandBuffer in_c);
  inline void cmdPushTex(VkCommandBuffer in_c);
  inline void cmdPushFlags(VkCommandBuffer in_c);
  inline void cmdPushAll(VkCommandBuffer in_c);
  inline void cmdDraw(VkCommandBuffer in_c);      // 3 vertices

  // constructor/destructor

  vkTriangle(Ix *in_ix);

private:
  void _init();
  Ix *_ix;
  vkDraw *parent;
  friend class vkDraw;
};



// ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
// █ vkQuad class █████████████████████████████████████████████████████████████████████████
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀
class vkQuad {
public:

  struct PConsts {
    vec4 color;                     // color
    float x0, y0, z;                // starting point x0, y0, z0
    float xe, ye;                   // ending point
    float tx0, ty0, tDepth;         // tex coords start (z= depth)
    float txe, tye;                 // tex coords end
    float hollow;                   // negative= no hollowing;
    // 0x0001= perspective camera
    // 0x0002= ortho camera
    // 0x0004= enable texture
    // 0x0008= disabled color (greyscale)
    int flags;                      // each byte meaning: 1= persp camera, 2= ortho camera, 3= enable texture, 4= disabled/ greyscale
  } push;

  ixShader *sl;

  // funcs

  inline void flagPersp(bool in_enable)    { if(in_enable) push.flags|= 0x0001; else push.flags&= ~(0x0001); }
  inline void flagOrtho(bool in_enable)    { if(in_enable) push.flags|= 0x0002; else push.flags&= ~(0x0002); }
  inline void flagTexture(bool in_enable)  { if(in_enable) push.flags|= 0x0004; else push.flags&= ~(0x0004); }
  inline void flagDisabled(bool in_enable) { if(in_enable) push.flags|= 0x0008; else push.flags&= ~(0x0008); }

  inline void setPos(float x0, float y0, float z, float xe, float ye) { push.x0= x0,   push.y0= y0,   push.z= z,     push.xe= xe,    push.ye= ye; }
  inline void setPosD(float x, float y, float z, float dx, float dy)  { push.x0= x,    push.y0= y,    push.z= z,     push.xe= x+ dx, push.ye= y+ dy; }
  inline void setPosR(const rectf &r)                                 { push.x0= r.x0, push.y0= r.y0, push.z= 0.0f,  push.xe= r.xe, push.ye= r.ye; }
  inline void setPosDi(int32 x, int32 y, int32 z, int32 dx, int32 dy) { push.x0= (float)x,       push.y0= (float)y,       push.z= (float)z, push.xe= (float)(x+ dx), push.ye= (float)(y+ dy); }
  inline void setPosRi(const recti &in_r)                             { push.x0= (float)in_r.x0, push.y0= (float)in_r.y0, push.z= 0.0f,     push.xe= (float)in_r.xe, push.ye= (float)in_r.ye; }
  
  inline void setTex(float x0, float y0, float xe, float ye, float depth= 0.0f) { push.tx0= x0, push.ty0= y0, push.txe= xe, push.tye= ye, push.tDepth= depth; }

  //inline void cmdBindPipeline(VkCommandBuffer in_cmd);
  inline void cmdTexture(VkCommandBuffer in_cmd, ixTexture *in_t); // if null, will bind vki.noTexture. Updates flag. No push
  inline void cmdPushColor(VkCommandBuffer in_c);
  inline void cmdPushPos(VkCommandBuffer in_c);
  inline void cmdPushTex(VkCommandBuffer in_c);
  inline void cmdPushHollow(VkCommandBuffer in_c);
  inline void cmdPushFlags(VkCommandBuffer in_c);
  inline void cmdPushAll(VkCommandBuffer in_c);
  inline void cmdDraw(VkCommandBuffer in_c);        // 4 vertices


  // constructors / destructors

  vkQuad(Ix *in_ix);

private:
  void _init();
  Ix *_ix;
  vkDraw *parent;
  friend class vkDraw;
};



// ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
// █ vkCircle class █████████████████████████████████████████████████████████████████████████
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀
class vkCircle {
public:

    struct PConsts {
    vec4 color;                     // color
    float x, y, z, radius;          // position, radius
    //float xe, ye;                   // ending position
    float tx0, ty0, tDepth;         // tex coords start (z= depth)
    float txe, tye;                 // tex coords end
    float hollow;                   // negative= no hollowing;
    // 0x0001= perspective camera
    // 0x0002= ortho camera
    // 0x0004= enable texture
    // 0x0008= disabled color (greyscale)
    int flags;                      // each byte meaning: 1= persp camera, 2= ortho camera, 3= enable texture, 4= disabled/ greyscale
  } push;

  ixShader *sl;

  // funcs

  inline void flagPersp(bool in_enable)    { if(in_enable) push.flags|= 0x0001; else push.flags&= ~(0x0001); }
  inline void flagOrtho(bool in_enable)    { if(in_enable) push.flags|= 0x0002; else push.flags&= ~(0x0002); }
  inline void flagTexture(bool in_enable)  { if(in_enable) push.flags|= 0x0004; else push.flags&= ~(0x0004); }
  inline void flagDisabled(bool in_enable) { if(in_enable) push.flags|= 0x0008; else push.flags&= ~(0x0008); }

  inline void setPos(float x, float y, float z, float radius) { push.x= x, push.y= y, push.z= z, push.radius= radius; }
  inline void setPosD(float x0, float y0, float z, float dx, float dy) { push.radius= MIN(dx, dy)/ 2.0f, push.x= x0+ push.radius, push.y= y0+ push.radius, push.z= z; }
  inline void setPosR(const recti &in_r, float in_z= 0.0f) { push.radius= (float)(MIN(in_r.dx, in_r.dy))/ 2.0f, push.x= (float)in_r.x0+ push.radius, push.y= (float)in_r.y0+ push.radius, push.z= in_z; }
  inline void setTex(float x0, float y0, float xe, float ye, float depth= 0.0f) { push.tx0= x0, push.ty0= y0, push.txe= xe, push.tye= ye, push.tDepth= depth; }

  inline void cmdTexture(VkCommandBuffer in_cmd, ixTexture *in_t); // if null, will bind _ix.vki.noTexture, and update the flag. No push
  inline void cmdPushColor(VkCommandBuffer in_c);
  inline void cmdPushPos(VkCommandBuffer in_c);
  inline void cmdPushTex(VkCommandBuffer in_c);
  inline void cmdPushHollow(VkCommandBuffer in_c);
  inline void cmdPushFlags(VkCommandBuffer in_c);
  inline void cmdPushAll(VkCommandBuffer in_c);
  inline void cmdDraw(VkCommandBuffer in_c);        // 4 vertices

  // constructor / destructor

  vkCircle(Ix *in_ix);

private:
  void _init();
  Ix *_ix;
  vkDraw *parent;
  friend class vkDraw;
};









// ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
// █ vkDRAW class █████████████████████████████████████████████████████████████████████████
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀

class vkDraw {
public:
  Ix *_ix;

  vkPoint    point;
  vkLine     line;
  vkTriangle triangle;
  vkQuad     quad;
  vkCircle   circle;

  void init();
  //void bindGlbBuffer(VkCommandBuffer in_cmd);

  vkDraw(Ix *in_ix);
  ~vkDraw();
  void delData();
};


/*
#endif /// IX_VK_DRAW

#else // defined IX_LOAD_INLINES - define inline funcs
#ifndef IX_VK_DRAW_INLINES
#define IX_VK_DRAW_INLINES 1
// POINT

void vkPoint::cmdPushColor(VkCommandBuffer in_c) { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, color), 16, &push.color); }
void vkPoint::cmdPushPos(VkCommandBuffer in_c)   { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, pos),   16, &push.pos); }
void vkPoint::cmdPushSize(VkCommandBuffer in_c)  { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, size),   4, &push.size); }
void vkPoint::cmdPushFlags(VkCommandBuffer in_c) { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, flags),  4, &push.flags); }
void vkPoint::cmdPushAll(VkCommandBuffer in_c)   { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push); }
void vkPoint::cmdDraw(VkCommandBuffer in_c) { _ix->vk.CmdDraw(in_c, 1, 1, 0, 0); }

// LINE

void vkLine::cmdUpdateWidth(VkCommandBuffer in_c) { _ix->vk.CmdSetLineWidth(in_c, push.size+ ((push.flags& 0x0008)? 2.0f: 0.0f)); }
void vkLine::cmdPushColor(VkCommandBuffer in_c) { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, color),  16,    &push.color); }
void vkLine::cmdPushPos(VkCommandBuffer in_c)   { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, pos[0]), 16* 2, &push.pos[0]); }
void vkLine::cmdPushSize(VkCommandBuffer in_c)  { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, size),   4,     &push.size); }
void vkLine::cmdPushFlags(VkCommandBuffer in_c) { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, flags),  4,     &push.flags); }
void vkLine::cmdPushAll(VkCommandBuffer in_c)   { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push); }
void vkLine::cmdDraw(VkCommandBuffer in_c) { _ix->vk.CmdDraw(in_c, 2, 1, 0, 0); }

// TRIANGLE

void vkTriangle::cmdTexture(VkCommandBuffer in_cmd, ixTexture *in_t) {
  if(!in_t) in_t= _ix->vki.noTexture;
  _ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sl->vk->pipelineLayout, 1, 1, &in_t->vkData.set->set, 0, null);
}
void vkTriangle::cmdPushColor(VkCommandBuffer in_c) { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, color),   16,    &push.color); }
void vkTriangle::cmdPushPos(VkCommandBuffer in_c)   { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, vert[0]), 16* 3, &push.vert[0]); }
void vkTriangle::cmdPushTex(VkCommandBuffer in_c)   { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, tex[0]),  16* 3, &push.tex[0]); }
void vkTriangle::cmdPushFlags(VkCommandBuffer in_c) { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, flags),   4,     &push.flags); }
void vkTriangle::cmdPushAll(VkCommandBuffer in_c)   { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push); }
void vkTriangle::cmdDraw(VkCommandBuffer in_c) { _ix->vk.CmdDraw(in_c, 3, 1, 0, 0); }

// QUAD

void vkQuad::cmdTexture(VkCommandBuffer in_cmd, ixTexture *in_t) {
  if(!in_t) in_t= _ix->vki.noTexture;
  _ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sl->vk->pipelineLayout, 1, 1, &in_t->vkData.set->set, 0, null);
}
void vkQuad::cmdPushColor(VkCommandBuffer in_c)  { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, color), 16, &push.color); }
void vkQuad::cmdPushPos(VkCommandBuffer in_c)    { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, x0),    20, &push.x0); }
void vkQuad::cmdPushTex(VkCommandBuffer in_c)    { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, tx0),   20, &push.tx0); }
void vkQuad::cmdPushHollow(VkCommandBuffer in_c) { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, hollow), 4, &push.hollow); }
void vkQuad::cmdPushFlags(VkCommandBuffer in_c)  { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, flags),  4, &push.flags); }
void vkQuad::cmdPushAll(VkCommandBuffer in_c)    { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push); }
void vkQuad::cmdDraw(VkCommandBuffer in_c) { _ix->vk.CmdDraw(in_c, 4, 1, 0, 0); }

// CIRCLE

inline void vkCircle::cmdTexture(VkCommandBuffer in_cmd, ixTexture *in_t) {
  if(!in_t) in_t= _ix->vki.noTexture;
  _ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sl->vk->pipelineLayout, 1, 1, &in_t->vkData.set->set, 0, null);
}
inline void vkCircle::cmdPushColor(VkCommandBuffer in_c)   { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, color), 16, &push.color); }
inline void vkCircle::cmdPushPos(VkCommandBuffer in_c)     { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, x),     16, &push.x); }
inline void vkCircle::cmdPushTex(VkCommandBuffer in_c)     { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, tx0),   20, &push.tx0); }
inline void vkCircle::cmdPushHollow(VkCommandBuffer in_c)  { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, hollow), 4, &push.hollow); }
inline void vkCircle::cmdPushFlags(VkCommandBuffer in_c)   { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, flags),  4, &push.flags); }
inline void vkCircle::cmdPushAll(VkCommandBuffer in_c)     { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push); }
inline void vkCircle::cmdDraw(VkCommandBuffer in_c) { _ix->vk.CmdDraw(in_c, 4, 1, 0, 0); }
#endif /// defined IX_VK_DRAW_INLINES
#endif /// IX_LOAD_INLINES

*/















