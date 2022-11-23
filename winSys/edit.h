#pragma once



class ixEdit: public ixBaseWindow {
public:
  
  ixTxtData text;
  bool enterPressed;  /// used with the oneline-type edit
  
  // edit object cfg
  struct Usage: public ixBaseWindow::Usage {
  protected:
    unsigned oneLine: 1;        // limit the editor to one line (for the special one line editor with a fixed buffer, call setOneLineFixed())
    unsigned fixedBuffer: 1;    // SPECIAL kind of editor: it works ONLY in conjunction with oneLine, it has a fixed number of unicodes - use setOneLineFixed()
  public:
    unsigned acceptCombs: 1;    // accepts combining diactriticals
    unsigned hasCursor: 1;      // text has a cursor that is movable
    unsigned selection: 1;      // text can be selected - all selection functions avaible
    unsigned readOnly: 1;       // text is read only or not

    unsigned onlyNumbers: 1;    // accepts only numbers - special type of edit, manipulates a number - to be expanded for floats??? maybe it's too much
    int64 numberMin, numberMax; // when using 'onlyNumbers', this sets a minimum and maximum for the number

    int32 limitUnicodes;        // limit total number of unicodes the text can have / this is the width of the fixed buffer if used

    bool setOneLineFixed(int32 nrUnicodes); // sets the editor in a special one-line, fixed buffer mode. the buffer is UTF-32 format (int32 per unicode value)
    char32 *getOneLineFixedBuffer() { return ((ixEdit *)_win)->text._fixedBuffer; } // if using the special fixed buffer, returns a pointer to it to be easily accessed

    void whiteList(int32 from, int32 to= -1);         // whiteLists an unicode or a sequence of unicodes
    void whiteListDel(int32 from= -1, int32 to= -1);  // removes the whole list if no params are passed, or searches for the unicode or unicodes to remove from the list
    void blackList(int32 from, int32 to= -1);         // blackLists an unicode or a sequence of unicodes
    void blackListDel(int32 from= -1, int32 to= -1);  // removes the whole list if no params are passed, or searches for the unicode or unicodes to remove from the list

    Usage(ixBaseWindow *in_p): ixBaseWindow::Usage(in_p) { delData(); }
    void delData() { oneLine= onlyNumbers= acceptCombs= fixedBuffer= 0; 
                     hasCursor= selection= readOnly= 1;
                     limitUnicodes= 0; numberMin= INT64_MIN; numberMax= INT64_MAX; }

  protected:
    friend class ixEdit;
    friend class ixTxtData;

    class _List: public chainData {
    public:
      uint32 from, to;
    };
    chainList _whiteList, _blackList;

  } usage;

  Is is;

  // funcs

  inline void clearText() { text.clearText(); }

  // virtual funcs that must be changed

  void resize(float dx, float dy);          // resizes window, this will move all children hooked on the right and bottom side
  void resizeDelta(float dx, float dy);     // resizes window (enlarges, shrinkens by specified amount), this will move all children hooked on the right and bottom side
  void setPos(float x0, float y0, float dx, float dy); // sets position and size of the window
  void _computeChildArea();     // computes the total child area - call after a child moves/resizes - updated with textDx/Dy

  // constructor / destructor

  ixEdit();
  ~ixEdit();
  void delData();


private:
  
  bool _update(bool updateChildren= true);

  #ifdef IX_USE_OPENGL
  virtual void _glDraw(Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif
  #ifdef IX_USE_VULKAN
  virtual void _vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif

  void _delSelection() { text.sel.delSelection(); }
  void _copy(str32 *out_str) { text.sel.copy(out_str); }
  void _paste(str32 *in_str) { text.sel.paste(in_str); }
  bool _checkLimits(char32 unicode);
  void _setClickLimits(int32 in_delta); // (default: 7x7 box) any mouse up that moved outisde this box is considered a drag instead

  void _applyColorsFromStyle() { ixBaseWindow::_applyColorsFromStyle(); colorBRD= ((ixWSgenericStyle *)style)->colorBRD, colorBRDfocus= ((ixWSgenericStyle *)style)->colorBRDfocus; }

  friend class ixWinSys;
  friend class ixTxtData;
};











