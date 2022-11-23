#pragma once

class ixButton: public ixBaseWindow {

public:
  
  // behaivour flags

  struct ButtonUsage: public ixBaseWindow::Usage {
    unsigned toggleable: 1;     // toggled button: when pressed, it stays pressed / on second press it depresses

    ButtonUsage(ixBaseWindow *in_p): Usage(in_p) { delData(); }
    void delData() { Usage::delData(); toggleable= 0; }
  } usage;

  // usage flags

  struct ButtonIs: public ixBaseWindow::Is {
    unsigned activated: 1;        // currently, the button is activated - this will happen once for a normal button, for toggled buttons will stay activated
    unsigned pressed: 1;          // a pressed button doesn't mean it's activated; the user can change it's mind when releasing the mouse button (esc or release not over the button)

    ButtonIs() { delData(); }
    void delData() { Is::delData(); activated= pressed= 0; }
  } is;

  ixWSgenericStyle *stylePressed;

  // children objects

  str8 text;                /// button text
  float textX, textY;       /// text position inside the button
  //ixFontStyle font;         /// font that will be used for the text

  void setText(cchar *s) { text= s; } /// sets the button text

  // funcs

  void setActivate(bool);


  // setup a function to be called when the button is pressed
  inline void setOnActivateFunc(void (*in_func)(ixButton *in_this)) { onActivate= in_func; }
  void (*onActivate)(ixButton *in_this);

  void setTextCentered(cchar *in_text);


  // constructor / destructor

  ixButton();
  ~ixButton();
  void delData();

private:
  bool _update(bool updateChildren= true); /// updates the window

  uint16 _specialAction;
  void _doSpecialAction();

  #ifdef IX_USE_OPENGL
  virtual void _glDraw(Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif
  #ifdef IX_USE_VULKAN
  virtual void _vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif

  void _applyColorsFromStyle() { ixBaseWindow::_applyColorsFromStyle(); colorBRD= ((ixWSgenericStyle *)style)->colorBRD, colorBRDfocus= ((ixWSgenericStyle *)style)->colorBRDfocus; }
  friend class ixWinSys;
};











