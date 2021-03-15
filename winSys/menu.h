#pragma once
        

// ixMenuBar - a bar filled with ixMenu classes
// ixMenu - a menu filled with ixMenuItem(s)
// ixMenu can have other ixMenu under it


class ixMenu;
class ixMenuBarItem;
class ixMenuBar;

class ixMenuItem: public chainData {
  str32 text;           // text of the menu item
  ixMenu *childMenu;    // [def:null] if it expands a new menu
  // ixMenu *pointMenu;    // [def:null] if it expands a new menu, but that menu won't really belong to it <<< MAYBE IN THE FUTURE
  ixMenu *parent;       // needed
  ixMenuItem(): childMenu(null), parent(null) {}
  
private:
  vec2i _textPos;
  recti _pos;           // the menu bounds or position (the classic pos), inside the whole ixMenu::pos
  

  friend class ixMenu;
  friend class ixMenuBar;
};


///==========///
// MENU class //
///==========///

class ixMenu: public ixBaseWindow {
public:


  // the menu can be:
  // 1. a free menu, not having any parentBar or parentItem, usually r-click mouse menus or so
  // 2. it can belong to a menu bar (parentItem is NULL)
  // 3. it can belong to another menu, so it will belong to a ixMenuItem (parentBar is NULL)
  // ixMenuBarItem *parentBar;   // bar that it belongs to, it CAN BE NULL
  //ixMenuItem *parentItem; // menu item it belongs to, it CAN BE NULL

  ixMenuItem *parentItem;   // menu item / menu bar item it belongs to, it CAN BE NULL
  ixMenuBarItem *parentBar; 
  ixBaseWindow *root;       // it can be a bar it originates from, or another menu.

  //maybe instead of these 2, a *root base window


  ixFontStyle font;

  chainList items;
  int32 menuBorder;         // [def:2] menu border width (in pixels)
  int32 itemBorder;         // [def:1] menu item border width (in pixels)

  // funcs

  void showAndSetPosition(int32 x, int32 y);      // shows the menu on a specific place, usually called by a r-click menu



  void addMenuItem(cchar *text);
  void addMenuItemAfter(cchar *text, ixMenuItem *afterPoint);
  void addMenuItemBefore(cchar *text, ixMenuItem *beforePoint);
  void addMenuItemPos(cchar *text, int32 nr);

  void delMenuItem(ixMenuItem *i);      // deletes the menu item; if another menu is expanded from it, that whole menu is deleted too
  void delMenuItemNr(int32 nr);         // deletes the menu item number <nr>; if another menu is expanded from it, that whole menu is deleted too.

  void attachToMenuBar(ixMenuBar *, ixMenuBarItem *);         // attaches menu to the specified menuBarItem of the menuBar
  void attachToMenuBarn(ixMenuBar *in_menuBar, int32 itemNr); // attaches menu to the specified menuBarItem number, of the menuBar
  void attachToMenu(ixMenu *, ixMenuItem *);                    // attaches to the specified *menu/ *menuItem

  void detach();    // dataches from menuItem / menuBarItem. 

  void hideMenu();                      // this will hide every menu attached to this menu, the whole tree.

  //void draw(Ix *in_ix, ixWSsubStyleBase *in_style= null); // draws the window; can pass another style to use - ATM this is a simple but not elegant way draw the window in another state
  
  void move(int32 x0, int32 y0);            // moves window and all children to specified coords
  void moveDelta(int32 dx, int32 dy);       // moves window and all children a delta distance (deltax, deltay)
  void resize(int32 dx, int32 dy);          // resizes window, this will move all children hooked on the right and bottom side
  void resizeDelta(int32 dx, int32 dy);     // resizes window (enlarges, shrinkens by specified amount), this will move all children hooked on the right and bottom side
  void setPos(int32 x0, int32 y0, int32 dx, int32 dy); // sets position and size of the window

  // constructor / destructor

  ixMenu();
  ~ixMenu();
  void delData();

protected:
  ixMenuItem *_highlighted;
  
  bool _update(bool mouseInside, bool updateChildren= true);
  #ifdef IX_USE_OPENGL
  virtual void _glDraw(Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif
  #ifdef IX_USE_VULKAN
  virtual void _vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif
  
  void _computeMenuItemPos();
  void _computeMenuPos();
  void _computeMenuAllPos();               // computes menu dx/dy, each item rect, text position

  //void _computeMenuPositions();

  bool _itemPartOfThis(ixMenuItem *in_i);     // returns true if the menu item is part of <this ixMenu>
  bool _useCustomPos;
  vec2i _customPos;

  int32 _dxRequired, _dyRequired;             // dx and dy required to fit all the menu items

  int32 _getTriangleWidth();                  // the triangle (arrow) pointing to the left or right meaning the menu expands

  //vec2i _initial;
  //void _computeInitial(int32 dx, int32 dy);      // menu dimensions must be known, it will check where the menu can fit (x0 and y0)
  friend class ixMenuBar;
};






///==============///
// MENU BAR class //
///==============///


class ixMenuBarItem: public chainData {
public:
  str32 text;           // text of the menu item
  ixMenu *menu;         // menu that expands from this barItem

  ixMenuBarItem(): chainData(), menu(null) {}

private:
  vec2i _textPos;
  recti _pos;           // the menu bounds or position (the classic pos), inside the whole ixMenu::pos
  friend class ixMenu;
  friend class ixMenuItem;
  friend class ixMenuBar;
};


class ixMenuBar: public ixBaseWindow {
public:

  ixFontStyle font;       // font of the menu bar
  chainList items;        // all the menu bar menus / items

  // funcs


  void addMenuBarItem(cchar *text);

  void attachMenu(ixMenuBarItem *in_barItem, ixMenu *in_menu);     // attaches in_menu to in_barItem
  void hideMenus();                           // will hide all menus attached to this bar, the whole tree.


  //void draw(Ix *in_ix, ixWSsubStyleBase *in_style= null);

  void move(int32 x0, int32 y0);            // moves window and all children to specified coords
  void moveDelta(int32 dx, int32 dy);       // moves window and all children a delta distance (deltax, deltay)
  void resize(int32 dx, int32 dy);          // DISABLED, menus cannot be resized
  void resizeDelta(int32 dx, int32 dy);     // DISABLED, menus cannot be resized
  void setPos(int32 x0, int32 y0, int32 dx, int32 dy); // sets position of the menu. the sizes are ignored

  

  // constructor / destructor

  ixMenuBar();
  ~ixMenuBar();
  void delData();


protected:
  ixMenuBarItem *_hovered;
  ixMenuBarItem *_selected;

  bool _update(bool mouseInside, bool updateChildren= true);
  #ifdef IX_USE_OPENGL
  virtual void _glDraw(Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif
  #ifdef IX_USE_VULKAN
  virtual void _vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_s= null);
  #endif

  void _computeBarPositions();
};










