#pragma once

//class ixWSshader;
class ixBaseWindow;

// the hooking class of any window
class ixWinHook {
public:
  // window hooking - set a point that the window will be tied to - it can be a ixBaseWindow / osiWindow / osiMonitor - it is the virtual desktop if every variable is 0/null/default
  // 4 0 5
  // 3   1   <- 8 borders, this is the order
  // 7 2 6

  // HOOKING RULES:
  //  - you can hook to a parent      - HOOKING TO PARENT MIGHT BE BY DEFAULT, WITH NO NEEDING TO SPECIFY ANYTHING <<<<<<<<,
  //  - you can hook to a window that has the same parent as this
  //  - if this is top window (no parent), hooked window has to be top object also

  int8 border;            // corner that is being hooked to [0-7] (same wheel used)
  vec3i pos;              // hooked point position in virtual desktop.
  ixBaseWindow *ixWin;    // ix base window hooked to
  osiWindow *osiWin;      // osiWindow hooked to
  osiMonitor *osiMon;     // osiMonitor hooked to
  ixBaseWindow *parent;   // window target / parent 
  
  void setAnchor(ixBaseWindow *, int8 in_border= 4);  // sets the hook to the ixBaseWindow - if window is null, it uses the virtual desktop
  void setAnchor(osiWindow *, int8 in_border= 4);     // sets the hook to the osiWindow
  void setAnchor(osiMonitor *, int8 in_border= 4);    // sets the hook to the osiMonitor
  void setAnchor(int8 in_border= 4);                  // sets the hook to the virtual desktop

  void set(ixBaseWindow *, int8 in_border1= 4, int8 in_border2= 4);  // hooks to ixBaseWindow (in point border1) and moves current window (point border2) to touch it
  void set(osiWindow *, int8 in_border1= 4, int8 in_border2= 4);     // hooks to osiWindow (in point border1) and moves current window (point border2) to touch it
  void set(osiMonitor *, int8 in_border1= 4, int8 in_border2= 4);    // hooks to osiMonitor (in point border1) and moves current window (point border2) to touch it
  void set(int8 in_border1= 4, int8 in_border2= 4);                  // hooks to virtual desktop (in point border1) and moves current window (point border2) to touch it

  //void setHookDefault();    // sets the hook to virtual desktop - 0, 0 bottom left border
  virtual void updateHooks(bool updateThis= true);       // !!! after a window is moved, the hook and all the children's hooks _must_ be updated !!!
  virtual void updateHooksDelta(int32 dx, int32 dy, bool updateThis= true); // this should be faster than updateHooks(), if delta changes per coords are known

  // constructor / destructor

  ixWinHook(void *in_parent);
  void delData();                   // call to clear the hook, usually on a parent change

private:
  
  bool _anchorToEdge;               // if true, the hooking happens to an edge of a window. if not, the hooking happens to the childArea. this has to be set on each hook switch or init

  void _compute();
  void _computeDelta(int32 in_dx, int32 in_dy);
  void _computeParentHooks();                               // updates hooked windows that are tied to this window
  void _computeParentHooksDelta(int32 in_dx, int32 in_dy);  // updates hooked windows that are tied to this window
  void _adjustHookPosWithScroll();
  void _setWinPosBorderTo0(int8 in_borderOfThis);
  bool _computeAnchorToEdgeVar(ixBaseWindow *in_window);    // calculates if the anchor will be a window edge or a childArea edge

  friend class ixBaseWindow;
};




class ixScroll;


class ixBaseWindow: public chainData {
public:


  // you can freely change these colors, will affect the window
  vec4 color;         // main background color of the window
  vec4 colorBRD;      // borders color
  vec4 colorFocus;    // main background color of the window when it has focus
  vec4 colorBRDfocus; // borders color when it has focus
  vec4 colorHover;    // color when the window is hovered (the cursor/pointer is over it, and it don't have current focus)

  vec4 *_colorToUse;
  vec4 *_colorBRDtoUse;

  ixTexture *customTex; // THIS CAN BE A THING. USE THIS FOR BACKGROUND, AND BUTTONS COULD HAVE ADDITIONAL ONE OR TWO FOR PRESS / HOVER
                        //INITIALLY CUSTOMTEX IS NULL, IF SO, USE THE STYLE.

  // behavior flags

  //vvv
  //ANY FLAG THAT CAN BE CHANGED SHOULD BE IN 'usage' IM THINKING
  //'is' STRUCT IS MORE LIKE INFO STRUCT
  //^^^

  struct Usage {

    // ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
    // THIS SHOULD BE THE DEFINING RULE OF ANY Usage CLASS OF ALL WINDOWS
    // ANY VARIABLE THAT REQUIRES MORE CHANGES INTERNALLY FOR THINGS TO CHANGE, MUST BE PRIVATE, AND FUNCTIONS TO CHANGE IT, MUST BE ADDED
    // ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄


    unsigned resizeable:1;    // it is possible to resize the object
    unsigned movable:1;       // it is possible to move the object
    unsigned minimizable:1;   // it is possible to minimize this object

    // scrollbars

    unsigned scrollbars: 1;     // has scrollbars (buttons and such small windows will ignore any scrollbar cfg, and probly never will use them)
    unsigned autoScrollbars: 1; // window will have scrollbars that are visible only when there is something to scroll

    Usage() { delData(); }
    virtual void delData() { 
      resizeable= movable= minimizable= 0;
      scrollbars= 0;
      autoScrollbars= 1;
    }
  } usage;

  // informational flags

  struct Is {
    unsigned visible: 1;     // if it is currently visible
    unsigned minimized: 1;   // it is currently minimized?
    unsigned disabled: 1;    // window is disabled - grayed out - watever effect to show it's not functioning
    unsigned opening: 1;     // opening underway (animations)
    unsigned closing: 1;     // closing underway (animations)
  
    //unsigned MOUSEfocus: 1;  // has mouse focus
    //unsigned KBfocus: 1;     // has keyboard focus
    //unsigned GPfocus: 1;     // has gamepad focus
    //unsigned JOYfocus: 1;    // has joystick focus

    //Is() { visible= 1; minimized= disabled= opening= closing= 0; }
    Is() { delData(); }
    virtual void delData() { visible= 1; minimized= 0; disabled= 0; opening= closing= 0; /*MOUSEfocus= KBfocus= GPfocus= JOYfocus= 0;*/ }
  } is;

  ixWinHook hook;       // window hooking - hook a window to another window with this
  recti pos;            // window position, origin is based on the hook
  
  // returns window coordinates based on the hook, in the virtual dektop
  inline void getVDcoords2f(float *out_x, float *out_y) { if(out_x) *out_x= (float)(hook.pos.x+ pos.x0); if(out_y) *out_y= (float)(hook.pos.y+ pos.y0); }
  inline void getVDcoords2i(int32 *out_x, int32 *out_y) { if(out_x) *out_x= hook.pos.x+ pos.x0;          if(out_y) *out_y= hook.pos.y+ pos.y0; }
  inline void getVDcoordsv3(vec3i *out) { if(out) out->set(hook.pos.x+ pos.x0, hook.pos.y+ pos.y0, 0); }
  inline void getVDcoordsRecti(recti *out) { if(out) out->setD(hook.pos.x+ pos.x0, hook.pos.y+ pos.y0, pos.dx, pos.dy); }

  ixWSsubStyleBase *style;       // object style: texture/ colors/ border props/ everything

  // link objects - parent + childrens

  // the childrens chainList, is more than just a list of the window's children
  //   -the drawing happens from last to first, and updating from first to last
  //   -when a children gains focus, that window moves to first
  //   -it's the simplest way to do the ordering of windows
  chainList childrens;      // all children objects that belong to this object
  ixBaseWindow *parent;     // parent of this window

  // funcs

  //virtual void draw(Ix *in_ix, ixWSsubStyleBase *in_style= null); // draws the window; can pass another style to use - ATM this is a simple but not elegant way draw the window in another state
  void draw(Ix *in_ix); // draws the window - use wsys::draw() to draw all windows, instead of this. it will be faster, less inits

  // update the window - returns true if an action happened on this window or it's children
  inline bool update(bool updateChildren= true) { if(parent) { recti r; parent->getVDcoordsRecti(&r); return _update(r.inside(in.m.x, in.m.y), updateChildren); } else return _update(true, updateChildren); }

  virtual void move(int32 x0, int32 y0);            // moves window and all children to specified coords
  virtual void moveDelta(int32 dx, int32 dy);       // moves window and all children a delta distance (deltax, deltay)
  virtual void resize(int32 dx, int32 dy);          // resizes window, this will move all children hooked on the right and bottom side
  virtual void resizeDelta(int32 dx, int32 dy);     // resizes window (enlarges, shrinkens by specified amount), this will move all children hooked on the right and bottom side
  virtual void setPos(int32 x0, int32 y0, int32 dx, int32 dy); // sets position and size of the window

  /// these could include normal buttons to be able to be shown, the'x', minimize, restore, and maybe some window icon
  virtual int32 getMinDx() { return 15; }           // returns the minimum dx of the window, in pixels
  virtual int32 getMinDy() { return 15; }           // returns the minimum dy of the window, in pixels

  void setDisable(int in_b) { is.disabled= (in_b? 1: 0); }
  void setVisible(bool in_b) { is.visible= (in_b? 1: 0); }

  void changeParent(ixBaseWindow *);
  void removeParent();

  //bool loadTex(cchar *file);        // loads a texture from the specified file; [tex] internal class will point to it
  //bool loadDef();                   // loads the default window (defWinTex.tga)

  // constructor / destructor

  ixBaseWindow();
  ~ixBaseWindow();
  virtual void delData();

protected:

  uint8 _getDisableColori(uint in_r, uint in_g, uint in_b) { return (uint8)((in_r+ in_g+ in_b)/ 3); }
  float _getDisableColorf(float in_r, float in_g, float in_b) { return (in_r+ in_g+ in_b)/ 3.0f; }
  vec4 _getDisableColor(vec4 in_c) { float n= (in_c.r+ in_c.g+ in_c.b)/ 3.0f;  return vec4(n, n, n, in_c.a); }

  ixScroll *hscroll, *vscroll;

  // child and view area
public:
  recti _viewArea;                  // the viewing area of the window. scrollbar heavily uses this in conjunction with _childArea
  recti _childArea;                 // total child area
  bool _clipUsePrentsParent;        // special windows child windows (scrolls/title) will use window's parent for the clipping
  recti _clip;                      // the clipping plane, for drawing
protected:
  


  virtual void _computeAll();
  virtual void _computeAllDelta(int32 dx, int32 dy);

  virtual void _computeViewArea();  // call after any window resize
  virtual void _computeChildArea(); // computes the total child area - call after a child moves/resizes - depends on _viewArea, so call that first
  inline void _getVDviewArea(recti *out_r) { out_r->setD(_viewArea.x0+ pos.x0+ hook.pos.x, _viewArea.y0+ pos.y0+ hook.pos.y, _viewArea.dx, _viewArea.dy); }

  virtual void _computeScrollBars();
  
  //inline void _computeClipPlane() { if(parent) { parent->_getVDviewArea(&_clip); _clip.intersectRect(parent->_clip); } else _clip.setD(osi.display.vx0, osi.display.vy0, osi.display.vdx, osi.display.vdy); }
  void _computeClipPlane();
  void _computeClipPlaneDelta(int32 dx, int32 dy);

  uint8 _type;                    // [constant] window type, this is set in constructor


  virtual bool _update(bool mouseInside, bool updateChildren= true);  // the update func that every derived class will have to further build upon
  
  #ifdef IX_USE_OPENGL
  virtual void _glDraw(Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  static void _glDrawInit(Ix *in_ix);   // call once before all windows draw, per ix
  static void _glDrawFinish(Ix *in_ix);                     // call once after all windows draw, per ix
  #endif

  #ifdef IX_USE_VULKAN
  virtual void _vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  static void _vkDrawInit(VkCommandBuffer in_cmd, Ix *in_ix);   // call once before all windows draw, per ix
  static void _vkDrawFinish(VkCommandBuffer in_cmd, Ix *in_ix);                     // call once after all windows draw, per ix
  #endif

  bool _updateChildren(bool mouseInside);
  bool _inBounds(Ix *in_ix);      // returns if true if the window is in bounds of the monitors tied to the <in_ix> engine

  virtual void _applyColorsFromStyle() { color= style->color, colorFocus= style->colorFocus, colorHover= style->colorHover; }

  void _createScrollbars();

  friend class ixScroll;
  friend class ixWinSys;
  friend class ixTxtData;
  friend class ixWinHook;
  friend class ixMenuBar;
  friend class ixEdit;
  friend class ixStaticText;
  friend class ixWindow;
  friend class ixMenu;
};
















