#include "ix/ix.h"



/*
TODO:

 >>>   the scale can be put directly in the orho matrix; <<<

 - PRINT SCALING A MUST <<<<<<<<<<<<<<<<<<<
   you could have a target size, original, and scale based on that; 
   the best way would be to have multiple font sizes too
 - an UI schema? it would be a huge wrapper basically, maybe it's going too far. It seems that the position origin would solve the re-applying of the scale
 - create menu/menubar don't have style / original position(this last one might not be needed)






 SCALING SYSTEM:
  -ON FULL SCREEN MODE (borderless, don't care), multiple monitors, ix on each monitor:
   there can be a system that draws windows bigger or smaller, based on the current monitor and what the UI target is
   i thout about it and it CAN be done, but this would work only in fullscreen;

  -in windowed mode, that's a problem:
   windows and other OS'es, use pixel by pixel stuff, a window moved to a diff monitor will retain it's size, and the fullscreen scaling will fail;

   there can be the system based on the current window size, not monitor size, that would work also


   so i see 4 modes:
   1. full scale based on the current window size, you have your target and base the scaling with that;

   well, fullscreen and windowed mode, actually can be the same thing, you scale your stuff based on the current window size, not monitor size...
   you cannot base upon the current monitor cuz the window will remain the same

   so there's 3 modes:
   1. based on osiWindow size, with target
   2. based on primaryWindow's monitor, with target, ONE scale value
   3. no scaling at all, you do it by hand;


   all 3 must be done, the more i think on it. ALL 3.
*/




#define _IX_WSYS_SCRWIDTH540 7
#define _IX_WSYS_DEF_SCALE_SYS ixWinScaleSystem::OSI_WIN_DY
#define _IX_WSYS_DEF_SCALE_TGT ixWinScale::s1080p
#define _IX_WSYS_DEF_SCALE_STEP 270

///===================================///
// standard CONSTRUCTORS / DESTRUCTORS //
///===================================///

ixWinSys::ixWinSys() {
  selStyle= null;
  _op.delData();
  focus= null;
  hover= null;

  scrollBarWidth= 14; // 7@540p, 14@1080p


  scale= 1.0f;
  scaleSys= _IX_WSYS_DEF_SCALE_SYS;
  scaleTarget= _IX_WSYS_DEF_SCALE_TGT;
  scaleStep= _IX_WSYS_DEF_SCALE_STEP;

  //setScalingStepScaling();
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




void ixWinSys::init(Ix *in_ix) {
  computeScale(in_ix);

}






///===============================///
// CREATE for all types of objects //
///===============================///


ixWindow *ixWinSys::createWindow(cchar *in_title, ixBaseWindow *in_parent, float in_x, float in_y, float in_dx, float in_dy) {
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
    w->setTitle(in_title, &selStyle->title);
  }

  w->_createScrollbars();                 /// scrollbars

  if(in_parent) {
    in_parent->_computeChildArea();
    in_parent->hook.updateHooks();
  }

  return w;
}




ixButton *ixWinSys::createButton(cchar *in_text, ixBaseWindow *in_parent, float in_x, float in_y, float in_dx, float in_dy) {
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


ixEdit *ixWinSys::createEdit(ixBaseWindow *in_parent, float in_x0, float in_y0, float in_dx, float in_dy) {
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


ixStaticText *ixWinSys::createStaticText(ixBaseWindow *in_parent, float in_x0, float in_y0, float in_dx, float in_dy) {
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


ixRadioButton *ixWinSys::createRadioButton(ixBaseWindow *in_parent, float in_x0, float in_y0, float in_dx, float in_dy) {
  ixRadioButton *b= new ixRadioButton;

  /// parent / children chain
  if(in_parent)
    in_parent->childrens.add(b);
  else
    topObjects.add(b);
  b->parent= in_parent;

  /// default vars
  b->style= &selStyle->button;
  b->_applyColorsFromStyle();
  
  b->font= *Ix::getMain()->pr.style;
  b->is.visible= true;

  


  /// position / hook
  b->buttonDx= in_dx;
  b->buttonDy= in_dy;
  b->setPos(in_x0, in_y0, b->buttonDx, b->buttonDy);
  b->hook.setAnchor(in_parent);            /// sets parent, initial hook

  if(in_parent)
    in_parent->_computeChildArea();

  return b;
}


ixDropList *ixWinSys::createDropList(ixBaseWindow *in_parent, float in_x0, float in_y0, float in_buttonDx, float in_buttonDy) {

  ixDropList *d= new ixDropList;

  /// parent / children chain
  if(in_parent)
    in_parent->childrens.add(d);
  else
    topObjects.add(d);
  d->parent= in_parent;

  /// default vars
  d->style= &selStyle->window;      // <<< NO STYLE FOR THIS
  d->_applyColorsFromStyle();

  d->font= *Ix::getMain()->pr.style;
  d->is.visible= true;

  /// position / hook
  d->setPos(in_x0, in_y0, in_buttonDx, in_buttonDy);    // buttonDx,Dy origin are here
  d->buttonDx= in_buttonDx;
  d->buttonDy= in_buttonDy;
  d->hook.setAnchor(in_parent);            /// sets parent, initial hook

  if(in_parent)
    in_parent->_computeChildArea();

  return d;
}


ixProgressBar *ixWinSys::createProgressBar(ixBaseWindow *in_parent, float in_x0, float in_y0, float in_dx, float in_dy) {
  ixProgressBar *p= new ixProgressBar;

  /// parent / children chain
  if(in_parent)
    in_parent->childrens.add(p);
  else
    topObjects.add(p);
  p->parent= in_parent;

  /// default vars
  p->style= &selStyle->window;      // NO STYLE FOR THIS<<<<<<<<<<<<<<<<<<<<
  p->_applyColorsFromStyle();

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
  m->style= &selStyle->window;      // NO STYLE FOR THIS<<<<<<<<<<<<<<<<<<<<
  m->_applyColorsFromStyle();

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
  m->style= &selStyle->window;        // NO STYLE FOR THIS <<<<<<<<<<<<<<<
  m->_applyColorsFromStyle();
  m->font= *Ix::getMain()->pr.style;
  m->is.visible= true;
  m->setVisible(true);
  
  /// position / hook

  //the menu bar must have a position; the menu, not so much, actually not at all;
  //or is it the parent's coords always, i guess? but if you want a bar someplace else?
    
  //m->setPos(in_x0, in_y0, in_dx, in_dy);
  m->hook.setAnchor(in_parent);            /// sets parent, initial hook

  if(in_parent)
    in_parent->_computeChildArea();

  return m;
}













///=====///
// funcs //
///=====///

void ixWinSys::setScaleSys(ixWinScaleSystem in_sys, Ix *in_ix) {
  scaleSys= in_sys;
  computeScale(in_ix== null? Ix::getMain(): in_ix);
}





float ixWinSys::computeScale(Ix *in_ix) {
  int32 unit;
  if(scaleSys== ixWinScaleSystem::NO_SCALING || scaleTarget== ixWinScale::NO_SCALE) {
    scale= 1.0f;

  } else if(scaleSys== ixWinScaleSystem::OSI_WIN_DY) {
    scale= (float)in_ix->win->dy/ (float)scaleTarget;

  } else if(scaleSys== ixWinScaleSystem::OSI_PRIMARY_MONITOR) {
    scale= (float)in_ix->win->monitor->dy/ (float)scaleTarget;

  } else if(scaleSys== ixWinScaleSystem::OSI_WIN_DY_STEP) {
    unit= MAX(1, in_ix->win->dy/ mlib::roundf(scaleStep))* mlib::roundf(scaleStep);
    scale= (float)unit/ (float)scaleTarget;

  } else if(scaleSys== ixWinScaleSystem::OSI_PRIMARY_MONITOR_STEP) {
    unit= MAX(1, in_ix->win->monitor->dy/ mlib::roundf(scaleStep))* mlib::roundf(scaleStep);
    scale= (float)unit/ (float)scaleTarget;
  }


  uiVD.setD(0.0f, 0.0f, (float)osi.display.vdx/ scale, (float)osi.display.vdy/ scale);    // coords scaled


  return scale;
}


/*
float ixWinSys::_getScaleMonitor(const osiMonitor *in_m) {
  int32 unit;
  float scale= 1.0f;
  if(in_m== null) return scale;

  if(Ix::wsys().scaleSys== ixWinScaleSystem::NO_SCALING)
    return scale;

  else if(Ix::wsys().scaleSys== ixWinScaleSystem::STEP_SCALING) {
    if(in_m->dy== 720)   // special case 720p, to be or not to be
      unit= 720;
    else
      unit= MAX(1, in_m->dy/ 540)* 540;

    scale= (float)unit/ (float)Ix::wsys().scaleTarget;

  } else if(Ix::wsys().scaleSys== ixWinScaleSystem::DIRECT_SCALING) {
    scale= (float)in_m->dy/ (float)Ix::wsys().scaleTarget;

  } else {
    error.detail("Unknown scaling system", __FUNCTION__, __LINE__);
  }

  return scale;
}


float ixWinSys::_getScaleMonitor(int32 in_x, int32 in_y) {
  if(scaleSys== ixWinScaleSystem::NO_SCALING) return 1.0f;

  osiMonitor *m= _getMonitorForCoords(in_x, in_y);
  if(m== null)
    m= _getClosestMonitorForCoords(in_x, in_y);
  return _getScaleMonitor(m);
}*/



/*
inline float ixWinSys::applyScale() {
  int32 x, y;
  osiMonitor *m;
  font.scale= _scale= 1.0f;
  
  if(Ix::wsys().scaleSys== ixWinScaleSystem::NO_SCALING)
    return _scale;

  getPosVD(&x, &y);
  m= ixBaseWindow::_getMonitorForCoords(x, y);
  if(m== null)
    m= ixBaseWindow::_getClosestMonitorForCoords(x, y);

  _scale= _getScale(m);
  
  pos.setD(mlib::roundf((float)posOrigin.x0* _scale),
           mlib::roundf((float)posOrigin.y0* _scale),
           mlib::roundf((float)posOrigin.dx* _scale),
           mlib::roundf((float)posOrigin.dy* _scale));

  font.scale= _scale;

  return _scale;
}
*/


osiMonitor *ixWinSys::_getOsiMonitorForCoords(float x, float y) {
  int32 _x= mlib::roundf(x), _y= mlib::roundf(y);

  for(int a= 0; a< osi.display.nrMonitors; a++) {
    osiMonitor *m= &osi.display.monitor[a];
    if(_x>= m->x0 && _x< (m->x0+ m->dx) && _y>= m->y0 && _y< (m->y0+ m->dy))
      return m;
  }
  return null;
}


osiMonitor *ixWinSys::_getClosestOsiMonitorForCoords(float in_x, float in_y) {
  int32 dist, x= mlib::roundf(in_x), y= mlib::roundf(in_y);
  osiMonitor *ret= null;


  for(int32 a= 0; a< osi.display.nrMonitors; a++) {
    osiMonitor *m= &osi.display.monitor[a];
    int32 xmiddle= (m->dx/ 2)+ m->x0;
    int32 ymiddle= (m->dy/ 2)+ m->y0;
    vec2i d(xmiddle- x, ymiddle- y);
    int32 mdist= d.length();
    if(a== 0)
      dist= mdist, ret= m;
    else
      if(dist> mdist)
        dist= mdist, ret= m;
  }
  return ret;
}


/*
void ixWinSys::setScalingNoScale() {
  scaleSys= ixWinScaleSystem::NO_SCALING;

}

void ixWinSys::setScalingStepScaling(ixWinScale in_targetScale) {
  scaleSys= ixWinScaleSystem::STEP_SCALING;
  scaleTarget= in_targetScale;
}

void ixWinSys::setScalingDirect(int32 in_targetResolutionDY) {
  scaleSys= ixWinScaleSystem::DIRECT_SCALING;
  scaleTarget= (ixWinScale)in_targetResolutionDY;
}
*/





// draws all windows, on all engines ==========================--------------------

void ixWinSys::draw() {

  for(Ix *i= (Ix *)Ix::ixList().first; i; i= (Ix *)i->next) {
    ixFontStyle *saveStyle= i->pr.style;
    i->pr.style= null;

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
  }
}


/// draws all the top objects on specified Ix engine
void ixWinSys::drawSpecific(Ix *in_ix) {
  ixFontStyle *saveStyle= in_ix->pr.style;
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
  in_ix->pr.style= saveStyle;
}






// updates all windows =======================---------------------

bool ixWinSys::update() {
  bool ret= false;
  
  // compute unit @ cursor location
  osiMonitor   *m= _getOsiMonitorForCoords((float)in.m.x, (float)in.m.y);
  if(m== null)  m= _getClosestOsiMonitorForCoords((float)in.m.x, (float)in.m.y);
  if(m== null) unitAtCursor= 1.0f;
  else         unitAtCursor= MAX(1.0f, (float)(m->dy/ 540i32));          // 720p[1] 1080p[2] 1620p[3] 2160p[4] etc


  flags= (uint32)ixeWSflags::none;      // set flags down

  if(osi.flags.windowMoved || osi.flags.windowResized)
    for(ixBaseWindow *p= (ixBaseWindow *)topObjects.first; p; p= (ixBaseWindow *)p->next) {
      p->hook.updateHooks();
      p->_computeAll();
      if(p->update()) ret= true;
    }

  else 
    for(ixBaseWindow *p= (ixBaseWindow *)topObjects.first; p; p= (ixBaseWindow *)p->next)
      if(p->update()) ret= true;

  eventSys._update();

  return ret;
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
  
  // set to def1Style as the active style
  selStyle= &Ix::glb.def1Style();
}








