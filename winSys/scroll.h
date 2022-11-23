#pragma once


class ixBaseClass;

class ixScroll: public ixBaseWindow {
public:

  int8 orientation;       // 0= horizontal, 1= vertical; [NYI:2= HOR+VERT?]
  float position;         // scroll position- on a certain step, or if a target window, it's the scroll window position. if no target or steps, position min and max is based on the scroll size in pixels
  int32 steps;            // [def:0] this could be 101 for 0%- 100% for example. it's used if the scroll has no target window, only. if target=steps=0, the amount of scrolling is based on the scroll size, in pixels
  float arrowScroll;      // [def:1] amount of steps / pixels to scroll on arrow press
  float scrWidth;         // the width of the scroll IF NOT TEXTURED; default is wsys::scrollbarWidth; it can be manually set

  ixBaseWindow *target;   // if not null, it will dirrectly scroll the target window

  vec4 colorArrows;       // color of the arrows
  vec4 colorDragbox;      // color of the movable button that you can drag for scrolling

  struct ScrollUsage: public Usage {
    unsigned fixedDragbox: 1;      // [def:0] the drag button will have a fixed size, the scrollbar minimum size will take that into account

    ScrollUsage(ixBaseWindow *in_p): Usage(in_p) { delData(); }
    void delData() { Usage::delData(); fixedDragbox= 0; }
  } usage;


  struct ScrollIs: public Is {

    ScrollIs() { delData(); }
    void delData() { Is::delData(); }
  } is;

  // main funcs


  // funcs

  void setPosition(float in_newPos);  // sets the scrollbar <position> variable, makes sure it is within possible range too
  void setPositionD(float in_delta);  // adjusts <position> by <in_delta>, makes sure it is within possible range too
  void setPositionMin();              // sets <position> to 0
  void setPositionMax();              // sets <position> to maximum possible, depending on the type of bar
  void drag(float dx, float dy);      // drag the scroll, in pixels



  float getMinDx();   // returns the minimum scroll horizontal size
  float getMinDy();   // returns the minimum scroll vertical size
  
  inline void setDragboxFixed(bool fixed= true) { usage.fixedDragbox= fixed; }
  // for 'free' scrollbars that you do what you want with, set the orientation, then the pos.x/ pos.y, then call this to set it's lengths
  void setScrollLength(float in_len) { if(orientation== 0) pos.dx= in_len; if(orientation== 1) pos.dy= in_len; _computeDeltas(); }

  // constructor / destructor

  ixScroll();
  ~ixScroll();
  void delData();

private:
  
  // arrow buttons + scrollbar drag button rects + funcs
  rectf _arrRect[2];          // up+down, left+right, this order, depending on scroll orientation
  rectf _drgRect;             // scroll drag button position
  rectf _barRect;             // bar background, starting from end of arrow1, to the begining of arrow2 - pgup/pgdown basically
  float _scroll;              // position of the scroll, in pixels
  float _scrLength;           // scroll bar length (without any buttons or dragbox)
  float _unit;

  //int32 _getOriginWidth();        // deduced width based on orientation, from posOrigin

  bool _isScrollTooShortForDragbox();

  virtual void _computeAll();           // computePos+computeButtons
  virtual void _computeAllDelta(float x, float y);  // should be faster

  void _computeButtons();               // all buttons size & pos - _arrRect[], _drgRect, _barRect, _scrLength
  void _computeArrRect();
  void _computeDrgRect();               // _drgRect : dragbox size & pos
  void _computeBarRect();               // _barRect
  void _computeScrLength();             // _scrLength


  //void _computeBarRectAndScrLength();   // _barRect + _scrLength

  inline void _moveButtonsD(float in_dx, float in_dy) { _arrRect[0].moveD(in_dx, in_dy); _arrRect[1].moveD(in_dx, in_dy); _drgRect.moveD(in_dx, in_dy); _barRect.moveD(in_dx, in_dy); _unit= (float)_getUnit(); }
  inline void _moveButtons(float in_x, float in_y) { _arrRect[0].move(in_x, in_y); _arrRect[1].move(in_x, in_y); _drgRect.move(in_x, in_y); _barRect.move(in_x, in_y); _unit= (float)_getUnit(); }

  float _getScrollFromPosition();
  float _getPositionFromScroll();
  float _getMaxPosition();
  void _asurePosInBounds();
  void _asureScrollInBounds();

  void _computePos();     // populates pos when it has a target; else it can populate only the length of the bar
  void _computeDeltas();

  void _applyColorsFromStyle() { ixBaseWindow::_applyColorsFromStyle(); colorArrows= ((ixWSscrollStyle *)style)->colorArrows, colorDragbox= ((ixWSscrollStyle *)style)->colorDragbox; }
  bool _update(bool updateChildren= true);     // update the window

  #ifdef IX_USE_OPENGL
  void _glDraw(Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif
  #ifdef IX_USE_VULKAN
  void _vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif

  friend class ixWinSys;
  friend class ixBaseWindow;
  friend class ixTxtData;
  friend class ixWinHook;
  friend class ixDropList;
  friend class ixStaticText;
  friend class ixWindow;
  friend class ixEdit;
};


















