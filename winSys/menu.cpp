#include "ix/ix.h"
#include "ix/winSys/_privateDef.h"

/* TODO:
 - menu::update needs more work - MENU BARELY FUNCTIONAL ACTUALLY
 - menu::draw needs more work
 - SELECTED COLOR in vkDraw is const, needs a proper color

- a 'pointing' system... more than one menu item can point to the same menu ?
  maybe the mouse could r-click into some menu that can be accessed from a diff place?
  it could be some neat feature, but it can make things excessivly hard and easy to be bugged...

  - move/resize/etc virtuals, must be handled in the menu i think. either disabled or think about it
  
  im thinking the dx/dy of the (normal) menu could be set in stone, no more computations

    cuz moving a menu to x and y would be way faster that way...
    that can be a very good optimisation
    cuz computing the whole menu items and everything every time...
    

    
 Disabled (if that's ok?):
  void ixMenu::move(int32 x0, int32 y0)
  void ixMenu::moveDelta(int32 dx, int32 dy)
  void ixMenu::resize(int32 dx, int32 dy)
  void ixMenu::resizeDelta(int32 dx, int32 dy)
  void ixMenu::setPos(int32 x0, int32 y0, int32 dx, int32 dy)
*/



// constructors / destructors

ixMenu::ixMenu(): ixBaseWindow(&is, &usage), usage(this) {
  _type= ixeWinType::menu;

  _customPos.set(0.0f, 0.0f);
  _highlighted= null;

  parentItem= null;
  parentBar= null; 
  root= null;

  

  borderAutoSize= true;
  menuBorder= (float)_getUnitMonitor(&osi.display.monitor[0]);   // don't think these need computing here
  itemBorder= (float)_getThinUnitMonitor(&osi.display.monitor[0]);

  _dxRequired= 0, _dyRequired= 0;
  //_scale= 1.0f;
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



void ixMenu::showAndSetPosition(float x, float y) {
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


void ixMenu::hideMenu() {
  for(ixMenuItem *p= (ixMenuItem *)items.first; p; p= (ixMenuItem *)p->next)
    if(p->childMenu)
      p->childMenu->hideMenu();
  setVisible(false);
}

// disabled
void ixMenu::move(float x0, float y0) { return; }
void ixMenu::moveDelta(float dx, float dy) { return; }
void ixMenu::resize(float dx, float dy) { return; }
void ixMenu::resizeDelta(float dx, float dy) { return; }
void ixMenu::setPos(float x0, float y0, float dx, float dy) { return; }









float ixMenu::_getTriangleWidth() {
  float charDy= font.getCharDy();
  float charDx= font.getCharDx(' ');

  return MAX(charDx, charDy)/ 2.0f;
}


inline bool ixMenu::_itemPartOfThis(ixMenuItem *in_i) {
  return (in_i->parent== this);
}


void ixMenu::_computeMenuItemPos() {
  float charDy= font.getCharDy(); //  mlib::roundf((float)posOrigin.dy* _scale);
  float x= menuBorder, y= menuBorder;
  float arrow= _getTriangleWidth();

  _dxRequired= 0.0f, _dyRequired= 0.0f;          // required dx & dy to fit all the items in
  _dyRequired+= menuBorder;                      // the whole menu border

  // find out each item position and bar dimensions, taking 0,0 as starting position
  for(ixMenuItem *i= (ixMenuItem *)items.first; i; i= (ixMenuItem *)i->next) {
    float tx= font.getTextLen32(i->text)+ itemBorder+ itemBorder; // 1 pixels left, 1 pixels right - or itemBorder
    float ty= charDy+ itemBorder+ itemBorder;                                           // 1 pixel up, 1 pixel down     - or itemBorder
    if(i->childMenu)
      tx+= arrow+ itemBorder+ itemBorder;
    
    /// item dimensions
    i->_pos.setD(x, y, tx, ty);
    i->_textPos.x= x+ itemBorder;
    i->_textPos.y= y+ itemBorder;

    /// whole window dimension adjust
    float a= i->_pos.dx+ menuBorder+ menuBorder;     // 2 pixels left, 2 pixels right, the menu whole border
    if(_dxRequired< a) _dxRequired= a;
    _dyRequired+= i->_pos.dy;

    y+= ty;
  }
  _dyRequired+= menuBorder;   // the whole menu border
}


void ixMenu::_computeMenuBorderSize(osiMonitor *in_m) {
  if(borderAutoSize) {
    menuBorder= (float)_getUnitMonitor(in_m);
    itemBorder= (float)_getThinUnitMonitor(in_m);
  }
}




void ixMenu::_computeMenuPos() {
  
  osiMonitor *m= null;

  // only the monitor is taken into consideration

  // compute pos x0/y0
  if(parentItem) {
    
    /// find the monitor the menu will be on
    m=   Ix::wsys()._getOsiMonitorForCoords(_scaleDiv(parentItem->_pos.xe- 1.0f+ hook.pos.x), _scaleDiv(parentItem->_pos.ye- 1.0f+ hook.pos.y));
    if(m== null)
      m= Ix::wsys()._getOsiMonitorForCoords(_scaleDiv(parentItem->_pos.x0+ hook.pos.x),       _scaleDiv(parentItem->_pos.ye- 1.0f+ hook.pos.y));
    if(m== null)
      m= Ix::wsys()._getOsiMonitorForCoords(_scaleDiv(parentItem->_pos.x0+ hook.pos.x),       _scaleDiv(parentItem->_pos.y0+ hook.pos.y));
    if(m== null)
      m= Ix::wsys()._getOsiMonitorForCoords(_scaleDiv(parentItem->_pos.xe- 1.0f+ hook.pos.x), _scaleDiv(parentItem->_pos.y0+ hook.pos.y));
    if(m== null)
      m= Ix::wsys()._getClosestOsiMonitorForCoords(_scaleDiv(parentItem->_pos.xe- 1.0f+ hook.pos.x), _scaleDiv(parentItem->_pos.ye- 1.0f+ hook.pos.y));

    _computeMenuBorderSize(m);
    _computeMenuItemPos();      // dx, dy, all items dx, dy

    // _initial x
    if(parentItem->_pos.xe+ _dxRequired- 1.0f+ hook.pos.x<= m->x0+ m->dx- 1.0f)
      pos.x0= parentItem->_pos.xe;
    else 
      pos.x0= parentItem->_pos.x0- _dxRequired;
  
    // _initial y
    if(parentItem->_pos.y0+ _dyRequired- 1.0f+ hook.pos.y<= m->y0+ m->dy- 1.0f)
      pos.y0= parentItem->_pos.ye;
    else
      pos.y0= parentItem->_pos.y0- _dyRequired- 1.0f;

    /// adjust _initial to be in the monitor
    if(pos.x0< m->x0             - hook.pos.x) pos.x0= m->x0             - hook.pos.x;
    if(pos.x0> m->x0+ m->dx- 1.0f- hook.pos.x) pos.x0= m->x0+ m->dx- 1.0f- hook.pos.x;
    if(pos.y0< m->y0             - hook.pos.y) pos.y0= m->y0             - hook.pos.y;
    if(pos.y0> m->y0+ m->dy- 1.0f- hook.pos.y) pos.y0= m->y0+ m->dy- 1.0f- hook.pos.y;

  } else if(parentBar) {
    /// find the monitor the menu will be on
    m=   Ix::wsys()._getOsiMonitorForCoords(_scaleDiv(parentBar->_pos.x0+ hook.pos.x),    _scaleDiv(parentBar->_pos.y0+ hook.pos.y));
    if(m== null)
      m= Ix::wsys()._getOsiMonitorForCoords(_scaleDiv(parentBar->_pos.xe- 1+ hook.pos.x), _scaleDiv(parentBar->_pos.y0+ hook.pos.y));
    if(m== null)
      m= Ix::wsys()._getOsiMonitorForCoords(_scaleDiv(parentBar->_pos.x0+ hook.pos.x),    _scaleDiv(parentBar->_pos.ye- 1+ hook.pos.y));
    if(m== null)
      m= Ix::wsys()._getOsiMonitorForCoords(_scaleDiv(parentBar->_pos.xe- 1+ hook.pos.x), _scaleDiv(parentBar->_pos.ye- 1+ hook.pos.y));
    if(m== null)
      m= Ix::wsys()._getClosestOsiMonitorForCoords(_scaleDiv(parentBar->_pos.xe- 1+ hook.pos.x), _scaleDiv(parentBar->_pos.ye- 1+ hook.pos.y));

    _computeMenuBorderSize(m);
    _computeMenuItemPos();      // dx, dy, all items dx, dy

    // offset.x0
    if(parentBar->_pos.x0+ _dxRequired+ hook.pos.x<= m->x0+ m->dx- 1.0f)
      pos.x0= parentBar->_pos.x0;
    else
      pos.x0= parentBar->_pos.x0- 1.0f- _dxRequired;

    // offset.y0
    if(parentBar->_pos.y0+ _dyRequired+ hook.pos.y<= m->y0+ m->dx- 1.0f)
      pos.y0= parentBar->_pos.ye;
    else
      pos.y0= parentBar->_pos.y0- _dyRequired- 1.0f;

    /// adjust _initial to be in the monitor
    if(pos.x0< m->x0             - hook.pos.x) pos.x0= m->x0             - hook.pos.x;
    if(pos.x0> m->x0+ m->dx- 1.0f- hook.pos.x) pos.x0= m->x0+ m->dx- 1.0f- hook.pos.x;
    if(pos.y0< m->y0             - hook.pos.y) pos.y0= m->y0             - hook.pos.y;
    if(pos.y0> m->y0+ m->dy- 1.0f- hook.pos.y) pos.y0= m->y0+ m->dy- 1.0f- hook.pos.y;

  } else if(_useCustomPos) {
    m= Ix::wsys()._getOsiMonitorForCoords(_scaleDiv(_customPos.x), _scaleDiv(_customPos.y));
    if(m== null)
      m= Ix::wsys()._getClosestOsiMonitorForCoords(_scaleDiv(_customPos.x), _scaleDiv(_customPos.y));
    
    _computeMenuBorderSize(m);
    _computeMenuItemPos();      // dx, dy, all items dx, dy

    // pos.x
    if(_customPos.x+ _dxRequired<= m->x0+ m->dx- 1.0f) pos.x0= _customPos.x;
    else                                               pos.x0= _customPos.x- _dxRequired;

    // pos.y
    if(_customPos.y+ _dyRequired<= m->y0+ m->dy- 1.0f) pos.y0= _customPos.y;
    else                                               pos.y0= _customPos.y- _dyRequired;
  }

  // got everything needed for pos
  pos.setD(pos.x0, pos.y0, _dxRequired, _dyRequired);

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



















//  ##    ##    ######      ######        ####      ########    ########
//  ##    ##    ##    ##    ##    ##    ##    ##       ##       ##
//  ##    ##    ######      ##    ##    ##    ##       ##       ######      func
//  ##    ##    ##          ##    ##    ########       ##       ##
//    ####      ##          ######      ##    ##       ##       ########

bool ixMenu::_update(bool updateChildren) {
  if((!is.visible) || (is.disabled))
    return false;

  float mx, my; //, mdx, mdy;
  rectf posVD, r;
  bool insideThis;

  // update it's children first
  if(updateChildren)
    if(_updateChildren())
      return true;

  getPosVD(&posVD);
  mx= _scaleDiv(in.m.x), my= _scaleDiv(in.m.y);
  //mdx= _scaleDiv(in.m.dx), mdy= _scaleDiv(in.m.dy);

  insideThis= posVD.inside(mx, my);

  // FUNCTION CORE

  bool anyHighlight= false;
  for(ixMenuItem *p= (ixMenuItem *)items.first; p; p= (ixMenuItem *)p->next) {
    r= p->_pos; r.moveD(posVD.x0, posVD.y0);
    if(r.inside(mx, my)) {
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
  return ixBaseWindow::_update(false);
}












// DDDDDD      RRRRRR        AAAA      WW      WW
// DD    DD    RR    RR    AA    AA    WW      WW
// DD    DD    RRRRRR      AA    AA    WW  WW  WW     func
// DD    DD    RR    RR    AAAAAAAA    WW  WW  WW
// DDDDDD      RR    RR    AA    AA      WW  WW

#ifdef IX_USE_OPENGL
void ixMenu::_glDraw(Ix *in_ix, ixWSsubStyleBase *in_style) {
  error.makeme();
}
#endif /// IX_USE_OPENGL



#ifdef IX_USE_VULKAN
void ixMenu::_vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style) {
  if(!is.visible) return;
  if(!_clip.exists()) return;
  float triangleWidth= _getTriangleWidth();
  float _x, _y;
  getPosVD(&_x, &_y);

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
  in_ix->vki.draw.quad.setPosD(_x, _y, 0.0f, pos.dx, pos.dy);
  in_ix->vki.draw.quad.flagTexture(false);
  in_ix->vki.draw.quad.cmdPushAll(in_cmd);
  in_ix->vki.draw.quad.cmdDraw(in_cmd);

  /// border
  in_ix->vki.draw.quad.push.color= colorBRD;
  in_ix->vki.draw.quad.push.hollow= menuBorder;
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
    float offset= 0.0f;
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
        float dy= (p->_pos.dy- triangleWidth)/ 2.0f;
        in_ix->vki.draw.triangle.setPos(0, _x+ p->_pos.x0+ triangleWidth+ 1.0f, _y+ p->_pos.y0+ dy);
        in_ix->vki.draw.triangle.setPos(1, _x+ p->_pos.x0+ 1.0f,                _y+ p->_pos.y0+ (p->_pos.dy/ 2.0f));
        in_ix->vki.draw.triangle.setPos(2, _x+ p->_pos.x0- triangleWidth- 1.0f, _y+ p->_pos.y0+ p->_pos.dy- dy);
        in_ix->vki.draw.triangle.cmdPushAll(in_cmd);
        in_ix->vki.draw.triangle.cmdDraw(in_cmd);

      // right arrow
      } else {
        //adx= ixPrint::getCharDy(font.selFont)/ 2; <<< must use font one if this will be used
        float dy= (p->_pos.dy- triangleWidth)/ 2.0f;
        in_ix->vki.draw.triangle.setPos(0, _x+ p->_pos.xe- triangleWidth- 1.0f, _y+ p->_pos.y0+ dy);
        in_ix->vki.draw.triangle.setPos(1, _x+ p->_pos.xe- triangleWidth- 1.0f, _y+ p->_pos.y0+ p->_pos.dy- dy);
        in_ix->vki.draw.triangle.setPos(2, _x+ p->_pos.xe- 1.0f,                _y+ p->_pos.y0+ (p->_pos.dy/ 2.0f));
        
        in_ix->vki.draw.triangle.cmdPushAll(in_cmd);
        in_ix->vki.draw.triangle.cmdDraw(in_cmd);

        // MAKE ME<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<,,

      }
    }

    in_ix->pr.txt32_2f(_x+ p->_textPos.x+ offset, _y+ p->_textPos.y, p->text);
  }
}
#endif /// IX_USE_VULKAN



























// ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
// █ MENU BAR class ███████████████████████████████████████████████████████████████████
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀




// constructor / destructor

ixMenuBar::ixMenuBar(): ixBaseWindow(&is, &usage), usage(this) {
  
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
  osiMonitor  *m= Ix::wsys()._getOsiMonitorForCoords(_scaleDiv(hook.pos.x+ pos.x0), _scaleDiv(hook.pos.y+ pos.y0));
  if(m== null) m= Ix::wsys()._getClosestOsiMonitorForCoords(_scaleDiv(hook.pos.x+ pos.x0), _scaleDiv(hook.pos.y+ pos.y0));

  float textBorder= (float)_getThinUnitMonitor(m);        // a border inside the window, then the text follows inside it
  float spaceSize= font.getCharDx(' ');                   // to the left and right of the text, there's gonna be a space
  float charDy= font.getCharDy();

  float x= pos.x0+ textBorder,
        y= pos.y0+ textBorder,
        dx= textBorder+ textBorder,
        dy= textBorder+ textBorder+ charDy;

  for(ixMenuBarItem *i= (ixMenuBarItem *)items.first; i; i= (ixMenuBarItem *)i->next) {
    float idx= font.getTextLen32(i->text);
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









void ixMenuBar::move(float in_x0, float in_y0) {
  ixBaseWindow::move(in_x0, in_y0);
  _computeBarPositions();
}


void ixMenuBar::moveDelta(float in_dx, float in_dy) {
  ixBaseWindow::moveDelta(in_dx, in_dy);
  _computeBarPositions();
}


void ixMenuBar::resize(float in_dx, float in_dy) {
  return;   // disabled
}


void ixMenuBar::resizeDelta(float in_dx, float in_dy) {
  return;   // disabled
}


void ixMenuBar::setPos(float in_x0, float in_y0, float in_dx, float in_dy) {
  ixBaseWindow::move(in_x0, in_y0);
  _computeBarPositions();
}


















bool ixMenuBar::_update(bool updateChildren) {
  if((!is.visible) || (is.disabled))
    return false;

  rectf posvd, r2;
  bool insideThis;
  float mx, my; //, mdx, mdy;

  // update it's children first
  if(updateChildren)
    if(_updateChildren())
      return true;

  getPosVD(&posvd);
  mx= _scaleDiv(in.m.x), my= _scaleDiv(in.m.y);
  //mdx= _scaleDiv(in.m.dx), mdy= _scaleDiv(in.m.dy);

  insideThis= posvd.inside(mx, my);
  
  // FUNCTION CORE
  

  // an operation is in progress
  if(Ix::wsys()._op.win) {
    if(Ix::wsys()._op.win== this) {
      if(Ix::wsys()._op.mLclick) {
        if(!in.m.but[0].down) {
          if(Ix::wsys()._op.p) {
            r2= ((ixMenuBarItem *)Ix::wsys()._op.p)->_pos;
            r2.moveD(hook.pos.x, hook.pos.y);
            if(r2.inside(mx, my)) {
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
  } else if(insideThis) {
    if(in.m.but[0].down) {
      //if(mPos(x, y, pos.dx, pos.dy)) {
      //if(insideThis) {
        for(ixMenuBarItem *p= (ixMenuBarItem *)items.first; p; p= (ixMenuBarItem *)p->next) {
          r2= p->_pos; r2.moveD(hook.pos.x, hook.pos.y);
          if(r2.inside(mx, my)) {
            Ix::wsys()._op.mLclick= true;
            Ix::wsys()._op.win= this;
            Ix::wsys()._op.p= p;
            Ix::wsys().bringToFront(this);
            Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
            return true;
          }
        }
      //}
    } /// left mouse button is being pressed
  } /// operation in progress / no operation in progress



  // hover highlight
  bool anyHovered= false;
  for(ixMenuBarItem *p= (ixMenuBarItem *)items.first; p; p= (ixMenuBarItem *)p->next) {
    r2= p->_pos;
    r2.moveD(hook.pos.x, hook.pos.y);
    if(r2.inside(mx, my)) {
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
  return ixBaseWindow::_update(false);
}

#ifdef IX_USE_OPENGL
void ixMenuBar::_glDraw(Ix *in_ix, ixWSsubStyleBase *in_style) {
  error.makeme();
}
#endif



#ifdef IX_USE_VULKAN
void ixMenuBar::_vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style) {
  if(!is.visible) return;
  if(!_clip.exists()) return;
  rectf posvd; getPosVD(&posvd);

  in_ix->pr.style= &font;
  //in_ix->pr.setScissor(&_clip); disabled for some reason

  // menu draw
  
  in_ix->vk.CmdBindPipeline(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipeline);
  in_ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipelineLayout, 0, 1, &in_ix->vki.glb[in_ix->vki.fi]->set->set, 0, null);
  in_ix->vki.draw.quad.cmdTexture(in_cmd, null);

  in_ix->vki.cmdScissor(in_cmd, &_clip);

  in_ix->vki.draw.quad.push.color= colorBRD;
  in_ix->vki.draw.quad.push.hollow= 1.0f;
  in_ix->vki.draw.quad.flagTexture(false);
  in_ix->vki.draw.quad.setPosR(posvd);
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
      in_ix->vki.draw.quad.setPosD(p->_pos.x0+ hook.pos.x, p->_pos.y0+ hook.pos.y, 0.0f, p->_pos.dx, p->_pos.dy);
      in_ix->vki.draw.quad.cmdPushAll(in_cmd);
      in_ix->vki.draw.quad.cmdDraw(in_cmd);

      if(p->menu && _selected== p) {
        p->menu->setVisible(true);
        p->menu->_vkDraw(in_cmd, in_ix);
      }
    } else 
      if(p->menu)
        p->menu->setVisible(false);

    in_ix->pr.txt32_2f(p->_textPos.x+ hook.pos.x, p->_textPos.y+ hook.pos.y, p->text);
  }

  in_ix->vki.draw.quad.push.hollow= -1.0f;
}
#endif /// IX_USE_VULKAN













