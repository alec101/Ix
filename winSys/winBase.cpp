#include "ix/ix.h"


/*
==================================================================
WINDOW 0,0 POSITION: THIS IS NOT 100% SET IN STONE ATM, THERE'S MUCH TO TAKE INTO CONSIDERATION
window positions rant:
-there could be the option for 0,0 to be top-left
 in this case, window y will be negative, so not positive
 so keep the normal + and - directions

  so children would be on the negative position... sigh
==================================================================
  */






/* TODO:
 - window border must use ixQuad imediate funcs, tons of uniforms are sent for nothing
 - see _setWinPosBorderTo0() for detailed thinking. childrens that are in parent's childArea, must stick to the borders of childArea
   exceptions are the title (if inside), scrolls (both).
   windows that are hooked from the outside, will always hook to the window's edges
   so there's 2 hooks that must happen, it's not hard, few funcs have to have bits of changes

     
       

  // THE RIGHT WAY WOULD BE THE CHILDAREA. but not all stick to that area. only children of that window, and not all of those children.
  // so for now, im gonna stick to the exotic method.
*/

/*
hook funcs must not touch the children/parents
changing the parent like this, is not good. you might not want that to happen, and most times you don't

you moved/resized? update all your children's hooks
then send the parent a msg "update the windows tied to me"
*/


/*
CHILD AREA NOTES:
i think it should always start on 0,0
there will be negative positioned children, and those you just cannot show i think
it's either that, or the child area could have negative values...
but where would you have a start ... i dono it's weird to be able to have negative values
you'd need the window somehow to have an area... and be kinda known, fixed...
alowing the view to go on a negative...

neah
negative would just be outside the clipping, i think, it's the best option
go with 0,0 see if problems arise, but i doubt it

or, there could be set manually somehow
maybe this option should exist
setChildViewAreaFixed()
and if this flag is set to true, no _computeChildArea() happens...
*/




/* hooking: child area? window border?
  basically
    for sure, there's a need for all outside windows to hook to the window's edge
      but, the children that form the child area, should not
      also scrolls and title should not
      the things get bit complicated hmmm
      there is the option for everything to stick to the edges, no sticking to the child area...
      this text should remain....



      the more i think on it... i think it's best to define the window, and everything about it will be in that space
      it's the best option
      there's already the _view
      and the combo of some outside, some inside is bad i think
      if everything would be on the outside, there will stillb e computations to know the boundaries of all that conglomeration that forms the window
        so...
*/

#include "ix/winSys/_privateDef.h"

using namespace mlib;


ixBaseWindow::ixBaseWindow(): hook(this), pos(0) {
  _type= _IX_BASE_WINDOW;
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

  //_handlesTex= false;
  delData();
}


ixBaseWindow::~ixBaseWindow() {
  delData();
}


void ixBaseWindow::delData() {

  usage.delData();
  is.delData();

  //setHookAnchor();    // defaults to border 7 - virtual desktop (bottom left) - CAUSES CRASH ON DESTRUCTOR

  /// base parameters
  pos.set(0, 0, 0, 0);
  //style->delData();

  // ALL CHILDRENS WILL BE DELETED
  while(childrens.first)
    childrens.del(childrens.first);

  hscroll= vscroll= null;
}





bool ixBaseWindow::_inBounds(Ix *in_ix) {
  recti r(pos); r.moveD(hook.pos.x, hook.pos.y);
  for(int a= 0; a< in_ix->gpu->nrMonitors; a++) {
    osiMonitor *m= in_ix->gpu->monitor[a];
    if(r.intersect(recti(m->x0, m->y0, m->x0+ m->dx, m->y0+ m->dy)))
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
    hscroll->style= &Ix::wsys().selStyle->scroll;
    hscroll->target= this;
    hscroll->orientation= 0;

    hscroll->hook.set(this, 4, 4);
    
  }

  if(!vscroll) {
    vscroll= new ixScroll;
    childrens.add(vscroll);
    vscroll->_clipUsePrentsParent= true;

    vscroll->parent= this;
    vscroll->target= this;
    vscroll->style= &Ix::wsys().selStyle->scroll;
    vscroll->orientation= 1;

    vscroll->hook.set(this, 4, 4);
    
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

void ixBaseWindow::move(int32 in_x, int32 in_y) {
  int32 deltax= in_x- pos.x0;
  int32 deltay= in_y- pos.y0;
  pos.move(in_x, in_y);

  _computeAllDelta(deltax, deltay);   // THIS1 HAD hook.updateHooksDelta(x,y), SO IT DID UPDATE this (HOPEFULLY AN ERROR)
}


// moves window and all children a delta distance (deltax, deltay)
void ixBaseWindow::moveDelta(int32 in_dx, int32 in_dy) {
  pos.moveD(in_dx, in_dy);
  _computeAllDelta(in_dx, in_dy);
}

// resizes window, this will move all children hooked on the right and bottom side
void ixBaseWindow::resize(int32 in_dx, int32 in_dy) {
  pos.resize(in_dx, in_dy);
  _computeAll();
}


void ixBaseWindow::resizeDelta(int32 in_dx, int32 in_dy) {
  pos.resizeD(in_dx, in_dy);
  _computeAll();
}


void ixBaseWindow::setPos(int32 x0,int32 y0,int32 dx,int32 dy) {
  pos.setD(x0, y0, dx, dy);
  _computeAll();
}

//
//int32 ixBaseWindow::getMinDx() {
//  return 15;
//}
//
//int32 ixBaseWindow::getMinDy() {
//  return 15;
//}
//

/*
// returns window coordinates based on the hook, in the virtual dektop
void ixBaseWindow::getVDcoords2f(float *out_x, float *out_y) {
  if(out_x) *out_x= (float)(hook.pos.x+ pos.x0);
  if(out_y) *out_y= (float)(hook.pos.y+ pos.y0);
}


void ixBaseWindow::getVDcoords2i(int32 *out_x, int32 *out_y) {
  if(out_x) *out_x= hook.pos.x+ pos.x0;
  if(out_y) *out_y= hook.pos.y+ pos.y0;
}


void ixBaseWindow::getVDcoordsv3(vec3i *out) {
  if(out) out->set(hook.pos.x+ pos.x0, hook.pos.y+ pos.y0, 0);
}


void ixBaseWindow::getVDcoordsRecti(recti *out) {
  if(out) out->setD(hook.pos.x+ pos.x0, hook.pos.y+ pos.y0, pos.dx, pos.dy);
}
*/

void ixBaseWindow::_computeAll() {
  _computeViewArea();
  _computeChildArea();

  /// update either the parent or directly the hooks
  if(parent) {
    parent->_computeChildArea();
    parent->hook.updateHooks(false);
    if(parent->hscroll) parent->hscroll->_computeButtons();
    if(parent->vscroll) parent->vscroll->_computeButtons();
    
  } else
    hook.updateHooks();
  
  if(vscroll) { vscroll->_computePos(); vscroll->_computeButtons(); }
  if(hscroll) { hscroll->_computePos(); hscroll->_computeButtons(); }
  _computeClipPlane();
}


void ixBaseWindow::_computeAllDelta(int32 in_dx, int32 in_dy) {
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
  //if(_childArea.xe< _viewArea.dx) _childArea.xe= _viewArea.dx;  /// minimum the window dx
  //if(_childArea.ye< _viewArea.dy) _childArea.ye= _viewArea.dy;  /// minimum the window dy

  for(ixBaseWindow *p= (ixBaseWindow *)childrens.first; p; p= (ixBaseWindow *)p->next) {
    if(p== vscroll || p== hscroll) continue;
    if(_type== _IX_WINDOW)
      if(p== ((ixWindow *)this)->title) continue;

    if(p->hook.ixWin== this) {  //  OPTION 1, if ixWin is this, else somehow compute the real pos in this
      recti &r(p->pos);

      if(p->hook.border== IX_BORDER_BOTTOMLEFT) {
        if(_childArea.xe< r.xe)  _childArea.xe= r.xe;
        if(_childArea.ye< -r.y0) _childArea.ye= -r.y0;

      } else if(p->hook.border== IX_BORDER_TOPLEFT) {
        if(_childArea.xe< r.xe)  _childArea.xe= r.xe;
        if(_childArea.ye< r.ye)  _childArea.ye= r.ye;

      } else if(p->hook.border== IX_BORDER_TOPRIGHT) {
        if(_childArea.xe< -r.x0) _childArea.xe= -r.x0;
        if(_childArea.ye< r.ye)  _childArea.ye= r.ye;

      } else if(p->hook.border== IX_BORDER_BOTTOMRIGHT) {
        if(_childArea.xe< -r.x0) _childArea.xe= -r.x0;
        if(_childArea.ye< -r.y0) _childArea.ye= -r.y0;

      } else if(p->hook.border== IX_BORDER_TOP) {
        if(_childArea.xe< (int32)r.dx) _childArea.xe= (int32)r.dx;
        if(_childArea.ye< r.ye)        _childArea.ye= r.ye;

      } else if(p->hook.border== IX_BORDER_RIGHT) {
        if(_childArea.xe< -r.x0)       _childArea.xe= -r.x0;
        if(_childArea.ye< (int32)r.dy) _childArea.ye= (int32)r.dy;

      } else if(p->hook.border== IX_BORDER_BOTTOM) {
        if(_childArea.xe< (int32)r.dx) _childArea.xe= (int32)r.dx;
        if(_childArea.ye< -r.y0)       _childArea.ye= -r.y0;

      } else if(p->hook.border== IX_BORDER_LEFT) {
        if(_childArea.xe< r.xe)        _childArea.xe= r.xe;
        if(_childArea.ye< (int32)r.dy) _childArea.ye= (int32)r.dy;

      } else
        error.detail("unknown border", __FUNCTION__);


    // window is hooked to another window in this parent's childArea
    } else {
      recti r= p->pos;
      if(p->hook.ixWin) {
        /* original
        if(p->hook.border== IX_BORDER_BOTTOMLEFT); // do nothing
        else if(p->hook.border== IX_BORDER_TOPLEFT)     r.moveD(0,                        p->hook.ixWin->pos.dy);
        else if(p->hook.border== IX_BORDER_TOPRIGHT)    r.moveD(p->hook.ixWin->pos.dx,    p->hook.ixWin->pos.dy);
        else if(p->hook.border== IX_BORDER_BOTTOMRIGHT) r.moveD(p->hook.ixWin->pos.dx,    0);
        else if(p->hook.border== IX_BORDER_TOP)         r.moveD(p->hook.ixWin->pos.dx/ 2, p->hook.ixWin->pos.dy);
        else if(p->hook.border== IX_BORDER_RIGHT)       r.moveD(p->hook.ixWin->pos.dx,    p->hook.ixWin->pos.dy/ 2);
        else if(p->hook.border== IX_BORDER_BOTTOM)      r.moveD(p->hook.ixWin->pos.dx/ 2, 0);
        else if(p->hook.border== IX_BORDER_LEFT)        r.moveD(0,                        p->hook.ixWin->pos.dy/ 2);
        */

             if(p->hook.border== IX_BORDER_TOPLEFT); // do nothing
        else if(p->hook.border== IX_BORDER_BOTTOMLEFT)  r.moveD(0,                        p->hook.ixWin->pos.dy);
        else if(p->hook.border== IX_BORDER_TOPRIGHT)    r.moveD(p->hook.ixWin->pos.dx,    0);
        else if(p->hook.border== IX_BORDER_BOTTOMRIGHT) r.moveD(p->hook.ixWin->pos.dx,    p->hook.ixWin->pos.dy);
        else if(p->hook.border== IX_BORDER_TOP)         r.moveD(p->hook.ixWin->pos.dx/ 2, 0);
        else if(p->hook.border== IX_BORDER_BOTTOM)      r.moveD(p->hook.ixWin->pos.dx/ 2, p->hook.ixWin->pos.dy);
        else if(p->hook.border== IX_BORDER_LEFT)        r.moveD(0,                        p->hook.ixWin->pos.dy/ 2);
        else if(p->hook.border== IX_BORDER_RIGHT)       r.moveD(p->hook.ixWin->pos.dx,    p->hook.ixWin->pos.dy/ 2);


        r.moveD(p->hook.ixWin->pos.x0, p->hook.ixWin->pos.y0);
      }

      if(_childArea.xe< r.xe)  _childArea.xe= r.xe;
      if(_childArea.ye< r.ye)  _childArea.ye= r.ye;
    }
  }

  _childArea.compDeltas();
}


void ixBaseWindow::_computeViewArea() {
  _viewArea.setD(0, 0, pos.dx, pos.dy);
  if(vscroll)
    if(vscroll->is.visible)
      _viewArea.xe-= vscroll->pos.dx;

  if(hscroll)
    if(hscroll->is.visible)
      _viewArea.ye-= hscroll->pos.dy;   // ORIGIN UPDATED

  _viewArea.compDeltas();
  _viewArea.setD(_viewArea.x0, _viewArea.y0, (_viewArea.dx< 0? 0: _viewArea.dx), (_viewArea.dy< 0? 0: _viewArea.dy));

  if(_type== _IX_EDIT)
    ((ixEdit *)this)->text._computeWrapLen();
  else if(_type== _IX_STATIC_TEXT)
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
        _clip.setD(osi.display.vx0, osi.display.vy0, osi.display.vdx, osi.display.vdy);
      }
    } else {
      parent->_getVDviewArea(&_clip);
      _clip.intersectRect(parent->_clip);
    }
  } else
    _clip.setD(osi.display.vx0, osi.display.vy0, osi.display.vdx, osi.display.vdy);

  // update all children's clipping
  for(ixBaseWindow *p= (ixBaseWindow *)childrens.first; p; p= (ixBaseWindow *)p->next)
    p->_computeClipPlane();
}


void ixBaseWindow::_computeClipPlaneDelta(int32 in_dx, int32 in_dy) {
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












///=======================================///
// window HOOKING class =======----------- //
///==================== (no not that one) ///

ixWinHook::ixWinHook(void *in_p): pos(0) {
  parent= (ixBaseWindow *)in_p;
  border= 0;
  ixWin= null;
  osiWin= null;
  osiMon= null;
  _anchorToEdge= true;
}


void ixWinHook::delData() {
  border= 0;
  ixWin= null;
  osiWin= null;
  osiMon= null;
  _anchorToEdge= true;
  pos.set(0, 0, 0);
}


inline void ixWinHook::_compute() {
  int32 dx, dy;
  if(ixWin) {
    pos= ixWin->hook.pos;
    pos.x+= ixWin->pos.x0;
    pos.y+= ixWin->pos.y0;

    if(_anchorToEdge)
      dx= ixWin->pos.dx, dy= ixWin->pos.dy;
    else {
      dx= ixWin->_childArea.dx, dy= ixWin->_childArea.dy;

      //if(ixWin->hscroll)          //            BOTTOM ORIGIN <<<<<
      //  if(ixWin->hscroll->is.visible)
      //    pos.y-= ixWin->hscroll->pos.dy;
    }

  } else if(osiWin) {
    pos.set(osiWin->x0, osiWin->y0, 0);
    dx= osiWin->dx, dy= osiWin->dy;

  } else if(osiMon) {
    pos.set(osiMon->x0, osiMon->y0, 0);
    dx= osiMon->dx, dy= osiMon->dy;
  } else {
    pos.set(0, 0, 0);
    dx= osi.display.vdx, dy= osi.display.vdy;
  }

  // bottom left don't have to do nothin, only the scroll adjust
       if(border== IX_BORDER_TOPLEFT);
  else if(border== IX_BORDER_BOTTOMLEFT)                 pos.y+= dy;
  else if(border== IX_BORDER_TOPRIGHT)    pos.x+= dx;
  else if(border== IX_BORDER_BOTTOMRIGHT) pos.x+= dx,    pos.y+= dy;
  else if(border== IX_BORDER_TOP)         pos.x+= dx/ 2;
  else if(border== IX_BORDER_BOTTOM)      pos.x+= dx/ 2, pos.y+= dy;
  else if(border== IX_BORDER_LEFT)                       pos.y+= dy/ 2;
  else if(border== IX_BORDER_RIGHT)       pos.x+= dx,    pos.y+= dy/ 2;
  

  // adjust the hook with the scrolling
  /// skip updating the hook if anchored to another window on same parent - the hooks are already scroll-computed
  if(ixWin)
    if(ixWin== parent->parent)
      _adjustHookPosWithScroll();
}

// adjusts the window hookPos with scrolling, if it is required. window title+ other children are not affected
inline void ixWinHook::_adjustHookPosWithScroll(/*ixBaseWindow *out_window*/) {
  /// see if scrolling will affect the window
  if(parent->parent== null) return;             /// no parent, no scroll
  if(parent->parent->hscroll== parent) return;  /// parent's scroll is this window, nothing to do here
  if(parent->parent->vscroll== parent) return;  /// parent's scroll is this window, nothing to do here
  if(parent->parent->_type== _IX_WINDOW)        /// parent's title is this window, nothing to do here
    if(((ixWindow *)parent->parent)->title== parent) return;

  // scrolling affects this window if reached this point
  if(parent->parent->hscroll) pos.x-= parent->parent->hscroll->position;
  if(parent->parent->vscroll) pos.y-= parent->parent->vscroll->position;
}


void ixWinHook::_computeDelta(int32 in_dx, int32 in_dy) {
  pos.x+= in_dx,
  pos.y+= in_dy;
  //_adjustHookPosWithScroll();
}


void ixWinHook::_computeParentHooks() {
  ixBaseWindow *p= (ixBaseWindow *)(parent->parent? parent->parent->childrens.first: Ix::wsys().topObjects.first);
  
  for(; p; p= (ixBaseWindow *)p->next)
    if(p->hook.ixWin== parent)
      p->hook.updateHooks();
}


void ixWinHook::_computeParentHooksDelta(int32 in_dx, int32 in_dy) {
  //_computeParentHooks();
  ixBaseWindow *p= (ixBaseWindow *)(parent->parent? parent->parent->childrens.first: Ix::wsys().topObjects.first);

  for(; p; p= (ixBaseWindow *)p->next)
    if(p->hook.ixWin== parent)
    //if(p->parent== parent)
      p->hook.updateHooksDelta(in_dx, in_dy);
}


// in_border, is the border of <this> not the target window - basically what border to touch what border
void ixWinHook::_setWinPosBorderTo0(int8 in_border) {
  // this func will act only on window position, nothing to do with hook or parent
  int32 dx= parent->pos.dx, dy= parent->pos.dy;

       if(in_border== IX_BORDER_TOPLEFT)     parent->pos.move(0,      0     );  // was 0,     -dy
  else if(in_border== IX_BORDER_BOTTOMLEFT)  parent->pos.move(0,      -dy   );  // was 0,     0
  else if(in_border== IX_BORDER_TOPRIGHT)    parent->pos.move(-dx,    0     );  // was -dx,   -dy
  else if(in_border== IX_BORDER_BOTTOMRIGHT) parent->pos.move(-dx,    -dy   );  // was -dx,   0
  else if(in_border== IX_BORDER_TOP)         parent->pos.move(-dx/ 2, 0     );  // was -dx/2, -dy
  else if(in_border== IX_BORDER_BOTTOM)      parent->pos.move(-dx/ 2, -dy   );  // was -dx/2, 0
  else if(in_border== IX_BORDER_LEFT)        parent->pos.move(0,      -dy/ 2);  // was 0,     -dy/2
  else if(in_border== IX_BORDER_RIGHT)       parent->pos.move(-dx,    -dy/ 2);  // was -dx,   -dy/2
}


inline bool ixWinHook::_computeAnchorToEdgeVar(ixBaseWindow *in_window) {
  if(in_window== null) return true;
  
  // the anchor is the parent, then the border will be the childArea (most cases, ofc)
  if(in_window== parent->parent) {
    if(parent== parent->parent->vscroll || parent== parent->parent->hscroll)
      return true;        // this is the scroll of the parent
    
    if(parent->parent->_type== _IX_WINDOW)
      if(((ixWindow *)parent->parent)->title== parent)
        return true;      // this is the title of a window
  } else
    return true;          // the anchor is another window that has the same parent

  return false;           // the window is another children like this
}






// window hooking functions =============-----------------------

void ixWinHook::setAnchor(ixBaseWindow *in_window, int8 in_border) {
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


void ixWinHook::setAnchor(osiWindow *in_window, int8 in_border) {
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


void ixWinHook::setAnchor(osiMonitor *in_window, int8 in_border) {
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
void ixWinHook::setAnchor(int8 in_border) {
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
void ixWinHook::set(ixBaseWindow *in_window, int8 in_border1, int8 in_border2) {
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
void ixWinHook::set(osiWindow *in_window, int8 in_border1, int8 in_border2) {
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
void ixWinHook::set(osiMonitor *in_window, int8 in_border1, int8 in_border2) {
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
void ixWinHook::set(int8 in_border1, int8 in_border2) {
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


void ixWinHook::updateHooksDelta(int32 in_dx, int32 in_dy, bool in_updateThis) {
  //updateHooks(in_updateThis);
  
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


///=============================================================///
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
  error.makeme(__FUNCTION__);
  /*
  if(!sl)
    sl= in_ix->wsys.getShader(in_ix);
  if(!sl) return;

  ixFontStyle *_saveFont= in_ix->pr.style;
  
  //glEnable(GL_TEXTURE_2D);
  glUseProgram(sl->gl->id);
  glUniform4f(sl->u_color, 1.0f, 1.0f, 1.0f, 1.0f);
  
  // any osiWindow move -> update the camera
  if(osi.flags.windowMoved || osi.flags.windowResized)
    sl->glUpdateViewportPos();

  glUniformMatrix4fv(sl->u_camera, 1, GL_FALSE,  in_ix->camera->cameraMat);
  glUniform1ui(sl->u_useTexture, 1);

  glEnable(GL_SCISSOR_TEST);

  // ixDraw inits
  // >>>>>>>  in_ix->shaders.updateAllViewportPos(); <<<<<<<<<<

  in_ix->glo.draw.quad.useProgram();
  in_ix->glo.draw.quad.setCamera(in_ix->camera);
  in_ix->glo.draw.quad.disableTexture();
  in_ix->glo.draw.quad.delClipPlane();
  in_ix->glo.draw.quad.setColor(1.0f, 1.0f, 1.0f, 1.0f);


  in_ix->glo.draw.circle.useProgram();
  in_ix->glo.draw.circle.setCamera(in_ix->camera);
  in_ix->glo.draw.circle.disableTexture();
  in_ix->glo.draw.circle.setThick(2.0f);
  

  in_ix->glo.draw.triangle.useProgram();
  in_ix->glo.draw.triangle.setCamera(in_ix->camera);
  in_ix->glo.draw.triangle.disableTexture();


  glDisable(GL_DEPTH_TEST);   /// no depth testing - just draw over what is behind
  //glDisable(GL_TEXTURE_2D);
  */
}



void ixBaseWindow::_glDrawFinish(Ix *in_ix) {
  error.makeme(__FUNCTION__);
  /*
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_SCISSOR_TEST);
  in_ix->pr.delScissor();
  in_ix->pr.style= _saveFont;
  */
}



void ixBaseWindow::_glDraw(Ix *in_ix, ixWSsubStyleBase *in_style) {
  error.makeme(__FUNCTION__);
  /*
  // visibility is not that easy, check for a minimized button first...

  if(!_clip.exists())
    return;

  if(!_inBounds(in_ix)) return;     // nothing to draw, the ix+gpu don't draw this window

  bool _debug= true;

  ixWSgenericStyle *s= (ixWSgenericStyle *)in_style;
  if(!s)
    s= (ixWSgenericStyle *)style;
  
  ixWSstyle::GPU *sGPU= s->parent->getGPUassets(in_ix);
  if(!sGPU) return;


  /// get all required assets
  //ixWinSys::ixWSshader *sl= in_ix->wsys.getShader(in_ix);   //if(!sl) sl= in_ix->wsys.loadShader(in_ix);    WHY LOAD HERE? IF THERE'S AN ERROR, IT'S GONNA LOAD EVERY FRAME...
  //if(!sl) return;           // at this point, there's an error    


  /// tmp vars
  recti clp;                  /// will be used for further clip-in-clip
  int nrS, nrT;               /// these will hold the number of times the texture will repeat on S and T axis
  int32 _x, _y;
  getVDcoords2i(&_x, &_y);
  
  if(_debug) {
    char buf[256];  // DEBUG print buffer
    int cy= in_ix->pr.getCharDy(in_ix->fnt5x6)+ 1;
    ixFontStyle *sav= in_ix->pr.style;
    in_ix->pr.style= &in_ix->debugStyle;

    sprintf(buf, "pos[%d,%d %dx%d] hookPos[%d,%d]", pos.x0, pos.y0, pos.dx, pos.dy, hook.pos.x, hook.pos.y);
    in_ix->pr.txt2i(_x, _y- cy, buf);
    sprintf(buf, "childArea[%d,%d %dx%d] viewArea[%d,%d %dx%d]", _childArea.x0, _childArea.y0, _childArea.dx, _childArea.dy, _viewArea.x0, _viewArea.y0, _viewArea.dx, _viewArea.dy);
    in_ix->pr.txt2i(_x, _y- cy* 2, buf);

    in_ix->pr.style= sav;
  }


  // this should be placed in the calling program, tho...
  // placing multiple disables & enables is not good i think
  // enable - draw scene / disable - draw menus
  

  in_ix->glo.draw.quad.useProgram();
  in_ix->glo.draw.quad.setClipPlaneR(_clip);

  glUseProgram(in_sl->gl->id);
  glBindVertexArray(sGPU->VAOid);
  
  glBindBuffer(GL_ARRAY_BUFFER, sGPU->VBOid);

  glUniform1ui(in_sl->u_useTexture, 1);
  glUniform3f(in_sl->u_origin, 0.0f, 0.0f, 0.0f);  // TO BE OR NOT TO BE <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  glUniform1ui(in_sl->u_disabled, is.disabled);

  glScissor(_clip.x0, _clip.y0, _clip.dx, _clip.dy);
  
  // WINDOW BACKGROUND =========-----------
  
  /// useBackColor usage flag - draw a rectangle using current color
  if(s->useBackColor) {
    in_ix->glo.draw.quad.useProgram();
    in_ix->glo.draw.quad.setColorv(color);
    in_ix->glo.draw.quad.setCoordsDi(_x, _y, pos.dx, pos.dy);
    in_ix->glo.draw.quad.render();

    /* PRE ixDraw CHANGE ALL
    glUniform4fv(in_sl->u_color, 1, color.v);
    
    //must be able to draw this simple box... atm nothing is drawn for some reason... check if i made the change in the camera or not, too
    //glUniform1ui(in_sl->u_useTexture, 0);
    //glUniform1ui(in_sl->u_customPos, 1);            // enable shader custom quad draw (vert positions)
    //glUniform1ui(in_sl->u_customTex, 1);            // enable shader custom quad draw (tex coords)
    //glUniform2f(in_sl->u_quadPos0, (float)_x, (float)_y);
    //glUniform2f(in_sl->u_quadPosE, (float)(_x+ pos.dx), (float)(_y+ pos.dy));
    //glUniform2f(in_sl->u_quadTex0, 0.0f, 0.0f);
    //glUniform2f(in_sl->u_quadTexE, 1.0f, 1.0f);
    //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //glUniform1ui(in_sl->u_customPos, 0);            // disable shader custom quad draw
    //glUniform1ui(in_sl->u_customTex, 0);            // disable shader custom quad draw
    //glUniform1ui(in_sl->u_useTexture, 1);
    
  }



  /// useColorOnTexture usage flag - use color for current texture
  glUseProgram(in_sl->gl->id);
  if(s->useColorOnTexture)
    glUniform4fv(in_sl->u_color, 1, color.v);
  else
    glUniform4f(in_sl->u_color, 1.0f, 1.0f, 1.0f, 1.0f);
  
  if(s->useTexture) 
    glBindTexture(sGPU->tex->glData.target, sGPU->tex->glData.id);
  
  
  /// BACKGROUND texturing
  if(s->useTexture && s->bTexBG) {
 
    /// nr of times the texture repeats
    nrS= pos.dx/ s->texBG.dx+ ((pos.dx% s->texBG.dx)? 1: 0);
    nrT= pos.dy/ s->texBG.dy+ ((pos.dy% s->texBG.dy)? 1: 0);
    
    // FIXED background
    if(s->texBGwrap== 0) {
      clp.setD(_x, _y, pos.dx, pos.dy);
      clp.intersectRect(_clip);
      glScissor(clp.x0, clp.y0, clp.dx, clp.dy);

      glUniform3f(in_sl->u_origin, (float)_x, (float)_y, 0.0f);

      glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex, 4);

      in_sl->glSetScissor(_clip);


    // STRETCHED background
    } else if(s->texBGwrap== 1) {

      glUniform1ui(in_sl->u_customPos, 1);         /// enable custom quad position
      glUniform2f(in_sl->u_quadPos0, (float)_x, (float)_y);
      glUniform2f(in_sl->u_quadPosE, (float)(_x+ pos.dx), (float)(_y+ pos.dy));
      glUniform3f(in_sl->u_origin, 0.0f, 0.0f, 0.0f);

      glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex, 4);

      glUniform1ui(in_sl->u_customPos, 0);         /// disable custom quad position

    
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

      clp.setD(_x, _y, pos.dx, pos.dy);
      clp.intersectRect(_clip);
      in_sl->glSetScissor(clp);
      
      for(int a= 0; a< nrT; a++) 
        for(int b= 0; b< nrS; b++) {
          glUniform3f(in_sl->u_origin, (float)(_x+ b* s->texBG.dx), (float)(_y+ a* s->texBG.dy), 0.0f);
          glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex, 4);
        }

      in_sl->glSetScissor(_clip);

    } else if(s->texBGwrap== 3) {
      clp.setD(_x, _y, pos.dx, pos.dy);
      clp.intersectRect(_clip);
      in_sl->glSetScissor(clp);

      glUniform1ui(in_sl->u_customTex, 1);         /// enable custom texcoords

      ixSubTex *p= &s->texBG;

      for(short a= 0; a< nrT; a++) {
        for(short b= 0; b< nrS; b++) {
          glUniform3f(in_sl->u_origin, (float)(_x+ b* s->texBG.dx), (float)(_y+ a* s->texBG.dy), 0.0f);

          /// inverse tex coords not even
          glUniform2f(in_sl->u_quadTex0, (b% 2? p->se: p->s0), (a% 2? p->te: p->t0));
          glUniform2f(in_sl->u_quadTexE, (b% 2? p->s0: p->se), (a% 2? p->t0: p->te));

          glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex, 4);
        }
      }

      glUniform1ui(in_sl->u_customTex, 0);         /// disable custom texcoords
      in_sl->glSetScissor(_clip);

    } /// pass thru all possible background texturing types
  } /// if using a texture
  



  /// useColorOnTexture usage flag - use color for current texture
  if(s->useColorOnTexture)
    glUniform4fv(in_sl->u_color, 1, colorBRD.v);
  else
    glUniform4f(in_sl->u_color, 1.0f, 1.0f, 1.0f, 1.0f);
  
  ///---------------------------------------------------------------
  // WINDOW BORDER ========================-------------------------
  ///---------------------------------------------------------------


  // TOP border ===============----------------------------------
  if(s->useTexture && s->bTexBRD[_BRD_TOP]) {     /// there's texture for it
    /// nr of times the texture repeats
    nrS= pos.dx/ s->texBRD[_BRD_TOP].dx+ ((pos.dx% s->texBRD[_BRD_TOP].dx)? 1: 0);
    nrT= pos.dy/ s->texBRD[_BRD_TOP].dy+ ((pos.dy% s->texBRD[_BRD_TOP].dy)? 1: 0);

    float yorg= (float)(_y+ pos.dy+ s->texBRDdist[_BRD_TOP]- s->texBRD[_BRD_TOP].dy);
    clp.setD(_x, mlib::roundf(yorg), pos.dx, pos.dy);
    clp.intersectRect(_clip);
    in_sl->glSetScissor(clp);


    /// fixed border
    if(s->texBRDwrap[_BRD_TOP]== 0) {
      glUniform3f(in_sl->u_origin, (float)_x, yorg, 0.0f);
      glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex+ _VBOID_BRD_TOP, 4);

    /// stretched border
    } else if(s->texBRDwrap[_BRD_TOP]== 1) {
      glUniform3f(in_sl->u_origin, (float)_x, yorg, 0.0f);
      glUniform1ui(in_sl->u_customPos, 1);         /// enable custom vertex position
      glUniform2f(in_sl->u_quadPos0, 0.0f, 0.0f);
      glUniform2f(in_sl->u_quadPosE, (float)pos.dx, (float)s->texBRD[_BRD_TOP].dy);

      glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex+ _VBOID_BRD_TOP, 4);

      glUniform1ui(in_sl->u_customPos, 0);         /// disable custom vertex position

    /// repeat border
    } else if(s->texBRDwrap[_BRD_TOP]== 2) {
      for(int a= 0; a< nrS; a++) {
        glUniform3f(in_sl->u_origin, (float)(_x+ a* s->texBRD[_BRD_TOP].dx), yorg, 0.0f);
        glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex+ _VBOID_BRD_TOP, 4);
      }

    /// mirrored repeat border
    } else if(s->texBRDwrap[_BRD_TOP]== 3) {
      glUniform1ui(in_sl->u_customTex, 1);         /// enable custom tex coords
      
      for(int a= 0; a< nrS; a++) {
        glUniform3f(in_sl->u_origin, (float)_x+ a* s->texBRD[_BRD_TOP].dx, yorg, 0.0f);
        glUniform2f(in_sl->u_quadTex0, (a% 2? s->texBRD[_BRD_TOP].se: s->texBRD[_BRD_TOP].s0), s->texBRD[_BRD_TOP].t0);
        glUniform2f(in_sl->u_quadTexE, (a% 2? s->texBRD[_BRD_TOP].s0: s->texBRD[_BRD_TOP].se), s->texBRD[_BRD_TOP].te);

        glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex+ _VBOID_BRD_TOP, 4);
      }

      glUniform1ui(in_sl->u_customTex, 0);         /// disable custom tex coords
    }
    in_sl->glSetScissor(_clip);
  } /// if there's a TOP texture


  // BOTTOM border ===============----------------------------------
  if(s->useTexture && s->bTexBRD[_BRD_BOTTOM]) {   /// there's texture for it
    /// nr of times the texture repeats
    nrS= pos.dx/ s->texBRD[_BRD_BOTTOM].dx+ ((pos.dx% s->texBRD[_BRD_BOTTOM].dx)? 1: 0);
    nrT= pos.dy/ s->texBRD[_BRD_BOTTOM].dy+ ((pos.dy% s->texBRD[_BRD_BOTTOM].dy)? 1: 0);
    float yorg= _y- s->texBRDdist[_BRD_BOTTOM];
    
    clp.setD(_x, mlib::roundf(yorg), pos.dx, pos.dy);
    clp.intersectRect(_clip);
    in_sl->glSetScissor(clp);

    /// fixed border
    if(s->texBRDwrap[_BRD_BOTTOM]== 0) {
      glUniform3f(in_sl->u_origin, (float)_x, yorg, 0.0f);
      glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex+ _VBOID_BRD_BOTTOM, 4);

    /// stretched border
    } else if(s->texBRDwrap[_BRD_BOTTOM]== 1) {
      glUniform3f(in_sl->u_origin, (float)_x, yorg, 0.0f);
      glUniform1ui(in_sl->u_customPos, 1);         /// enable custom vertex position
      glUniform2f(in_sl->u_quadPos0, 0, 0);
      glUniform2f(in_sl->u_quadPosE, (float)pos.dx, (float)s->texBRD[_BRD_BOTTOM].dy);

      glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex+ _VBOID_BRD_BOTTOM, 4);

      glUniform1ui(in_sl->u_customPos, 0);         /// disable custom vertex position

    /// repeat border
    } else if(s->texBRDwrap[_BRD_BOTTOM]== 2) {
      for(int a= 0; a< nrS; a++) {
        glUniform3f(in_sl->u_origin, (float)(_x+ a* s->texBRD[_BRD_BOTTOM].dx), yorg, 0.0f);
        glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex+ _VBOID_BRD_BOTTOM, 4);
      }

    /// mirrored repeat border
    } else if(s->texBRDwrap[_BRD_BOTTOM]== 3) {
      glUniform1ui(in_sl->u_customTex, 1);         /// enable custom tex coords
      
      for(int a= 0; a< nrS; a++) {
        glUniform3f(in_sl->u_origin, (float)(_x+ a* s->texBRD[_BRD_BOTTOM].dx), yorg, 0.0f);
        glUniform2f(in_sl->u_quadTex0, (a% 2? s->texBRD[_BRD_BOTTOM].se: s->texBRD[_BRD_BOTTOM].s0), s->texBRD[_BRD_BOTTOM].t0);
        glUniform2f(in_sl->u_quadTexE, (a% 2? s->texBRD[_BRD_BOTTOM].s0: s->texBRD[_BRD_BOTTOM].se), s->texBRD[_BRD_BOTTOM].te);

        glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex+ _VBOID_BRD_BOTTOM, 4);
      }

      glUniform1ui(in_sl->u_customTex, 0);         /// disable custom tex coords
    }
    in_sl->glSetScissor(_clip);
  } /// if there's a BOTTOM texture
  

  // RIGHT border ===============----------------------------------
  if(s->useTexture && s->bTexBRD[_BRD_RIGHT]) {     /// there's texture for it
    /// nr of times the texture repeats
    nrS= pos.dx/ s->texBRD[_BRD_RIGHT].dx+ ((pos.dx% s->texBRD[_BRD_RIGHT].dx)? 1: 0);
    nrT= pos.dy/ s->texBRD[_BRD_RIGHT].dy+ ((pos.dy% s->texBRD[_BRD_RIGHT].dy)? 1: 0);
    float xorg= (float)(_x+ pos.dx+ s->texBRDdist[_BRD_RIGHT]- s->texBRD[_BRD_RIGHT].dx);
    float yorg= (float)(_y+ pos.dy- s->texBRD[_BRD_RIGHT].dy);
    
    clp.setD(mlib::roundf(xorg), _y, pos.dx, pos.dy);
    clp.intersectRect(_clip);
    in_sl->glSetScissor(clp);

    /// fixed border
    if(s->texBRDwrap[_BRD_RIGHT]== 0) {
      glUniform3f(in_sl->u_origin, xorg, yorg, 0.0f);
      glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex+ _VBOID_BRD_RIGHT, 4);

    /// stretched border
    } else if(s->texBRDwrap[_BRD_RIGHT]== 1) {
      glUniform3f(in_sl->u_origin, xorg, (float)_y, 0.0f);
      glUniform1ui(in_sl->u_customPos, 1);         /// enable custom vertex position
      glUniform2f(in_sl->u_quadPos0, 0.0f, 0.0f);
      glUniform2f(in_sl->u_quadPosE, (float)s->texBRD[_BRD_RIGHT].dx, (float)pos.dy);

      glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex+ _VBOID_BRD_RIGHT, 4);

      glUniform1ui(in_sl->u_customPos, 0);         /// disable custom vertex position

    /// repeat border
    } else if(s->texBRDwrap[_BRD_RIGHT]== 2) {
      for(int a= 0; a< nrT; a++) {
        glUniform3f(in_sl->u_origin, xorg, yorg- a* s->texBRD[_BRD_RIGHT].dy, 0.0f);
        glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex+ _VBOID_BRD_RIGHT, 4);
      }

    /// mirrored repeat border
    } else if(s->texBRDwrap[_BRD_RIGHT]== 3) {
      glUniform1ui(in_sl->u_customTex, 1);         /// enable custom tex coords
      
      for(int a= 0; a< nrT; a++) {
        glUniform3f(in_sl->u_origin, xorg, yorg- a* s->texBRD[_BRD_RIGHT].dy, 0.0f);
        glUniform2f(in_sl->u_quadTex0, s->texBRD[_BRD_RIGHT].s0, (a% 2? s->texBRD[_BRD_RIGHT].te: s->texBRD[_BRD_RIGHT].t0));
        glUniform2f(in_sl->u_quadTexE, s->texBRD[_BRD_RIGHT].se, (a% 2? s->texBRD[_BRD_RIGHT].t0: s->texBRD[_BRD_RIGHT].te));

        glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex+ _VBOID_BRD_RIGHT, 4);
      }

      glUniform1ui(in_sl->u_customTex, 0);         /// disable custom tex coords
    }
    in_sl->glSetScissor(_clip);
  } /// if there's a RIGHT texture


  // LEFT border ===============----------------------------------
  if(s->useTexture && s->bTexBRD[3]) {     /// there's texture for it
    /// nr of times the texture repeats
    nrS= pos.dx/ s->texBRD[_BRD_LEFT].dx+ ((pos.dx% s->texBRD[_BRD_LEFT].dx)? 1: 0);
    nrT= pos.dy/ s->texBRD[_BRD_LEFT].dy+ ((pos.dy% s->texBRD[_BRD_LEFT].dy)? 1: 0);
    float xorg= (float)(_x- s->texBRDdist[_BRD_LEFT]);
    float yorg= (float)(_y+ pos.dy- s->texBRD[_BRD_LEFT].dy);

    clp.setD(mlib::roundf(xorg), _y, pos.dx, pos.dy);
    clp.intersectRect(_clip);
    in_sl->glSetScissor(clp);

    /// fixed border
    if(s->texBRDwrap[_BRD_LEFT]== 0) {
      glUniform3f(in_sl->u_origin, xorg, yorg, 0.0f);
      glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex+ _VBOID_BRD_LEFT, 4);

    /// stretched border
    } else if(s->texBRDwrap[_BRD_LEFT]== 1) {
      glUniform3f(in_sl->u_origin, xorg, (float)_y, 0.0f);
      glUniform1ui(in_sl->u_customPos, 1);         /// enable custom vertex position
      glUniform2f(in_sl->u_quadPos0, 0.0f, 0.0f);
      glUniform2f(in_sl->u_quadPosE, (float)s->texBRD[_BRD_LEFT].dx, (float)pos.dy);

      glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex+ _VBOID_BRD_LEFT, 4);

      glUniform1ui(in_sl->u_customPos, 0);         /// disable custom vertex position

    /// repeat border
    } else if(s->texBRDwrap[_BRD_LEFT]== 2) {
      for(int a= 0; a< nrT; a++) {
        glUniform3f(in_sl->u_origin, xorg, yorg- a* s->texBRD[_BRD_LEFT].dy, 0.0f);
        glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex+ _VBOID_BRD_LEFT, 4);
      }

    /// mirrored repeat border
    } else if(s->texBRDwrap[_BRD_LEFT]== 3) {
      glUniform1ui(in_sl->u_customTex, 1);         /// enable custom tex coords
      
      for(int a= 0; a< nrT; a++) {
        glUniform3f(in_sl->u_origin, xorg, yorg- a* s->texBRD[_BRD_LEFT].dy, 0.0f);
        glUniform2f(in_sl->u_quadTex0, s->texBRD[_BRD_LEFT].s0, (a% 2? s->texBRD[_BRD_LEFT].te: s->texBRD[_BRD_LEFT].t0));
        glUniform2f(in_sl->u_quadTexE, s->texBRD[_BRD_LEFT].se, (a% 2? s->texBRD[_BRD_LEFT].t0: s->texBRD[_BRD_LEFT].te));

        glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex+ _VBOID_BRD_LEFT, 4);
      }

      glUniform1ui(in_sl->u_customTex, 0);         /// disable custom tex coords
    }
    in_sl->glSetScissor(_clip);
  } /// if there's a LEFT texture


  // CORNERS ==============-----------------------

  // TOP- LEFT corner
  if(s->useTexture && s->bTexBRD[_BRD_TOPLEFT]) {
    glUniform3f(in_sl->u_origin, _x- s->texBRDdist[_BRD_TOPLEFT],
                              _y+ pos.dy- s->texBRD[_BRD_TOPLEFT].dy+ s->texBRDdist[_BRD_TOPLEFT],
                              0.0f);
    glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex+ _VBOID_BRD_TOPLEFT, 4);
  }

  // TOP- RIGHT corner
  if(s->useTexture && s->bTexBRD[_BRD_TOPRIGHT]) {
    glUniform3f(in_sl->u_origin, _x+ pos.dx- s->texBRD[_BRD_TOPRIGHT].dx+ s->texBRDdist[_BRD_TOPRIGHT],
                              _y+ pos.dy- s->texBRD[_BRD_TOPRIGHT].dy+ s->texBRDdist[_BRD_TOPRIGHT],
                              0.0f);
    glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex+ _VBOID_BRD_TOPRIGHT, 4);

  }

  // BOTTOM- RIGHT corner
  if(s->useTexture && s->bTexBRD[_BRD_BOTTOMRIGHT]) {
    glUniform3f(in_sl->u_origin, _x+ pos.dx- s->texBRD[_BRD_BOTTOMRIGHT].dx+ s->texBRDdist[_BRD_BOTTOMRIGHT],
                              _y- s->texBRDdist[_BRD_BOTTOMRIGHT],
                              0.0f);
    glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex+ _VBOID_BRD_BOTTOMRIGHT, 4);
  }

  // BOTTOM- LEFT corner
  if(s->useTexture && s->bTexBRD[_BRD_BOTTOMLEFT]) {
    glUniform3f(in_sl->u_origin, _x- s->texBRDdist[_BRD_BOTTOMLEFT],
                              _y- s->texBRDdist[_BRD_BOTTOMLEFT],
                              0.0f);
    glDrawArrays(GL_TRIANGLE_STRIP, s->VBOindex+ _VBOID_BRD_BOTTOMLEFT, 4);
  }


  // border untextured - 3 pixels width
  if(!s->useTexture) {
    in_ix->glo.draw.quad.useProgram();
    in_ix->glo.draw.quad.setColorv(is.disabled? _getDisableColor(s->colorBRD): s->colorBRD );

    in_ix->glo.draw.quad.setCoordsDi(_x, _y, pos.dx, 3);
    in_ix->glo.draw.quad.render();
    in_ix->glo.draw.quad.setCoordsDi(_x, _y+ pos.dy- 3, pos.dx, 3);
    in_ix->glo.draw.quad.render();
    in_ix->glo.draw.quad.setCoordsDi(_x, _y, 3, pos.dy);
    in_ix->glo.draw.quad.render();
    in_ix->glo.draw.quad.setCoordsDi(_x+ pos.dx- 3, _y, 3, pos.dy);
    in_ix->glo.draw.quad.render();
  }


  //in_sl->delClipPlane();
  */
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
  recti clp;                  /// will be used for further clip-in-clip
  int nrS, nrT;               /// these will hold the number of times the texture will repeat on S and T axis
  int32 _x, _y;
  getVDcoords2i(&_x, &_y);

  if(_debug) {
    char buf[256];  // DEBUG print buffer
    int cy= in_ix->pr.getCharDy(in_ix->fnt5x6)+ 1;
    ixFontStyle *sav= in_ix->pr.style;
    in_ix->pr.style= &in_ix->debugStyle;

    sprintf(buf, "pos[%d,%d %dx%d] hookPos[%d,%d]", pos.x0, pos.y0, pos.dx, pos.dy, hook.pos.x, hook.pos.y);
    in_ix->pr.txt2i(_x, _y+ pos.dy+ (cy* 0)+ 1, buf);
    sprintf(buf, "childArea[%d,%d %dx%d] viewArea[%d,%d %dx%d]", _childArea.x0, _childArea.y0, _childArea.dx, _childArea.dy, _viewArea.x0, _viewArea.y0, _viewArea.dx, _viewArea.dy);
    in_ix->pr.txt2i(_x, _y+ pos.dy+ (cy* 1)+ 1, buf);

    in_ix->pr.style= sav;
  }
  
  // setup vkQuad
  /// THE _debug WILL SWITCH PIPELINE...
  in_ix->vk.CmdBindPipeline(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipeline);
  in_ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipelineLayout, 0, 1, &in_ix->vki.glb[in_ix->vki.fi]->set->set, 0, nullptr);
  in_ix->vki.cmdScissor(in_cmd, &_clip);

  in_ix->vki.draw.quad.cmdTexture(in_cmd, t);
  in_ix->vki.draw.quad.flagDisabled(is.disabled);
  in_ix->vki.draw.quad.cmdPushAll(in_cmd);

  // WINDOW BACKGROUND =========-----------

  /// useBackColor usage flag - draw a rectangle using current color
  if(s->useBackColor) {
    in_ix->vki.draw.quad.push.color= color;
    in_ix->vki.draw.quad.flagTexture(false);
    in_ix->vki.draw.quad.setPosDi(_x, _y, 0, pos.dx, pos.dy);
    in_ix->vki.draw.quad.cmdPushAll(in_cmd);
    in_ix->vki.draw.quad.cmdDraw(in_cmd);
  }

  /// useColorOnTexture usage flag - use color for current texture
  if(s->useColorOnTexture) in_ix->vki.draw.quad.push.color= color;
  else                     in_ix->vki.draw.quad.push.color.set(1.0f, 1.0f, 1.0f, 1.0f);;
  in_ix->vki.draw.quad.cmdPushColor(in_cmd);

  in_ix->vki.draw.quad.flagTexture(s->useTexture);
  in_ix->vki.draw.quad.cmdPushFlags(in_cmd);

  /// BACKGROUND texturing
  if(s->useTexture && s->bTexBG) {
 
    /// nr of times the texture repeats
    nrS= pos.dx/ s->texBG.dx+ ((pos.dx% s->texBG.dx)? 1: 0);
    nrT= pos.dy/ s->texBG.dy+ ((pos.dy% s->texBG.dy)? 1: 0);

    /// set scissor
    clp.setD(_x, _y, pos.dx, pos.dy);
    clp.intersectRect(_clip);
    in_ix->vki.cmdScissor(in_cmd, &clp);

    // FIXED/STRETCHED background
    if(s->texBGwrap<= 1) {

      if(s->texBGwrap== 0) in_ix->vki.draw.quad.setPosDi(_x, _y, 0, s->texBG.dx, s->texBG.dy);
      else                 in_ix->vki.draw.quad.setPosDi(_x, _y, 0, pos.dx,      pos.dy);

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
          in_ix->vki.draw.quad.setPosDi(_x+ (b* s->texBG.dx), _y+ (a* s->texBG.dy), 0, s->texBG.dx, s->texBG.dy);
          
          in_ix->vki.draw.quad.cmdPushPos(in_cmd);
          in_ix->vki.draw.quad.cmdDraw(in_cmd);
        }


    // MIRROR REPEAT background wrap
    } else if(s->texBGwrap== 3) {

      ixSubTex *p= &s->texBG;

      for(int a= 0; a< nrT; a++) {
        for(int b= 0; b< nrS; b++) {
          in_ix->vki.draw.quad.setPosDi(_x+ (b* s->texBG.dx), _y+ (a* s->texBG.dy), 0, s->texBG.dx, s->texBG.dy);
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
  if(s->useColorOnTexture) in_ix->vki.draw.quad.push.color= colorBRD;
  else                     in_ix->vki.draw.quad.push.color.set(1.0f, 1.0f, 1.0f, 1.0f);;
  in_ix->vki.draw.quad.cmdPushColor(in_cmd);

  ///---------------------------------------------------------------
  // WINDOW BORDER ========================-------------------------
  ///---------------------------------------------------------------

  // TOP border ===============----------------------------------
  if(s->useTexture && s->bTexBRD[_BRD_TOP]) {     /// there's texture for it
    /// nr of times the texture repeats
    nrS= (pos.dx/ s->texBRD[_BRD_TOP].dx)+ ((pos.dx% s->texBRD[_BRD_TOP].dx)? 1: 0);
    //nrT= (pos.dy/ s->texBRD[_BRD_TOP].dy)+ ((pos.dy% s->texBRD[_BRD_TOP].dy)? 1: 0);
    int32 yorg= _y- (int32)s->texBRDdist[_BRD_BOTTOM];

    clp.setD(_x, yorg, pos.dx, pos.dy);
    clp.intersectRect(_clip);
    //in_ix->vk.CmdSetScissor(in_cmd, 0, 1, &clp.getVkRect2D());
    in_ix->vki.cmdScissor(in_cmd, &clp);
    in_ix->vki.draw.quad.flagTexture(1);
    in_ix->vki.draw.quad.cmdTexture(in_cmd, t);

    /// FIXED & STRETCHED border
    if(s->texBRDwrap[_BRD_TOP]<= 1) {

      if(s->texBRDwrap[_BRD_TOP]== 0)
        in_ix->vki.draw.quad.setPosDi(_x, yorg, 0, s->texBRD[_BRD_TOP].dx, s->texBRD[_BRD_TOP].dy);
      else
        in_ix->vki.draw.quad.setPosDi(_x, yorg, 0, pos.dx, s->texBRD[_BRD_TOP].dy);

      in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_TOP].s0, s->texBRD[_BRD_TOP].t0, s->texBRD[_BRD_TOP].se, s->texBRD[_BRD_TOP].te);
      
      in_ix->vki.draw.quad.cmdPushPos(in_cmd);
      in_ix->vki.draw.quad.cmdPushTex(in_cmd);
      in_ix->vki.draw.quad.cmdDraw(in_cmd);

    /// REPEAT border
    } else if(s->texBRDwrap[_BRD_TOP]== 2) {
      in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_TOP].s0, s->texBRD[_BRD_TOP].t0, s->texBRD[_BRD_TOP].se, s->texBRD[_BRD_TOP].te);
      in_ix->vki.draw.quad.cmdPushTex(in_cmd);

      for(int a= 0; a< nrS; a++) {
        in_ix->vki.draw.quad.setPosDi(_x+ (a* s->texBRD[_BRD_TOP].dx), yorg, 0, s->texBRD[_BRD_TOP].dx, s->texBRD[_BRD_TOP].dy);
        in_ix->vki.draw.quad.cmdPushPos(in_cmd);
        in_ix->vki.draw.quad.cmdDraw(in_cmd);
      }

    /// MIRRORED REPEAT border
    } else if(s->texBRDwrap[_BRD_TOP]== 3) {
      for(int a= 0; a< nrS; a++) {
        in_ix->vki.draw.quad.setTex((a% 2? s->texBRD[_BRD_TOP].se: s->texBRD[_BRD_TOP].s0), s->texBRD[_BRD_TOP].t0,
                                    (a% 2? s->texBRD[_BRD_TOP].s0: s->texBRD[_BRD_TOP].se), s->texBRD[_BRD_TOP].te);

        in_ix->vki.draw.quad.setPosDi(_x+ (a* s->texBRD[_BRD_TOP].dx), yorg, 0, s->texBRD[_BRD_TOP].dx, s->texBRD[_BRD_TOP].dy);

        in_ix->vki.draw.quad.cmdPushPos(in_cmd);
        in_ix->vki.draw.quad.cmdPushTex(in_cmd);
        in_ix->vki.draw.quad.cmdDraw(in_cmd);
      }
    }
  } /// if there's a TOP texture


  // BOTTOM border ===============----------------------------------
  if(s->useTexture && s->bTexBRD[_BRD_BOTTOM]) {   /// there's texture for it
    /// nr of times the texture repeats
    nrS= pos.dx/ s->texBRD[_BRD_BOTTOM].dx+ ((pos.dx% s->texBRD[_BRD_BOTTOM].dx)? 1: 0);
    //nrT= pos.dy/ s->texBRD[_BRD_BOTTOM].dy+ ((pos.dy% s->texBRD[_BRD_BOTTOM].dy)? 1: 0);
    int32 yorg= (_y+ pos.dy- s->texBRD[_BRD_BOTTOM].dy+ (int32)s->texBRDdist[_BRD_BOTTOM]);
                
    clp.setD(_x, yorg, pos.dx, pos.dy);
    clp.intersectRect(_clip);
    in_ix->vki.cmdScissor(in_cmd, &clp);

    /// fixed border
    if(s->texBRDwrap[_BRD_BOTTOM]<= 1) {
      if(s->texBRDwrap[_BRD_BOTTOM]== 0)
        in_ix->vki.draw.quad.setPosDi(_x, yorg, 0, s->texBRD[_BRD_BOTTOM].dx, s->texBRD[_BRD_BOTTOM].dy);
      else
        in_ix->vki.draw.quad.setPosDi(_x, yorg, 0, pos.dx, s->texBRD[_BRD_BOTTOM].dy);

      in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_BOTTOM].s0, s->texBRD[_BRD_BOTTOM].t0, s->texBRD[_BRD_BOTTOM].se, s->texBRD[_BRD_BOTTOM].te);
      in_ix->vki.draw.quad.cmdPushPos(in_cmd);
      in_ix->vki.draw.quad.cmdPushTex(in_cmd);
      in_ix->vki.draw.quad.cmdDraw(in_cmd);

    /// repeat border
    } else if(s->texBRDwrap[_BRD_BOTTOM]== 2) {
      in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_BOTTOM].s0, s->texBRD[_BRD_BOTTOM].t0, s->texBRD[_BRD_BOTTOM].se, s->texBRD[_BRD_BOTTOM].te);
      in_ix->vki.draw.quad.cmdPushTex(in_cmd);

      for(int a= 0; a< nrS; a++) {
        in_ix->vki.draw.quad.setPosDi(_x+ (a* s->texBRD[_BRD_BOTTOM].dx), yorg, 0, s->texBRD[_BRD_BOTTOM].dx, s->texBRD[_BRD_BOTTOM].dy);
        in_ix->vki.draw.quad.cmdPushPos(in_cmd);
        in_ix->vki.draw.quad.cmdDraw(in_cmd);
      }

    /// mirrored repeat border
    } else if(s->texBRDwrap[_BRD_BOTTOM]== 3) {
      for(int a= 0; a< nrS; a++) {
        in_ix->vki.draw.quad.setTex((a% 2? s->texBRD[_BRD_BOTTOM].se: s->texBRD[_BRD_BOTTOM].s0), s->texBRD[_BRD_BOTTOM].t0,
                                    (a% 2? s->texBRD[_BRD_BOTTOM].s0: s->texBRD[_BRD_BOTTOM].se), s->texBRD[_BRD_BOTTOM].te);

        in_ix->vki.draw.quad.setPosDi(_x+ (a* s->texBRD[_BRD_BOTTOM].dx), yorg, 0, s->texBRD[_BRD_BOTTOM].dx, s->texBRD[_BRD_BOTTOM].dy);

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
    nrT= pos.dy/ s->texBRD[_BRD_RIGHT].dy+ ((pos.dy% s->texBRD[_BRD_RIGHT].dy)? 1: 0);
    int32 xorg= (_x+ pos.dx+ (int32)s->texBRDdist[_BRD_RIGHT]- s->texBRD[_BRD_RIGHT].dx);
    
    clp.setD(xorg, _y, pos.dx, pos.dy);
    clp.intersectRect(_clip);
    //in_ix->vk.CmdSetScissor(in_cmd, 0, 1, &clp.getVkRect2D());
    in_ix->vki.cmdScissor(in_cmd, &clp);

    /// FIXED & STRETCHED border
    if(s->texBRDwrap[_BRD_RIGHT]<= 1) {
      in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_RIGHT].s0, s->texBRD[_BRD_RIGHT].t0, s->texBRD[_BRD_RIGHT].se, s->texBRD[_BRD_RIGHT].te);
      in_ix->vki.draw.quad.cmdPushTex(in_cmd);

      if(s->texBRDwrap[_BRD_RIGHT]== 0)
        in_ix->vki.draw.quad.setPosDi(xorg, _y, 0, s->texBRD[_BRD_RIGHT].dx, s->texBRD[_BRD_RIGHT].dy);
      else 
        in_ix->vki.draw.quad.setPosDi(xorg, _y, 0, s->texBRD[_BRD_RIGHT].dx, pos.dy);

      in_ix->vki.draw.quad.cmdPushPos(in_cmd);
      in_ix->vki.draw.quad.cmdDraw(in_cmd);

    /// REPEAT border
    } else if(s->texBRDwrap[_BRD_RIGHT]== 2) {
      in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_RIGHT].s0, s->texBRD[_BRD_RIGHT].t0, s->texBRD[_BRD_RIGHT].se, s->texBRD[_BRD_RIGHT].te);
      in_ix->vki.draw.quad.cmdPushTex(in_cmd);

      for(int a= 0; a< nrT; a++) {
        in_ix->vki.draw.quad.setPosDi(xorg, _y+ (a* s->texBRD[_BRD_RIGHT].dy), 0, s->texBRD[_BRD_RIGHT].dx, s->texBRD[_BRD_RIGHT].dy);
        in_ix->vki.draw.quad.cmdPushPos(in_cmd);
        in_ix->vki.draw.quad.cmdDraw(in_cmd);
      }

    /// MIRRORED REPEAT border
    } else if(s->texBRDwrap[_BRD_RIGHT]== 3) {
      
      for(int a= 0; a< nrT; a++) {
        in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_RIGHT].s0, (a% 2? s->texBRD[_BRD_RIGHT].te: s->texBRD[_BRD_RIGHT].t0),
                                    s->texBRD[_BRD_RIGHT].se, (a% 2? s->texBRD[_BRD_RIGHT].t0: s->texBRD[_BRD_RIGHT].te));
        in_ix->vki.draw.quad.setPosDi(xorg, _y+ (a* s->texBRD[_BRD_RIGHT].dy), 0, s->texBRD[_BRD_RIGHT].dx, s->texBRD[_BRD_RIGHT].dy);

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
    nrT= pos.dy/ s->texBRD[_BRD_LEFT].dy+ ((pos.dy% s->texBRD[_BRD_LEFT].dy)? 1: 0);
    int32 xorg= (_x- (int32)s->texBRDdist[_BRD_LEFT]);

    clp.setD(xorg, _y, pos.dx, pos.dy);
    clp.intersectRect(_clip);
    in_ix->vki.cmdScissor(in_cmd, &clp);
    //in_ix->vk.CmdSetScissor(in_cmd, 0, 1, &clp.getVkRect2D());

    /// FIXED & STRETCHED border
    if(s->texBRDwrap[_BRD_LEFT]<= 1) {
      in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_LEFT].s0, s->texBRD[_BRD_LEFT].t0, s->texBRD[_BRD_LEFT].se, s->texBRD[_BRD_LEFT].te);
      in_ix->vki.draw.quad.cmdPushTex(in_cmd);

      if(s->texBRDwrap[_BRD_LEFT]== 0)
        in_ix->vki.draw.quad.setPosDi(xorg, _y, 0, s->texBRD[_BRD_LEFT].dx, s->texBRD[_BRD_LEFT].dy);
      else 
        in_ix->vki.draw.quad.setPosDi(xorg, _y, 0, s->texBRD[_BRD_LEFT].dx, pos.dy);

      in_ix->vki.draw.quad.cmdPushPos(in_cmd);
      in_ix->vki.draw.quad.cmdDraw(in_cmd);

    /// REPEAT border
    } else if(s->texBRDwrap[_BRD_LEFT]== 2) {
      in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_LEFT].s0, s->texBRD[_BRD_LEFT].t0, s->texBRD[_BRD_LEFT].se, s->texBRD[_BRD_LEFT].te);
      in_ix->vki.draw.quad.cmdPushTex(in_cmd);

      for(int a= 0; a< nrT; a++) {
        in_ix->vki.draw.quad.setPosDi(xorg, _y+ (a* s->texBRD[_BRD_LEFT].dy), 0, s->texBRD[_BRD_LEFT].dx, s->texBRD[_BRD_LEFT].dy);
        in_ix->vki.draw.quad.cmdPushPos(in_cmd);
        in_ix->vki.draw.quad.cmdDraw(in_cmd);
      }

    /// MIRRORED REPEAT border
    } else if(s->texBRDwrap[_BRD_LEFT]== 3) {
      
      for(int a= 0; a< nrT; a++) {
        in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_LEFT].s0, (a% 2? s->texBRD[_BRD_LEFT].te: s->texBRD[_BRD_LEFT].t0),
                                    s->texBRD[_BRD_LEFT].se, (a% 2? s->texBRD[_BRD_LEFT].t0: s->texBRD[_BRD_LEFT].te));
        in_ix->vki.draw.quad.setPosDi(xorg, _y+ (a* s->texBRD[_BRD_LEFT].dy), 0, s->texBRD[_BRD_LEFT].dx, s->texBRD[_BRD_LEFT].dy);

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
    in_ix->vki.draw.quad.setPosDi(_x- (int32)s->texBRDdist[_BRD_TOPLEFT],
                                  _y- (int32)s->texBRDdist[_BRD_TOPLEFT], 0,
                                  s->texBRD[_BRD_TOPLEFT].dx, s->texBRD[_BRD_TOPLEFT].dy);

    in_ix->vki.draw.quad.cmdPushTex(in_cmd);
    in_ix->vki.draw.quad.cmdPushPos(in_cmd);
    in_ix->vki.draw.quad.cmdDraw(in_cmd);
  }
  
  // TOP- RIGHT corner
  if(s->useTexture && s->bTexBRD[_BRD_TOPRIGHT]) {
    in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_TOPRIGHT].s0, s->texBRD[_BRD_TOPRIGHT].t0, s->texBRD[_BRD_TOPRIGHT].se, s->texBRD[_BRD_TOPRIGHT].te);
    in_ix->vki.draw.quad.setPosDi(_x+ pos.dx- s->texBRD[_BRD_TOPRIGHT].dx+ (int32)s->texBRDdist[_BRD_TOPRIGHT],
                                  _y- (int32)s->texBRDdist[_BRD_TOPRIGHT], 0,
                                  s->texBRD[_BRD_TOPRIGHT].dx, s->texBRD[_BRD_TOPRIGHT].dy);

    in_ix->vki.draw.quad.cmdPushTex(in_cmd);
    in_ix->vki.draw.quad.cmdPushPos(in_cmd);
    in_ix->vki.draw.quad.cmdDraw(in_cmd);
  }

  // BOTTOM- RIGHT corner
  if(s->useTexture && s->bTexBRD[_BRD_BOTTOMRIGHT]) {
    in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_BOTTOMRIGHT].s0, s->texBRD[_BRD_BOTTOMRIGHT].t0, s->texBRD[_BRD_BOTTOMRIGHT].se, s->texBRD[_BRD_BOTTOMRIGHT].te);
    in_ix->vki.draw.quad.setPosDi(_x+ pos.dx- s->texBRD[_BRD_BOTTOMRIGHT].dx+ (int32)s->texBRDdist[_BRD_BOTTOMRIGHT],
                                  _y+ pos.dy- s->texBRD[_BRD_BOTTOMRIGHT].dx+ (int32)s->texBRDdist[_BRD_BOTTOMRIGHT], 0,
                                  s->texBRD[_BRD_BOTTOMRIGHT].dx, s->texBRD[_BRD_BOTTOMRIGHT].dy);

    in_ix->vki.draw.quad.cmdPushTex(in_cmd);
    in_ix->vki.draw.quad.cmdPushPos(in_cmd);
    in_ix->vki.draw.quad.cmdDraw(in_cmd);
  }
  
  // BOTTOM- LEFT corner
  if(s->useTexture && s->bTexBRD[_BRD_BOTTOMLEFT]) {
    in_ix->vki.draw.quad.setTex(s->texBRD[_BRD_BOTTOMLEFT].s0, s->texBRD[_BRD_BOTTOMLEFT].t0, s->texBRD[_BRD_BOTTOMLEFT].se, s->texBRD[_BRD_BOTTOMLEFT].te);
    in_ix->vki.draw.quad.setPosDi(_x- (int32)s->texBRDdist[_BRD_BOTTOMLEFT],
                                  _y+ pos.dy- s->texBRD[_BRD_BOTTOMLEFT].dx+ (int32)s->texBRDdist[_BRD_BOTTOMLEFT], 0,
                                  s->texBRD[_BRD_BOTTOMLEFT].dx, s->texBRD[_BRD_BOTTOMLEFT].dy);

    in_ix->vki.draw.quad.cmdPushTex(in_cmd);
    in_ix->vki.draw.quad.cmdPushPos(in_cmd);
    in_ix->vki.draw.quad.cmdDraw(in_cmd);
  }
  
  // border untextured - 3 pixels width
  if(!s->useTexture) {
    in_ix->vki.draw.quad.flagTexture(false);
    in_ix->vki.draw.quad.push.hollow= 3.0f;
    in_ix->vki.draw.quad.push.color= s->colorBRD;
    in_ix->vki.draw.quad.setPosDi(_x, _y, 0, pos.dx, pos.dy);

    in_ix->vki.draw.quad.cmdPushAll(in_cmd);
    in_ix->vki.draw.quad.cmdDraw(in_cmd);
    /// restore hollow, it's usually off
    in_ix->vki.draw.quad.push.hollow= -1.0f;
    in_ix->vki.draw.quad.cmdPushHollow(in_cmd);
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


bool ixBaseWindow::_update(bool in_mouseInside, bool in_updateChildren) {
  /*
  i think the update must happen if the click happens inside this first
    either that
    or...
    there has to be a limitation
    or, for the window there's a bool, it's in parent limits or not
    cuz there's update for other controlers, that might not have issues with the clipping

    i think the mouse bool, passed to all updates might be the key
    */

  recti r; getVDcoordsRecti(&r);
  if(in_updateChildren)
    if(_updateChildren(in_mouseInside? r.inside(in.m.x, in.m.y): false))
      return true;

  if(is.disabled) return false;
    


  // these can be in one variable... if(currentAction== _RESIZE_BOTTOM) ... currentAction can be 0, meaning no active stuff is happening.
  // for many windows, this must be changed i think. problems can happen, must further test with many buttons/windows


  // if an action is in progress
  if(Ix::wsys()._op.win) {
    if(Ix::wsys()._op.win!= this)      /// if the action is not done on this window, any update is ceased. (one action on one window only)
      return false;

    // THERE HAVE TO BE WINDOW DRAGGING USING JOYSTICK, KEYBOARDS, TOO
    // ACTION IN PROGRESS MUST BE RESET WHEN APPLICATION LOSES FOCUS

    /// button released - the action stopped
    if(!in.m.but[0].down) {
      Ix::wsys()._op.delData();
      return true;
    }

    /// a window drag is in progress
    if(Ix::wsys()._op.moving) {
      moveDelta(in.m.dx, in.m.dy);

      return true;
    }

    /// different resizes from here on
    if(Ix::wsys()._op.resizeBottom) {
      if(pos.dy+ in.m.dy>= getMinDy()) {
        //pos.moveD(0, in.m.dy); // bottom origin, -in.m.dy on next line
        resizeDelta(0, in.m.dy);
      }
      return true;
    }

    if(Ix::wsys()._op.resizeLeft) {
      if(pos.dx- in.m.dx>= getMinDx()) {
        pos.moveD(in.m.dx, 0);
        resizeDelta(-in.m.dx, 0);
      }
      return true;
    }
    if(Ix::wsys()._op.resizeRight) {
      if(pos.dx+ in.m.dx>= getMinDx()) {
        resizeDelta(in.m.dx, 0);
      }
      return true;
    }





  // no current operation is in progress - check if a new operation is being started
  } else {

    if(in_mouseInside) {
      if(in.m.but[0].down) {
        //int32 x= hook.pos.x+ pos.x0, y= hook.pos.y+ pos.y0;

        /// check if user wants to drag the window
        if(usage.movable) {
        
          /// if window has no title, moving is done by dragging anywhere inside the window
          //if(mPos(x+ 1, y+ 1, pos.dx- 2, pos.dy- 2)) {      /// 5 pixels ok?
          if(r.inside(in.m.x, in.m.y)) {
            Ix::wsys()._op.moving= true;
            Ix::wsys()._op.win= this;
            Ix::wsys().bringToFront(this);
            Ix::wsys().focus= this;
            return true;
          }

        } /// useMovable

        /// check if user wants to resize the window
        if(usage.resizeable) {
          /// left resize
          if(mPos(r.x0- 10, r.y0, 10, (int32)pos.dy)) {
            Ix::wsys()._op.resizeLeft= true;
            Ix::wsys()._op.win= this;
            Ix::wsys().bringToFront(this);
            Ix::wsys().focus= this;
            return true;
          }
          /// resize right
          if(mPos(r.xe, r.y0, 10, (int32)pos.dy)) {
            Ix::wsys()._op.resizeRight= true;
            Ix::wsys()._op.win= this;
            Ix::wsys().bringToFront(this);
            Ix::wsys().focus= this;
            return true;
          }
          /// resize bottom
          if(mPos(r.x0, r.ye, (int32)pos.dx, 10)) {
            Ix::wsys()._op.resizeBottom= true;
            Ix::wsys()._op.win= this;
            Ix::wsys().bringToFront(this);
            Ix::wsys().focus= this;
            return true;
          }
        } /// useResizeable

        // a click in the window makes it have focus
        //if(mPos(r.x0, y, pos.dx, pos.dy)) {
        if(r.inside(in.m.x, in.m.y)) {
          Ix::wsys().focus= this;
          return true;
        }

      } /// if left mouse button is down
    } /// mouse is inside (parent)
  } /// check if starting of a drag or resize

  return false; // no operation was done on this window or it's children
}


bool ixBaseWindow::_updateChildren(bool in_mouseInside) {
  bool ret= false;

  // first the scrolls
  if(vscroll) if(vscroll->update(in_mouseInside)) return true;
  if(hscroll) if(hscroll->update(in_mouseInside)) return true;

  // the rest of children here, excluding scrolls
  for(ixBaseWindow *p= (ixBaseWindow *)childrens.first; p; p= (ixBaseWindow *)p->next)
    if(p!= vscroll || p!= hscroll)
      if(p->update(in_mouseInside))
        ret= true;

  return ret;
}


















