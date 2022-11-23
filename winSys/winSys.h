#pragma once

enum class ixEBorder: uint8 {
  top= 0,
  right= 1,
  bottom= 2,
  left= 3,
  topLeft= 4,
  topRight= 5,
  bottomRight= 6,
  bottomLeft= 7
};

// window types <_type> private var
enum class ixeWinType: uint16 {
  baseWindow= 0,  // ixBaseWindow
  window,         // ixWindow
  button,         // ixButton
  staticText,     // ixStatic
  edit,           // ixEdit
  title,          // ixTitle
  scrollBar,      // ixScroll
  menu,           // ixMenu         "menu.h"
  menuBar,        // ixMenuBar      "menu.h"
  radioButton,    // ixRadioButton  
  dropList,       // ixDropList     
  progressBar,    // ixProgressBar  

  endOfList
};

#include "ix/winSys/style.h"
#include "ix/winSys/winBase.h"
#include "ix/winSys/txtShared.h"
#include "ix/winSys/scroll.h"
#include "ix/winSys/button.h"
#include "ix/winSys/edit.h"
#include "ix/winSys/static.h"
#include "ix/winSys/window.h"
#include "ix/winSys/radioButton.h"
#include "ix/winSys/dropList.h"
#include "ix/winSys/menu.h"
#include "ix/winSys/progressBar.h"

#include "ix/winSys/eventSys.h"




enum class ixeWSflags: uint32 {
  none=         0x0000,

  mouseUsed=    0x0001,     // any mouse activity used in any of the windows
  keyboardUsed= 0x0002,     // any keyboard activity used in any of the windows
  joyUsed=      0x0004,     // any joystick activity used in any of the windows
  gpUsed=       0x0008,     // any gamepad activity used in any of the windows
  gwUsed=       0x0010,     // any gamewheel activity used in any of the windows

  endOfList
};



enum class ixWinScale: int32 {
  // this can be forced for any resolution; If using step system, 540 will always be the basic unit ([1080p= 540x2], [1620p= 540x3], [2160p= 540x4])
  NO_SCALE= 0,
  s720p=  720,  s1k= 720,   // special case, if using step system, it will be a special step
  s1080p= 1080, s2k= 1080,
  s1620p= 1620,
  s2160p= 2160, s4k= 2160,
  s4320p= 4320, s8k= 4320
};



//enum class ixWinScaleSystem: int32 {
//  NO_SCALING,             // no scaling will be done
//  STEP_SCALING,           // [default] ixWinScale system will be used, with specific sizes for 720p / 1080p / 2160p range resolutions
//  DIRECT_SCALING          // a target resolution will be used, everything different than that resolution, will be scaled
//};

enum class ixWinScaleSystem: int32 {
  NO_SCALING,             // no scaling will be done; scaleTarget is ignored
  OSI_WIN_DY,             // [default] based on the osiWindow's DY size (height) and scaleTarget
  OSI_PRIMARY_MONITOR,     // based on primary osiMonitor (the one main IX is created on) and scaleTarget

  OSI_WIN_DY_STEP,            // COULD BE<<<<<<<< it is implemented, maybe it has a use
  OSI_PRIMARY_MONITOR_STEP    // COULD BE<<<<<<<< it is implemented, maybe it has a use
};


// core of the whole window system //
///===============================///

class ixWinSys {
public:

  // UI SCALING ******************************************************

  ixWinScaleSystem scaleSys;      // [def:] see ixWinScaleSystem enum class; - use functions below to setup
  ixWinScale scaleTarget;         // target resolution the UI is created upon; call computeScale() if manually changed
  float scale;                    // global scale, main shader UI buffers use this
  float scaleStep;                // default 270 or 360 or 540 or 720?

  rectf uiVD;                     // Virtual Desktop with scale applied to it
  
  void setScaleSys(ixWinScaleSystem in_newSys, Ix *in_ix= null); // in_ix: if left null, main used
  float computeScale(Ix *in_ix);  // must be called on res change


  float unitAtCursor;             // a decent viewable unit @ cursor position

  float scrollBarWidth;       // [def:14@1080p] default width of all scrollbars in PIXELS; depending on the scaling system, this will be multiplied by the scale or be fixed; each scrollbar can be fine-tuned for another width if needed

  

  //void setScalingNoScale();                             // set no scaling - ix window system will not touch the window sizes provided in any way- buid your own scaling
  // [DEFAULT system] - set a step scaling system and provide the target resolution (step) the UI will be built upon;
  // the base step (unit) is 540: [1080p= 540x2] [1620p= 540x3] [2160p= 540x4] etc; anything under 1080p considered the first unit(step), 720p is special step
  // scalling (up or down) will be based only on this step system
  //void setScalingStepScaling(ixWinScale in_targetScale= ixWinScale::s1080p);
  //void setScalingDirect(int32 in_targetResDY= 1080);   // set a direct scaling system, and provide <in_targetResDY> what the target resolution is


  ixWSstyle *selStyle;      // current 'selected' style that will be used when creating a new object

  ixBaseWindow *focus;      // WIP - window that has focus - USING IT FOR IXMENUS AND THE SIMPLICITY IS MAKING IT WORK GREAT
  ixBaseWindow *hover;      // window that is hovered
  ixWinEventSys eventSys;   // event system


  ixFlags32 flags;          // [ixeWSflags: flag enum] 

  // All top windows are put here - windows that are directly placed on the virtual desktop.
  //   -the drawing is done from last to first
  //   -the updating is done from first to last
  //   -when focus on a control is gained, that window moves to first
  chainList topObjects;

  ixWindow *createWindow(cchar *name, ixBaseWindow *parent, float x, float y, float dx, float dy);
  ixButton *createButton(cchar *text, ixBaseWindow *parent, float x, float y, float dx, float dy);
  ixEdit *createEdit(ixBaseWindow *, float x0, float y0, float dx, float dy);
  ixRadioButton *createRadioButton(ixBaseWindow *parent, float x, float y, float dx, float dy);
  ixDropList *createDropList(ixBaseWindow *parent, float x, float y, float buttonDx, float buttonDy);
  ixProgressBar *createProgressBar(ixBaseWindow *parent, float x, float y, float dx, float dy);
  ixMenu *createMenu(ixBaseWindow *parent);
  ixMenuBar *createMenuBar(ixBaseWindow *parent);

  ixStaticText *createStaticText(ixBaseWindow *parent, float in_x0, float in_y0, float in_dx, float in_dy);

  void bringToFront(ixBaseWindow *);                // brings window to the top in the chainlist it's from (the parent chainlist); it will be drawn last, updated first
  void bringToBack(ixBaseWindow *);                 // brings window to bottom in the chainlist it's from (the parent chainlist); it will be drawn first, updated last
  void bringAfter(ixBaseWindow *win, ixBaseWindow *after);    // aranges window to be after the selected window
  void bringBefore(ixBaseWindow *win, ixBaseWindow *before);  // arranges window to be before the selected window

  void switchParent(ixBaseWindow *win, ixBaseWindow *parent); // parent can be null, so it will be a top window

  void delWindow(ixBaseWindow *);   // this func will delete safely any kind of window, and will handle every child/parent link
  void loadDef1Style();         // loads def1Style style - textured everything (defWinTex.tga)

  void blablaWindow();

  void draw();                  // draws all the top objects on all Ix engines
  void drawSpecific(Ix *in_ix); // draws all the top objects, on the specified engine only
  bool update();                // sets [flags] down, returns true if any HID activity happened with any of the windows, and flags what HID was used in [flags]
  void updateHooks();           // updates all top objects and their children hooks (in case hooked target moved)




  // constructor / destructor

  ixWinSys();
  ~ixWinSys();
  void delData();



protected:
  
  void init(Ix *in_ix);

  // there can be only one operation performed to only one window
  
  struct _Op {
    ixBaseWindow *win;      // window that has an action being performed to. THIS IS NULL IF NO OP IS BEING MADE
    uint64 time;            // time started, mostly, but it's up for every message to handle the time as it sees fit
    //float x, y;             // cursor position on operation start. Can be used to switch to a move operation only if movement is bigger than a delta
    //float wx, wy;           // window position/size on op start
    vec2 posOrg;
    vec2 posWin;

    unsigned resizeLeft:1, resizeRight:1, resizeBottom:1;
    unsigned moving:1;
    unsigned mLclick:1, mRclick: 1, mMclick:1, m4click:1, m5click:1;

    unsigned scrArr1:1;    // up / right arrow press, depending on orientation
    unsigned scrArr2:1;    // down / left arrow press, depending on orientation
    unsigned scrBar:1;     // the bar is being pressed, pageup/down until the dragbox middle is on the cursor
    unsigned scrDragbox:1; // the dragbox is being dragged
    void *p;               // a pointer that can point to anything

    _Op() { delData(); }
    void delData() { resizeLeft= resizeRight= resizeBottom= moving= mLclick= mRclick= mMclick= m4click= m5click=
                     scrArr1= scrArr2= scrBar= scrDragbox= 0;
                     time= 0;
                     win= null, p= null; }
  } _op;  // operation in progress

  
  //float _getScaleMonitor(int32 in_x, int32 in_y);
  //float _getScaleMonitor(const osiMonitor *in_m);
  static osiMonitor *_getOsiMonitorForCoords(float x, float y);          // checks what monitor has the VD coords inside
  static osiMonitor *_getClosestOsiMonitorForCoords(float x, float y);   // [slow] checks the closest monitor for the specified coords


  friend class Ix;
  friend class ixBaseWindow;
  friend class ixWindow;
  friend class ixButton;
  friend class ixEdit;
  friend class ixStaticText;
  friend class ixScroll;
  friend class ixTxtData;
  friend class ixRadioButton;
  friend class ixDropList;
  friend class ixMenu;
  friend class ixMenuBar;
};







