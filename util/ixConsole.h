#pragma once


/* thouths on ixConsole

- cmd line should not exist, basically, because it can just be a cursor position in the buffer
- left-right should work in cmd line
- up/down should be able to copy a whole line from the buffer to current line buffer (cmd line)
- ctrl-c / ctrl-v should work, for current cmd line
- so you could edit lines from the buffer this way, specific text, etc
- there should be a blinking cursor (dx size should be half of dy size)

- MOUSE could be used to select a line, maybe (like the up/down) 
  - right click clear the cmd line
  - left-drag should be able to select _from cmd line_, not to over complicate stuff



  this buffer width... text width for a certain font... what a res changes? what then? reformat the whole console? wipe it?
  i just don't like it... i think a buffer line should hold a max line, and a line can be on multiple lines

*/

class Ix;

class ixConsole {
  Ix *_ix;
public:

  // vars
  
  vec4 bgColor;
  ixTexture *bgTex;             // set bgTex.fileName
  int showPercentage;           // what percentage of the screen will be used to show the camera (100 is fullscreen, 50, half the screen, etc)
  std::mutex mutex;
  //void *font;                 // console ixPrint font
  ixFontStyle fontStyle;        // console ixPrint font style
  str8 texFile;                 // texture file to use for the background

  // funcs

  inline void hide() { mutex.lock(); bShow= false; mutex.unlock(); }
  inline void show() { mutex.lock(); bShow= true; mutex.unlock(); }
  inline void toggle() { mutex.lock(); bShow= (bShow? false: true); mutex.unlock(); }
  inline void setBgTexture(cchar *fname) { texFile= fname; _loadTexture(); }

  inline void saveBuffers() { mutex.lock(); _saveBuffers(); mutex.unlock(); }   // saves the console buffers
  inline void clearBuffers() { mutex.lock(); _clearBuffers(); mutex.unlock(); }        // clears the console buffers

  void update();
  void _draw(int32 in_x0, int32 in_y0, int32 in_dx, int32 in_dy);
  void _glDraw(int32 in_x0, int32 in_y0, int32 in_dx, int32 in_dy);
  void _vkDraw(int32 in_x0, int32 in_y0, int32 in_dx, int32 in_dy);
  void print(cchar *txt);
  void printf(cchar *txt, ...);

  // constructor / destructor

  ixConsole();
  ~ixConsole();
  void delData();

private:

  bool bShow;
  
  // with 256 buffer lengths very smart loops can be done with uint8 for array index (0 - 1 = 255 -> loop to the last)
  //struct { uint8 txt[256]; uint8 saved; uint8 nlines; } buffer[256];

  class TxtData: public chainData {
  public:
    str8 txt;                 // text of the line
    uint8 saved;              // line was saved on disk or not; initial true, empty lines won't save to disk;
    uint32 lineNr;            // actual line number, from total lines that were typed
    int32 nlines;            // nr lines that will be required to print the buffer on screen
    TxtData(): saved(1), lineNr(0) { next= prev= null; }
  };
  chainList lines;
  const uint32 _maxLines;     // the number of lines kept in memory at a time, 
  uint32 nrLinesFromStart;    // total number of lines in the console (hopefully this won't ever overflow lol)

  uint8 _sel1, _sel2;         // selected text point 1 and 2 (start and end points, but any cannot be trully named a start or an end)
  str8 cmd;                   // current command text
  //uint8 cmdPos;               // one of the buffer line numbers [0-255]
  int32 cursorPos;            // cursor position in cmdLine
  int16 scrollY;              // scrollX - cmd line scroll / scrollY - console buffer scroll
  uint8 visibleX0, visibleXe;

  // command line scrolling helpers
  void _adjustVisibleX0(uint8 c, int editSize); // makes sure that character number c is visible, scrolling to the left if neccesary
  void _adjustVisibleXe(uint8 c, int editSize); // makes sure that character number c is visible, scrolling to the right if neccesary

  

  friend class Ix;
  

  void init();
  void _rawPrint(cchar *, int);      // prints the line, no checks, no nothing, n chars printed
  void _saveBuffers();
  void _clearBuffers();
  void _loadTexture();


  // internal++ don't think will ever be needed for outside
  void __delSelection();  // [can be made inline]
  friend class ixTxtData;
};

//extern ixConsole console;










