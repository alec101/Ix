#pragma once


class ixWindow: public ixBaseWindow {
public:
  
  // window cfg

  Usage usage;
  Is is;

  // window title cfg

  bool useTitle;
  ixStaticText *title;      /// title is of object type StaticText
  void setTitle(cchar *text, ixWSgenericStyle *style= null);   // sets the window title & updates the title size, according to the text size (use title.updateSizeFromText() if not using this func)
  void setTitlePosition(ixEBorder in_hookBorder, int16 in_orientation, int32 in_distance, bool in_inside);  // set window title positions/characteristics: hookBorder- border that it hooks to the main window; orientation 90/270 - horizontal 0/180 vertical; distance- distance from the border, in pixels; inside- the title is inside the window

  // funcs





  // constructor / destructor

  ixWindow();
  ~ixWindow();
  void delData();


protected:
  bool _update(bool in_mouseInside, bool in_updateChildren= true);  /// updates the window

  #ifdef IX_USE_OPENGL
  void _glDraw(Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif
  #ifdef IX_USE_VULKAN
  void _vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif

  void _applyColorsFromStyle() { ixBaseWindow::_applyColorsFromStyle(); colorBRD= ((ixWSgenericStyle *)style)->colorBRD, colorBRDfocus= ((ixWSgenericStyle *)style)->colorBRDfocus; if(title) title->_applyColorsFromStyle(); }
  friend class ixWinSys;
};







