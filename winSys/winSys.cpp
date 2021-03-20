#include "ix/ix.h"



/*
TODO:
 - focus handling funcs - when a window is destroyed, focus must not point to it anymore for example
 - usage class change: usage should be a pointer to the class, and allocated, once.
                       before calling baseclass constructor, allocate it maybe. baseclass checks for null, if it is, allocates.
                       or, make a func that handles it, checking _type, maybe, etc. <<<< MIGHT BE THE WINNER. (a static func, part of usage, private)
 - is class change: exactly like usage.
*/




///===================================///
// standard CONSTRUCTORS / DESTRUCTORS //
///===================================///

ixWinSys::ixWinSys() {
  selStyle= null;
  _op.delData();
  //delShaders();
  focus= null;
  hover= null;
  //kbFocus= joyFocus= null;
}


ixWinSys::~ixWinSys() {
  delData();
}


void ixWinSys::delData() {
  /// delete all constructed objects
  while(topObjects.first)
    topObjects.del(topObjects.first);  
  _op.delData();
}

/*
void ixWinSys::loadAssets(Ix *in_ix) {
  #ifdef IX_USE_OPENGL
  if(!in_ix->glIsActive()) { error.simple("ogl renderer not active"); return; }
  #endif
  loadShader(in_ix);
}
*/











///===============================///
// CREATE for all types of objects //
///===============================///


ixWindow *ixWinSys::createWindow(cchar *in_title, ixBaseWindow *in_parent, int32 in_x, int32 in_y, int32 in_dx, int32 in_dy) {
  ixWindow *w= new ixWindow;

  /// parent / children chain
  if(in_parent)
    in_parent->childrens.add(w);
  else
    Ix::wsys().topObjects.add(w);
  w->parent= in_parent;

  /// default vars
  w->style= &selStyle->window;
  w->_applyColorsFromStyle();
  w->is.visible= true;

  /// position and hook
  w->setPos(in_x, in_y, in_dx, in_dy);
  w->hook.setAnchor(in_parent);            /// sets parent + initial hook
  

  /// window title
  if(((ixWSwindowStyle *)w->style)->useTitle) {
    //w->title->text.font= *Ix::getMain()->pr.style;
    //w->title->setFont(Ix::getMain()->pr.style->selFont);

    w->setTitle(in_title, &selStyle->title);
    //w->title->pos.dx;
  }

  /// scrollbars
  w->_createScrollbars();

  if(in_parent) {
    in_parent->_computeChildArea();
    in_parent->hook.updateHooks();
  }

  return w;
}


ixButton *ixWinSys::createButton(cchar *in_text, ixBaseWindow *in_parent, int32 in_x, int32 in_y, int32 in_dx, int32 in_dy) {
  ixButton *b= new ixButton;

  /// parent / children chain
  if(in_parent)
    in_parent->childrens.add(b);
  else
    topObjects.add(b);
  b->parent= in_parent;

  /// default vars
  b->style= &selStyle->button;
  b->stylePressed= &selStyle->buttonPressed;
  b->_applyColorsFromStyle();
  b->font= *Ix::getMain()->pr.style;
  b->is.visible= true;

  /// position / hook
  b->hook.setAnchor(in_parent);
  b->setPos(in_x, in_y, in_dx, in_dy);

  b->setTextCentered(in_text);


  if(in_parent)
    in_parent->_computeChildArea();

  return b;
}


ixEdit *ixWinSys::createEdit(ixBaseWindow *in_parent, int32 in_x0, int32 in_y0, int32 in_dx, int32 in_dy) {
  ixEdit *e= new ixEdit;

  /// parent / children chain
  if(in_parent)
    in_parent->childrens.add(e);
  else
    topObjects.add(e);
  e->parent= in_parent;

  /// default vars
  e->style= &selStyle->edit;
  e->_applyColorsFromStyle();
  e->text.font= *(Ix::getMain()->pr.style);
  e->is.visible= true;

  /// position / hook
  e->setPos(in_x0, in_y0, in_dx, in_dy);
  e->hook.setAnchor(in_parent);            /// sets parent, initial hook
  e->_createScrollbars();

  if(in_parent)
    in_parent->_computeChildArea();

  return e;
}


ixStaticText *ixWinSys::createStaticText(ixBaseWindow *in_parent, int32 in_x0, int32 in_y0, int32 in_dx, int32 in_dy) {
  ixStaticText *t= new ixStaticText;

  /// parent / children chain
  if(in_parent)
    in_parent->childrens.add(t);
  else
    topObjects.add(t);
  t->parent= in_parent;

  /// default vars
  t->style= &selStyle->text;
  t->_applyColorsFromStyle();
  t->setFont(Ix::getMain()->pr.style->selFont);

  //t->text.font= *(Ix::getMain()->pr.style);
  t->is.visible= true;

  /// position / hook
  t->setPos(in_x0, in_y0, in_dx, in_dy);
  t->hook.setAnchor(in_parent);
  t->_createScrollbars();

  if(in_parent)
    in_parent->_computeChildArea();

  return t;
}


ixRadioButton *ixWinSys::createRadioButton(ixBaseWindow *in_parent, int32 in_x0, int32 in_y0, int32 in_dx, int32 in_dy) {
  ixRadioButton *b= new ixRadioButton;

  /// parent / children chain
  if(in_parent)
    in_parent->childrens.add(b);
  else
    topObjects.add(b);
  b->parent= in_parent;

  /// default vars
  //b->style= selStyle->MAKEME
  b->font= *Ix::getMain()->pr.style;
  b->is.visible= true;

  b->buttonDx= in_dx;
  b->buttonDy= in_dy;
  


  /// position / hook
  b->setPos(in_x0, in_y0, 0, 0);
  b->hook.setAnchor(in_parent);            /// sets parent, initial hook

  if(in_parent)
    in_parent->_computeChildArea();

  return b;
}


ixDropList *ixWinSys::createDropList(ixBaseWindow *in_parent, int32 in_x0, int32 in_y0, int32 in_buttonDx, int32 in_buttonDy) {

  ixDropList *d= new ixDropList;

  /// parent / children chain
  if(in_parent)
    in_parent->childrens.add(d);
  else
    topObjects.add(d);
  d->parent= in_parent;

  /// default vars
  //d->style= selStyle->MAKEME
  d->font= *Ix::getMain()->pr.style;
  d->is.visible= true;

  d->buttonDx= in_buttonDx;
  d->buttonDy= in_buttonDy;
  


  /// position / hook
  d->setPos(in_x0, in_y0, 0, 0);
  d->hook.setAnchor(in_parent);            /// sets parent, initial hook

  if(in_parent)
    in_parent->_computeChildArea();

  return d;
}


ixProgressBar *ixWinSys::createProgressBar(ixBaseWindow *in_parent, int32 in_x0, int32 in_y0, int32 in_dx, int32 in_dy) {
  ixProgressBar *p= new ixProgressBar;

  /// parent / children chain
  if(in_parent)
    in_parent->childrens.add(p);
  else
    topObjects.add(p);
  p->parent= in_parent;

  /// default vars
  //p->style= selStyle->MAKEME
  p->font= *Ix::getMain()->pr.style;
  p->is.visible= true;
  
  
  /// position / hook
  p->setPos(in_x0, in_y0, in_dx, in_dy);
  p->hook.setAnchor(in_parent);            /// sets parent, initial hook

  if(in_parent)
    in_parent->_computeChildArea();

  return p;
}


ixMenu *ixWinSys::createMenu(ixBaseWindow *in_parent) {
  ixMenu *m= new ixMenu;

    /// parent / children chain
  if(in_parent)
    in_parent->childrens.add(m);
  else
    topObjects.add(m);
  m->parent= in_parent;


  /// default vars
  //m->style= selStyle->MAKEME
  m->font= *Ix::getMain()->pr.style;
  m->setVisible(false); // ?
  
  /// position / hook
  //m->setPos(in_x0, in_y0, in_dx, in_dy);
  m->hook.setAnchor(in_parent);            /// sets parent, initial hook

  if(in_parent)
    in_parent->_computeChildArea();

  return m;
}


ixMenuBar *ixWinSys::createMenuBar(ixBaseWindow *in_parent) {
  ixMenuBar *m= new ixMenuBar;

    /// parent / children chain
  if(in_parent)
    in_parent->childrens.add(m);
  else
    topObjects.add(m);
  m->parent= in_parent;


  /// default vars
  //m->style= selStyle->MAKEME
  m->font= *Ix::getMain()->pr.style;
  m->is.visible= true;
  m->setVisible(true);
  
  /// position / hook
  //m->setPos(in_x0, in_y0, in_dx, in_dy);
  m->hook.setAnchor(in_parent);            /// sets parent, initial hook

  if(in_parent)
    in_parent->_computeChildArea();

  return m;
}













///=====///
// funcs //
///=====///


/// draws all windows, on all engines
void ixWinSys::draw() {
  
  for(Ix *i= (Ix *)Ix::ixList().first; i; i= (Ix *)i->next) {
    ixFontStyle *saveStyle= i->pr.style;
    i->pr.style= null;

    //i->pr.saveStyle();

    #ifdef IX_USE_OPENGL
    if(i->renOpenGL()) {
      ixBaseWindow::_glDrawInit(i);
      for(ixBaseWindow *w= (ixBaseWindow *)topObjects.last; w; w= (ixBaseWindow *)w->prev)
        w->_glDraw(i);
      ixBaseWindow::_glDrawFinish(i);
    }
    #endif  

    #ifdef IX_USE_VULKAN
    if(i->renVulkan()) {
      VkCommandBuffer cmd= *i->vki.ortho.cmd[i->vki.fi];
      ixBaseWindow::_vkDrawInit(cmd, i);
      for(ixBaseWindow *w= (ixBaseWindow *)topObjects.last; w; w= (ixBaseWindow *)w->prev)
        w->_vkDraw(cmd, i);
      ixBaseWindow::_vkDrawFinish(cmd, i);
    }
    #endif
    i->pr.style= saveStyle;
    //i->pr.restoreStyle();
  }
}


/// draws all the top objects on specified Ix engine
void ixWinSys::drawSpecific(Ix *in_ix) {
  #ifdef IX_USE_OPENGL
  if(in_ix->renOpenGL()) {
    ixBaseWindow::_glDrawInit(in_ix);
    for(ixBaseWindow *p= (ixBaseWindow *)topObjects.last; p; p= (ixBaseWindow *)p->prev)
      p->_glDraw(in_ix);
    ixBaseWindow::_glDrawFinish(in_ix);
  }
  #endif

  #ifdef IX_USE_VULKAN
  if(in_ix->renVulkan()) {
    VkCommandBuffer cmd= *in_ix->vki.ortho.cmd[in_ix->vki.fi]; // only ortho atm

    ixBaseWindow::_vkDrawInit(cmd, in_ix);
    for(ixBaseWindow *p= (ixBaseWindow *)topObjects.last; p; p= (ixBaseWindow *)p->prev)
      p->_vkDraw(cmd, in_ix);
    ixBaseWindow::_vkDrawFinish(cmd, in_ix);
  }
  #endif
}






/// updates all windows
void ixWinSys::update() {
  if(osi.flags.windowMoved || osi.flags.windowResized)
    for(ixBaseWindow *p= (ixBaseWindow *)topObjects.first; p; p= (ixBaseWindow *)p->next) {
      p->hook.updateHooks();
      p->_computeAll();
      //if(p->hscroll) { p->hscroll->_computePos(); p->hscroll->_computeButtons(); }
      //if(p->vscroll) { p->vscroll->_computePos(); p->vscroll->_computeButtons(); }
      p->update();
    }

  else 
    for(ixBaseWindow *p= (ixBaseWindow *)topObjects.first; p; p= (ixBaseWindow *)p->next)
      p->update();

  eventSys._update();
}


/// updates all top objects and their children hooks (in case hooked target moved)
void ixWinSys::updateHooks() {
  for(ixBaseWindow *p= (ixBaseWindow *)topObjects.first; p; p= (ixBaseWindow *)p->next)
    p->hook.updateHooks();
}

















// brings window to the top in the chainlist it's from (the parent chainlist); it will be drawn last, updated first
void ixWinSys::bringToFront(ixBaseWindow *in_w) {
  focus= in_w;
  // it has a parent
  if(in_w->parent) {
    in_w->parent->childrens.release(in_w);
    in_w->parent->childrens.addFirst(in_w);

  // it is a top window
  } else {
    topObjects.release(in_w);
    topObjects.addFirst(in_w);
  }
}


void ixWinSys::bringToBack(ixBaseWindow *in_w) {
  // it has a parent
  if(in_w->parent) {
    in_w->parent->childrens.release(in_w);
    in_w->parent->childrens.add(in_w);

  // it is a top window
  } else {
    topObjects.release(in_w);
    topObjects.add(in_w);
  }
}


void ixWinSys::bringAfter(ixBaseWindow *in_w, ixBaseWindow *in_after) {
  // it has a parent
  if(in_w->parent) {
    in_w->parent->childrens.release(in_w);
    in_w->parent->childrens.addAfter(in_w, in_after);

  // it is a top window
  } else {
    topObjects.release(in_w);
    topObjects.addAfter(in_w, in_after);
  }
}


void ixWinSys::bringBefore(ixBaseWindow *in_w, ixBaseWindow *in_before) {
  // it has a parent
  if(in_w->parent) {
    in_w->parent->childrens.release(in_w);
    in_w->parent->childrens.addBefore(in_w, in_before);

  // it is a top window
  } else {
    topObjects.release(in_w);
    topObjects.addBefore(in_w, in_before);
  }
}


void ixWinSys::switchParent(ixBaseWindow *in_w, ixBaseWindow *in_p) {
  if(in_w->parent)
    in_w->parent->childrens.release(in_w);
  else
    topObjects.release(in_w);

  if(in_p)
    in_p->childrens.add(in_w);
  else
    topObjects.add(in_w);
}


void ixWinSys::delWindow(ixBaseWindow *w) {
  /// an ongoing operation is tied to this window
  if(_op.win== w)
    _op.delData();

  if(focus== w)
    focus= null;
  
  if(w->parent)
    w->parent->childrens.del(w);
  else
    topObjects.del(w);
}


void ixWinSys::loadDef1Style() {
  Ix::glb.def1Style().loadStyle("defWinTex.txt");
  /* OLD WAY (FIRST WAY) TO CREATE A STYLE
  def1Style.loadTex("defWinTex.tga");

  // WINDOW STYLE =============---------------
  def1Style.window.useBackColor= true;
  def1Style.window.bTexBG= false;
    
  /// borders
  def1Style.window.setBRDcoords(0, 2,  15, 10, 9);
  def1Style.window.setBRDcoords(1, 14, 14, 9,  10);
  def1Style.window.setBRDcoords(2, 25, 15, 10, 9);
  def1Style.window.setBRDcoords(3, 37, 14, 9,  10);

  /// corners
  def1Style.window.setBRDcoords(4, 2,  77, 49, 49);
  def1Style.window.setBRDcoords(5, 53, 77, 49, 49);
  def1Style.window.setBRDcoords(6, 2,  26, 49, 49);
  def1Style.window.setBRDcoords(7, 53, 26, 49, 49);
  def1Style.window.setBRDallWrap(2);                 /// repeat wrap

  /// borders have a 1 pixel dist value
  def1Style.window.setBRDdist(0, 1.0f);
  def1Style.window.setBRDdist(1, 1.0f);
  def1Style.window.setBRDdist(2, 1.0f);
  def1Style.window.setBRDdist(3, 1.0f);

  /// corners have a 4 pixel dist value
  def1Style.window.setBRDdist(4, 5.0f);
  def1Style.window.setBRDdist(5, 5.0f);
  def1Style.window.setBRDdist(6, 5.0f);
  def1Style.window.setBRDdist(7, 5.0f);


  // TITLE STYLE =============---------------
  def1Style.title.useTexture= true;
  def1Style.title.useBackColor= true;
  def1Style.title.bTexBG= false;

  def1Style.title.setBRDcoords(0, 106, 57, 4, 5);
  def1Style.title.setBRDcoords(1, 106, 64, 20, 30);
  def1Style.title.setBRDcoords(2, 112, 57, 4, 5);
  def1Style.title.setBRDcoords(3, 106, 96, 20, 30);

  def1Style.title.setBRDallWrap(1);
  def1Style.title.setBRDdist(1, 17);
  def1Style.title.setBRDdist(3, 17);

  // BUTTON STYLE ============----------------
  def1Style.button.useTexture= true;
  def1Style.button.useBackColor= true;
  def1Style.button.bTexBG= false;
  def1Style.button.colorBG.set(.3f, .3f, .3f, 1); /// opaque

  def1Style.button.setBRDcoords(0, 49, 20, 2, 4);
  def1Style.button.setBRDcoords(1, 54, 21, 4, 2);
  def1Style.button.setBRDcoords(2, 55, 20, 2, 4);
  def1Style.button.setBRDcoords(3, 48, 21, 4, 2);
  def1Style.button.setBRDcoords(4, 60, 20, 4, 4);
  def1Style.button.setBRDcoords(5, 66, 20, 4, 4);
  def1Style.button.setBRDcoords(6, 72, 20, 4, 4);
  def1Style.button.setBRDcoords(7, 78, 20, 4, 4);
  def1Style.button.setBRDallDist(4);    // borders are on the outside... there should be an inside variant, but there has to be a wrap BETWEEN corners options on the baseObject
  def1Style.button.setBRDallWrap(1);   /// stretch

  // EDIT STYLE ==============----------------
  def1Style.edit.useTexture= true;
  def1Style.edit.useBackColor= true;
  def1Style.edit.bTexBG= false;

  def1Style.edit.setBRDcoords(0, 49, 15, 1, 3);
  def1Style.edit.setBRDcoords(1, 53, 16, 3, 1);
  def1Style.edit.setBRDcoords(2, 59, 15, 1, 3);
  def1Style.edit.setBRDcoords(3, 63, 16, 3, 1);
  def1Style.edit.setBRDcoords(4, 68, 15, 3, 3);
  def1Style.edit.setBRDcoords(5, 73, 15, 3, 3);
  def1Style.edit.setBRDcoords(6, 78, 15, 3, 3);
  def1Style.edit.setBRDcoords(7, 83, 15, 3, 3);

  // WRAP BETWEEN CORNERS NEEDED HERE TOO
  def1Style.edit.setBRDallWrap(1);   /// stretch

  // STATIC STYLE ===========-----------------
  */


  // set to def1Style as the active style
  selStyle= &Ix::glb.def1Style();
}



// style ASSETS ==========---------- //















/*

///======================================================================///
// Window System SHADER =======================-------------------------- //
///======================================================================///

ixWSshader::ixWSshader(Ix *in_ix): ixShader(in_ix) {
  #ifdef IX_USE_OPENGL
  u_camera= u_color= u_viewportPos= u_origin= u_useTexture= u_disabled= -1; 
  u_customPos= u_customTex= u_quadPos0= u_quadPosE= u_quadTex0= u_quadTexE= -1;
  #endif
}

#ifdef IX_USE_OPENGL
void ixWSshader::glInitUniforms() {
  glUseProgram(gl->id);
  u_camera=       glGetUniformLocation(gl->id, "u_camera");
  u_color=        glGetUniformLocation(gl->id, "u_color");
  u_viewportPos=  glGetUniformLocation(gl->id, "u_viewportPos");
  u_origin=       glGetUniformLocation(gl->id, "u_origin");
  u_useTexture=   glGetUniformLocation(gl->id, "u_useTexture");
  u_disabled=     glGetUniformLocation(gl->id, "u_disabled");
  u_customPos=    glGetUniformLocation(gl->id, "u_customPos");
  u_quadPos0=     glGetUniformLocation(gl->id, "u_quadPos0");
  u_quadPosE=     glGetUniformLocation(gl->id, "u_quadPosE");
  u_customTex=    glGetUniformLocation(gl->id, "u_customTex");
  u_quadTex0=     glGetUniformLocation(gl->id, "u_quadTex0");
  u_quadTexE=     glGetUniformLocation(gl->id, "u_quadTexE");

  glUniform4f(u_color, 1.0f, 1.0f, 1.0f, 1.0f);
  glUniform2f(u_viewportPos, 0.0f, 0.0f);
}
#endif


#ifdef IX_USE_OPENGL
void ixWSshader::glUpdateViewportPos() {
  glUniform2f(u_viewportPos, (float)_ix->win->x0, (float)_ix->win->y0);
}
#endif /// IX_USE_OPENGL



ixWSshader *ixWinSys::loadShader(Ix *in_ix) {
  _SList *p= _getSList(in_ix);
  if(p) { error.detail("Window shader already loaded", __FUNCTION__); return p->sl; }

  p= new _SList;
  p->ix= in_ix;

  p->sl= new ixWSshader(in_ix);
    
  #ifdef IX_USE_OPENGL
  if(in_ix->renOpenGL()) {
    p->sl->gl->load(Ix::Config::shaderDIR+ "ixWindowsV.glsl", Ix::Config::shaderDIR+ "ixWindowsF.glsl");
    p->sl->gl->build();

    glUseProgram(p->sl->gl->id);
    p->sl->glInitUniforms();
    glUniform1i(p->sl->u_customPos, 0);   // default var
    glUniform1i(p->sl->u_customTex, 0);   // default var
    p->sl->glUpdateViewportPos(); //glUniform2f(p->sl->u_viewportPos, in_ix->win->x0, in_ix->win->y0);
  }
  #endif

  #ifdef IX_USE_VULKAN
  if(in_ix->renVulkan()) {
      
    p->sl->vk->loadModuleVert(Ix::Config::shaderDIR+ "ixWindows.vert.spv");
    p->sl->vk->loadModuleFrag(Ix::Config::shaderDIR+ "ixWindows.frag.spv");

    p->sl->vk->addDescriptorSetFromExisting(in_ix->vki.glb[in_ix->vki.fi]->layout); /// set 0, binding 0 - global buffer
    p->sl->vk->addDescriptorSetFromExisting(in_ix->texSys.vkData.staticLayout);     /// set 1, binding 0 - texture
  
    p->sl->vk->setInputAssembly(VK_PRIMITIVE_TOPOLOGY_POINT_LIST, VK_FALSE);
    p->sl->vk->setFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE);
    p->sl->vk->setCullModeFlags(VK_CULL_MODE_BACK_BIT);
    //sl->vk->setCullModeFlags(VK_CULL_MODE_NONE);

    p->sl->vk->addPushConsts(sizeof(ixWSshader::PConsts), 0, VK_SHADER_STAGE_ALL);
    p->sl->vk->setRenderPass(*in_ix->vki.render.handle);
    p->sl->vk->setDynamicViewports(1);

    p->sl->vk->addColorBlendAttachement(true, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
                                              VK_BLEND_FACTOR_ONE,       VK_BLEND_FACTOR_ZERO,                VK_BLEND_OP_ADD,
                                              VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);

    if(!p->sl->vk->build()) { error.window(in_ix->vk.errorText, __FUNCTION__); delete p; return null; }
  }
  #endif

  _sList.add(p);
  return p->sl;
}




ixWSshader *ixWinSys::getShader(Ix *in_ix) {
  for(_SList *p= (_SList *)_sList.first; p; p= (_SList *)p->next)
    if(p->ix== in_ix)
      return p->sl;
  return null;
}


void ixWinSys::delShader(Ix *in_ix) {
  ixWSshader *p= getShader(in_ix);
  if(!p) return;

  in_ix->shaders.delShader(p);

  _SList *s= _getSList(in_ix);
  _sList.del(s);
}


void ixWinSys:: delShaders() {
  while(_sList.first) {
    _SList *p= (_SList *)_sList.first;
    delShader(p->ix);
    _sList.del(p);
  }
}








ixWinSys::_SList *ixWinSys::_getSList(Ix *in_i) {
  for(_SList *p= (_SList *)_sList.first; p; p= (_SList *)p->next)
    if(p->ix== in_i) return p;

  return null;
}

*/





