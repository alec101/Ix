#include "ix/ix.h"
#include "ix/winSys/_privateDef.h"

/* TODO / IDEEAS:
 - animations? smooth scroll if jumping more than 1 pixel? it could happen but it's not top prio right now
 - scrolls must be drawn last, updated first, skip updating them/drawing them when on the children update/draw loops

 - computeAll is virtual, but not used. It must be integrated more;

*/





// constructor / destructor ===============--------------------

ixScroll::ixScroll(): usage(this), ixBaseWindow(&is, &usage) {
  // posOrigin.dx/dy have origin for buttonDx/Dy !!!

  _type= ixeWinType::scrollBar;
  target= null;

  colorArrows.set (0.1f, 0.1f, 0.1f, 1.0f);
  colorDragbox.set(0.1f, 0.1f, 0.1f, 1.0f);
  scrWidth= Ix::wsys().scrollBarWidth;

  delData();
}


ixScroll::~ixScroll() {
}


void ixScroll::delData() {
  ixBaseWindow::delData();
  orientation= 0;
  position= 0.0f;
  arrowScroll= 1.0f;
  steps= 0; // 101= 0% - 100%  if there's no target window to scroll, this var is used to set the scrolling units amount. if set to zero, it's not used also

  _scroll= 0.0f;
  _scrLength= 0.0f;
}





///=========================================///
// funcs ===============-------------------- //
///=========================================///

// returns the minimum scroll horizontal size
float ixScroll::getMinDx() {
  float dx= 0.0f;
  ixWSscrollStyle *s= (ixWSscrollStyle *)style;
  
  // horizontal
  if(orientation== 0) {
    /// arrow size
    if(s? s->useTexture && s->bTexArrows: false)
      dx= (float)s->texArrows[1].dx+ (float)s->texArrows[3].dx;
    else
      dx= scrWidth* 2.0f;
    
    /// drag button size (if it's fixed)
    if(usage.fixedDragbox) {
      if(s? s->useTexture && s->bTexDragbox: false)
        dx+= (float)s->texDragbox[0].dx+ 1.0f;
      else
        dx+= scrWidth+ 1.0f;       // add another dy + one pixel for the minimum possible scroll
    }
    return dx;
  }


  // vertical
  if(orientation== 1) {
    /// arrows size
    if(s? s->useTexture && s->bTexArrows: false) {
      dx= (float)MAX(s->texArrows[0].dx, s->texArrows[2].dx);
    } else
      dx= scrWidth;

    if(s) {
      /// dragbutton size
      if(s->useTexture && s->bTexDragbox)
        if((float)s->texDragbox[1].dx> dx)
          dx= (float)s->texDragbox[1].dx;

      /// scroll background size
      if(s->useTexture && s->bTexScrollBack)
        if((float)s->texScrollBack[1].dx> dx)
          dx= (float)s->texScrollBack[1].dx;
    }

    return dx;
  }
  return 0.0f;
}


// returns the minimum scroll vertical size
float ixScroll::getMinDy() {
  float dy= 0.0f;
  ixWSscrollStyle *s= (ixWSscrollStyle *)style;

  // horizontal scrollbar
  if(orientation== 0) {
    /// arrows size
    if(s? s->useTexture && s->bTexArrows: false) {
      dy= (float)MAX(s->texArrows[3].dy, s->texArrows[1].dy);
    } else
      dy= scrWidth;

    if(s) {
      if(s->useTexture && s->bTexDragbox)     /// drag button size
        if((float)s->texDragbox[0].dy> dy)
          dy= (float)s->texDragbox[0].dy;

      if(s->useTexture && s->bTexScrollBack)  /// scroll background size
        if((float)s->texScrollBack[0].dy> dy)
          dy= (float)s->texScrollBack[0].dy;
    }
    return dy;
  }

  // vertical scrollbar
  if(orientation== 1) {
    if(s? s->useTexture && s->bTexArrows: false)
      dy= (float)s->texArrows[0].dy+ (float)s->texArrows[2].dy;
    else
      dy= scrWidth* 2.0f;

    /// drag button size (if it's fixed)
    if(usage.fixedDragbox) {
      if(s? s->useTexture && s->bTexDragbox: false)
        dy+= (float)s->texDragbox[1].dy+ 1.0f;
      else
        dy+= scrWidth+ 1.0f;       // add another dx + one pixel for the minimum possible scroll
    }
    return dy;
  }
  
  return 0.0f;
}



// input delta x+y, pixels, sets the bar as close as possible to the best step (if any), in the limits
void ixScroll::drag(float in_dx, float in_dy) {
  /// there is no scroll dragbox? this should even post an error/warning
  if(!_scrLength) return; // { Ix::console.print("ixScroll::drag() was called - _scrLength is 0"); return; }

  _scroll+= (orientation== 0? in_dx: in_dy);   // _scroll and position must be always positive, on the window/text coords, top-left
  _asureScrollInBounds();
  position= _getPositionFromScroll();
  /// this makes the scroll position in pixels jump to the closest step
  if(target) {
    if(_scrLength> ((orientation== 0)? (target->_childArea.dx- target->_viewArea.dx): (target->_childArea.dy- target->_viewArea.dy)))
      _scroll= position;

    if(target->_type== ixeWinType::staticText)
      ((ixStaticText *)target)->text._view.moveToScrollPosition();
    if( target->_type== ixeWinType::edit)
      ((ixEdit *)target)->text._view.moveToScrollPosition();

    target->hook.updateHooks(false);

  } else if(steps)
    if(_scrLength> (float)steps)
      _scroll= position;
}


void ixScroll::setPosition(float in_p) {
  position= in_p;
  _asurePosInBounds();
  _scroll= _getScrollFromPosition();
  //_computeDrgRect();
  //_computeScrLength();
  if(target) {
    if(target->_type== ixeWinType::staticText)
      ((ixStaticText *)target)->text._view.moveToScrollPosition();
    if(target->_type== ixeWinType::edit)
      ((ixEdit *)target)->text._view.moveToScrollPosition();
    target->hook.updateHooks(false);
  }
}


void ixScroll::setPositionD(float in_delta) {
  position+= in_delta;
  _asurePosInBounds();
  _scroll= _getScrollFromPosition();
  //_computeDragbox();
  if(target) {
    if(target->_type== ixeWinType::staticText)
      ((ixStaticText *)target)->text._view.moveToScrollPosition();
    if(target->_type== ixeWinType::edit)
      ((ixEdit *)target)->text._view.moveToScrollPosition();
    target->hook.updateHooks(false);
  }
}


void ixScroll::setPositionMin() {
  position= 0.0f;
  _asurePosInBounds();
  _scroll= _getScrollFromPosition();
  //_computeDragbox();
  if(target) {
    if(target->_type== ixeWinType::staticText)
      ((ixStaticText *)target)->text._view.moveToScrollPosition();
    if( target->_type== ixeWinType::edit)
      ((ixEdit *)target)->text._view.moveToScrollPosition();
  }
}


void ixScroll::setPositionMax() {
  position= _getMaxPosition();
  _asurePosInBounds();
  _scroll= _getScrollFromPosition();
  //_computeDragbox();
  if(target) {
    if(target->_type== ixeWinType::staticText)
      ((ixStaticText *)target)->text._view.moveToScrollPosition();
    if( target->_type== ixeWinType::edit)
      ((ixEdit *)target)->text._view.moveToScrollPosition();
  }
}




/*
int32 ixScroll::_getOriginWidth() {
  if(orientation== 0) {
    return posOrigin.dy;
  } else {
    return posOrigin.dx;
  }
}
*/

inline bool ixScroll::_isScrollTooShortForDragbox() {
  if(orientation== 0)
    return pos.dx< (_arrRect[0].dx+ _arrRect[1].dx+ 2.0f);
  else
    return pos.dy< (_arrRect[0].dy+ _arrRect[1].dy+ 2.0f);
}


void ixScroll::_computeButtons() {
  _computeArrRect();
  _computeDrgRect();
  //_computeBarRectAndScrLength();
  _computeBarRect();
  _computeScrLength();
  _asurePosInBounds();
  _scroll= _getScrollFromPosition();
  _unit= (float)_getUnit();
}



void ixScroll::_computeArrRect() {
  float d= 0.0f;
  ixWSscrollStyle *s= (ixWSscrollStyle *)style;
  bool textured= (s? s->useTexture && s->bTexArrows: false);

  // ARROWS textured
  if(textured) {
    /// horizontal
    if(orientation== 0) {
      if((float)s->texArrows[3].dy< pos.dy)
        d= (pos.dy- (float)s->texArrows[3].dy)/ 2.0f;

      _arrRect[0].setD(pos.x0,                            pos.y0+ d, (float)s->texArrows[3].dx, (float)s->texArrows[3].dy);  /// left
      _arrRect[1].setD(pos.xe- (float)s->texArrows[1].dx, pos.y0+ d, (float)s->texArrows[1].dx, (float)s->texArrows[1].dy);  /// right

    /// vertical
    } else if(orientation== 1) {
      if((float)s->texArrows[0].dx< pos.dx)
        d= (pos.dx- (float)s->texArrows[0].dx)/ 2.0f;
      _arrRect[0].setD(pos.x0+ d, pos.y0,                            (float)s->texArrows[2].dx, (float)s->texArrows[2].dy);  /// up
      _arrRect[1].setD(pos.x0+ d, pos.ye- (float)s->texArrows[0].dy, (float)s->texArrows[0].dx, (float)s->texArrows[0].dy);  /// down
    } /// orientation

  // ARROWS not textured
  } else {
    /// horizontal
    if(orientation== 0) {         
      _arrRect[0].setD(pos.x0,         pos.ye- pos.dy, pos.dy, pos.dy); /// left
      _arrRect[1].setD(pos.xe- pos.dy, pos.ye- pos.dy, pos.dy, pos.dy); /// right
    /// vertical
    } else if(orientation== 1) {
      _arrRect[0].setD(pos.x0, pos.y0,         pos.dx, pos.dx); /// up
      _arrRect[1].setD(pos.x0, pos.ye- pos.dx, pos.dx, pos.dx); /// down
    } /// orientation
  } /// arrows - textured / not textured
}



void ixScroll::_computeDrgRect() {
  if(_isScrollTooShortForDragbox()) {
    _drgRect.setD(0.0f, 0.0f, 0.0f, 0.0f);
    return;
  }

  /// tmp vars
  ixWSscrollStyle *s= (ixWSscrollStyle *)style;
  float d= 0;
  float dx, dy;

  bool textured= (s? s->useTexture && s->bTexDragbox: false);

  // horizontal
  if(orientation== 0) {
    dy= (textured? (float)s->texDragbox[0].dy: pos.dy);

    // fixed drag button size
    if(usage.fixedDragbox) {
      dx= (textured? (float)s->texDragbox[0].dx: pos.dy);  /// non tex ver is a square

    // dynamically sized drag button
    } else {

      float scrSize= pos.dx- _arrRect[0].dx- _arrRect[1].dx;

      /// has a target window
      if(target) {
        if(target->_viewArea.dx< target->_childArea.dx) {

          /// _childViewArea.dx = drag button size
          /// _childArea.dx     = scrollbar size
          dx= (target->_childArea.dx? (target->_viewArea.dx* scrSize)/ target->_childArea.dx: 0.0f);

          // the drag button minimum size
          if(textured) {
            if(dx< (float)s->texDragbox[0].dx)
              dx= (float)s->texDragbox[0].dx;
          } else
            if(dx< pos.dy)
              dx= pos.dy;

          /// asure it's always 1 pixel lower than the scrolling size - further reduce the size if needed
          if(dx>= scrSize)
            dx= scrSize- 1.0f;

        } else
          dx= scrSize;

      /// no target
      } else {
        dx= (textured?  (float)s->texDragbox[0].dx:  pos.dy);

        /// dragbutton can shrink - this case it will move onliy 1 pixel left or right
        if(dx> scrSize- 1.0f)
          dx= scrSize- 1.0f;
      } /// target / no target
    } /// dynamically sized drag button

    d= ((dy< pos.dy)? ((pos.dy- dy)/ 2.0f): 0.0f);      /// if the scrollbar is thinner than the total width

    // button size assign
    _drgRect.setD(_arrRect[0].xe, pos.ye- pos.dy+ d, dx, dy);


  // vertical scrollbar
  } else if(orientation== 1) {

    dx= (textured?  (float)s->texDragbox[1].dx:  pos.dx);

    // fixed drag button size
    if(usage.fixedDragbox) {
      dy= (textured?  (float)s->texDragbox[1].dy:  pos.dx);

    // dynamically sized drag button
    } else {

      float scrSize= pos.dy- _arrRect[0].dy- _arrRect[1].dy;

      /// has a target window
      if(target) {
        if(target->_viewArea.dy< target->_childArea.dy) {

          /// _childViewArea.dy = drag button size
          /// _childArea.dy     = scrollbar size
          dy= (target->_childArea.dy? (target->_viewArea.dy* scrSize)/ target->_childArea.dy: 0);

          // the drag button minimum size
          if(textured) {
            if(dy< (float)s->texDragbox[1].dy)
              dy= (float)s->texDragbox[1].dy;
          } else
            if(dy< pos.dx)
              dy= pos.dx;

          /// asure it's always 1 pixel lower than the scrolling size - further reduce size to min 1 pixel if needed
          if(dy>= scrSize) dy= scrSize- 1.0f;

        } else
          dy= scrSize;

      /// no target
      } else {
        dy= (textured?  (float)s->texDragbox[1].dy:  pos.dx);

        /// dragbutton can shrink - this case it will move onliy 1 pixel left or right
        if(dy> scrSize- 1.0f)
          dy= scrSize- 1.0f;
      } /// target / no target
    } /// dynamically sized drag button

    d= ((dx< pos.dx)?  ((pos.dx- dx)/ 2.0f):  0.0f);      /// if the scrollbar is thinner than the total width
    // button size assign
    _drgRect.setD(pos.x0+ d, _arrRect[0].bottom, dx, dy);

  } /// orientation
}


void ixScroll::_computeBarRect() {
  /// tmp vars
  ixWSscrollStyle *s= (ixWSscrollStyle *)style;
  bool textured= (s? s->useTexture && s->bTexScrollBack: false);
  float d= 0.0f;

  // scrollbar length (excluding arrow buttons)
  if(_drgRect.dx || _drgRect.dy) {
    if(orientation== 0) {         // horizontal bar
      /// textured
      if(textured) {
        d= (pos.dy- (float)s->texScrollBack[0].dy)/ 2.0f;
        _barRect.set(_arrRect[0].right, pos.y0+ d, _arrRect[1].left, pos.y0+ d+ (float)s->texScrollBack[0].dy);
      /// not textured
      } else
        _barRect.set(_arrRect[0].right, pos.y0, _arrRect[1].left, pos.ye);

    } else if(orientation== 1) {  // vertical bar
      /// textured
      if(textured) {
        d= (pos.dx- (float)s->texScrollBack[1].dx)/ 2.0f;
        _barRect.set(pos.x0+ d, _arrRect[0].bottom, pos.x0+ d+ (float)s->texScrollBack[1].dx, _arrRect[1].top);
      /// not textured
      } else
        _barRect.set(pos.right, _arrRect[0].bottom, pos.left, _arrRect[1].top);

    } /// horiz / vert
  } else {  /// no dragbar
    _barRect.delData();
  }
}


inline void ixScroll::_computeScrLength() {
  if(_drgRect.dx && _drgRect.dy) {
    if(orientation== 0)
      _scrLength= _barRect.dx- _drgRect.dx;
    else if(orientation== 1)
      _scrLength= _barRect.dy- _drgRect.dy;
  } else
    _scrLength= 0.0f;
}





inline float ixScroll::_getScrollFromPosition() {

  if(target) {
    /// maximum position value on a targeted scroll. if it is negative or zero, then the scroll is not usable - there is no movement
    float maxPos= (orientation== 0? target->_childArea.dx- target->_viewArea.dx: 
                                    target->_childArea.dy- target->_viewArea.dy);
    if(maxPos<= 0.0f)
      return 0.0f;

    //return mlib::roundf((float)(position* _scrLength)/ (float)maxPos);
    //return (int32)mlib::roundd(((double)position/ (double)maxPos)* (double)_scrLength);  /// division first, so overflow is unlikely
    //return mlib::roundf(((float)position/ (float)maxPos)* (float)_scrLength);  /// division first, so overflow is unlikely
    return (position/ maxPos)* _scrLength;  /// division first, so overflow is unlikely
  }
  
  if(steps)
    //return ((steps> 1)? mlib::roundf((float)(position* _scrLength)/ (float)(steps- 1)): 0);
    //return ((steps> 1)? mlib::roundf((float)position/ (float)(steps- 1)* (float)_scrLength): 0);  /// division first, so overflow is unlikely
    return ((steps> 1)?  position/ ((float)steps- 1.0f)* _scrLength:  0.0f);  /// division first, so overflow is unlikely

  return position;
}


inline float ixScroll::_getPositionFromScroll() {
  //if(!_scrLength) { error.console("ixScroll:_getPositionFromScroll(): _scrLength is 0"); }
  if(_scrLength<= 0.0f) return 0.0f;

  /// position is    0 to (_childArea- _childViewArea)
  /// _scroll     -  position
  /// _scrLength  -  (_childArea- _childViewArea)
  if(target) {
    float maxPos= (orientation== 0? target->_childArea.dx- target->_viewArea.dx: 
                                    target->_childArea.dy- target->_viewArea.dy);
    
    if(maxPos<= 0.0f) return 0.0f;      /// if it's negative or zero, the view area incorporates the child area totally, no need for scrolling

    //return mlib::roundf((float)(_scroll* maxPos)/ (float)_scrLength); /// original
    //return (int32)mlib::roundd(((double)_scroll/ (double)_scrLength)* (double)maxPos);  /// division first, so overflow is unlikely
    //return mlib::roundf(((float)_scroll/ (float)_scrLength)* (float)maxPos);  /// division first, so overflow is unlikely
    return (_scroll/ _scrLength)* maxPos;  /// division first, so overflow is unlikely
  }

  /// position is  [0] to [steps- 1]
  /// _scroll    - position
  /// _scrLength - steps
  if(steps)
    //return ((steps> 1)? mlib::roundf((float)(_scroll* (steps- 1))/ (float)_scrLength): 0);    /// original
    //return ((steps> 1)? mlib::roundf((float)_scroll/ (float)_scrLength* (float)(steps- 1)): 0);  /// division first, so overflow is unlikely
    return ((steps> 1)? (_scroll/ _scrLength* (float)(steps- 1)): 0.0f);  /// division first, so overflow is unlikely
  return _scroll;
}


inline float ixScroll::_getMaxPosition() {
  if(target) {
    float maxPos= (orientation== 0? target->_childArea.dx- target->_viewArea.dx:
                                    target->_childArea.dy- target->_viewArea.dy);
    if(maxPos< 0.0f) maxPos= 0.0f;
    return maxPos;
  }
  if(steps) return (float)(steps- 1);
  return _scrLength;
}


inline void ixScroll::_asurePosInBounds() {
  if(position< 0.0f) position= 0.0f;
  float m= _getMaxPosition();
  if(position> m)
    position= m;
}


inline void ixScroll::_asureScrollInBounds() {
  if(_scroll< 0.0f)       _scroll= 0.0f;
  if(_scroll> _scrLength) _scroll= _scrLength;
}






void ixScroll::_computePos() {
  if(!target) return _computeDeltas();   /// if the scroll is free, has no target window, it's length is set by the user, only the width of it can be computed

  float mindx= getMinDx(), mindy= getMinDy();

  /// horizontal
  if(orientation== 0) {
    float x= (target->vscroll?  target->vscroll->getMinDx():  0.0f);
    pos.setD(0.0f, target->pos.dy- mindy, MAX(mindx, target->pos.dx- x), mindy);
  /// vertical
  } else if(orientation== 1) {
    float y= (target->hscroll?  target->hscroll->getMinDy():  0.0f);
    pos.setD(target->pos.dx- mindx, 0.0f, mindx, MAX(mindy, target->pos.dy- y));
  }

  hook.pos.x= target->hook.pos.x+ target->pos.x0;
  hook.pos.y= target->hook.pos.y+ target->pos.y0;
}


void ixScroll::_computeDeltas() {
  if(orientation== 0)
    pos.resizeD(pos.dx, getMinDy());
  else if(orientation== 1)
    pos.resizeD(getMinDx(), pos.dy);
}





void ixScroll::_computeAll() {
  _computePos();
  //ixBaseWindow::_computeAll(); <<< this should be computed. MUST THINK THIS THRU, I THINK
  _computeButtons();
}


void ixScroll::_computeAllDelta(float x, float y) {
  
}























//   ######     ######       ####     ##      ##
//   ##    ##   ##    ##   ##    ##   ##  ##  ##
//   ##    ##   ######     ########   ##  ##  ##
//   ##    ##   ##    ##   ##    ##   ##  ##  ##
//   ######     ##    ##   ##    ##     ##  ##


///====================================================///
// DRAWING FUNCTION ===============-------------------- //
///====================================================///

#ifdef IX_USE_OPENGL
void ixScroll::_glDraw(Ix *in_ix, ixWSsubStyleBase *in_style) {
  error.makeme();
}
#endif /// IX_USE_OPENGL

#ifdef IX_USE_VULKAN
void ixScroll::_vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style) {
  if(!is.visible) return;
  if(!_inBounds(in_ix)) return;     // nothing to draw, the ix+gpu don't draw this window
  
  /// tmp vars

  int n;                      /// this will hold the number of times the texture will repeat
  rectf r, clp;
  r= _barRect; r.moveD(hook.pos.x, hook.pos.y);
  
  ixWSscrollStyle *s= (ixWSscrollStyle *)(in_style? in_style: style); /// s will be the style
  ixTexture *t= s->parent->getTexture(in_ix);
  //if(s->useTexture== null) t= null;

  bool _debug= false;

  /// debug info print
  if(_debug && orientation== 1) {
    char buf[256];  // DEBUG print buffer
    ixFontStyle *sav= in_ix->pr.style;
    in_ix->pr.style= &in_ix->debugStyle;

    in_ix->vki.cmdScissorDefault(in_cmd);
    in_ix->pr.setFont(in_ix->fnt5x6);
    in_ix->pr.style->setOrientation(IX_TXT_RIGHT);

    sprintf(buf, "vert scrbar[%.1f/%.1f]", position, _scroll);
    in_ix->pr.txt2f(pos.xe+ hook.pos.x, pos.ye+ hook.pos.y- in_ix->debugStyle.getCharDy(), buf);

    in_ix->pr.style= sav;
  }

  /// inits

  in_ix->vk.CmdBindPipeline(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipeline);
  in_ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipelineLayout, 0, 1, &in_ix->vki.glb[in_ix->vki.fi]->set->set, 0, nullptr);
  in_ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipelineLayout, 1, 1, &t->vkd.set->set, 0, nullptr);

  in_ix->vki.draw.quad.cmdTexture(in_cmd, t);
  in_ix->vki.cmdScissor(in_cmd, &_clip);

  in_ix->vki.draw.quad.flagDisabled(is.disabled);
  in_ix->vki.draw.quad.flagTexture(s->useTexture);
  in_ix->vki.draw.quad.push.color= s->color;
  in_ix->vki.draw.quad.cmdPushAll(in_cmd);


  // scroll background ---------------------------------------------------
  
  /// simple square colored background, no texture
  if((!s->bTexScrollBack) || (!s->useTexture)) {
    in_ix->vki.draw.quad.flagTexture(false);
    in_ix->vki.draw.quad.setPosR(r);
    in_ix->vki.draw.quad.cmdPushFlags(in_cmd);
    in_ix->vki.draw.quad.cmdPushPos(in_cmd);
    in_ix->vki.draw.quad.cmdDraw(in_cmd);
  }


  /// useColorOnTexture usage flag - use color for current texture
  if(s->useColorOnTexture) in_ix->vki.draw.quad.push.color= *_colorToUse;
  else                     in_ix->vki.draw.quad.push.color.set(1.0f, 1.0f, 1.0f, 1.0f);;
  in_ix->vki.draw.quad.cmdPushColor(in_cmd);

  in_ix->vki.draw.quad.flagTexture(s->useTexture);
  in_ix->vki.draw.quad.cmdPushFlags(in_cmd);


  // BACKGROUND texturing ========---------
  
  if(s->useTexture && s->bTexScrollBack) {
    clp= r; clp.intersectRect(_clip);
    in_ix->vki.cmdScissor(in_cmd, &clp);

    // horizontal scroll background
    if(orientation== 0) {
      float bdx= (float)s->texScrollBack[0].dx, bdy= (float)s->texScrollBack[0].dy;
      n= (int)(_barRect.dx/ bdx)+ 1;  //n= ((int)_barRect.dx/ (int)b)+ ((int)_barRect.dx% (int)b?  1:  0);

      /// FIXED / STRETCHED background
      if(s->texScrollBackWrap<= 1) {
        if(s->texScrollBackWrap== 0)
          in_ix->vki.draw.quad.setPosD(r.x0, r.y0, 0.0f, bdx, bdy);
        else
          in_ix->vki.draw.quad.setPosR(r);

        in_ix->vki.draw.quad.setTex(s->texScrollBack[0].s0, s->texScrollBack[0].t0, s->texScrollBack[0].se, s->texScrollBack[0].te);

        in_ix->vki.draw.quad.cmdPushPos(in_cmd);
        in_ix->vki.draw.quad.cmdPushTex(in_cmd);
        in_ix->vki.draw.quad.cmdDraw(in_cmd);

      /// REPEATED background
      } else if(s->texScrollBackWrap== 2) {
        in_ix->vki.draw.quad.setTex(s->texScrollBack[0].s0, s->texScrollBack[0].t0, s->texScrollBack[0].se, s->texScrollBack[0].te);
        in_ix->vki.draw.quad.cmdPushTex(in_cmd);

        for(int a= 0; a< n; a++) {
          in_ix->vki.draw.quad.setPosD(r.x0+ ((float)a* bdx), r.y0, 0.0f, bdx, bdy);
          in_ix->vki.draw.quad.cmdPushPos(in_cmd);
          in_ix->vki.draw.quad.cmdDraw(in_cmd);
        }

      /// MIRRORED REPEATED background
      } else if(s->texScrollBackWrap== 3) {
        ixSubTex *p= &s->texScrollBack[0];

        for(int a= 0; a< n; a++) {
          in_ix->vki.draw.quad.setPosD((r.x0+ ((float)a* bdx)), r.y0, 0.0f, bdx, bdy);
          /// inverse tex coords not even
          in_ix->vki.draw.quad.setTex((a% 2? p->se: p->s0), p->t0, (a% 2? p->s0: p->se), p->te);

          in_ix->vki.draw.quad.cmdPushPos(in_cmd);
          in_ix->vki.draw.quad.cmdPushTex(in_cmd);
          in_ix->vki.draw.quad.cmdDraw(in_cmd);
        }
      }


    // vertical scroll background
    } else if(orientation== 1) {
      float bdx= (float)s->texScrollBack[1].dx, bdy= (float)s->texScrollBack[1].dy;
      n= (int)(_barRect.dy/ bdy)+ 1;   //n= (_barRect.dy/ b)+ ((_barRect.dy% b)? 1: 0);

      /// FIXED / STRETCHED background
      if(s->texScrollBackWrap<= 1) {
        if(s->texScrollBackWrap== 0)
          in_ix->vki.draw.quad.setPosD(r.x0, r.y0, 0.0f, bdx, bdy);
        else
          in_ix->vki.draw.quad.setPosR(r);

        in_ix->vki.draw.quad.setTex(s->texScrollBack[1].s0, s->texScrollBack[1].t0, s->texScrollBack[1].se, s->texScrollBack[1].te);

        in_ix->vki.draw.quad.cmdPushPos(in_cmd);
        in_ix->vki.draw.quad.cmdPushTex(in_cmd);
        in_ix->vki.draw.quad.cmdDraw(in_cmd);

      /// REPEATED background
      } else if(s->texScrollBackWrap== 2) {
        in_ix->vki.draw.quad.setTex(s->texScrollBack[1].s0, s->texScrollBack[1].t0, s->texScrollBack[1].se, s->texScrollBack[1].te);
        in_ix->vki.draw.quad.cmdPushTex(in_cmd);

        for(int a= 0; a< n; a++) {
          in_ix->vki.draw.quad.setPosD(r.x0, (r.y0+ ((float)a* bdy)), 0.0f, bdx, bdy);
          in_ix->vki.draw.quad.cmdPushPos(in_cmd);
          in_ix->vki.draw.quad.cmdDraw(in_cmd);
        }

        // THIS IS NOT ON THE HORIZONTAL VVVVV COULD CAUSE PROBLEMS EITHER THIS OR THE ABSENCE OF IT ON THE HORIZ
        in_ix->vki.cmdScissor(in_cmd, &_clip);    //in_ix->vk.CmdSetScissor(in_cmd, 0, 1, &((VkRect2D &)_clip));

      /// MIRRORED REPEATED background
      } else if(s->texScrollBackWrap== 3) {
        ixSubTex *p= &s->texScrollBack[1];

        for(int a= 0; a< n; a++) {
          in_ix->vki.draw.quad.setPosD(r.x0, r.y0+ ((float)a* bdy), 0.0f, bdx, bdy);
          /// inverse tex coords not even
          in_ix->vki.draw.quad.setTex(p->s0, (a% 2? p->t0: p->te), p->se, (a% 2? p->te: p->t0));

          in_ix->vki.draw.quad.cmdPushPos(in_cmd);
          in_ix->vki.draw.quad.cmdPushTex(in_cmd);
          in_ix->vki.draw.quad.cmdDraw(in_cmd);
        }
      } /// wrap type
    } /// orientation - vert/horiz
  } /// textured background bar

    
  // scroll dragbox drawing ========---------
  if(orientation== 0)
    r.setD(_drgRect.x0+ hook.pos.x+ _scroll, _drgRect.y0+ hook.pos.y,          _drgRect.dx, _drgRect.dy);
  else
    r.setD(_drgRect.x0+ hook.pos.x,          _drgRect.y0+ hook.pos.y+ _scroll, _drgRect.dx, _drgRect.dy);

  if((!s->useTexture) || (!s->bTexDragbox)) {    // no texture
    in_ix->vki.draw.quad.push.color= s->colorDragbox;
    in_ix->vki.draw.quad.flagTexture(false);
    in_ix->vki.draw.quad.setPosR(r);
    in_ix->vki.draw.quad.cmdPushAll(in_cmd);
    in_ix->vki.draw.quad.cmdDraw(in_cmd);
  }

  if(s->useTexture && s->bTexDragbox) {          // textured
    int32 i;
    float bdx, bdy;   //int32 b= 0.0f;

    if(orientation== 0)
      i= 0,
      bdx= s->texDragbox[i].dx, bdy= s->texDragbox[i].dy, //b= s->texDragbox[i].dx,
      n= (int)(_drgRect.dx/ bdx)+ 1;                      // n= (_drgRect.dx/ b)+ ((_drgRect.dx% b)? 1: 0);
      
    else if(orientation== 1)
      i= 1,
      bdx= s->texDragbox[i].dx, bdy= s->texDragbox[i].dy, //b= s->texDragbox[i].dy,
      n= (int)(_drgRect.dy/ bdy)+ 1;                      // n= (_drgRect.dy/ b)+ ((_drgRect.dy% b)? 1: 0);

    clp= r; clp.intersectRect(_clip);
    in_ix->vki.cmdScissor(in_cmd, &clp);

    in_ix->vki.draw.quad.push.color.set(1.0f, 1.0f, 1.0f, 1.0f);
    in_ix->vki.draw.quad.flagTexture(true);
    in_ix->vki.draw.quad.setTex(s->texDragbox[i].s0, s->texDragbox[i].t0, s->texDragbox[i].se, s->texDragbox[i].te);
    in_ix->vki.draw.quad.cmdPushAll(in_cmd);

    /// stretched / fixed
    if(s->texDragboxWrap<= 1) {

      if(s->texDragboxWrap== 1) in_ix->vki.draw.quad.setPosR(r);
      else                      in_ix->vki.draw.quad.setPosD(r.x0, r.y0, 0.0f, bdx, bdy);
      
      in_ix->vki.draw.quad.cmdPushPos(in_cmd);
      in_ix->vki.draw.quad.cmdDraw(in_cmd);

    } else if(s->texDragboxWrap== 2) {  /// repeat

      if(orientation== 0) {
        for(int a= 0; a< n; a++) {
          in_ix->vki.draw.quad.setPosD((r.x0+ ((float)a* bdx)), r.y0, 0.0f, bdx, bdy);
          in_ix->vki.draw.quad.cmdPushPos(in_cmd);
          in_ix->vki.draw.quad.cmdDraw(in_cmd);
        }
      } else if(orientation== 1) {
        for(int a= 0; a< n; a++) {
          in_ix->vki.draw.quad.setPosD(r.x0, (r.y0+ ((float)a* bdy)), 0.0f, bdx, bdy);
          in_ix->vki.draw.quad.cmdPushPos(in_cmd);
          in_ix->vki.draw.quad.cmdDraw(in_cmd);
        }
      }

    } else if(s->texDragboxWrap== 3) {  /// mirrored repeat
      ixSubTex *p= &s->texDragbox[i];

      if(orientation== 0) {
        for(int a= 0; a< n; a++) {
          in_ix->vki.draw.quad.setPosD((r.x0+ ((float)a* bdx)), r.y0, 0.0f, bdx, bdy);
          in_ix->vki.draw.quad.setTex((a% 2? p->se: p->s0), p->t0, (a% 2? p->s0: p->se), p->te);  /// inverse tex coords not even
          in_ix->vki.draw.quad.cmdPushTex(in_cmd);
          in_ix->vki.draw.quad.cmdPushPos(in_cmd);
          in_ix->vki.draw.quad.cmdDraw(in_cmd);
        }
      } else if(orientation== 1) {

        for(int a= 0; a< n; a++) {
          in_ix->vki.draw.quad.setPosD(r.x0, (r.y0+ ((float)a* bdy)), 0.0f, bdx, bdy);
          in_ix->vki.draw.quad.setTex(p->s0, (a% 2? p->t0: p->te), p->se, (a% 2? p->te: p->t0));  /// inverse tex coords not even
          in_ix->vki.draw.quad.cmdPushTex(in_cmd);
          in_ix->vki.draw.quad.cmdPushPos(in_cmd);
          in_ix->vki.draw.quad.cmdDraw(in_cmd);
        }
      }
    }
  } /// textured dragbox



  // arrows drawing ========---------
  /// quad sl should still be binded at this point

  if(s->useTexture && s->bTexArrows) {  /// textured
    in_ix->vki.cmdScissor(in_cmd, &_clip);

    int aid1, aid2;
    if(orientation== 0) aid1= 3, aid2= 1; /// left then right
    else                aid1= 0, aid2= 2; /// up   then down
    
    in_ix->vki.draw.quad.setPosD(_arrRect[0].x0+ hook.pos.x, _arrRect[0].y0+ hook.pos.y, 0.0f, (float)s->texArrows[aid1].dx, (float)s->texArrows[aid1].dy);
    in_ix->vki.draw.quad.setTex(s->texArrows[aid1].s0, s->texArrows[aid1].t0, s->texArrows[aid1].se, s->texArrows[aid1].te);
    in_ix->vki.draw.quad.cmdPushTex(in_cmd);
    in_ix->vki.draw.quad.cmdPushPos(in_cmd);
    in_ix->vki.draw.quad.cmdDraw(in_cmd);

    in_ix->vki.draw.quad.setPosD(_arrRect[1].x0+ hook.pos.x, _arrRect[1].y0+ hook.pos.y, 0.0f, (float)s->texArrows[aid2].dx, (float)s->texArrows[aid2].dy);
    in_ix->vki.draw.quad.setTex(s->texArrows[aid2].s0, s->texArrows[aid2].t0, s->texArrows[aid2].se, s->texArrows[aid2].te);
    in_ix->vki.draw.quad.cmdPushTex(in_cmd);
    in_ix->vki.draw.quad.cmdPushPos(in_cmd);
    in_ix->vki.draw.quad.cmdDraw(in_cmd);
  } /// textured arrows


  /// triangle sl bind
  
  if((!s->useTexture) || (!s->bTexArrows)) { /// not textured
    in_ix->vk.CmdBindPipeline(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.triangle.sl->vk->pipeline);
    in_ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.triangle.sl->vk->pipelineLayout, 0, 1, &in_ix->vki.glb[in_ix->vki.fi]->set->set, 0, nullptr);
    in_ix->vki.draw.triangle.cmdTexture(in_cmd, t);
    in_ix->vki.cmdScissor(in_cmd, &_clip);

    in_ix->vki.draw.triangle.flagDisabled(is.disabled);
    in_ix->vki.draw.triangle.flagTexture(false);

    in_ix->vki.draw.triangle.push.color= colorArrows;
    //in_ix->vki.draw.triangle.push.color.set(1.0f, 1.0f, 1.0f, 1.0f);

    in_ix->vki.draw.triangle.cmdPushFlags(in_cmd);
    in_ix->vki.draw.triangle.cmdPushColor(in_cmd);
    
    if(orientation== 0) {
      /// left arrow
      r= _arrRect[0]; r.moveD(hook.pos.x, hook.pos.y);
      in_ix->vki.draw.triangle.setPos(0, (r.x0+ _unit), (r.y0+ (r.dy/ 2.0f)));
      in_ix->vki.draw.triangle.setPos(1, (r.xe- _unit), (r.ye- _unit));
      in_ix->vki.draw.triangle.setPos(2, (r.xe- _unit), (r.y0+ _unit));
      in_ix->vki.draw.triangle.cmdPushPos(in_cmd);
      in_ix->vki.draw.triangle.cmdDraw(in_cmd);
      
      /// right arrow
      r= _arrRect[1]; r.moveD(hook.pos.x, hook.pos.y);
      in_ix->vki.draw.triangle.setPos(0, (r.x0+ _unit), r.y0+ _unit);
      in_ix->vki.draw.triangle.setPos(1, (r.x0+ _unit), r.ye- _unit);
      in_ix->vki.draw.triangle.setPos(2, (r.xe- _unit), r.y0+ (r.dy/ 2.0f));
      in_ix->vki.draw.triangle.cmdPushPos(in_cmd);
      in_ix->vki.draw.triangle.cmdDraw(in_cmd);

    } else if(orientation== 1) {

      /// up arrow
      r= _arrRect[0]; r.moveD(hook.pos.x, hook.pos.y);
      in_ix->vki.draw.triangle.setPos(0, r.x0+ (r.dx/ 2.0f), r.y0+ _unit);
      in_ix->vki.draw.triangle.setPos(1, r.x0+ _unit,        r.ye- _unit);
      in_ix->vki.draw.triangle.setPos(2, r.xe- _unit,        r.ye- _unit);
      
      in_ix->vki.draw.triangle.cmdPushPos(in_cmd);
      in_ix->vki.draw.triangle.cmdDraw(in_cmd);

      /// down arrow
      r= _arrRect[1]; r.moveD(hook.pos.x, hook.pos.y);
      in_ix->vki.draw.triangle.setPos(0, r.x0+ (r.dx/ 2.0f), r.ye- _unit);
      in_ix->vki.draw.triangle.setPos(1, r.xe- _unit,        r.y0+ _unit);
      in_ix->vki.draw.triangle.setPos(2, r.x0+ _unit,        r.y0+ _unit);
      
      in_ix->vki.draw.triangle.cmdPushPos(in_cmd);
      in_ix->vki.draw.triangle.cmdDraw(in_cmd);
    }
  } /// not textured arrows
}

#endif /// IX_USE_VULKAN





// ##    ##   ######     ######       ####     ##########   ########
// ##    ##   ##    ##   ##    ##   ##    ##       ##       ##
// ##    ##   ######     ##    ##   ########       ##       ######
// ##    ##   ##         ##    ##   ##    ##       ##       ##
//   ####     ##         ######     ##    ##       ##       ########



///=====================================================///
// UPDATING FUNCTION ===============-------------------- //
///=====================================================///

bool ixScroll::_update(bool in_updateChildren) {
  rectf r;
  bool inside;
  float mx, my, mdx, mdy, mx2, my2;

  if(!is.visible) return false;

  // this window should not have childrens, right?
  if(in_updateChildren)
    if(_updateChildren())
      return true;

  if(is.disabled|| !is.visible)
    return false;


  getPosVD(&r);

  // NO KEYBOARD/GP/JOY INTERACTION DONE AT ALL

  // MOUSE interaction
  mx= _scaleDiv(in.m.x), my= _scaleDiv(in.m.y);
  mdx= _scaleDiv(in.m.dx), mdy= _scaleDiv(in.m.dy);
  inside= r.inside(mx, my);
  mx2= mx- hook.pos.x;
  my2= my- hook.pos.y;

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
    if(Ix::wsys()._op.scrDragbox) {
      drag(mdx, mdy);
      Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
      return true;
    }

    /// up / left arrow
    if(Ix::wsys()._op.scrArr1) {
      if(osi.present>= Ix::wsys()._op.time) {       /// op.time is handled like a countdown here
        if(_arrRect[0].inside(mx2, my2)) {          // still the mouse is inside the rect
          Ix::wsys()._op.time+= 100000000;          /// do another scroll in 100 miliseconds
          setPositionD(-arrowScroll);
        }
      }
      Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
      return true;
    }

    /// down / right arrow
    if(Ix::wsys()._op.scrArr2) {
      if(osi.present>= Ix::wsys()._op.time) {       /// op.time is handled like a countdown here
        if(_arrRect[1].inside(mx2, my2)) {          // still the mouse is inside the rect
          Ix::wsys()._op.time+= 100000000;          /// do another scroll in 100 miliseconds
          setPositionD(arrowScroll);
        }
      }
      Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
      return true;
    }

    /// click to the scroll, outside the dragbox
    if(Ix::wsys()._op.scrBar) {
      // this is not done like in windows. a click on the bar jumps to that exact location
      _scroll= (orientation== 0? mx2- _barRect.x0: my2- _barRect.y0);
      _asureScrollInBounds();
      setPosition(_getPositionFromScroll());
      Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
      return true;
    }


  // no current operation is in progress - check if a new operation is being started
  } else {

    if(in.m.but[0].down) {
      
      /// down / left press start
      if(_arrRect[0].inside(mx2, my2)) {
        Ix::wsys()._op.scrArr1= 1;
        Ix::wsys()._op.win= this;
        Ix::wsys()._op.time= osi.present;
        if(target) Ix::wsys().bringToFront(target);
        Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
        return true;
      }

      /// up / right press start
      if(_arrRect[1].inside(mx2, my2)) {
        Ix::wsys()._op.scrArr2= 1;
        Ix::wsys()._op.win= this;
        Ix::wsys()._op.time= osi.present;
        if(target) Ix::wsys().bringToFront(target);
        Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
        return true;
      }

      /// dragbox drag start
      if(orientation? _drgRect.inside(mx2, my2- _scroll): _drgRect.inside(mx2- _scroll, my2)) {
        Ix::wsys()._op.scrDragbox= 1;
        Ix::wsys()._op.win= this;
        if(target) Ix::wsys().bringToFront(target);
        Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
        return true;
      }
      /// bar click
      if(_barRect.inside(mx2, my2)) {
        Ix::wsys()._op.scrBar= 1;
        Ix::wsys()._op.win= this;
        if(target) Ix::wsys().bringToFront(target);
        Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
        return true;
      }
    } /// mouse button down
  } /// check if starting of a drag or resize

  return false; // no operation was done on this window or it's children
}
















