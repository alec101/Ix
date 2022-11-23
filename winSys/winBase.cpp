#include "ix/ix.h"
#include "ix/winSys/_privateDef.h"
using namespace mlib;

/* TODO:
 - drawing positions could cast to int, always drawing on fixed pixels <<<<<<< an ideea worth checking, more complications COULD happen tho

 - updateHooksDelta() might need attention
 - there could be a special case for resolutions divisible by 720p

 int vs float difference in speed.
 https://stackoverflow.com/questions/2550281/floating-point-vs-integer-calculations-on-modern-hardware
 mul/div can be faster on float, add/sub is not that far but ofc slower

*/

/*
ok. things are looking better and better;
the scale tho, has multiple SYSTEMS that are not taken into consideration.
unit, must be implemented as core in the base window, and updated only with wsys.
direct draw of a window and update, must disapear. you must use wsys. i see no benefit in usind direct drawing of one.
unit must take into consideration the scale system, because, you won't care what the monitor size is if the system is diff.
*/

ixBaseWindow::ixBaseWindow(Is *i, Usage *u): hook(this), pos(0), _is(i), _usage(u), scaleUI(Ix::wsys().scale) {
  _type= ixeWinType::baseWindow;
  parent= null;
  style= null;
  vscroll= hscroll= null;
  customTex= null;
  _clipUsePrentsParent= false;

  color.set        (0.1f, 0.1f, 0.1f, 0.5f);
  colorBRD.set     (0.0f, 0.0f, 0.0f, 1.0f);
  colorFocus.set   (0.2f, 0.2f, 0.2f, 0.5f);
  colorBRDfocus.set(1.0f, 1.0f, 1.0f, 1.0f);
  colorHover.set   (0.2f, 0.2f, 0.1f, 0.6f);

  _colorToUse= &color;
  _colorBRDtoUse= &colorBRD;

  delData();
}


ixBaseWindow::~ixBaseWindow() {
  delData();
}


void ixBaseWindow::delData() {
  pos.set(0.0f, 0.0f, 0.0f, 0.0f);

  // ALL CHILDRENS WILL BE DELETED
  while(childrens.first)
    childrens.del(childrens.first);

  hscroll= vscroll= null;
}







bool ixBaseWindow::_inBounds(Ix *in_ix) {
  rectf r; getPosVDnoScale(&r);

  for(int a= 0; a< in_ix->gpu->nrMonitors; a++) {
    osiMonitor *m= in_ix->gpu->monitor[a];
    if(r.intersect(rectf((float)m->x0, (float)m->y0, (float)(m->dx), (float)(m->dy)  )))
      return true;
  }
  return false;
}


void ixBaseWindow::_createScrollbars() {
  _computeViewArea();      // view area is updated / shrinken
  if(!hscroll) {
    hscroll= new ixScroll;
    childrens.add(hscroll);

    hscroll->parent= this;
    hscroll->_clipUsePrentsParent= true;
    hscroll->target= this;
    hscroll->style= &Ix::wsys().selStyle->scroll;
    hscroll->orientation= 0;

    hscroll->hook.set(this, ixEBorder::topLeft, ixEBorder::topLeft);
  }

  if(!vscroll) {
    vscroll= new ixScroll;
    childrens.add(vscroll);

    vscroll->parent= this;
    vscroll->_clipUsePrentsParent= true;
    vscroll->target= this;
    vscroll->style= &Ix::wsys().selStyle->scroll;
    vscroll->orientation= 1;

    vscroll->hook.set(this, ixEBorder::topLeft, ixEBorder::topLeft);
  }
  
  hscroll->_computePos();
  hscroll->_computeButtons();
  hscroll->_applyColorsFromStyle();

  vscroll->_computePos();
  vscroll->_computeButtons();
  vscroll->_applyColorsFromStyle();

  _computeViewArea();      // view area is updated / shrinken

  hscroll->_computeClipPlane();
  vscroll->_computeClipPlane();
}





///==============================///
// MAIN WINDOW MANIPULATION funcs //
///==============================///

void ixBaseWindow::move(float in_x, float in_y) {
  float deltax= in_x- pos.x0;
  float deltay= in_y- pos.y0;

  pos.moveD(deltax, deltay);
  _computeAllDelta(deltax, deltay);
}


// moves window and all children a delta distance (deltax, deltay)
void ixBaseWindow::moveDelta(float in_dx, float in_dy) {
  pos.moveD(in_dx, in_dy);
  _computeAllDelta(in_dx, in_dy);
}

// resizes window, this will move all children hooked on the right and bottom side
void ixBaseWindow::resize(float in_dx, float in_dy) {
  pos.resize(in_dx, in_dy);
  _computeAll();
}


void ixBaseWindow::resizeDelta(float in_dx, float in_dy) {
  pos.resizeD(in_dx, in_dy);
  _computeAll();
}


void ixBaseWindow::setPos(float x0, float y0, float dx, float dy) {
  pos.setD(x0, y0, dx, dy);
  _computeAll();
}

//void ixBaseWindow::setPosOrigin(float in_x0, float in_y0, float in_dx, float in_dy) {
//  posOrigin.setD(in_x0, in_y0, in_dx, in_dy);
//  setPos(in_x0, in_y0, in_dx, in_dy);
//}



float ixBaseWindow::getMinDx() {
  int32 u= _getUnitCoords(hook.pos.x+ pos.x0, hook.pos.y+ pos.y0);
  return (float)(u* 7);
}


float ixBaseWindow::getMinDy() {
  int32 u= _getUnitCoords(hook.pos.x+ pos.x0, hook.pos.y+ pos.y0);
  return (float)(u* 7);
}




// compute funcs - behind the scenes all the do-computings

void ixBaseWindow::_computeAll() {
  _computeViewArea();
  _computeChildArea();

  /// update either the parent or directly the hooks
  if(parent) {
    parent->_computeChildArea();
    parent->hook.updateHooks(false);
    parent->_computeScrollBars();
    
  } else
    hook.updateHooks();
  
  if(vscroll) { vscroll->_computePos(); vscroll->_computeButtons(); }
  if(hscroll) { hscroll->_computePos(); hscroll->_computeButtons(); }
  _computeClipPlane();
}


void ixBaseWindow::_computeAllDelta(float in_dx, float in_dy) {
  _computeViewArea();
  _computeChildArea();
  /// update either the parent or directly the hooks
  if(parent) {
    parent->_computeChildArea();
    parent->hook.updateHooks(false);
    if(parent->hscroll) parent->hscroll->_computeButtons();
    if(parent->vscroll) parent->vscroll->_computeButtons();
  } else
    hook.updateHooksDelta(in_dx, in_dy, false);
  _computeClipPlane();
}



void ixBaseWindow::_computeChildArea() {
  _childArea.setD(0, 0, _viewArea.dx, _viewArea.dy);    // _viewArea usually is not 0, 0. _childArea is

  for(ixBaseWindow *p= (ixBaseWindow *)childrens.first; p; p= (ixBaseWindow *)p->next) {
    if(p== vscroll || p== hscroll) continue;
    if(_type== ixeWinType::window)
      if(p== ((ixWindow *)this)->title) continue;

    if(p->hook.ixWin== this) {  //  OPTION 1, if ixWin is this, else somehow compute the real pos in this
      rectf &r(p->pos);

      if(p->hook.border== ixEBorder::bottomLeft) {
        if(_childArea.xe< r.xe)  _childArea.xe= r.xe;
        if(_childArea.ye< -r.y0) _childArea.ye= -r.y0;

      } else if(p->hook.border== ixEBorder::topLeft) {
        if(_childArea.xe< r.xe)  _childArea.xe= r.xe;
        if(_childArea.ye< r.ye)  _childArea.ye= r.ye;

      } else if(p->hook.border== ixEBorder::topRight) {
        if(_childArea.xe< -r.x0) _childArea.xe= -r.x0;
        if(_childArea.ye< r.ye)  _childArea.ye= r.ye;

      } else if(p->hook.border== ixEBorder::bottomRight) {
        if(_childArea.xe< -r.x0) _childArea.xe= -r.x0;
        if(_childArea.ye< -r.y0) _childArea.ye= -r.y0;

      } else if(p->hook.border== ixEBorder::top) {
        if(_childArea.xe< r.dx) _childArea.xe= r.dx;
        if(_childArea.ye< r.ye) _childArea.ye= r.ye;

      } else if(p->hook.border== ixEBorder::right) {
        if(_childArea.xe< -r.x0) _childArea.xe= -r.x0;
        if(_childArea.ye< r.dy)  _childArea.ye= r.dy;

      } else if(p->hook.border== ixEBorder::bottom) {
        if(_childArea.xe< r.dx)  _childArea.xe= r.dx;
        if(_childArea.ye< -r.y0) _childArea.ye= -r.y0;

      } else if(p->hook.border== ixEBorder::left) {
        if(_childArea.xe< r.xe) _childArea.xe= r.xe;
        if(_childArea.ye< r.dy) _childArea.ye= r.dy;

      } else
        error.detail("unknown border", __FUNCTION__);

    // window is hooked to another window in this parent's childArea
    } else {
      rectf r(p->pos);
      if(p->hook.ixWin) {
             if(p->hook.border== ixEBorder::topLeft); // do nothing
        else if(p->hook.border== ixEBorder::bottomLeft)  r.moveD(0,                        p->hook.ixWin->pos.dy);
        else if(p->hook.border== ixEBorder::topRight)    r.moveD(p->hook.ixWin->pos.dx,    0);
        else if(p->hook.border== ixEBorder::bottomRight) r.moveD(p->hook.ixWin->pos.dx,    p->hook.ixWin->pos.dy);
        else if(p->hook.border== ixEBorder::top)         r.moveD(p->hook.ixWin->pos.dx/ 2, 0);
        else if(p->hook.border== ixEBorder::bottom)      r.moveD(p->hook.ixWin->pos.dx/ 2, p->hook.ixWin->pos.dy);
        else if(p->hook.border== ixEBorder::left)        r.moveD(0,                        p->hook.ixWin->pos.dy/ 2);
        else if(p->hook.border== ixEBorder::right)       r.moveD(p->hook.ixWin->pos.dx,    p->hook.ixWin->pos.dy/ 2);

        r.moveD(p->hook.ixWin->pos.x0, p->hook.ixWin->pos.y0);
      }

      if(_childArea.xe< r.xe)  _childArea.xe= r.xe;
      if(_childArea.ye< r.ye)  _childArea.ye= r.ye;
    }
  }

  _childArea.compDeltas();
}


void ixBaseWindow::_computeViewArea() {
  _viewArea.setD(0.0f, 0.0f, pos.dx, pos.dy);
  if(vscroll)
    if(vscroll->is.visible)
      _viewArea.xe-= vscroll->pos.dx;

  if(hscroll)
    if(hscroll->is.visible)
      _viewArea.ye-= hscroll->pos.dy;

  _viewArea.compDeltas();
  _viewArea.setD(_viewArea.x0, _viewArea.y0, (_viewArea.dx< 0.0f? 0.0f: _viewArea.dx), (_viewArea.dy< 0.0f? 0.0f: _viewArea.dy));

  if(_type== ixeWinType::edit)
    ((ixEdit *)this)->text._computeWrapLen();
  else if(_type== ixeWinType::staticText)
    ((ixStaticText *)this)->text._computeWrapLen();
}


inline void ixBaseWindow::_computeScrollBars() {
  if(hscroll) hscroll->_computeButtons();
  if(vscroll) vscroll->_computeButtons();
}



void ixBaseWindow::_computeClipPlane() {
  if(parent) {
    if(_clipUsePrentsParent) {
      if(parent->parent) {
        parent->parent->_getVDviewArea(&_clip);
        _clip.intersectRect(parent->parent->_clip);
      } else {
        _clip.setD(_scaleDiv(osi.display.vx0), _scaleDiv(osi.display.vy0), _scaleDiv(osi.display.vdx), _scaleDiv(osi.display.vdy));
      }
    } else {
      parent->_getVDviewArea(&_clip);
      _clip.intersectRect(parent->_clip);
    }
  } else
    _clip.setD(_scaleDiv(osi.display.vx0), _scaleDiv(osi.display.vy0), _scaleDiv(osi.display.vdx), _scaleDiv(osi.display.vdy));

  /// update all children's clipping
  for(ixBaseWindow *p= (ixBaseWindow *)childrens.first; p; p= (ixBaseWindow *)p->next)
    p->_computeClipPlane();
}


void ixBaseWindow::_computeClipPlaneDelta(float in_dx, float in_dy) {
  if(_clip.exists()) {
    _clip.moveD(in_dx, in_dy);

    for(ixBaseWindow *p= (ixBaseWindow *)childrens.first; p; p= (ixBaseWindow *)p->next)
      p->_computeClipPlaneDelta(in_dx, in_dy);
  }
}





void ixBaseWindow::changeParent(ixBaseWindow *in_w) {
  if(parent== in_w) return;

  if(parent) {
    parent->childrens.release(this);
    parent->_computeAll();
  } else {
    Ix::wsys().topObjects.release(this);
  }

  hook.delData();
  parent= in_w;

  if(parent) {
    parent->childrens.add(this);
    hook.setAnchor(in_w);
    
  } else
    Ix::wsys().topObjects.add(this);

  _computeAll();
}


void ixBaseWindow::removeParent() {
  if(parent) {
    parent->childrens.release(this);
    parent->_computeAll();
    parent= null;
  }
  hook.delData();
}



void ixBaseWindow::Usage::autoScrollbars(bool in_b) {
  _autoScrollbars= in_b;
}


void ixBaseWindow::Usage::scrollbars(bool in_b) {
  _scrollbars= in_b;
  if(_win->hscroll) _win->hscroll->setVisible(in_b);
  if(_win->vscroll) _win->vscroll->setVisible(in_b);

  _win->_computeAll();
}






///==============================================
// Small printing system that any window can use ===============
///==============================================


void *ixBaseWindow::printStatic(cchar *in_text, vec3 *in_pos, ixFontStyle *in_style) {
  if(in_text== null) { error.detail("<in_text> is null", __FUNCTION__, __LINE__); return null; }
  
  StaticPrint *p= new StaticPrint;
  p->text= in_text;
  p->pos= *in_pos;
  
  if(in_style) p->fntStyle= *in_style;
  else         p->fntStyle= *Ix::getMain()->pr.style;

  p->visible= true;

  staticPrintList.add(p);

  return p;
}


void ixBaseWindow::printStaticModify(void *in_handle, cchar *in_newText, vec3 *in_newPos, ixFontStyle *in_newStyle) {
  if(in_handle== null) return;
  if(in_newText) ((StaticPrint *)in_handle)->text= in_newText;
  if(in_newPos) ((StaticPrint *)in_handle)->pos= *in_newPos;
  if(in_newStyle) ((StaticPrint *)in_handle)->fntStyle= *in_newStyle;
}


void ixBaseWindow::printStaticSetVisible(void *in_handle, bool in_visible) {
  if(in_handle== null) return;
  ((StaticPrint *)in_handle)->visible= in_visible;
}






// monitor funcs - used for thin lines or medium lines, depending on monitor resolution

int32 ixBaseWindow::_getUnit() const { 
  /// find the monitor the menu will be on
  rectf r; getPosVDnoScale(&r);     // NO SCALE
  osiMonitor   *m= Ix::wsys()._getOsiMonitorForCoords(r.x0,       r.y0);
  if(m== null)  m= Ix::wsys()._getOsiMonitorForCoords(r.xe- 1.0f, r.y0);
  if(m== null)  m= Ix::wsys()._getOsiMonitorForCoords(r.x0,       r.ye- 1.0f);
  if(m== null)  m= Ix::wsys()._getOsiMonitorForCoords(r.xe- 1.0f, r.ye- 1.0f);
  if(m== null)  m= Ix::wsys()._getClosestOsiMonitorForCoords(r.x0, r.y0);
  if(m== null) return 1;

  return MAX(1, m->dy/ 540);          // 720p[1] 1080p[2] 1620p[3] 2160p[4] etc
}

int32 ixBaseWindow::_getThinUnit() const {
  return MAX(1, _getUnit()/ 2);
}


//inline int32 ixBaseWindow::_getUnitMonitor(osiMonitor *in_m) {
//  return MAX(1, in_m->dy/ 540);
//}

//inline int32 ixBaseWindow::_getThinUnitMonitor(osiMonitor *in_m) {
//  return MAX(1, _getUnitMonitor(in_m)/ 2);
//}


int32 ixBaseWindow::_getUnitCoords(float x, float y) const {
  /// find the monitor the menu will be on
  x= _scaleDiv(x), y= _scaleDiv(y);
  osiMonitor   *m= Ix::wsys()._getOsiMonitorForCoords(x, y);
  if(m== null)  m= Ix::wsys()._getClosestOsiMonitorForCoords(x, y);
  if(m== null) return 1;

  return MAX(1, m->dy/ 540);          // 720p[1] 1080p[2] 1620p[3] 2160p[4] etc
}




/// WINDOW

// ##    ##    ####      ####    ##    ##  ######  ##    ##    ####
// ##    ##  ##    ##  ##    ##  ##  ##      ##    ####  ##  ##
// ########  ##    ##  ##    ##  ####        ##    ## ## ##  ##  ####
// ##    ##  ##    ##  ##    ##  ##  ##      ##    ##  ####  ##    ##
// ##    ##    ####      ####    ##    ##  ######  ##    ##    ######

/// class ===================================== (no not that one) ///

ixWinHook::ixWinHook(void *in_p): pos(0.0f) {
  parent= (ixBaseWindow *)in_p;
  border= ixEBorder::top;
  ixWin= null;
  osiWin= null;
  osiMon= null;
  _anchorToEdge= true;
}


void ixWinHook::delData() {
  border= ixEBorder::top;
  ixWin= null;
  osiWin= null;
  osiMon= null;
  _anchorToEdge= true;
  pos.set(0.0f, 0.0f, 0.0f);
}


inline void ixWinHook::_compute() {
  float dx, dy;
  
  if(ixWin) {                       // hooked to an ixWindow
    pos= ixWin->hook.pos;
    pos.x+= ixWin->pos.x0;
    pos.y+= ixWin->pos.y0;

    if(_anchorToEdge)
      dx= ixWin->pos.dx, dy= ixWin->pos.dy;
    else {
      dx= ixWin->_childArea.dx, dy= ixWin->_childArea.dy;
    }
  
  } else if(osiWin) {               // hooked to an osiWindow
    pos.set(parent->_scaleDiv(osiWin->x0), parent->_scaleDiv(osiWin->y0), 0.0f);
    dx= parent->_scaleDiv(osiWin->dx),
    dy= parent->_scaleDiv(osiWin->dy);
  
  } else if(osiMon) {               // hooked to an osiMonitor
    pos.set(parent->_scaleDiv(osiMon->x0), parent->_scaleDiv(osiMon->y0), 0.0f);
    dx= parent->_scaleDiv(osiMon->dx),
    dy= parent->_scaleDiv(osiMon->dy);
  
  } else {                          // no hooking - or hooked to the virtual desktop
    pos.set(0.0f, 0.0f, 0.0f);
    dx= parent->_scaleDiv(osi.display.vdx),
    dy= parent->_scaleDiv(osi.display.vdy);
  }

       if(border== ixEBorder::topLeft);     // top left don't have to do nothin
  else if(border== ixEBorder::bottomLeft)                 pos.y+= dy;
  else if(border== ixEBorder::topRight)    pos.x+= dx;
  else if(border== ixEBorder::bottomRight) pos.x+= dx,    pos.y+= dy;
  else if(border== ixEBorder::top)         pos.x+= dx/ 2;
  else if(border== ixEBorder::bottom)      pos.x+= dx/ 2, pos.y+= dy;
  else if(border== ixEBorder::left)                       pos.y+= dy/ 2;
  else if(border== ixEBorder::right)       pos.x+= dx,    pos.y+= dy/ 2;
  
  // adjust the hook with the scrolling
  /// skip updating the hook if anchored to another window on same parent - the hooks are already scroll-computed
  if(ixWin)
    if(ixWin== parent->parent)
      _adjustHookPosWithScroll();
}

// adjusts the window hookPos with scrolling, if it is required. window title+ other children are not affected
inline void ixWinHook::_adjustHookPosWithScroll() {
  /// see if scrolling will affect the window
  if(parent->parent== null) return;             /// no parent, no scroll
  if(parent->parent->hscroll== parent) return;  /// parent's scroll is this window, nothing to do here
  if(parent->parent->vscroll== parent) return;  /// parent's scroll is this window, nothing to do here
  if(parent->parent->_type== ixeWinType::window)/// parent's title is this window, nothing to do here
    if(((ixWindow *)parent->parent)->title== parent) return;

  // scrolling affects this window if reached this point
  if(parent->parent->hscroll) pos.x-= parent->parent->hscroll->position;
  if(parent->parent->vscroll) pos.y-= parent->parent->vscroll->position;
}


void ixWinHook::_computeDelta(float in_dx, float in_dy) {
  pos.x+= in_dx,
  pos.y+= in_dy;
}


void ixWinHook::_computeParentHooks() {
  ixBaseWindow *p= (ixBaseWindow *)(parent->parent? parent->parent->childrens.first: Ix::wsys().topObjects.first);
  
  for(; p; p= (ixBaseWindow *)p->next)
    if(p->hook.ixWin== parent)
      p->hook.updateHooks();
}


void ixWinHook::_computeParentHooksDelta(float in_dx, float in_dy) {
  ixBaseWindow *p= (ixBaseWindow *)(parent->parent? parent->parent->childrens.first: Ix::wsys().topObjects.first);

  for(; p; p= (ixBaseWindow *)p->next)
    if(p->hook.ixWin== parent)
      p->hook.updateHooksDelta(in_dx, in_dy);
}


// in_border, is the border of <this> not the target window - basically what border to touch what border
void ixWinHook::_setWinPosBorderTo0(ixEBorder in_border) {
  // this func will act only on window position, nothing to do with hook or parent
  float dx= parent->pos.dx, dy= parent->pos.dy;

       if(in_border== ixEBorder::topLeft)     parent->pos.move(0.0f,      0.0f);      // was 0,     -dy
  else if(in_border== ixEBorder::bottomLeft)  parent->pos.move(0.0f,      -dy);       // was 0,     0
  else if(in_border== ixEBorder::topRight)    parent->pos.move(-dx,       0.0f);      // was -dx,   -dy
  else if(in_border== ixEBorder::bottomRight) parent->pos.move(-dx,       -dy);       // was -dx,   0
  else if(in_border== ixEBorder::top)         parent->pos.move(-dx/ 2.0f, 0.0f);      // was -dx/2, -dy
  else if(in_border== ixEBorder::bottom)      parent->pos.move(-dx/ 2.0f, -dy);       // was -dx/2, 0
  else if(in_border== ixEBorder::left)        parent->pos.move(0.0f,      -dy/ 2.0f); // was 0,     -dy/2
  else if(in_border== ixEBorder::right)       parent->pos.move(-dx,       -dy/ 2.0f); // was -dx,   -dy/2
}


inline bool ixWinHook::_computeAnchorToEdgeVar(ixBaseWindow *in_window) {
  if(in_window== null) return true;
  
  // the anchor is the parent, then the border will be the childArea (most cases, ofc)
  if(in_window== parent->parent) {
    if(parent== parent->parent->vscroll || parent== parent->parent->hscroll)
      return true;        // this is the scroll of the parent
    
    if(parent->parent->_type== ixeWinType::window)
      if(((ixWindow *)parent->parent)->title== parent)
        return true;      // this is the title of a window
  } else
    return true;          // the anchor is another window that has the same parent

  return false;           // the window is another children like this
}






// window hooking functions =============-----------------------

void ixWinHook::setAnchor(ixBaseWindow *in_window, ixEBorder in_border) {
  if(in_window== null) return setAnchor(in_border);

  if(!((parent->parent== in_window) || (in_window->parent== parent))) {
    error.detail("not possible to hook to that window - must have same parent or both no parent at all", __FUNCTION__, __LINE__);
    return;
  }

  border= in_border;
  ixWin= in_window;
  osiWin= null;
  osiMon= null;
  _anchorToEdge= _computeAnchorToEdgeVar(in_window);

  _compute();
  /// this window's parent must be udpated, cuz the window basically moved
  if(parent->parent) {
    parent->parent->_computeChildArea();
    if(parent->parent->vscroll)
      parent->parent->vscroll->_computeButtons();
    if(parent->parent->hscroll)
      parent->parent->hscroll->_computeButtons();
  }
  updateHooks();
}


void ixWinHook::setAnchor(osiWindow *in_window, ixEBorder in_border) {
  if(parent->parent) {
    error.detail("not possible to hook to osiWindow- <this> has a parent", __FUNCTION__, __LINE__);
    return;
  }

  border= in_border;
  ixWin= null;
  osiWin= in_window;
  osiMon= null;
  _anchorToEdge= true;

  _compute();
  /// this window's parent must be udpated, cuz the window basically moved
  if(parent->parent) {
    parent->parent->_computeChildArea();
    if(parent->parent->vscroll)
      parent->parent->vscroll->_computeButtons();
    if(parent->parent->hscroll)
      parent->parent->hscroll->_computeButtons();
  }
  updateHooks();
}


void ixWinHook::setAnchor(osiMonitor *in_window, ixEBorder in_border) {
  if(parent->parent) {
    error.detail("not possible to hook to osiMonitor- <this> has a parent", __FUNCTION__, __LINE__);
    return;
  }

  border= in_border;
  ixWin= null;
  osiWin= null;
  osiMon= in_window;
  _anchorToEdge= true;
  
  _compute();
  /// this window's parent must be udpated, cuz the window basically moved
  if(parent->parent) {
    parent->parent->_computeChildArea();
    if(parent->parent->vscroll)
      parent->parent->vscroll->_computeButtons();
    if(parent->parent->hscroll)
      parent->parent->hscroll->_computeButtons();
  }
  updateHooks();
}

/// sets the hook to the virtual desktop
void ixWinHook::setAnchor(ixEBorder in_border) {
  if(parent->parent) {
    error.detail("not possible to hook to virtual desktop- <this> has a parent", __FUNCTION__, __LINE__);
    return;
  }

  border= in_border;
  ixWin= null;
  osiWin= null;
  osiMon= null;
  _anchorToEdge= true;

  _compute();
  /// this window's parent must be udpated, cuz the window basically moved
  if(parent->parent) {
    parent->parent->_computeChildArea();
    if(parent->parent->vscroll)
      parent->parent->vscroll->_computeButtons();
    if(parent->parent->hscroll)
      parent->parent->hscroll->_computeButtons();
  }
  updateHooks();
}


// hooks to ixBaseWindow (in point border1) and moves current window (point border2) to touch it
void ixWinHook::set(ixBaseWindow *in_window, ixEBorder in_border1, ixEBorder in_border2) {
  if(!((parent->parent== in_window) || (in_window->parent== parent->parent))) {
    error.detail("not possible to hook to that window - must have same parent or both no parent at all", __FUNCTION__, __LINE__);
    return;
  }

  border= in_border1;
  ixWin= in_window;
  osiWin= null;
  osiMon= null;
  _anchorToEdge= _computeAnchorToEdgeVar(in_window);

  _compute();      /// set hook anchor
  _setWinPosBorderTo0(in_border2);        /// set current window position to touch anchor
  /// this window's parent must be udpated, cuz the window basically moved
  if(parent->parent) {
    parent->parent->_computeChildArea();
    if(parent->parent->vscroll)
      parent->parent->vscroll->_computeButtons();
    if(parent->parent->hscroll)
      parent->parent->hscroll->_computeButtons();
    parent->parent->hook.updateHooks();
  } else
    updateHooks();
}

// hooks to osiWindow (in point border1) and moves current window (point border2) to touch it
void ixWinHook::set(osiWindow *in_window, ixEBorder in_border1, ixEBorder in_border2) {
  if(parent->parent) {
    error.detail("not possible to hook to osiWindow- <this> has a parent", __FUNCTION__, __LINE__);
    return;
  }

  border= in_border1;
  ixWin= null;
  osiWin= in_window;
  osiMon= null;
  _anchorToEdge= true;

  _compute();
  _setWinPosBorderTo0(in_border2);
  /// this window's parent must be udpated, cuz the window basically moved
  if(parent->parent) {
    parent->parent->_computeChildArea();
    if(parent->parent->vscroll)
      parent->parent->vscroll->_computeButtons();
    if(parent->parent->hscroll)
      parent->parent->hscroll->_computeButtons();
  }
  updateHooks();
}

// hooks to osiMonitor (in point border1) and moves current window (point border2) to touch it
void ixWinHook::set(osiMonitor *in_window, ixEBorder in_border1, ixEBorder in_border2) {
  if(parent->parent) {
    error.detail("not possible to hook to osiMonitor- <this> has a parent", __FUNCTION__, __LINE__);
    return;
  }

  border= in_border1;
  ixWin= null;
  osiWin= null;
  osiMon= in_window;
  _anchorToEdge= true;

  _compute();
  _setWinPosBorderTo0(in_border2);
  /// this window's parent must be udpated, cuz the window basically moved
  if(parent->parent) {
    parent->parent->_computeChildArea();
    if(parent->parent->vscroll) parent->parent->vscroll->_computeButtons();
    if(parent->parent->hscroll) parent->parent->hscroll->_computeButtons();
  }
  updateHooks();
}
 
// hooks to virtual desktop (in point border1) and moves current window (point border2) to touch it
void ixWinHook::set(ixEBorder in_border1, ixEBorder in_border2) {
  if(parent->parent) {
    error.detail("not possible to hook to virtual desktop- <this> has a parent", __FUNCTION__, __LINE__);
    return;
  }

  border= in_border1;
  ixWin= null;
  osiWin= null;
  osiMon= null;
  _anchorToEdge= true;

  _compute();
  _setWinPosBorderTo0(in_border2);
  /// this window's parent must be udpated, cuz the window basically moved
  if(parent->parent) {
    parent->parent->_computeChildArea();
    if(parent->parent->vscroll) parent->parent->vscroll->_computeButtons();
    if(parent->parent->hscroll) parent->parent->hscroll->_computeButtons();
  }
  updateHooks();
}


void ixWinHook::updateHooks(bool in_updateThis) {
  // simple list of things this func does:
  // _computeHook()
  // parentUpdateOnlyHookedWindowsToMeAndTheirChildren(this);
  // updateAllChildren();

  /// update this window's hook
  if(in_updateThis)
    _compute();

  /// update hooked windows to this that are not children - these windows could exist only in parent / top objects
  _computeParentHooks();

  /// update childrens
  for(ixBaseWindow *p= (ixBaseWindow *)parent->childrens.first; p; p= (ixBaseWindow *)p->next)
    p->hook.updateHooks();
}


void ixWinHook::updateHooksDelta(float in_dx, float in_dy, bool in_updateThis) {
  /// update this window's hook
  if(in_updateThis)
    _computeDelta(in_dx, in_dy);

  /// update childrens
  for(ixBaseWindow *p= (ixBaseWindow *)parent->childrens.first; p; p= (ixBaseWindow *)p->next)
    p->hook.updateHooksDelta(in_dx, in_dy);

  // THIS SEEMS NOT TO BE NEEDED EVEN
  // I TRIED TO MOVE IT AFTER THE UPDATE CHILDREN, BUT IT MIGHT NOT BE NEEDED AT ALL
  /// update hooked windows to this that are not children - these windows could exist only in parent / top objects
  //_computeParentHooksDelta(in_dx, in_dy); // THIS DON'T WORK RIGHT, MUST FURTHER CHECK IF IT'S POSSIBLE OR JUST USE THE NORMAL FUNC
  //_computeParentHooks();
}



















//   ######     ######       ####     ##      ##
//   ##    ##   ##    ##   ##    ##   ##  ##  ##
//   ##    ##   ######     ########   ##  ##  ##
//   ##    ##   ##    ##   ##    ##   ##  ##  ##
//   ######     ##    ##   ##    ##     ##  ##

// Base drawing function, usually called by derived object first //
///=============================================================///



inline void ixBaseWindow::draw(Ix *in_ix) {
  #ifdef IX_USE_OPENGL
  if(in_ix->renOpenGL()) {
    _glDrawInit(in_ix);
    _glDraw(in_ix);
    _glDrawFinish(in_ix);
  }
  #endif

  #ifdef IX_USE_VULKAN
  if(in_ix->renVulkan()) {
    VkCommandBuffer cmd= *in_ix->vki.ortho.cmd[in_ix->vki.fi];
    _vkDrawInit  (cmd, in_ix);
    _vkDraw      (cmd, in_ix);
    _vkDrawFinish(cmd, in_ix);
  }
  #endif
}

#ifdef IX_USE_OPENGL
void ixBaseWindow::_glDrawInit(Ix *in_ix) {
  // too many changes
  error.makeme(__FUNCTION__);
}


void ixBaseWindow::_glDrawFinish(Ix *in_ix) {
  error.makeme(__FUNCTION__);
}


void ixBaseWindow::_glDraw(Ix *in_ix, ixWSsubStyleBase *in_style) {
  error.makeme(__FUNCTION__);
}
#endif




#ifdef IX_USE_VULKAN
void ixBaseWindow::_vkDrawInit(VkCommandBuffer in_cmd, Ix *in_ix) {

  // if the descriptor set is compatible, and is at top (0 / 1), you can only bind it once and it can stay that way if the other shaders have same layout for those sets
  // ... in theory... lol

  // draw class init values - _for a top window_

  in_ix->vk.CmdBindPipeline(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipeline);
  in_ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipelineLayout, 0, 1, &in_ix->vki.glb[in_ix->vki.fi]->set->set, 0, nullptr);

  in_ix->vki.draw.circle.flagOrtho(true);
  in_ix->vki.draw.circle.flagPersp(false);
  in_ix->vki.draw.circle.flagTexture(false);
  in_ix->vki.draw.circle.push.hollow= 2.0f;
  
  in_ix->vki.draw.triangle.flagOrtho(true);
  in_ix->vki.draw.triangle.flagPersp(false);
  in_ix->vki.draw.triangle.flagTexture(false);
  
  in_ix->vki.draw.quad.push.color.set(1.0f, 1.0f, 1.0f, 1.0f);
  in_ix->vki.draw.quad.push.hollow= -1.0f;
  in_ix->vki.draw.quad.flagOrtho(true);
  in_ix->vki.draw.quad.flagPersp(false);
}



void ixBaseWindow::_vkDrawFinish(VkCommandBuffer in_cmd, Ix *in_ix) {
  in_ix->vk.CmdSetScissor(in_cmd, 0, 1, &in_ix->vki.render.scissor);
}



void ixBaseWindow::_vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style) {
  // visibility is not that easy, check for a minimized button first...
  
  if(!_clip.exists()) return;
  if(!_inBounds(in_ix)) return;     // nothing to draw, the ix+gpu don't draw this window
  
  /// tmp vars
  bool _debug= false;
  ixWSgenericStyle *s= (ixWSgenericStyle *)(in_style? in_style: style);

  ixTexture *t= s->parent->getTexture(in_ix);
  if(s->useTexture== null) t= null;

  rectf clp;                  /// will be used for further clip-in-clip
  int nrS, nrT;               /// these will hold the number of times the texture will repeat on S and T axis

  float _x, _y; getPosVD(&_x, &_y);
  // VVVVVVVVVVVVVVVV
  // _x and _y CAN BE FORCED TO INT. POSITION CAN BE 0.5 OR SOMETHING SIMILAR WHEN MOVING WITH CURSOR, A SCALED WINDOW.
  // ^^^^^^^^^^^^^^^^


  if(_debug) {
    char buf[256];  // DEBUG print buffer
    float cy= in_ix->debugStyle.getCharDy()+ 1.0f;
    ixFontStyle *sav= in_ix->pr.style;
    in_ix->pr.style= &in_ix->debugStyle;

    sprintf(buf, "pos[%.1f,%.1f %.1fx%.1f] hookPos[%.1f,%.1f]", pos.x0, pos.y0, pos.dx, pos.dy, hook.pos.x, hook.pos.y);
    in_ix->pr.txt2f(_x, _y+ pos.dy+ (cy* 0)+ 1, buf);
    sprintf(buf, "childArea[%.1f,%.1f %.1fx%.1f] viewArea[%.1f,%.1f %.1fx%.1f]", _childArea.x0, _childArea.y0, _childArea.dx, _childArea.dy, _viewArea.x0, _viewArea.y0, _viewArea.dx, _viewArea.dy);
    in_ix->pr.txt2f(_x, _y+ pos.dy+ (cy* 1)+ 1, buf);

    in_ix->pr.style= sav;
  }
  
  // setup vkQuad
  /// THE _debug WILL SWITCH PIPELINE...
  in_ix->vk.CmdBindPipeline(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipeline);
  in_ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipelineLayout, 0, 1, &in_ix->vki.glb[in_ix->vki.fi]->set->set, 0, nullptr);
  in_ix->vki.cmdScissor(in_cmd, &_clip);

  in_ix->vki.draw.quad.cmdTexture(in_cmd, t);
  in_ix->vki.draw.quad.flagDisabled(_is->disabled);
  in_ix->vki.draw.quad.cmdPushAll(in_cmd);

  // WINDOW BACKGROUND =========-----------

  /// useBackColor usage flag - draw a rectangle using current color
  if(s->useBackColor) {
    in_ix->vki.draw.quad.push.color= *_colorToUse;
    in_ix->vki.draw.quad.flagTexture(false);
    in_ix->vki.draw.quad.setPosD(_x, _y, 0, pos.dx, pos.dy);
    in_ix->vki.draw.quad.cmdPushAll(in_cmd);
    in_ix->vki.draw.quad.cmdDraw(in_cmd);
  }

  /// useColorOnTexture usage flag - use color for current texture
  if(s->useColorOnTexture) in_ix->vki.draw.quad.push.color= *_colorToUse;
  else                     in_ix->vki.draw.quad.push.color.set(1.0f, 1.0f, 1.0f, 1.0f);;
  in_ix->vki.draw.quad.cmdPushColor(in_cmd);

  in_ix->vki.draw.quad.flagTexture(s->useTexture);
  in_ix->vki.draw.quad.cmdPushFlags(in_cmd);

  /// BACKGROUND texturing
  if(s->useTexture && s->bTexBG) {
 
    /// nr of times the texture repeats
    nrS= (int)(pos.dx/ (float)s->texBG.dx)+ 1;  //nrS= (int)pos.dx/ (int)s->texBG.dx+ (((int)pos.dx% (int)s->texBG.dx)? 1: 0);
    nrT= (int)(pos.dy/ (float)s->texBG.dy)+ 1;  //nrT= (int)pos.dy/ (int)s->texBG.dy+ (((int)pos.dy% (int)s->texBG.dy)? 1: 0);

    /// set scissor
    clp= pos;
    //clp.setD(_x, _y, pos.dx, pos.dy);
    clp.intersectRect(_clip);
    in_ix->vki.cmdScissor(in_cmd, &clp);

    // FIXED/STRETCHED background
    if(s->texBGwrap<= 1) {

      if(s->texBGwrap== 0) in_ix->vki.draw.quad.setPosD(_x, _y, 0.0f, (float)s->texBG.dx, (float)s->texBG.dy);
      else                 in_ix->vki.draw.quad.setPosD(_x, _y, 0.0f, pos.dx,      pos.dy);

      in_ix->vki.draw.quad.setTex(s->texBG.s0, s->texBG.t0, s->texBG.se, s->texBG.te);

      in_ix->vki.draw.quad.cmdPushAll(in_cmd);
      in_ix->vki.draw.quad.cmdDraw(in_cmd);

    
    // REPEAT background wrap
    } else if(s->texBGwrap== 2) {
    // REPEATED / MIRRORED REPEATED background
      // must be tied to a point, when stretching. if not, the background will move and that CANNOT happen
      //
      // X-+-++
      // | | ||
      // +-+-++
      // | | ||
      // +-+-++
      // +-+-++


      // ++-+-+-++
      // ++-+-+-++
      // || | | || <<< WRONG IDEEA. background will move, this won't work. MUST BE TIED TO up-left point (or a different one)
      // ++-+-+-++
      // || |X| ||
      // ++-+-+-++
      // || | | ||
      // ++-+-+-++
      // ++-+-+-++

      in_ix->vki.draw.quad.setTex(s->texBG.s0, s->texBG.t0, s->texBG.se, s->texBG.te);
      in_ix->vki.draw.quad.cmdPushTex(in_cmd);

      for(int a= 0; a< nrT; a++) 
        for(int b= 0; b< nrS; b++) {
          in_ix->vki.draw.quad.setPosD(_x+ (float)(b* s->texBG.dx), _y+ (float)(a* s->texBG.dy), 0.0f, (float)s->texBG.dx, (float)s->texBG.dy);
          
          in_ix->vki.draw.quad.cmdPushPos(in_cmd);
          in_ix->vki.draw.quad.cmdDraw(in_cmd);
        }


    // MIRROR REPEAT background wrap
    } else if(s->texBGwrap== 3) {

      ixSubTex *p= &s->texBG;

      for(int a= 0; a< nrT; a++) {
        for(int b= 0; b< nrS; b++) {
          in_ix->vki.draw.quad.setPosD(_x+ (float)(b* s->texBG.dx), _y+ (float)(a* s->texBG.dy), 0.0f, (float)s->texBG.dx, (float)s->texBG.dy);
          in_ix->vki.draw.quad.setTex((b% 2? p->se: p->s0), (a% 2? p->te: p->t0), (b% 2? p->s0: p->se), (a% 2? p->t0: p->te), 0.0f);  /// inverse tex coords not even

          in_ix->vki.draw.quad.cmdPushPos(in_cmd);
          in_ix->vki.draw.quad.cmdPushTex(in_cmd);
          in_ix->vki.draw.quad.cmdDraw(in_cmd);
        }
      }
    } /// pass thru all possible background texturing types

    /// restore scissor
    in_ix->vki.cmdScissor(in_cmd, &_clip);
    //in_ix->vk.CmdSetScissor(in_cmd, 0, 1, &_clip.getVkRect2D());

  } /// if using a texture
  
  


  /// useColorOnTexture usage flag - use color for current texture
  if(s->useColorOnTexture) in_ix->vki.draw.quad.push.color= *_colorBRDtoUse;
  else                     in_ix->vki.draw.quad.push.color.set(1.0f, 1.0f, 1.0f, 1.0f);;
  in_ix->vki.draw.quad.cmdPushColor(in_cmd);
  
  ///---------------------------------------------------------------
  // WINDOW BORDER ========================-------------------------
  ///---------------------------------------------------------------

  // TOP border ===============----------------------------------
  if(s->useTexture && s->bTexBRD[_BRD_TOP]) {     /// there's texture for it
    /// nr of times the texture repeats
    nrS= (int)(pos.dx/ (float)s->texBRD[_BRD_TOP].dx)+ 1; //nrS= ((int)pos.dx/ (int)s->texBRD[_BRD_TOP].dx)+ (((int)pos.dx% (int)s->texBRD[_BRD_TOP].dx)? 1: 0);
    //nrT= (pos.dy/ s->texBRD[_BRD_TOP].dy)+ ((pos.dy% s->texBRD[_BRD_TOP].dy)? 1: 0);
    float yorg= _y- s->texBRDdist[_BRD_BOTTOM];

    clp.setD(_x, yorg, pos.dx, pos.dy);
    clp.intersectRect(_clip);
    in_ix->vki.cmdScissor(in_cmd, &clp);
    in_ix->vki.draw.quad.flagTexture(1);
    in_ix->vki.draw.quad.cmdTexture(in_cmd, t);

    /// FIXED & STRETCHED border
    if(s->texBRDwrap[_BRD_TOP]<= 1) {

      if(s->texBRDwrap[_BRD_TOP]== 0)
        in_ix->vki.draw.quad.setPosD(_x, yorg, 0.0f, (float)s->texBRD[_BRD_TOP].dx, (float)s->texBRD[_BRD_TOP].dy);
      else
        in_ix->vki.draw.quad.setPosD(_x, yorg, 0.0f, pos.dx, (float)s->texBRD[_BRD_TOP].dy);

      in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_TOP].s0, s->texBRD[_BRD_TOP].t0, s->texBRD[_BRD_TOP].se, s->texBRD[_BRD_TOP].te);
      
      in_ix->vki.draw.quad.cmdPushPos(in_cmd);
      in_ix->vki.draw.quad.cmdPushTex(in_cmd);
      in_ix->vki.draw.quad.cmdDraw(in_cmd);

    /// REPEAT border
    } else if(s->texBRDwrap[_BRD_TOP]== 2) {
      in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_TOP].s0, s->texBRD[_BRD_TOP].t0, s->texBRD[_BRD_TOP].se, s->texBRD[_BRD_TOP].te);
      in_ix->vki.draw.quad.cmdPushTex(in_cmd);

      for(int a= 0; a< nrS; a++) {
        in_ix->vki.draw.quad.setPosD(_x+ (float)(a* s->texBRD[_BRD_TOP].dx), yorg, 0.0f, (float)s->texBRD[_BRD_TOP].dx, (float)s->texBRD[_BRD_TOP].dy);
        in_ix->vki.draw.quad.cmdPushPos(in_cmd);
        in_ix->vki.draw.quad.cmdDraw(in_cmd);
      }

    /// MIRRORED REPEAT border
    } else if(s->texBRDwrap[_BRD_TOP]== 3) {
      for(int a= 0; a< nrS; a++) {
        in_ix->vki.draw.quad.setTex((a% 2? s->texBRD[_BRD_TOP].se: s->texBRD[_BRD_TOP].s0), s->texBRD[_BRD_TOP].t0,
                                    (a% 2? s->texBRD[_BRD_TOP].s0: s->texBRD[_BRD_TOP].se), s->texBRD[_BRD_TOP].te);

        in_ix->vki.draw.quad.setPosD(_x+ (float)(a* s->texBRD[_BRD_TOP].dx), yorg, 0.0f, (float)s->texBRD[_BRD_TOP].dx, (float)s->texBRD[_BRD_TOP].dy);

        in_ix->vki.draw.quad.cmdPushPos(in_cmd);
        in_ix->vki.draw.quad.cmdPushTex(in_cmd);
        in_ix->vki.draw.quad.cmdDraw(in_cmd);
      }
    }
  } /// if there's a TOP texture


  // BOTTOM border ===============----------------------------------
  if(s->useTexture && s->bTexBRD[_BRD_BOTTOM]) {   /// there's texture for it
    /// nr of times the texture repeats
    nrS= (int)(pos.dx/ (float)s->texBRD[_BRD_BOTTOM].dx)+ 1; //nrS= (int)pos.dx/ (int)s->texBRD[_BRD_BOTTOM].dx+ (((int)pos.dx% (int)s->texBRD[_BRD_BOTTOM].dx)? 1: 0);
    //nrT= pos.dy/ s->texBRD[_BRD_BOTTOM].dy+ ((pos.dy% s->texBRD[_BRD_BOTTOM].dy)? 1: 0);
    float yorg= (_y+ pos.dy- (float)s->texBRD[_BRD_BOTTOM].dy+ s->texBRDdist[_BRD_BOTTOM]);
                
    clp.setD(_x, yorg, pos.dx, pos.dy);
    clp.intersectRect(_clip);
    in_ix->vki.cmdScissor(in_cmd, &clp);

    /// fixed border
    if(s->texBRDwrap[_BRD_BOTTOM]<= 1) {
      if(s->texBRDwrap[_BRD_BOTTOM]== 0)
        in_ix->vki.draw.quad.setPosD(_x, yorg, 0.0f, (float)s->texBRD[_BRD_BOTTOM].dx, (float)s->texBRD[_BRD_BOTTOM].dy);
      else
        in_ix->vki.draw.quad.setPosD(_x, yorg, 0.0f, pos.dx, (float)s->texBRD[_BRD_BOTTOM].dy);

      in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_BOTTOM].s0, s->texBRD[_BRD_BOTTOM].t0, s->texBRD[_BRD_BOTTOM].se, s->texBRD[_BRD_BOTTOM].te);
      in_ix->vki.draw.quad.cmdPushPos(in_cmd);
      in_ix->vki.draw.quad.cmdPushTex(in_cmd);
      in_ix->vki.draw.quad.cmdDraw(in_cmd);

    /// repeat border
    } else if(s->texBRDwrap[_BRD_BOTTOM]== 2) {
      in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_BOTTOM].s0, s->texBRD[_BRD_BOTTOM].t0, s->texBRD[_BRD_BOTTOM].se, s->texBRD[_BRD_BOTTOM].te);
      in_ix->vki.draw.quad.cmdPushTex(in_cmd);

      for(int a= 0; a< nrS; a++) {
        in_ix->vki.draw.quad.setPosD(_x+ (float)(a* s->texBRD[_BRD_BOTTOM].dx), yorg, 0.0f, (float)s->texBRD[_BRD_BOTTOM].dx, (float)s->texBRD[_BRD_BOTTOM].dy);
        in_ix->vki.draw.quad.cmdPushPos(in_cmd);
        in_ix->vki.draw.quad.cmdDraw(in_cmd);
      }

    /// mirrored repeat border
    } else if(s->texBRDwrap[_BRD_BOTTOM]== 3) {
      for(int a= 0; a< nrS; a++) {
        in_ix->vki.draw.quad.setTex((a% 2? s->texBRD[_BRD_BOTTOM].se: s->texBRD[_BRD_BOTTOM].s0), s->texBRD[_BRD_BOTTOM].t0,
                                    (a% 2? s->texBRD[_BRD_BOTTOM].s0: s->texBRD[_BRD_BOTTOM].se), s->texBRD[_BRD_BOTTOM].te);

        in_ix->vki.draw.quad.setPosD(_x+ (float)(a* s->texBRD[_BRD_BOTTOM].dx), yorg, 0.0f, (float)s->texBRD[_BRD_BOTTOM].dx, (float)s->texBRD[_BRD_BOTTOM].dy);

        in_ix->vki.draw.quad.cmdPushPos(in_cmd);
        in_ix->vki.draw.quad.cmdPushTex(in_cmd);
        in_ix->vki.draw.quad.cmdDraw(in_cmd);
      }
    }
  } /// if there's a BOTTOM texture
  

  // RIGHT border ===============----------------------------------
  if(s->useTexture && s->bTexBRD[_BRD_RIGHT]) {     /// there's texture for it
    /// nr of times the texture repeats
    //nrS= pos.dx/ s->texBRD[_BRD_RIGHT].dx+ ((pos.dx% s->texBRD[_BRD_RIGHT].dx)? 1: 0);
    nrT= (int)(pos.dy/ (float)s->texBRD[_BRD_RIGHT].dy)+ 1; //nrT= (int)pos.dy/ (int)s->texBRD[_BRD_RIGHT].dy+ (((int)pos.dy% (int)s->texBRD[_BRD_RIGHT].dy)? 1: 0);
    float xorg= (_x+ pos.dx+ s->texBRDdist[_BRD_RIGHT]- (float)s->texBRD[_BRD_RIGHT].dx);
    
    clp.setD(xorg, _y, pos.dx, pos.dy);
    clp.intersectRect(_clip);
    in_ix->vki.cmdScissor(in_cmd, &clp);

    /// FIXED & STRETCHED border
    if(s->texBRDwrap[_BRD_RIGHT]<= 1) {
      in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_RIGHT].s0, s->texBRD[_BRD_RIGHT].t0, s->texBRD[_BRD_RIGHT].se, s->texBRD[_BRD_RIGHT].te);
      in_ix->vki.draw.quad.cmdPushTex(in_cmd);

      if(s->texBRDwrap[_BRD_RIGHT]== 0)
        in_ix->vki.draw.quad.setPosD(xorg, _y, 0.0f, (float)s->texBRD[_BRD_RIGHT].dx, (float)s->texBRD[_BRD_RIGHT].dy);
      else 
        in_ix->vki.draw.quad.setPosD(xorg, _y, 0.0f, (float)s->texBRD[_BRD_RIGHT].dx, pos.dy);

      in_ix->vki.draw.quad.cmdPushPos(in_cmd);
      in_ix->vki.draw.quad.cmdDraw(in_cmd);

    /// REPEAT border
    } else if(s->texBRDwrap[_BRD_RIGHT]== 2) {
      in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_RIGHT].s0, s->texBRD[_BRD_RIGHT].t0, s->texBRD[_BRD_RIGHT].se, s->texBRD[_BRD_RIGHT].te);
      in_ix->vki.draw.quad.cmdPushTex(in_cmd);

      for(int a= 0; a< nrT; a++) {
        in_ix->vki.draw.quad.setPosD(xorg, _y+ (float)(a* s->texBRD[_BRD_RIGHT].dy), 0.0f, (float)s->texBRD[_BRD_RIGHT].dx, (float)s->texBRD[_BRD_RIGHT].dy);
        in_ix->vki.draw.quad.cmdPushPos(in_cmd);
        in_ix->vki.draw.quad.cmdDraw(in_cmd);
      }

    /// MIRRORED REPEAT border
    } else if(s->texBRDwrap[_BRD_RIGHT]== 3) {
      
      for(int a= 0; a< nrT; a++) {
        in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_RIGHT].s0, (a% 2? s->texBRD[_BRD_RIGHT].te: s->texBRD[_BRD_RIGHT].t0),
                                    s->texBRD[_BRD_RIGHT].se, (a% 2? s->texBRD[_BRD_RIGHT].t0: s->texBRD[_BRD_RIGHT].te));
        in_ix->vki.draw.quad.setPosD(xorg, _y+ (float)(a* s->texBRD[_BRD_RIGHT].dy), 0.0f, (float)s->texBRD[_BRD_RIGHT].dx, (float)s->texBRD[_BRD_RIGHT].dy);

        in_ix->vki.draw.quad.cmdPushTex(in_cmd);
        in_ix->vki.draw.quad.cmdPushPos(in_cmd);
        in_ix->vki.draw.quad.cmdDraw(in_cmd);
      }
    }
  } /// if there's a RIGHT texture


  // LEFT border ===============----------------------------------
  if(s->useTexture && s->bTexBRD[3]) {     /// there's texture for it
    /// nr of times the texture repeats
    //nrS= pos.dx/ s->texBRD[_BRD_LEFT].dx+ ((pos.dx% s->texBRD[_BRD_LEFT].dx)? 1: 0);
    nrT= (int)(pos.dy/ (float)s->texBRD[_BRD_LEFT].dy)+ 1;  //nrT= (int)pos.dy/ (int)s->texBRD[_BRD_LEFT].dy+ (((int)pos.dy% (int)s->texBRD[_BRD_LEFT].dy)? 1: 0);
    float xorg= (_x- s->texBRDdist[_BRD_LEFT]);

    clp.setD(xorg, _y, pos.dx, pos.dy);
    clp.intersectRect(_clip);
    in_ix->vki.cmdScissor(in_cmd, &clp);

    /// FIXED & STRETCHED border
    if(s->texBRDwrap[_BRD_LEFT]<= 1) {
      in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_LEFT].s0, s->texBRD[_BRD_LEFT].t0, s->texBRD[_BRD_LEFT].se, s->texBRD[_BRD_LEFT].te);
      in_ix->vki.draw.quad.cmdPushTex(in_cmd);

      if(s->texBRDwrap[_BRD_LEFT]== 0)
        in_ix->vki.draw.quad.setPosD(xorg, _y, 0.0f, (float)s->texBRD[_BRD_LEFT].dx, (float)s->texBRD[_BRD_LEFT].dy);
      else 
        in_ix->vki.draw.quad.setPosD(xorg, _y, 0.0f, (float)s->texBRD[_BRD_LEFT].dx, pos.dy);

      in_ix->vki.draw.quad.cmdPushPos(in_cmd);
      in_ix->vki.draw.quad.cmdDraw(in_cmd);

    /// REPEAT border
    } else if(s->texBRDwrap[_BRD_LEFT]== 2) {
      in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_LEFT].s0, s->texBRD[_BRD_LEFT].t0, s->texBRD[_BRD_LEFT].se, s->texBRD[_BRD_LEFT].te);
      in_ix->vki.draw.quad.cmdPushTex(in_cmd);

      for(int a= 0; a< nrT; a++) {
        in_ix->vki.draw.quad.setPosD(xorg, _y+ (float)(a* s->texBRD[_BRD_LEFT].dy), 0.0f, (float)s->texBRD[_BRD_LEFT].dx, (float)s->texBRD[_BRD_LEFT].dy);
        in_ix->vki.draw.quad.cmdPushPos(in_cmd);
        in_ix->vki.draw.quad.cmdDraw(in_cmd);
      }

    /// MIRRORED REPEAT border
    } else if(s->texBRDwrap[_BRD_LEFT]== 3) {
      
      for(int a= 0; a< nrT; a++) {
        in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_LEFT].s0, (a% 2? s->texBRD[_BRD_LEFT].te: s->texBRD[_BRD_LEFT].t0),
                                    s->texBRD[_BRD_LEFT].se, (a% 2? s->texBRD[_BRD_LEFT].t0: s->texBRD[_BRD_LEFT].te));
        in_ix->vki.draw.quad.setPosD(xorg, _y+ (float)(a* s->texBRD[_BRD_LEFT].dy), 0.0f, (float)s->texBRD[_BRD_LEFT].dx, (float)s->texBRD[_BRD_LEFT].dy);

        in_ix->vki.draw.quad.cmdPushTex(in_cmd);
        in_ix->vki.draw.quad.cmdPushPos(in_cmd);
        in_ix->vki.draw.quad.cmdDraw(in_cmd);
      }
    }
  } /// if there's a LEFT texture

  
  // CORNERS ==============-----------------------

  in_ix->vki.cmdScissor(in_cmd, &_clip);

  // TOP- LEFT corner
  if(s->useTexture && s->bTexBRD[_BRD_TOPLEFT]) {
    in_ix->vki.draw.quad.cmdTexture(in_cmd, t);
    in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_TOPLEFT].s0, s->texBRD[_BRD_TOPLEFT].t0, s->texBRD[_BRD_TOPLEFT].se, s->texBRD[_BRD_TOPLEFT].te);
    in_ix->vki.draw.quad.setPosD(_x- s->texBRDdist[_BRD_TOPLEFT],
                                 _y- s->texBRDdist[_BRD_TOPLEFT], 0.0f,
                                 (float)s->texBRD[_BRD_TOPLEFT].dx, (float)s->texBRD[_BRD_TOPLEFT].dy);

    in_ix->vki.draw.quad.cmdPushTex(in_cmd);
    in_ix->vki.draw.quad.cmdPushPos(in_cmd);
    in_ix->vki.draw.quad.cmdDraw(in_cmd);
  }
  
  // TOP- RIGHT corner
  if(s->useTexture && s->bTexBRD[_BRD_TOPRIGHT]) {
    in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_TOPRIGHT].s0, s->texBRD[_BRD_TOPRIGHT].t0, s->texBRD[_BRD_TOPRIGHT].se, s->texBRD[_BRD_TOPRIGHT].te);
    in_ix->vki.draw.quad.setPosD(_x+ pos.dx- (float)s->texBRD[_BRD_TOPRIGHT].dx+ s->texBRDdist[_BRD_TOPRIGHT],
                                 _y- s->texBRDdist[_BRD_TOPRIGHT], 0.0f,
                                 (float)s->texBRD[_BRD_TOPRIGHT].dx, (float)s->texBRD[_BRD_TOPRIGHT].dy);

    in_ix->vki.draw.quad.cmdPushTex(in_cmd);
    in_ix->vki.draw.quad.cmdPushPos(in_cmd);
    in_ix->vki.draw.quad.cmdDraw(in_cmd);
  }

  // BOTTOM- RIGHT corner
  if(s->useTexture && s->bTexBRD[_BRD_BOTTOMRIGHT]) {
    in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_BOTTOMRIGHT].s0, s->texBRD[_BRD_BOTTOMRIGHT].t0, s->texBRD[_BRD_BOTTOMRIGHT].se, s->texBRD[_BRD_BOTTOMRIGHT].te);
    in_ix->vki.draw.quad.setPosD(_x+ pos.dx- (float)s->texBRD[_BRD_BOTTOMRIGHT].dx+ s->texBRDdist[_BRD_BOTTOMRIGHT],
                                 _y+ pos.dy- (float)s->texBRD[_BRD_BOTTOMRIGHT].dx+ s->texBRDdist[_BRD_BOTTOMRIGHT], 0.0f,
                                 (float)s->texBRD[_BRD_BOTTOMRIGHT].dx, (float)s->texBRD[_BRD_BOTTOMRIGHT].dy);

    in_ix->vki.draw.quad.cmdPushTex(in_cmd);
    in_ix->vki.draw.quad.cmdPushPos(in_cmd);
    in_ix->vki.draw.quad.cmdDraw(in_cmd);
  }
  
  // BOTTOM- LEFT corner
  if(s->useTexture && s->bTexBRD[_BRD_BOTTOMLEFT]) {
    in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_BOTTOMLEFT].s0, s->texBRD[_BRD_BOTTOMLEFT].t0, s->texBRD[_BRD_BOTTOMLEFT].se, s->texBRD[_BRD_BOTTOMLEFT].te);
    in_ix->vki.draw.quad.setPosD(_x- s->texBRDdist[_BRD_BOTTOMLEFT],
                                 _y+ pos.dy- (float)s->texBRD[_BRD_BOTTOMLEFT].dx+ s->texBRDdist[_BRD_BOTTOMLEFT], 0.0f,
                                 (float)s->texBRD[_BRD_BOTTOMLEFT].dx, (float)s->texBRD[_BRD_BOTTOMLEFT].dy);

    in_ix->vki.draw.quad.cmdPushTex(in_cmd);
    in_ix->vki.draw.quad.cmdPushPos(in_cmd);
    in_ix->vki.draw.quad.cmdDraw(in_cmd);
  }
  
  // border untextured - nonTexturedBorderWidth pixels width
  if(!s->useTexture) {
    in_ix->vki.draw.quad.flagTexture(false);
    in_ix->vki.draw.quad.push.hollow= s->nonTexturedBorderWidth;
    in_ix->vki.draw.quad.push.color= *_colorBRDtoUse;
    in_ix->vki.draw.quad.setPosD(_x, _y, 0.0f, pos.dx, pos.dy);

    in_ix->vki.draw.quad.cmdPushAll(in_cmd);
    in_ix->vki.draw.quad.cmdDraw(in_cmd);
    /// restore hollow, it's usually off
    in_ix->vki.draw.quad.push.hollow= -1.0f;
    in_ix->vki.draw.quad.cmdPushHollow(in_cmd);
  }


  // print all the static text
  if(staticPrintList.nrNodes) {
    ixFontStyle *saveStyle= in_ix->pr.style;
    for(StaticPrint *p= (StaticPrint *)staticPrintList.first; p; p= (StaticPrint *)p->next) {
      if(!p->visible) continue;
      in_ix->pr.style= &p->fntStyle;
      in_ix->pr.txt3f(_x+ p->pos.x, _y+ p->pos.y, p->pos.z, p->text);
    }
    in_ix->pr.style= saveStyle;
  }
}


#endif /// IX_USE_VULKAN
































// ##    ##   ######     ######       ####     ##########   ########
// ##    ##   ##    ##   ##    ##   ##    ##       ##       ##
// ##    ##   ######     ##    ##   ########       ##       ######
// ##    ##   ##         ##    ##   ##    ##       ##       ##
//   ####     ##         ######     ##    ##       ##       ########



///================================================================///
// base main UPDATE func ****************************************** //
///================================================================///


bool ixBaseWindow::_update(bool in_updateChildren) {
  rectf r;
  bool insideThis;
  float mx, my, mdx, mdy;
  if(!_is->visible) return false;

  if(in_updateChildren)
    if(_updateChildren())
      return true;

  if(_is->disabled) return false;

  mx= _scaleDiv(in.m.x), my= _scaleDiv(in.m.y);
  mdx= _scaleDiv(in.m.dx), mdy= _scaleDiv(in.m.dy);

  getPosVD(&r);
  insideThis= r.inside(mx, my);

  ixWinSys::_Op *dbg= &Ix::wsys()._op;      // current [op]eration
  // if an action is in progress
  if(Ix::wsys()._op.win) {
    if(Ix::wsys()._op.win!= this)      /// if the action is not done on this window, any update is ceased. (one action on one window only)
      return false;

    // THERE HAVE TO BE WINDOW DRAGGING USING JOYSTICK, KEYBOARDS, TOO
    // ACTION IN PROGRESS MUST BE RESET WHEN APPLICATION LOSES FOCUS

    /// button released - the action stopped
    if(!in.m.but[0].down) {
      Ix::wsys()._op.delData();
      Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
      return true;
    }
    
    /// a window drag is in progress
    if(Ix::wsys()._op.moving) {
      //move(Ix::wsys()._op.wx+ (mx- Ix::wsys()._op.x),
      //     Ix::wsys()._op.wy+ (my- Ix::wsys()._op.y));
      moveDelta(mdx, mdy);
      Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
      return true;
    }

    /// different resizes from here on
    if(Ix::wsys()._op.resizeBottom) {
      //if(Ix::wsys()._op.wy
      if(pos.dy+ mdy>= getMinDy()) {
        resizeDelta(0.0f, mdy);
      }
      Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
      return true;
    }

    if(Ix::wsys()._op.resizeLeft) {
      if(pos.dx- mdx>= getMinDx()) {
        pos.moveD(mdx, 0.0f);
        resizeDelta(-mdx, 0.0f);
      }
      Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
      return true;
    }
    if(Ix::wsys()._op.resizeRight) {
      if(pos.dx+ mdx>= getMinDx()) {
        resizeDelta(mdx, 0.0f);
      }
      Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
      return true;
    }





  // no current operation is in progress - check if a new operation is being started
  } else {
    

    if(insideThis) {
      //Ix::wsys().unitAtCursor; >>>> //float unit= (float)_getUnitCoords(mx, my)* scaleUI;   // 720p[1], 1080p[2], 1620p[3], 2160p[4] etc, minimum 1
      
      if(in.m.but[0].down) {
        /// check if user wants to drag the window
        if(_usage->movable) {
        
          /// if window has no title, moving is done by dragging anywhere inside the window
          //if(_mINSIDE(x+ 1, y+ 1, pos.dx- 2, pos.dy- 2)) {      /// 5 pixels ok?
          //if(insideThis) {
            Ix::wsys()._op.moving= true;
            Ix::wsys()._op.win= this;
            Ix::wsys()._op.posOrg.set(mx, my);
            Ix::wsys()._op.posWin.set(pos.x0, pos.y0);
            Ix::wsys().bringToFront(this);
            Ix::wsys().focus= this;
            Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);

            return true;
          //}

        } /// useMovable

        /// check if user wants to resize the window
        if(_usage->resizeable) {
          /// left resize
          if(_mINSIDE(r.x0- (Ix::wsys().unitAtCursor* 5.0f), r.y0, Ix::wsys().unitAtCursor* 5.0f, r.dy)) {  // unit: <1080p 1, 1080p[2], 1620p[3], 2160p[4], etc
            Ix::wsys()._op.resizeLeft= true;
            Ix::wsys()._op.win= this;
            Ix::wsys()._op.posOrg.set(mx, my);
            Ix::wsys()._op.posWin.set(pos.x0, pos.y0);
            Ix::wsys().bringToFront(this);
            Ix::wsys().focus= this;
            Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
            return true;
          }
          /// resize right
          if(_mINSIDE(r.xe, r.y0, Ix::wsys().unitAtCursor* 5.0f, r.dy)) {
            Ix::wsys()._op.resizeRight= true;
            Ix::wsys()._op.win= this;
            Ix::wsys()._op.posOrg.set(mx, my);
            Ix::wsys()._op.posWin.set(pos.x0, pos.y0);
            Ix::wsys().bringToFront(this);
            Ix::wsys().focus= this;
            Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
            return true;
          }
          /// resize bottom
          if(_mINSIDE(r.x0, r.ye, r.dx, Ix::wsys().unitAtCursor* 5.0f)) {
            Ix::wsys()._op.resizeBottom= true;
            Ix::wsys()._op.win= this;
            Ix::wsys()._op.posOrg.set(mx, my);
            Ix::wsys()._op.posWin.set(pos.x0, pos.y0);
            Ix::wsys().bringToFront(this);
            Ix::wsys().focus= this;
            Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
            return true;
          }
        } /// useResizeable

        // a click in the window makes it have focus
        //if(insideThis) {
          Ix::wsys().focus= this;
          Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed); 
          return true;
        //}

      } /// if left mouse button is down
    } /// mouse is inside (parent)
  } /// check if starting of a drag or resize

  return false; // no operation was done on this window or it's children
}


bool ixBaseWindow::_updateChildren() {
  bool ret= false;

  // first the scrolls
  if(vscroll) if(vscroll->update()) return true;
  if(hscroll) if(hscroll->update()) return true;

  // the rest of children here, excluding scrolls
  for(ixBaseWindow *p= (ixBaseWindow *)childrens.first; p; p= (ixBaseWindow *)p->next)
    if(p!= vscroll || p!= hscroll)
      if(p->update())
        ret= true;

  return ret;
}


















