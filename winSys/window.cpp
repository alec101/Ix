#include "ix/ix.h"
#include "ix/winSys/_privateDef.h"




ixWindow::ixWindow() {
  ixBaseWindow();
  _type= _IX_WINDOW;
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
void ixWindow::setTitlePosition(int8 in_hookBorder, int16 in_orientation, int32 in_distance, bool in_inside) {
  if(title== null) {
    error.console("ixWindow::setTitlePosition(): changes to window title were requested, but title is not created");
    return;
  }

  /// title border hook (snap) based on orientation
  int8 b= in_hookBorder;

  if(!in_inside) {
    if(b== IX_BORDER_TOP)               b= IX_BORDER_BOTTOM;
    else if(b== IX_BORDER_RIGHT)        b= IX_BORDER_LEFT;
    else if(b== IX_BORDER_BOTTOM)       b= IX_BORDER_TOP;
    else if(b== IX_BORDER_LEFT)         b= IX_BORDER_RIGHT;

    if(in_orientation== 90|| in_orientation== 270) {
      if(b== IX_BORDER_TOPLEFT)           b= IX_BORDER_BOTTOMLEFT;
      else if(b== IX_BORDER_TOPRIGHT)     b= IX_BORDER_BOTTOMRIGHT;
      else if(b== IX_BORDER_BOTTOMRIGHT)  b= IX_BORDER_TOPRIGHT;
      else if(b== IX_BORDER_BOTTOMLEFT)   b= IX_BORDER_TOPLEFT;
    } else {
      if(b== IX_BORDER_TOPLEFT)           b= IX_BORDER_TOPRIGHT;
      else if(b== IX_BORDER_TOPRIGHT)     b= IX_BORDER_TOPLEFT;
      else if(b== IX_BORDER_BOTTOMRIGHT)  b= IX_BORDER_BOTTOMLEFT;
      else if(b== IX_BORDER_BOTTOMLEFT)   b= IX_BORDER_BOTTOMRIGHT;
    }
  }

  title->hook.set(this, in_hookBorder, b);

  /// distance from the border
  if(in_orientation== 90 || in_orientation== 270) {
    if(in_hookBorder== IX_BORDER_TOP || in_hookBorder== IX_BORDER_TOPLEFT|| in_hookBorder== IX_BORDER_TOPRIGHT)
      pos.y0-= in_distance;
    else if(in_hookBorder== IX_BORDER_BOTTOM || in_hookBorder== IX_BORDER_BOTTOMLEFT || in_hookBorder== IX_BORDER_BOTTOMRIGHT)
      pos.y0+= in_distance;
    else if(in_hookBorder== IX_BORDER_LEFT)
      pos.x0-= in_distance;
    else if(in_hookBorder== IX_BORDER_RIGHT)
      pos.x0+= in_distance;
  } else {
    if(in_hookBorder== IX_BORDER_LEFT || in_hookBorder== IX_BORDER_TOPLEFT || in_hookBorder== IX_BORDER_BOTTOMLEFT)
      pos.x0-= in_distance;
    else if(in_hookBorder== IX_BORDER_RIGHT || in_hookBorder== IX_BORDER_TOPRIGHT || in_hookBorder== IX_BORDER_BOTTOMRIGHT)
      pos.x0+= in_distance;
    else if(in_hookBorder== IX_BORDER_TOP)
      pos.y0-= in_distance;
    else if(in_hookBorder== IX_BORDER_BOTTOM)
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

  if(usage.scrollbars || usage.autoScrollbars) {
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
  ixBaseWindow::_vkDraw(in_cmd, in_ix, in_style);       /// start by drawing the base
  
  if(useTitle && title)
    title->_vkDraw(in_cmd, in_ix);            /// draw the window title

  if(usage.scrollbars || usage.autoScrollbars) {
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

bool ixWindow::_update(bool in_mouseInside, bool in_updateChildren) {
  if(ixBaseWindow::_update(in_mouseInside, in_updateChildren)) return true;

  // an operation is in progress
  if(Ix::wsys()._op.win) {



  // no operation is in progress
  } else {
    if(in_mouseInside) {
      if(in.m.but[0].down) {
        /// if using a window title, moving is done by dragging the title
        if(usage.movable && useTitle && title)
          if(mPos(title->hook.pos.x+ title->pos.x0, title->hook.pos.y+ title->pos.y0, (int32)title->pos.dx, (int32)title->pos.dy)) {
            Ix::wsys()._op.moving= true;
            Ix::wsys()._op.win= this;
            Ix::wsys().bringToFront(this);
            return true;
          }

      } /// left mouse button is being pressed
    }
  }

  return false; // no operation was done on this window or it's children
}














