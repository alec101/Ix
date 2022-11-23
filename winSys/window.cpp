#include "ix/ix.h"
#include "ix/winSys/_privateDef.h"




ixWindow::ixWindow(): usage(this), ixBaseWindow(&is, &usage) {
  _type= ixeWinType::window;
  title= null;
}


ixWindow::~ixWindow() {
  delData();
}


void ixWindow::delData() {
  ixBaseWindow::delData();
}









// funcs


void ixWindow::setTitle(cchar *in_text, ixWSgenericStyle *in_style) {
  useTitle= true;
  str32 s32(in_text);
  /// if there's no title static text created, this window will create it and set it as child
  if(!title) {
    title= new ixStaticText;
    childrens.add(title);
    title->parent= this;
    title->_clipUsePrentsParent= true;

    if(in_style)
      title->style= in_style;
    else
      title->style= (ixWSsubStyleBase *)&style->parent->title;
    title->_applyColorsFromStyle();

    
    title->text.font= *Ix::getMain()->pr.style;        // USING CURRENTLY SELECTED FONT <<<<<<<<<<<<<<< this is placeholder stuff
    title->setFont(Ix::getMain()->pr.style->selFont);

    title->setText32(&s32);
    //title->text= in_text;
    title->updateSizeFromText();
    
    title->is.visible= true;

    title->color=         ((ixWSgenericStyle *)title->style)->color;
    title->colorFocus=    ((ixWSgenericStyle *)title->style)->colorFocus;
    title->colorBRD=      ((ixWSgenericStyle *)title->style)->colorBRD;
    title->colorBRDfocus= ((ixWSgenericStyle *)title->style)->colorBRDfocus;
    title->colorHover=    ((ixWSgenericStyle *)title->style)->colorHover;

    title->text.cur.makeSureInBounds();
    
  } /// window has a title

  else {
    title->setText32(&s32);
    title->text.cur.makeSureInBounds();
    title->updateSizeFromText();
  }


  ixWSwindowStyle *s= (ixWSwindowStyle *)style;
  setTitlePosition(s->titlePosition, s->titleOrientation, s->titleDist, s->titleInside);
}


// set window title positions/characteristics: hookBorder- border that it hooks to the main window; orientation 90/270 - horizontal 0/180 vertical; distance- distance from the border, in pixels; inside- the title is inside the window
void ixWindow::setTitlePosition(ixEBorder in_hookBorder, int16 in_orientation, int32 in_distance, bool in_inside) {
  if(title== null) {
    error.console("ixWindow::setTitlePosition(): changes to window title were requested, but title is not created");
    return;
  }

  /// title border hook (snap) based on orientation
  ixEBorder b= in_hookBorder;

  if(!in_inside) {
    if(b== ixEBorder::top)               b= ixEBorder::bottom;
    else if(b== ixEBorder::right)        b= ixEBorder::left;
    else if(b== ixEBorder::bottom)       b= ixEBorder::top;
    else if(b== ixEBorder::left)         b= ixEBorder::right;

    if(in_orientation== 90|| in_orientation== 270) {
      if(b== ixEBorder::topLeft)           b= ixEBorder::bottomLeft;
      else if(b== ixEBorder::topRight)     b= ixEBorder::bottomRight;
      else if(b== ixEBorder::bottomRight)  b= ixEBorder::topRight;
      else if(b== ixEBorder::bottomLeft)   b= ixEBorder::topLeft;
    } else {
      if(b== ixEBorder::topLeft)           b= ixEBorder::topRight;
      else if(b== ixEBorder::topRight)     b= ixEBorder::topLeft;
      else if(b== ixEBorder::bottomRight)  b= ixEBorder::bottomLeft;
      else if(b== ixEBorder::bottomLeft)   b= ixEBorder::bottomRight;
    }
  }

  title->hook.set(this, in_hookBorder, b);

  /// distance from the border
  if(in_orientation== 90 || in_orientation== 270) {
    if(in_hookBorder== ixEBorder::top|| in_hookBorder== ixEBorder::topLeft|| in_hookBorder== ixEBorder::topRight)
      pos.y0-= in_distance;
    else if(in_hookBorder== ixEBorder::bottom || in_hookBorder== ixEBorder::bottomLeft || in_hookBorder== ixEBorder::bottomRight)
      pos.y0+= in_distance;
    else if(in_hookBorder== ixEBorder::left)
      pos.x0-= in_distance;
    else if(in_hookBorder== ixEBorder::right)
      pos.x0+= in_distance;
  } else {
    if(in_hookBorder== ixEBorder::left || in_hookBorder== ixEBorder::topLeft || in_hookBorder== ixEBorder::bottomLeft)
      pos.x0-= in_distance;
    else if(in_hookBorder== ixEBorder::right || in_hookBorder== ixEBorder::topRight || in_hookBorder== ixEBorder::bottomRight)
      pos.x0+= in_distance;
    else if(in_hookBorder== ixEBorder::top)
      pos.y0-= in_distance;
    else if(in_hookBorder== ixEBorder::bottom)
      pos.y0+= in_distance;
  } /// orientation

  _computeAll();
}








//   ######     ######       ####     ##      ##
//   ##    ##   ##    ##   ##    ##   ##  ##  ##
//   ##    ##   ######     ########   ##  ##  ##
//   ##    ##   ##    ##   ##    ##   ##  ##  ##
//   ######     ##    ##   ##    ##     ##  ##


// window DRAW =====-------------------------------------------------

#ifdef IX_USE_OPENGL
void ixWindow::_glDraw(Ix *in_ix, ixWSsubStyleBase *in_style) {
  ixBaseWindow::_glDraw(in_ix, in_style);       /// start by drawing the base

  if(useTitle && title)
    title->_glDraw(in_ix);            /// draw the window title

  if(usage._scrollbars || usage._autoScrollbars) {
    if(hscroll) hscroll->_glDraw(in_ix);
    if(vscroll) vscroll->_glDraw(in_ix);
  }


  for(ixBaseWindow *p= (ixBaseWindow *)childrens.last; p; p= (ixBaseWindow *)p->prev)
    if(p!= title && p!= hscroll && p!= vscroll)
      p->_glDraw(in_ix);
}
#endif /// IX_USE_OPENGL


#ifdef IX_USE_VULKAN
void ixWindow::_vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style) {
  if(is.visible== 0) return;
  ixBaseWindow::_vkDraw(in_cmd, in_ix, in_style);       /// start by drawing the base
  
  if(useTitle && title)
    title->_vkDraw(in_cmd, in_ix);            /// draw the window title

  if(usage._scrollbars || usage._autoScrollbars) {
    if(hscroll) hscroll->_vkDraw(in_cmd, in_ix);
    if(vscroll) vscroll->_vkDraw(in_cmd, in_ix);
  }

  for(ixBaseWindow *p= (ixBaseWindow *)childrens.last; p; p= (ixBaseWindow *)p->prev)
    if(p!= title && p!= hscroll && p!= vscroll)
      p->_vkDraw(in_cmd, in_ix);
}
#endif /// IX_USE_VULKAN












// ##    ##   ######     ######       ####     ##########   ########
// ##    ##   ##    ##   ##    ##   ##    ##       ##       ##
// ##    ##   ######     ##    ##   ########       ##       ######
// ##    ##   ##         ##    ##   ##    ##       ##       ##
//   ####     ##         ######     ##    ##       ##       ########



// window UPDATE =====------------------------------------------------

bool ixWindow::_update(bool in_updateChildren) {
  rectf r;
  bool inside;
  float mx, my;

  if(!is.visible) return false;
  if(ixBaseWindow::_update()) return true;

  getPosVD(&r);
  mx= _scaleDiv(in.m.x), my= _scaleDiv(in.m.y);
  inside= r.inside(mx, my);

  // an operation is in progress
  if(Ix::wsys()._op.win) {

  // no operation is in progress
  } else {
    if(inside) {

      // mouse title drag-move
      if(in.m.but[0].down) {
        if(usage.movable && useTitle && title)
          if(_mINSIDE(title->hook.pos.x+ title->pos.x0, title->hook.pos.y+ title->pos.y0, title->pos.dx, title->pos.dy)) {
            Ix::wsys()._op.moving= true;
            Ix::wsys()._op.win= this;
            Ix::wsys().bringToFront(this);
            Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
            return true;
          }

      } /// left mouse button is being pressed
    }
  }

  return false; // no operation was done on this window or it's children
}














