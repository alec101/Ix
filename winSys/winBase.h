#pragma once

class ixBaseWindow;
class ixScroll;


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

  ixEBorder border;       // corner that is being hooked to [0-7] (same wheel used)
  vec3 pos;               // hooked point position in virtual desktop.
  ixBaseWindow *ixWin;    // ix base window hooked to
  osiWindow *osiWin;      // osiWindow hooked to
  osiMonitor *osiMon;     // osiMonitor hooked to
  ixBaseWindow *parent;   // window target / parent 
  
  void setAnchor(ixBaseWindow *, ixEBorder in_border= ixEBorder::topLeft);  // sets the hook to the ixBaseWindow - if window is null, it uses the virtual desktop
  void setAnchor(osiWindow *, ixEBorder in_border= ixEBorder::topLeft);     // sets the hook to the osiWindow
  void setAnchor(osiMonitor *, ixEBorder in_border= ixEBorder::topLeft);    // sets the hook to the osiMonitor
  void setAnchor(ixEBorder in_border= ixEBorder::topLeft);                  // sets the hook to the virtual desktop

  void set(ixBaseWindow *, ixEBorder in_border1= ixEBorder::topLeft, ixEBorder in_border2= ixEBorder::topLeft);  // hooks to ixBaseWindow (in point border1) and moves current window (point border2) to touch it
  void set(osiWindow *, ixEBorder in_border1= ixEBorder::topLeft, ixEBorder in_border2= ixEBorder::topLeft);     // hooks to osiWindow (in point border1) and moves current window (point border2) to touch it
  void set(osiMonitor *, ixEBorder in_border1= ixEBorder::topLeft, ixEBorder in_border2= ixEBorder::topLeft);    // hooks to osiMonitor (in point border1) and moves current window (point border2) to touch it
  void set(ixEBorder in_border1= ixEBorder::topLeft, ixEBorder in_border2= ixEBorder::topLeft);                  // hooks to virtual desktop (in point border1) and moves current window (point border2) to touch it

  //void setHookDefault();    // sets the hook to virtual desktop - 0, 0 bottom left border
  virtual void updateHooks(bool updateThis= true);       // !!! after a window is moved, the hook and all the children's hooks _must_ be updated !!!
  virtual void updateHooksDelta(float dx, float dy, bool updateThis= true); // this should be faster than updateHooks(), if delta changes per coords are known

  // constructor / destructor

  ixWinHook(void *in_parent);
  void delData();                   // call to clear the hook, usually on a parent change

private:
  
  bool _anchorToEdge;               // if true, the hooking happens to an edge of a window. if not, the hooking happens to the childArea. this has to be set on each hook switch or init

  void _compute();
  void _computeDelta(float in_dx, float in_dy);
  void _computeParentHooks();                               // updates hooked windows that are tied to this window
  void _computeParentHooksDelta(float in_dx, float in_dy);  // updates hooked windows that are tied to this window
  void _adjustHookPosWithScroll();
  void _setWinPosBorderTo0(ixEBorder in_borderOfThis);
  bool _computeAnchorToEdgeVar(ixBaseWindow *in_window);    // calculates if the anchor will be a window edge or a childArea edge

  friend class ixBaseWindow;
};







class ixBaseWindow: public chainData {
public:

  // you can freely change these colors, will affect the window
  vec4 color;         // main background color of the window
  vec4 colorBRD;      // borders color
  vec4 colorFocus;    // main background color of the window when it has focus
  vec4 colorBRDfocus; // borders color when it has focus
  vec4 colorHover;    // color when the window is hovered (the cursor/pointer is over it, and it don't have current focus)
  
  inline void setColors(vec4 *in_col1= null, vec4 *in_colHover= null, vec4 *in_colBRD= null, vec4 *in_colFocus= null, vec4 *in_colBRDfocus= null) {
    if(in_col1)        color= *in_col1;
    if(in_colHover)    colorHover= *in_colHover;
    if(in_colBRD)      colorBRD= *in_colBRD;
    if(in_colFocus)    colorFocus= *in_colFocus;
    if(in_colBRDfocus) colorBRDfocus= *in_colBRDfocus;
  }
  inline void setColors8(rgba *in_col1= null, rgba *in_colHover= null, rgba *in_colBRD= null, rgba *in_colFocus= null, rgba *in_colBRDfocus= null) {
    if(in_col1)        color.setColoru8v(*in_col1);
    if(in_colHover)    colorHover.setColoru8v(*in_colHover);
    if(in_colBRD)      colorBRD.setColoru8v(*in_colBRD);
    if(in_colFocus)    colorFocus.setColoru8v(*in_colFocus);
    if(in_colBRDfocus) colorBRDfocus.setColoru8v(*in_colBRDfocus);
  }

  vec4 *_colorToUse;
  vec4 *_colorBRDtoUse;

  ixTexture *customTex; // THIS CAN BE A THING. USE THIS FOR BACKGROUND, AND BUTTONS COULD HAVE ADDITIONAL ONE OR TWO FOR PRESS / HOVER
                        //INITIALLY CUSTOMTEX IS NULL, IF SO, USE THE STYLE.


  chainList staticPrintList;
  class StaticPrint: public chainData {
  public:
    str8 text;
    vec3 pos;
    ixFontStyle fntStyle;
    bool visible;
  };

  // prints a static text on the window
  // using current font style if <in_style> is left null
  // returns a pointer/_handle_ for the created static text, that can be used to further modify the text with other funcs
  // return value of null, marks an error
  void *printStatic(cchar *in_text, vec3 *in_pos, ixFontStyle *in_style= null);

  // in_handle, the _handle_ of the static text to be modified
  // any parameter left null, will be unchanged
  void printStaticModify(void *in_handle, cchar *in_newText= null, vec3 *in_newPos= null, ixFontStyle *in_newStyle= null);

  // shows/hides the text
  // <in_visible> [true: show text], [false: hide text]
  void printStaticSetVisible(void *in_handle, bool in_visible= true);

  // behavior flags

  struct Usage {
  protected:
    ixBaseWindow *_win;
  public:
    // ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
    // THIS SHOULD BE THE DEFINING RULE OF ANY Usage CLASS OF ALL WINDOWS
    // ANY VARIABLE THAT REQUIRES MORE CHANGES INTERNALLY FOR THINGS TO CHANGE, MUST BE PRIVATE, AND FUNCTIONS TO CHANGE IT, MUST BE ADDED
    // ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄


    unsigned resizeable:1;    // it is possible to resize the object
    unsigned movable:1;       // it is possible to move the object
    unsigned minimizable:1;   // it is possible to minimize this object

    
    // scrollbars

    unsigned _scrollbars: 1;     // [USE FUNC TO SET] has scrollbars (buttons and such small windows will ignore any scrollbar cfg, and probly never will use them)
    unsigned _autoScrollbars: 1; // [USE FUNC TO SET] window will have scrollbars that are visible only when there is something to scroll
    void autoScrollbars(bool);
    void scrollbars(bool);

    Usage(ixBaseWindow *in_p): _win(in_p) { delData(); }
    virtual void delData() { 
      resizeable= movable= minimizable= 0;
      _scrollbars= 0;
      _autoScrollbars= 1;
    }
  };

  // informational flags

  struct Is {
    unsigned visible: 1;     // if it is currently visible
    unsigned minimized: 1;   // it is currently minimized?
    unsigned disabled: 1;    // window is disabled - grayed out - watever effect to show it's not functioning
    unsigned opening: 1;     // opening underway (animations)
    unsigned closing: 1;     // closing underway (animations)
  
    Is() { delData(); }
    virtual void delData() { visible= 1; minimized= 0; disabled= 0; opening= closing= 0; }
  };

  ixWinHook hook;       // window hooking - hook a window to another window with this
  rectf pos;            // window position - in parent's area, whatever that parent is, if there's one; [(hook+offset)] MUST call computePos() after parent is updated, to update the values
  float &scaleUI;

  //recti posVD;          // maybe
  //inline void computePosVD() { posVD= pos; posVD.move(hook.pos.x, hook.pos.y); }

  inline void getPosVD(float *out_x, float *out_y) const { if(out_x) *out_x= hook.pos.x+ pos.x0; if(out_y) *out_y= hook.pos.y+ pos.y0; }
  //inline void getPosVD(int32 *out_x, int32 *out_y) const { if(out_x) *out_x= (int32)(hook.pos.x+ pos.x0);          if(out_y) *out_y= (int32)(hook.pos.y+ pos.y0); }
  inline void getPosVD(vec3 *out) const  { out->set(hook.pos.x+ pos.x0, hook.pos.y+ pos.y0, out->z); }
  inline void getPosVD(rectf *out) const { out->setD(hook.pos.x+ pos.x0, hook.pos.y+ pos.y0, pos.dx, pos.dy); }
  //inline void getPosVD(recti *out) const { out->setD(hook.pos.x+ pos.x0, hook.pos.y+ pos.y0, pos.dx, pos.dy); }

  inline void getPosVDnoScale(float *out_x, float *out_y) const { if(out_x) *out_x= _scaleMul(hook.pos.x+ pos.x0); if(out_y) *out_y= _scaleMul(hook.pos.y+ pos.y0); }
  //inline void getPosVDnoScale(int32 *out_x, int32 *out_y) const { if(out_x) *out_x= _scaleMul(hook.pos.x+ pos.x0); if(out_y) *out_y= _scaleMul(hook.pos.y+ pos.y0); }
  inline void getPosVDnoScale(rectf *out) const { out->setD(_scaleMul(hook.pos.x+ pos.x0), _scaleMul(hook.pos.y+ pos.y0), _scaleMul(pos.dx), _scaleMul(pos.dy)); }
  //inline void getPosVDnoScale(recti *out) const { out->setD(_scaleMuli(hook.pos.x+ pos.x0), _scaleMuli(hook.pos.y+ pos.y0), _scaleMuli(pos.dx), _scaleMuli(pos.dy)); }

  ixWSsubStyleBase *style;       // object style: texture/ colors/ border props/ everything

  ixFontStyle font;             // main font used for the window

  // link objects - parent + childrens

  // the childrens chainList, is more than just a list of the window's children
  //   -the drawing happens from last to first, and updating from first to last
  //   -when a children gains focus, that window moves to first
  //   -it's the simplest way to do the ordering of windows
  chainList childrens;      // all children objects that belong to this object
  ixBaseWindow *parent;     // parent of this window

  // funcs

  void draw(Ix *in_ix); // draws the window - use wsys::draw() to draw all windows, instead of this. it will be faster, less inits

  // update the window - returns true if an action happened on this window or it's children
  inline bool update(bool updateChildren= true) { if(parent) { return _update(updateChildren); } else return _update(updateChildren); }

  virtual void move(float x0, float y0);            // moves window and all children to specified coords
  virtual void moveDelta(float dx, float dy);       // moves window and all children a delta distance (deltax, deltay)
  virtual void resize(float dx, float dy);          // resizes window, this will move all children hooked on the right and bottom side
  virtual void resizeDelta(float dx, float dy);     // resizes window (enlarges, shrinkens by specified amount), this will move all children hooked on the right and bottom side
  virtual void setPos(float x0, float y0, float dx, float dy); // sets position and size of the window
  //virtual void setPosOrigin(float x0, float y0, float dx, float dy);  // the original position of the window. In case a scale is re-applied, this origin will be used
  //virtual float applyScale();                       // sets position to [original* scale], scale is based on what wsys has defined to be target resolution and type of scaling; returns the actual scaling done

  /// these could include normal buttons to be able to be shown, the'x', minimize, restore, and maybe some window icon
  virtual float getMinDx();                         // returns the minimum dx of the window, in pixels
  virtual float getMinDy();                         // returns the minimum dy of the window, in pixels

  void setDisable(bool in_b) { _is->disabled= (in_b? 1: 0); }
  void setVisible(bool in_b) { _is->visible= (in_b? 1: 0); }

  void changeParent(ixBaseWindow *);
  void removeParent();

  // constructor / destructor

  ixBaseWindow(Is *i, Usage *u);
  ~ixBaseWindow();
  virtual void delData();

public:
  // child and view area
  rectf _viewArea;                  // the viewing area of the window. scrollbar heavily uses this in conjunction with _childArea
  rectf _childArea;                 // total child area
  bool _clipUsePrentsParent;        // special windows child windows (scrolls/title) will use window's parent for the clipping
  rectf _clip;                      // the clipping plane, for drawing

protected:

  ixeWinType _type;                 // [constant] window type, this is set in constructor
  Is *_is;
  Usage *_usage;
  ixScroll *hscroll, *vscroll;
  //recti posOrigin;                  // created position; used to re-apply scale to it, or restore to original position

   

  virtual bool _update(bool updateChildren= true);  // the update func that every derived class will have to further build upon
  
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

  bool _updateChildren();
  bool _inBounds(Ix *in_ix);      // returns if true if the window is in bounds of the monitors tied to the <in_ix> engine


  virtual void _applyColorsFromStyle() { color= style->color, colorFocus= style->colorFocus, colorHover= style->colorHover; }
  
  inline uint8 _getDisableColori(uint in_r, uint in_g, uint in_b)    const { return (uint8)((in_r+ in_g+ in_b)/ 3); }
  inline float _getDisableColorf(float in_r, float in_g, float in_b) const { return (in_r+ in_g+ in_b)/ 3.0f; }
  inline vec4 _getDisableColor(vec4 in_c)                            const { float n= (in_c.r+ in_c.g+ in_c.b)/ 3.0f;  return vec4(n, n, n, in_c.a); }

  void _createScrollbars();

  virtual void _computeAll();
  virtual void _computeAllDelta(float dx, float dy);

  virtual void _computeViewArea();  // call after any window resize
  virtual void _computeChildArea(); // computes the total child area - call after a child moves/resizes - depends on _viewArea, so call that first
  inline void _getVDviewArea(rectf *out_r) { out_r->setD(hook.pos.x+ pos.x0+ _viewArea.x0, hook.pos.y+ pos.y0+ _viewArea.y0, _viewArea.dx, _viewArea.dy); }

  virtual void _computeScrollBars();
  
  void _computeClipPlane();
  void _computeClipPlaneDelta(float dx, float dy);

  int32 _getUnit() const;                             // 720p[1] 1080p[2] 1620p[3] 2160p[4] etc for current window position
  int32 _getThinUnit() const;                         // 720p[1] 1080p[1] 1620p[1] 2160p[2] etc, mediumUnit/ 2, minimum 1
  inline int32 _getUnitMonitor(osiMonitor *in_m) const { return MAX(1, in_m->dy/ 540); }      // returns the scaled unit for specified monitor
  inline int32 _getThinUnitMonitor(osiMonitor *in_m) { return MAX(1, _getUnitMonitor(in_m)/ 2); } // thin unit= unit / 2
  int32 _getUnitCoords(float x, float y) const;       // returns the scaled unit(1080p[2], 2160p[4], etc) for the specified coords - checks what monitor has those coords inside

  //static float _getScale(const osiMonitor *in_m);
  //static float _getScale(int32 in_x, int32 in_y);

  //float _scale;     // THIS COULD HAPPEN <<<<<<<<<<<<
  
  //inline int32 _scaleMuli(int32 in) const { return mlib::roundf((float)in* scaleUI); };
  //inline int32 _scaleMuli(float in) const { return mlib::roundf(in* scaleUI); }
  //inline float _scaleMulf(int32 in) const { return (float)in* scaleUI; };
  //inline float _scaleMulf(float in) const { return in* scaleUI; }

  //inline int32 _scaleDivi(int32 in) const { return mlib::roundf((float)in/ scaleUI); }
  //inline int32 _scaleDivi(float in) const { return mlib::roundf(in/ scaleUI); }
  //inline float _scaleDivf(int32 in) const { return (float)in/ scaleUI; }
  //inline float _scaleDivf(float in) const { return in/ scaleUI; }

  inline float _scaleMul(float in) const { return in* scaleUI; }
  inline float _scaleMul(int32 in) const { return (float)in* scaleUI; }
  inline float _scaleDiv(float in) const { return in/ scaleUI; }
  inline float _scaleDiv(int32 in) const { return (float)in/ scaleUI; }
  

  inline void _scaleMulApply(rectf *out) const { out->set(out->x0* scaleUI, out->y0* scaleUI, out->xe* scaleUI, out->ye* scaleUI); }
  //inline void _scaleMulApply(recti *out) const { out->set(_scaleMuli(out->x0), _scaleMuli(out->y0), _scaleMuli(out->xe), _scaleMuli(out->ye)); }

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








