#include "ix/ix.h"
#include "ix/winSys/_privateDef.h"

/* TODO:
- a 'pointing' system... more than one menu item can point to the same menu ?
  maybe the mouse could r-click into some menu that can be accessed from a diff place?
  it could be some neat feature, but it can make things excessivly hard and easy to be bugged...

  - move/resize/etc virtuals, must be handled in the menu i think. either disabled or think about it
  
  im thinking the dx/dy of the (normal) menu could be set in stone, no more computations

    cuz moving a menu to x and y would be way faster that way...
    that can be a very good optimisation
    cuz computing the whole menu items and everything every time...
    
*/




// constructors / destructors

ixMenu::ixMenu(): ixBaseWindow() {
  _type= ixeWinType::menu;

  _customPos= false;
  _highlighted= null;

  parentItem= null;
  parentBar= null; 
  root= null;

  menuBorder= 2;        // default value
  itemBorder= 1;        // default value

  _dxRequired= 0, _dyRequired= 0;
}


ixMenu::~ixMenu() {
  delData();
}


void ixMenu::delData() {
  // delete all menus tied to this menu
  for(ixMenuItem *i= (ixMenuItem *)items.first; i; i= (ixMenuItem *)i->next)
    if(i->childMenu)
      Ix::wsys().delWindow(i->childMenu);

  ixBaseWindow::delData();
}





// funcs



void ixMenu::addMenuItem(cchar *in_txt) {
  ixMenuItem *i= new ixMenuItem;
  i->text= in_txt;
  i->parent= this;
  i->childMenu= null;


  items.add(i);
  _computeMenuAllPos();
}


void ixMenu::addMenuItemAfter(cchar *in_txt, ixMenuItem *in_after) {
  if(!_itemPartOfThis(in_after)) {
    error.console("addMenuItemAfter() <in_after> is not part of current menu. aborting.");
    return;
  }
  
  ixMenuItem *i= new ixMenuItem;
  i->text= in_txt;
  i->parent= this;


  
  items.addAfter(i, in_after);
  _computeMenuAllPos();
}


void ixMenu::addMenuItemBefore(cchar *in_txt, ixMenuItem *in_before) {
  if(!_itemPartOfThis(in_before)) {
    error.console("addMenuItemBefore() <in_before> is not part of current menu. aborting.");
    return;
  }
  
  ixMenuItem *i= new ixMenuItem;
  i->text= in_txt;
  i->parent= this;


  
  items.addBefore(i, in_before);
  _computeMenuAllPos();
}


void ixMenu::addMenuItemPos(cchar *in_txt, int32 in_n) {
  ixMenuItem *i= new ixMenuItem;
  i->text= in_txt;
  i->parent= this;








  if(in_n< 0) { 
    in_n= 0;
    error.console("addMenuItemPos() <in_n> is less than 0. changed to 0.");
  }
  if(in_n> (int32)items.nrNodes) { 
    in_n= items.nrNodes;
    error.console("addMenuItemPos() <in_n> is over the number of items in the menu. changed to max.");
  }
  items.addi(i, in_n);
  _computeMenuAllPos();
}


void ixMenu::delMenuItem(ixMenuItem *in_i) {
  if(!_itemPartOfThis(in_i)) { error.console("ixMenu::delMenuItem() <in_i> not part of this menu. aborting."); return; }

  /// delete the menu tied to this item
  if(in_i->childMenu)
    Ix::wsys().delWindow(in_i->childMenu);

  items.del(in_i);
  _computeMenuAllPos();
}


void ixMenu::delMenuItemNr(int32 in_n) {
  if((in_n< 0) || (in_n> (int32)items.nrNodes- 1)) { error.console("ixMenu::delMenuItemNr() <in_n> is out of bounds. aborting."); return; }
  delMenuItem((ixMenuItem *)items.get(in_n));
}



void ixMenu::showAndSetPosition(int32 x, int32 y) {
  _useCustomPos= true;
  _customPos.set(x, y);
}





void ixMenu::attachToMenuBar(ixMenuBar *in_bar, ixMenuBarItem *in_item) {
  if(in_bar== null || in_item== null) { error.console("ixMenu::attachToMenuBar(): bar and/or item is/are null"); return; }
  
  if(in_item->menu!= null) {
    error.console("ixMenu::attachToMenuBar() menu bar item has already a menu attached. that menu must be dealt with first.");
    return;
  }

  detach();
  changeParent(in_bar->parent);

  in_item->menu= this;
  //parentBar= in_item;
  //parentItem= (ixBaseWindow *)in_item;
  parentBar= in_item;
  parentItem= null;
  root= in_bar;


  hook= in_bar->hook;
  _computeMenuAllPos();
}


void ixMenu::attachToMenuBarn(ixMenuBar *in_menuBar, int32 itemNr) {
  if(in_menuBar== null) { error.console("ixMenu::attachToMenuBarn(): target bar is null. aborting."); return; } 
  if(itemNr< 0 || itemNr> (int32)in_menuBar->items.nrNodes) { error.console("ixMenu::attachToMenuBarn(): itemNr out of bounds"); return; }
  attachToMenuBar(in_menuBar, (ixMenuBarItem *)in_menuBar->items.get(itemNr));
}


void ixMenu::attachToMenu(ixMenu *in_menu, ixMenuItem *in_item) {
  if(in_menu== null || in_item== null) { error.console("ixMenu::attachToMenu(): target(s) are null. aborting."); return; }
  if(in_item->childMenu) { error.console("ixMenu::attachToMenu(): target has already a menu attached. aborting."); return; }

  detach();
  changeParent(in_menu->parent);

  in_item->childMenu= this;
  parentItem= in_item;
  root= in_menu->root;

  hook= in_menu->hook;
  _computeMenuAllPos();
  in_menu->_computeMenuAllPos();
}


void ixMenu::detach() {
  if(parentBar)
    parentBar->menu= null;
  
  if(parentItem) {
    parentItem->childMenu= null;
    parentItem->parent->_computeMenuAllPos();
  }
  parentBar= null;
  parentItem= null;
  root= null;
}




inline bool ixMenu::_itemPartOfThis(ixMenuItem *in_i) {
  return (in_i->parent== this);
}



inline osiMonitor *_getMonitorForCoords(int32 x, int32 y) {
  for(int32 a= 0; a< osi.display.nrMonitors; a++) {
    osiMonitor *m= &osi.display.monitor[a];
    if(x>= m->x0 && x< m->x0+ m->dx && y>= m->y0 && y< m->y0+ m->dy)
      return m;
  }
  return null;
}


inline osiMonitor *_getClosestMonitorForCoords(int32 x, int32 y) {
  int32 dist;
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


void ixMenu::_computeMenuItemPos() {

  int32 charDy= ixPrint::getCharDy(font.selFont);
  int32 x= menuBorder, y= menuBorder;
  int32 arrow= _getTriangleWidth();

  _dxRequired= 0, _dyRequired= 0;     // required dx & dy to fit all the items in

  _dyRequired+= menuBorder;   // the whole menu border

  // find out each item position and bar dimensions, taking 0,0 as starting position
  for(ixMenuItem *i= (ixMenuItem *)items.first; i; i= (ixMenuItem *)i->next) {
    int32 tx= ixPrint::getTextLen32(i->text, 0, font.selFont)+ itemBorder+ itemBorder; // 1 pixels left, 1 pixels right - or itemBorder
    int32 ty= charDy+ itemBorder+ itemBorder;                                           // 1 pixel up, 1 pixel down     - or itemBorder
    if(i->childMenu)
      tx+= arrow+ itemBorder+ itemBorder;
    
    /// item dimensions
    i->_pos.setD(x, y, tx, ty);
    i->_textPos.x= x+ itemBorder;
    i->_textPos.y= y+ itemBorder;

    /// whole window dimension adjust
    int32 a= i->_pos.dx+ menuBorder+ menuBorder;     // 2 pixels left, 2 pixels right, the menu whole border
    if(_dxRequired< a) _dxRequired= a;
    _dyRequired+= i->_pos.dy;

    y+= ty;
  }
  _dyRequired+= menuBorder;   // the whole menu border
}



void ixMenu::_computeMenuPos() {
  
  osiMonitor *m= null;
  _computeMenuItemPos();

  // got dx, dy, all items dx, dy too

  // only the monitor is taken into consideration
  // the osiWindow could be taken too. must further think

  // compute pos x0/y0
  if(parentItem) {

    /// find the monitor the menu will be on
    m=   _getMonitorForCoords(parentItem->_pos.xe- 1+ hook.pos.x, parentItem->_pos.ye- 1+ hook.pos.y);
    if(m== null)
      m= _getMonitorForCoords(parentItem->_pos.x0+ hook.pos.x,    parentItem->_pos.ye- 1+ hook.pos.y);
    if(m== null)
      m= _getMonitorForCoords(parentItem->_pos.x0+ hook.pos.x,    parentItem->_pos.y0+ hook.pos.y);
    if(m== null)
      m= _getMonitorForCoords(parentItem->_pos.xe- 1+ hook.pos.x, parentItem->_pos.y0+ hook.pos.y);
    if(m== null)
      m= _getClosestMonitorForCoords(parentItem->_pos.xe- 1+ hook.pos.x, parentItem->_pos.ye- 1+ hook.pos.y);

    // _initial x
    if(parentItem->_pos.xe+ _dxRequired- 1+ hook.pos.x<= m->x0+ m->dx- 1)
      pos.x0= parentItem->_pos.xe;
    else 
      pos.x0= parentItem->_pos.x0- _dxRequired;

    // _initial y
    if(parentItem->_pos.y0+ _dyRequired- 1+ hook.pos.y<= m->y0+ m->dy- 1)
      pos.y0= parentItem->_pos.ye;
    else
      pos.y0= parentItem->_pos.y0- _dyRequired- 1;

    /// adjust _initial to be in the monitor
    if(pos.x0< m->x0          - hook.pos.x) pos.x0= m->x0          - hook.pos.x;
    if(pos.x0> m->x0+ m->dx- 1- hook.pos.x) pos.x0= m->x0+ m->dx- 1- hook.pos.x;
    if(pos.y0< m->y0          - hook.pos.y) pos.y0= m->y0          - hook.pos.y;
    if(pos.y0> m->y0+ m->dy- 1- hook.pos.y) pos.y0= m->y0+ m->dy- 1- hook.pos.y;

  } else if(parentBar) {
    /// find the monitor the menu will be on
    m=   _getMonitorForCoords(parentBar->_pos.x0+ hook.pos.x,    parentBar->_pos.y0+ hook.pos.y);
    if(m== null)
      m= _getMonitorForCoords(parentBar->_pos.xe- 1+ hook.pos.x, parentBar->_pos.y0+ hook.pos.y);
    if(m== null)
      m= _getMonitorForCoords(parentBar->_pos.x0+ hook.pos.x,    parentBar->_pos.ye- 1+ hook.pos.y);
    if(m== null)
      m= _getMonitorForCoords(parentBar->_pos.xe- 1+ hook.pos.x, parentBar->_pos.ye- 1+ hook.pos.y);
    if(m== null)
      m= _getClosestMonitorForCoords(parentBar->_pos.xe- 1+ hook.pos.x, parentBar->_pos.ye- 1+ hook.pos.y);

    // pos.x0
    if(parentBar->_pos.x0+ _dxRequired+ hook.pos.x<= m->x0+ m->dx- 1)
      pos.x0= parentBar->_pos.x0;
    else
      pos.x0= parentBar->_pos.x0- 1- _dxRequired;

    // pos.y0
    if(parentBar->_pos.y0+ _dyRequired+ hook.pos.y<= m->y0+ m->dx- 1)
      pos.y0= parentBar->_pos.ye;
    else
      pos.y0= parentBar->_pos.y0- _dyRequired- 1;

    /// adjust _initial to be in the monitor
    if(pos.x0< m->x0          - hook.pos.x) pos.x0= m->x0          - hook.pos.x;
    if(pos.x0> m->x0+ m->dx- 1- hook.pos.x) pos.x0= m->x0+ m->dx- 1- hook.pos.x;
    if(pos.y0< m->y0          - hook.pos.y) pos.y0= m->y0          - hook.pos.y;
    if(pos.y0> m->y0+ m->dy- 1- hook.pos.y) pos.y0= m->y0+ m->dy- 1- hook.pos.y;

  } else if(_useCustomPos) {
    m= _getMonitorForCoords(_customPos.x, _customPos.y);
    if(m== null)
      m= _getClosestMonitorForCoords(_customPos.x, _customPos.y);

    // pos.x
    if(_customPos.x+ _dxRequired<= m->x0+ m->dx- 1) pos.x0= _customPos.x;
    else                                            pos.x0= _customPos.x- _dxRequired;

    // pos.y
    if(_customPos.y+ _dyRequired<= m->y0+ m->dy- 1) pos.y0= _customPos.y;
    else                                            pos.y0= _customPos.y- _dyRequired;
  }

  // got everything needed for pos
  pos.setD(pos.x0, pos.y0, _dxRequired, _dyRequired);

  /*  DISABLED, IT'S 0, 0 AND YOU DRAW WHERE YOU WANT
  // adjust each item x0/y0 based on pos.x0/y0
  x= pos.x0, y= pos.y0;
  for(ixMenuItem *i= (ixMenuItem *)items.first; i; i= (ixMenuItem *)i->next)
    i->_pos.x0+= x, i->_pos.xe= i->_pos.x0+ i->_pos.dx,
    i->_pos.y0+= y, i->_pos.ye= i->_pos.y0+ i->_pos.dy,
    i->_textPos.x+= x, i->_textPos.y+= y;
  */


  if(parent) {
    parent->_computeViewArea();
    parent->_computeChildArea();
    parent->_computeClipPlane();
  }

  _computeClipPlane();
}





void ixMenu::_computeMenuAllPos() {
  _computeMenuItemPos();
  _computeMenuPos();
}


void ixMenu::hideMenu() {
  for(ixMenuItem *p= (ixMenuItem *)items.first; p; p= (ixMenuItem *)p->next)
    if(p->childMenu)
      p->childMenu->hideMenu();
  setVisible(false);
}








void ixMenu::move(int32 x0, int32 y0) {
  return;     // NOT THINKED UPON / DISABLED ATM
}


void ixMenu::moveDelta(int32 dx, int32 dy) {
  return;     // NOT THINKED UPON / DISABLED ATM
}


void ixMenu::resize(int32 dx, int32 dy) {
  return;     // NOT THINKED UPON / DISABLED ATM
}


void ixMenu::resizeDelta(int32 dx, int32 dy) {
  return;     // NOT THINKED UPON / DISABLED ATM
}


void ixMenu::setPos(int32 x0, int32 y0, int32 dx, int32 dy) {
  return;     // NOT THINKED UPON / DISABLED ATM
}





int32 ixMenu::_getTriangleWidth() {
  int32 charDy= ixPrint::getCharDy(font.selFont);
  int32 charDx= ixPrint::getCharDx(' ', font.selFont);
  return MAX(charDx, charDy)/ 2;
}















//  ##    ##    ######      ######        ####      ########    ########
//  ##    ##    ##    ##    ##    ##    ##    ##       ##       ##
//  ##    ##    ######      ##    ##    ##    ##       ##       ######      func
//  ##    ##    ##          ##    ##    ########       ##       ##
//    ####      ##          ######      ##    ##       ##       ########

bool ixMenu::_update(bool in_mIn, bool updateChildren) {
  if((!is.visible) || (is.disabled))
    return false;
  int32 _x, _y; getVDcoords2i(&_x, &_y);
  recti r;
  r.setD(_x, _y, pos.dx, pos.dy);

  // update it's children first
  if(updateChildren)
    if(_updateChildren((in_mIn? r.inside(in.m.x, in.m.y): false)))
      return true;




  // FUNCTION CORE

  bool anyHighlight= false;
  for(ixMenuItem *p= (ixMenuItem *)items.first; p; p= (ixMenuItem *)p->next) {
    r= p->_pos; r.moveD(_x, _y);
    if(r.inside(in.m.x, in.m.y)) {
      _highlighted= p;

      // THIS FUNC IS NOT DONE, THIS IS JUST HOVER WITH MOUSE
      Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);  /// flag mouse was used with this window

      anyHighlight= true;
      break;
    }
  }

  if(!anyHighlight) _highlighted= null;
    


  //makeme;



  // base window update, at this point - no childrens tho, those were updated first
  return ixBaseWindow::_update(in_mIn, false);
}












// DDDDDD      RRRRRR        AAAA      WW      WW
// DD    DD    RR    RR    AA    AA    WW      WW
// DD    DD    RRRRRR      AA    AA    WW  WW  WW     func
// DD    DD    RR    RR    AAAAAAAA    WW  WW  WW
// DDDDDD      RR    RR    AA    AA      WW  WW

#ifdef IX_USE_OPENGL
void ixMenu::_glDraw(Ix *in_ix, ixWSsubStyleBase *in_style) {
  if(!is.visible) return;
  if(!_clip.exists()) return;
  //in_ix->pr.setScissor(&_clip);

  // menu draw

  
  in_ix->glo.draw.quad.useProgram();
  in_ix->glo.draw.quad.setClipPlaneR(_clip);
  in_ix->glo.draw.quad.setColorv(colorBRD);
  in_ix->glo.draw.quad.setHollow(1.0f);
  in_ix->glo.draw.quad.setCoordsDi(pos.x0+ hook.pos.x, pos.y0+ hook.pos.y, pos.dx, pos.dy);
  in_ix->glo.draw.quad.render();
  in_ix->glo.draw.quad.setHollow(-1.0f);
  //in_sl->hollowRect(pos.x0+ hook.pos.x, pos.y0+ hook.pos.y, pos.dx, pos.dy);

  
  //  glUseProgram(in_sl->gl->id);
  //  in_sl->setClipPlaneR(_clip);
  //  glUniform4f(in_sl->u_color, 1.0f, 1.0f, 1.0f, 1.0f);
  //  glUniform3f(in_sl->u_origin, 0.0f, 0.0f, 0.0f);

  //in_sl->hollowRect(pos.x0+ hook.pos.x, pos.y0+ hook.pos.y, pos.dx, pos.dy);

  // menu items
  for(ixMenuItem *p= (ixMenuItem *)items.first; p; p= (ixMenuItem *)p->next) {
    if(_highlighted== p) {
      //glUseProgram(in_sl->gl->id);
      //glUniform4f(in_sl->u_color, 0.1f, 0.3f, 1.0f, 1.0f);
      //glUniform1i(in_sl->u_customPos, 1);             // enable custom quad
      //glUniform1i(in_sl->u_useTexture, 0);
      //glUniform2f(in_sl->u_quadPos0, (float)(p->_pos.x0+ hook.pos.x), (float)(p->_pos.y0+ hook.pos.y));
      //glUniform2f(in_sl->u_quadPosE, (float)(p->_pos.xe+ hook.pos.x), (float)(p->_pos.ye+ hook.pos.y));
      //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      //glUniform1ui(in_sl->u_customPos, 0);            // disable shader custom quad draw
      //glUniform1ui(in_sl->u_useTexture, 1);
    }

    // expanding menu item (to a new menu)
    int32 offset= 0;
    if(p->childMenu) {
      int32 adx= _getTriangleWidth();


      // left arrow
      if(p->childMenu->pos.x0< pos.x0) {
        offset= adx;

        //sl->
        //<<<<<<< MAKE ME I THINK


      // right arrow
      } else {


      }
    }

    in_ix->pr.txt32_2i(p->_textPos.x+ hook.pos.x+ offset, p->_textPos.y+ hook.pos.y, p->text);
  }
}
#endif /// IX_USE_OPENGL



#ifdef IX_USE_VULKAN
void ixMenu::_vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style) {
  if(!is.visible) return;
  if(!_clip.exists()) return;
  int32 triangleWidth= _getTriangleWidth();
  int32 _x, _y; getVDcoords2i(&_x, &_y);

  ixTexture *t= null;
  ixWSgenericStyle *s= (ixWSgenericStyle *)(in_style? in_style: style);
  if(s)
    s->parent->getTexture(in_ix);

  // menu draw

  /// background
  in_ix->vki.cmdScissor(in_cmd, &_clip);
  in_ix->vk.CmdBindPipeline(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipeline);
  in_ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipelineLayout, 0, 1, &in_ix->vki.glb[in_ix->vki.fi]->set->set, 0, null);
  in_ix->vki.draw.quad.cmdTexture(in_cmd, t);

  in_ix->vki.draw.quad.push.color= color;
  in_ix->vki.draw.quad.push.hollow= -1.0f;
  in_ix->vki.draw.quad.setPosDi(_x, _y, 0, pos.dx, pos.dy);
  in_ix->vki.draw.quad.flagTexture(false);
  in_ix->vki.draw.quad.cmdPushAll(in_cmd);
  in_ix->vki.draw.quad.cmdDraw(in_cmd);

  /// border
  in_ix->vki.draw.quad.push.color= colorBRD;
  in_ix->vki.draw.quad.push.hollow= (float)menuBorder;
  in_ix->vki.draw.quad.cmdPushAll(in_cmd);
  in_ix->vki.draw.quad.cmdDraw(in_cmd);

  
  // menu items
  for(ixMenuItem *p= (ixMenuItem *)items.first; p; p= (ixMenuItem *)p->next) {
    if(_highlighted== p) {
      in_ix->vk.CmdBindPipeline(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipeline);
      in_ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipelineLayout, 0, 1, &in_ix->vki.glb[in_ix->vki.fi]->set->set, 0, null);
      in_ix->vki.draw.quad.cmdTexture(in_cmd, t);
      in_ix->vki.draw.quad.push.hollow= -1.0f;
      in_ix->vki.draw.quad.push.color.set(0.1f, 0.3f, 1.0f, 1.0f); // <<< SELECTED COLOR
      in_ix->vki.draw.quad.setPos((float)(_x+ p->_pos.x0), (float)(_y+ p->_pos.y0), 0.0f, (float)(_x+ p->_pos.xe), (float)(_y+ p->_pos.ye));
      in_ix->vki.draw.quad.cmdPushAll(in_cmd);
      in_ix->vki.draw.quad.cmdDraw(in_cmd);
    }

    // expanding menu item (to a new menu)
    int32 offset= 0;
    if(p->childMenu) {
      //int32 adx= _getTriangleWidth();

      in_ix->vk.CmdBindPipeline(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.triangle.sl->vk->pipeline);
      in_ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.triangle.sl->vk->pipelineLayout, 0, 1, &in_ix->vki.glb[in_ix->vki.fi]->set->set, 0, null);
      in_ix->vki.draw.triangle.cmdTexture(in_cmd, t);
      in_ix->vki.draw.triangle.flagTexture(false);
      in_ix->vki.draw.triangle.push.color= font.color1;

      // left arrow
      if(p->childMenu->pos.x0< pos.x0) {
        offset= triangleWidth;
        int dy= (p->_pos.dy- triangleWidth)/ 2;
        in_ix->vki.draw.triangle.setPos(0, (float)(_x+ p->_pos.x0+ triangleWidth+ 1), (float)(_y+ p->_pos.y0+ dy));
        in_ix->vki.draw.triangle.setPos(1, (float)(_x+ p->_pos.x0+ 1),                (float)(_y+ p->_pos.y0+ (p->_pos.dy/ 2)));
        in_ix->vki.draw.triangle.setPos(2, (float)(_x+ p->_pos.x0- triangleWidth- 1), (float)(_y+ p->_pos.y0+ p->_pos.dy- dy));
        in_ix->vki.draw.triangle.cmdPushAll(in_cmd);
        in_ix->vki.draw.triangle.cmdDraw(in_cmd);

      // right arrow
      } else {
        //adx= ixPrint::getCharDy(font.selFont)/ 2;
        int dy= (p->_pos.dy- triangleWidth)/ 2;
        in_ix->vki.draw.triangle.setPos(0, (float)(_x+ p->_pos.xe- triangleWidth- 1), (float)(_y+ p->_pos.y0+ dy));
        in_ix->vki.draw.triangle.setPos(1, (float)(_x+ p->_pos.xe- triangleWidth- 1), (float)(_y+ p->_pos.y0+ p->_pos.dy- dy));
        in_ix->vki.draw.triangle.setPos(2, (float)(_x+ p->_pos.xe- 1),                (float)(_y+ p->_pos.y0+ (p->_pos.dy/ 2)));
        
        in_ix->vki.draw.triangle.cmdPushAll(in_cmd);
        in_ix->vki.draw.triangle.cmdDraw(in_cmd);

        // MAKE ME<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<,,

      }
    }

    in_ix->pr.txt32_2i(_x+ p->_textPos.x+ offset, _y+ p->_textPos.y, p->text);
  }
}
#endif /// IX_USE_VULKAN



























// ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
// █ MENU BAR class ███████████████████████████████████████████████████████████████████
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀




// constructor / destructor

ixMenuBar::ixMenuBar() {
  ixBaseWindow();
  _type= ixeWinType::menuBar;

  _hovered= _selected= null;
}


ixMenuBar::~ixMenuBar() {
  ixBaseWindow::~ixBaseWindow();
}


void ixMenuBar::delData() {
  ixBaseWindow::delData();
}





// funcs



void ixMenuBar::addMenuBarItem(cchar *in_txt) {
  ixMenuBarItem *i= new ixMenuBarItem;
  i->text= in_txt;

  items.add(i);
  _computeBarPositions();
}


void ixMenuBar::attachMenu(ixMenuBarItem *in_barItem, ixMenu *in_menu) {
  if(in_menu== null || in_barItem== null) { error.console("ixMenuBar::attachMenu(): parameters are null"); return; }

  //bool found= false;
  if(!items.isMember(in_barItem)) { error.console("ixMenuBar::attachMenu(): in_barItem is not part of this bar's list"); }


  in_menu->attachToMenuBar(this, in_barItem);
}


void ixMenuBar::_computeBarPositions() {
  int32 spaceSize= ixPrint::getCharDx(' ', font.selFont); // to the left and right of the text, there's gonna be a space
  int32 textBorder= 1;                                    // a border inside the window, then the text follows inside it
  int32 charDy= ixPrint::getCharDy(font.selFont);

  int32 x= pos.x0+ textBorder,
        y= pos.y0+ textBorder,
        dx= textBorder+ textBorder,
        dy= textBorder+ textBorder+ charDy;

  for(ixMenuBarItem *i= (ixMenuBarItem *)items.first; i; i= (ixMenuBarItem *)i->next) {
    int32 idx= ixPrint::getTextLen32(i->text, 0, font.selFont);
    idx+= spaceSize+ spaceSize;
    i->_pos.setD(x, y, idx, charDy);
    i->_textPos.set(x+ spaceSize, y);
    dx+= i->_pos.dx;
    x+= idx;
  }

  pos.setD(pos.x0, pos.y0, dx, dy);

}


void ixMenuBar::hideMenus() {
  for(ixMenuBarItem *p= (ixMenuBarItem *)items.first; p; p= (ixMenuBarItem *)p->next)
    if(p->menu)
      p->menu->hideMenu();
}









void ixMenuBar::move(int32 in_x0, int32 in_y0) {
  ixBaseWindow::move(in_x0, in_y0);
  _computeBarPositions();
}


void ixMenuBar::moveDelta(int32 in_dx, int32 in_dy) {
  ixBaseWindow::moveDelta(in_dx, in_dy);
  _computeBarPositions();
}


void ixMenuBar::resize(int32 in_dx, int32 in_dy) {
  return;   // disabled
}


void ixMenuBar::resizeDelta(int32 in_dx, int32 in_dy) {
  return;   // disabled
}


void ixMenuBar::setPos(int32 in_x0, int32 in_y0, int32 in_dx, int32 in_dy) {
  ixBaseWindow::move(in_x0, in_y0);
  _computeBarPositions();
}


















bool ixMenuBar::_update(bool in_mIn,bool updateChildren) {
  if((!is.visible) || (is.disabled))
    return false;

  recti r; getVDcoordsRecti(&r);
  recti r2;
  // update it's children first
  if(updateChildren)
    if(_updateChildren((in_mIn? r.inside(in.m.x, in.m.y): false)))
      return true;

  
  // FUNCTION CORE
  

  // an operation is in progress
  if(Ix::wsys()._op.win) {
    if(Ix::wsys()._op.win== this) {
      if(Ix::wsys()._op.mLclick) {
        if(!in.m.but[0].down) {
          if(Ix::wsys()._op.p) {
            r2= ((ixMenuBarItem *)Ix::wsys()._op.p)->_pos;
            r2.moveD(hook.pos.x, hook.pos.y);
            if(r2.inside(in.m.x, in.m.y)) {
              //_selected= (ixMenuBarItem *)Ix::wsys._op.p;
              /// toggle selection
              if(_selected== (ixMenuBarItem *)Ix::wsys()._op.p)
                _selected= null;
              else
                _selected= (ixMenuBarItem *)Ix::wsys()._op.p;

              Ix::wsys()._op.delData();
              Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
              return true;
            } /// mouse is depressed and still inside the operation menu bar item
          }
        } /// left mouse button depressed
      } /// left mouse click operation
    } /// operation on this window

    
  // no operation is in progress
  } else if(in_mIn) {
    if(in.m.but[0].down) {
      //if(mPos(x, y, pos.dx, pos.dy)) {
      if(r.inside(in.m.x, in.m.y)) {
        for(ixMenuBarItem *p= (ixMenuBarItem *)items.first; p; p= (ixMenuBarItem *)p->next) {
          r2= p->_pos; r2.moveD(hook.pos.x, hook.pos.y);
          if(r2.inside(in.m.x, in.m.y)) {
            Ix::wsys()._op.mLclick= true;
            Ix::wsys()._op.win= this;
            Ix::wsys()._op.p= p;
            Ix::wsys().bringToFront(this);
            Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
            return true;
          }
        }
      }
    } /// left mouse button is being pressed
  } /// operation in progress / no operation in progress



  // hover highlight
  bool anyHovered= false;
  for(ixMenuBarItem *p= (ixMenuBarItem *)items.first; p; p= (ixMenuBarItem *)p->next) {
    recti r= p->_pos;
    r.moveD(hook.pos.x, hook.pos.y);
    if(r.inside(in.m.x, in.m.y)) {
      if(_selected!= p) {
        _hovered= p;
        anyHovered= true;
        break;
      }
    }
  }

  if(!anyHovered) _hovered= null;


  
  // determining when to hide the menu
  
  /// 1. if current focus window is not part of this bar at all, hide
  bool hide= false;
  if(Ix::wsys().focus) {
    if(Ix::wsys().focus->_type== ixeWinType::menu) {
      if(((ixMenu *)Ix::wsys().focus)->root!= this)
        hide= true;

    } else if(Ix::wsys().focus->_type== ixeWinType::menuBar) {
      if(Ix::wsys().focus!= this)
        hide= true;

    } else 
      hide= true;
  }
  /// 2. ??? to further think of this, if more checks must happen.
  // APP FOCUS I THINK ... maybe... i dono... using wsys::focus just works fine and simple

  if(hide) {
    _selected= null;
    hideMenus();
  }



  // base window update, at this point - no childrens tho, those were updated first
  return ixBaseWindow::_update(in_mIn, false);
}

#ifdef IX_USE_OPENGL
void ixMenuBar::_glDraw(Ix *in_ix, ixWSsubStyleBase *in_style) {
if(!is.visible) return;
  //int32 charDy= ixPrint::getCharDy(font.selFont);
  //int32 prx, pry;

  
    /// get all required assets
  //ixWinSys::ixWSshader *sl= in_ix->wsys.getShader(in_ix);
  //if(!sl) return;           // at this point, there's an error    

  //ixWSstyle::GPU *sGPU= s->parent->getGPUassets(in_ix);
  //if(!sGPU) return;

  //ixFontStyle *saveStyle= in_ix->pr.style;
  in_ix->pr.style= &font;
  //in_ix->pr.setScissor(&_clip);

  /*
  mkay, everything works with draw class, and it's clean and lean
    liking the black border on a transparent window, i dono why, it's something about it
    it seems elegant and simple
    well, further polishes... and think on vulkan. i must know what platforms support it
    what platforms support opengl
    and what consoles use, probly i won't touch that, but info is important
    */


  // menu draw
  
  in_ix->glo.draw.quad.useProgram();
  in_ix->glo.draw.quad.setClipPlaneR(_clip);
  in_ix->glo.draw.quad.setColorv(colorBRD);
  in_ix->glo.draw.quad.setHollow(1.0f);
  in_ix->glo.draw.quad.setCoordsDi(pos.x0+ hook.pos.x, pos.y0+ hook.pos.y, pos.dx, pos.dy);
  in_ix->glo.draw.quad.render();
  in_ix->glo.draw.quad.setHollow(-1.0f);

  //glUseProgram(sl->gl->id);
  //sl->setClipPlaneR(_clip);
  //glUniform4f(sl->u_color, 1.0f, 1.0f, 1.0f, 1.0f);
  //glUniform3f(sl->u_origin, 0.0f, 0.0f, 0.0f);
  //sl->hollowRect(pos.x0+ hook.pos.x, pos.y0+ hook.pos.y, pos.dx, pos.dy);


  // menu items
  for(ixMenuBarItem *p= (ixMenuBarItem *)items.first; p; p= (ixMenuBarItem *)p->next) {
    if(_hovered== p|| _selected== p) {
      //glUseProgram(sl->gl->id);
      
      //glUniform4fv(sl->u_color, 1, (_hovered== p? colorHover: colorFocus));

      //glUniform1i(sl->u_customPos, 1);             // enable custom quad
      //glUniform1i(sl->u_useTexture, 0);
      //glUniform2f(sl->u_quadPos0, (float)(p->_pos.x0+ hook.pos.x), (float)(p->_pos.y0+ hook.pos.y));
      //glUniform2f(sl->u_quadPosE, (float)(p->_pos.xe+ hook.pos.x), (float)(p->_pos.ye+ hook.pos.y));
      //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      //glUniform1ui(sl->u_customPos, 0);            // disable shader custom quad draw
      //glUniform1ui(sl->u_useTexture, 1);

      if(p->menu && _selected== p) {
        p->menu->setVisible(true);
        p->menu->_glDraw(in_ix);
      }
    } else 
      if(p->menu)
        p->menu->setVisible(false);

    in_ix->pr.txt32_2i(p->_textPos.x+ hook.pos.x, p->_textPos.y+ hook.pos.y, p->text);
  }
}
#endif



#ifdef IX_USE_VULKAN
void ixMenuBar::_vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style) {
  if(!is.visible) return;
  if(!_clip.exists()) return;
  
  in_ix->pr.style= &font;
  //in_ix->pr.setScissor(&_clip);

  // menu draw
  recti r(pos); r.moveD(hook.pos.x, hook.pos.y);
  
  in_ix->vk.CmdBindPipeline(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipeline);
  in_ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipelineLayout, 0, 1, &in_ix->vki.glb[in_ix->vki.fi]->set->set, 0, null);
  in_ix->vki.draw.quad.cmdTexture(in_cmd, null);

  in_ix->vki.cmdScissor(in_cmd, &_clip);

  in_ix->vki.draw.quad.push.color= colorBRD;
  in_ix->vki.draw.quad.push.hollow= 1.0f;
  in_ix->vki.draw.quad.flagTexture(false);
  in_ix->vki.draw.quad.setPosR(r);
  in_ix->vki.draw.quad.cmdPushAll(in_cmd);
  in_ix->vki.draw.quad.cmdDraw(in_cmd);

  // menu items
  for(ixMenuBarItem *p= (ixMenuBarItem *)items.first; p; p= (ixMenuBarItem *)p->next) {
    if(_hovered== p|| _selected== p) {
      in_ix->vk.CmdBindPipeline(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipeline);
      in_ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipelineLayout, 0, 1, &in_ix->vki.glb[in_ix->vki.fi]->set->set, 0, null);
      in_ix->vki.draw.quad.cmdTexture(in_cmd, null);

      in_ix->vki.draw.quad.push.color= (_hovered== p? colorHover: colorFocus);
      in_ix->vki.draw.quad.push.hollow= -1;
      in_ix->vki.draw.quad.setPosDi(p->_pos.x0+ hook.pos.x, p->_pos.y0+ hook.pos.y, 0, p->_pos.dx, p->_pos.dy);
      in_ix->vki.draw.quad.cmdPushAll(in_cmd);
      in_ix->vki.draw.quad.cmdDraw(in_cmd);

      if(p->menu && _selected== p) {
        p->menu->setVisible(true);
        p->menu->_vkDraw(in_cmd, in_ix);
      }
    } else 
      if(p->menu)
        p->menu->setVisible(false);

    in_ix->pr.txt32_2i(p->_textPos.x+ hook.pos.x, p->_textPos.y+ hook.pos.y, p->text);
  }

  in_ix->vki.draw.quad.push.hollow= -1.0f;
}
#endif /// IX_USE_VULKAN













