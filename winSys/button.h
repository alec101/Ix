#pragma once

class ixButton: public ixBaseWindow {

public:
  
  // behaivour flags

  struct ButtonUsage: public ixBaseWindow::Usage {
    unsigned toggleable: 1;     // toggled button: when pressed, it stays pressed / on second press it depresses

    ButtonUsage() { delData(); }
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
  int32 textX, textY;       /// text position inside the button
  ixFontStyle font;         /// font that will be used for the text

  void setText(cchar *s) { text= s; } /// sets the button text

  // funcs

  //void setText(cchar *s);
  void setTextCentered(cchar *in_text);
  

  //void draw(Ix *in_ix, ixWSsubStyleBase *dummy);   /// draws the window
  //void draw();


  // constructor / destructor

  ixButton();
  ~ixButton();
  void delData();

private:
  bool _update(bool mouseInside, bool updateChildren= true); /// updates the window

  #ifdef IX_USE_OPENGL
  virtual void _glDraw(Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif
  #ifdef IX_USE_VULKAN
  virtual void _vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif

  void _applyColorsFromStyle() { ixBaseWindow::_applyColorsFromStyle(); colorBRD= ((ixWSgenericStyle *)style)->colorBRD, colorBRDfocus= ((ixWSgenericStyle *)style)->colorBRDfocus; }
  friend class ixWinSys;
};











