#include "ix/ix.h"
#include <mutex>
#include "osi/include/util/fileOp.h"
#include <stdarg.h>

#define IX_CONSOLE_OUTFILE "console.txt"
#define mBounds(__x1, __y1, __x2, __y2) ((in.m.x>= (__x1)) && (in.m.x<= (__x2)) && (in.m.y>= (__y1)) && (in.m.y<= (__y2)))
#define mBoundsD(__x1, __y1, __dx, __dy) ((in.m.x>= (__x1)) && (in.m.x<= ((__x1)+ (__dx))) && (in.m.y>= (__y1)) && (in.m.y<= ((__y1)+ (__dy))))

/*
console's mutex must be locked all the time, and let lose only when it is let to do it's "set variable newval"
this is an option, but strange delays can happen this way, depending on how the threads are set

TODO:
- [med priority] ctrl+left/right jump from word to word
- [med priority] ctrl+shift+left/right select jump from word to word
- [low priority] insert on/off
- [low priority] mouse cmdLine text selection - hold left to drag selection
*/

//extern ixCamera *camera;
//extern ixPrint pr;
using namespace Str;
//ixConsole console;

//void ixErrorPrint(const char *txt, bool exit, void (*exitFunc)(void));
void ixErrorPrint(const char *txt, bool exit, void (*exitFunc)(void));

// constructor / destructor ===========--------------------

ixConsole::ixConsole(): _ix(null), _maxLines(256) {
  mutex.lock();
  bgColor.set(0.05f, 0.1f, 0.07f, 0.75f);
  //_sl= null;
  
  showPercentage= 50;
  //cmdPos= 
  cursorPos= 0;
  scrollY= 0;
  nrLinesFromStart= 0;

  //font= null;
  fontStyle.drawMode= 0x0001;
  fontStyle.outlineSize= 1;
  fontStyle.color1.set(1.0f, 0.9f, 0.8f, 1.0f);
  fontStyle.color2.set(0.1f, 0.1f, 0.1f, 0.9f);
  fontStyle.selFont= null;
  

  /// create the internal buffer
  for(uint32 a= 0; a< _maxLines; a++) {
    TxtData *p= new TxtData;
    if(!p) {
      p= new TxtData;
      if(!p) error.detail("console buffer allocation failed", __FUNCTION__, __LINE__, true);
    }
    lines.add(p);
  }
  _clearBuffers();

  /// backup console file if there is one
  FILE *f1= fopen(IX_CONSOLE_OUTFILE, "rb");
  if(f1) {
    FILE *f2= fopen("console.bak", "wb");
    str8 s;
    while(readLine8(f1, &s))
      fwrite(s.d, s.len- 1, 1, f2);

    fclose(f2);
    fclose(f1);
  }

  /// wipe console output file
  f1= fopen(IX_CONSOLE_OUTFILE, "wb");
  fclose(f1);
  mutex.unlock();
}


void ixConsole::init() {
  // with all the locks and unlocks this func is not thread safe. to be fair, all mutex calls could be removed
  // it's best to make sure all inits happen in a nice orderly fashion
  
  mutex.lock();
  _FUNCconsole= &ixErrorPrint;
  _ix= Ix::getMain();
  mutex.unlock();

  /// in case errors happen, they might be printed on the console, so NO MUTEX here
  fontStyle.selFont= _ix->pr.loadFont("mono821.fnt", 18);
  if(!fontStyle.selFont)
    error.detail("Console cannot load the font", __FUNCTION__, __LINE__, true);

  mutex.lock();

  // OpenGL init
  #ifdef IX_USE_OPENGL
  if(_ix->renOpenGL()) {
    if(!_ix->glIsActive()) error.detail("OpenGL renderer is not active", __FUNCTION__, __LINE__, true);
    _loadTexture();   /// background texture
  }
  #endif

  // Vulkan init
  #ifdef IX_USE_VULKAN
  if(_ix->renVulkan()) {
    _loadTexture();
  }
  #endif

  mutex.unlock();
}


void ixConsole::_loadTexture() {
  if(texFile.d== null) return;
  //if(_ix->ren== null) return;

  if(bgTex== null)
    bgTex= _ix->res.tex.add.ixStaticTexture();

  if(bgTex->fileName!= texFile)
    _ix->res.tex.load(bgTex, texFile);
}


ixConsole::~ixConsole() {
  mutex.lock();
  _saveBuffers();
  delData();
  mutex.unlock();
}


void ixConsole::delData() {
  //if(bgTex) bgTex->delData();
}



// main functions ===========----------------------------

void ixErrorPrint(const char *txt, bool exit, void (*exitFunc)(void)) {
  // same as the terminal func
  Ix::console().print(txt);
  
  if(exit) {
    if(exitFunc)
      (*exitFunc)();
    else {
      Ix::console().saveBuffers();
      abort();    // this is the best option, it triggers a breakpoint+ callstack that point exactly to the source
    //::exit(EXIT_FAILURE);
    }
  }
}



void ixConsole::_glDraw(int32 in_x0, int32 in_y0, int32 in_dx, int32 in_dy) {
  #ifdef IX_USE_OPENGL
  if(!_ix->glIsActive())
    return;
  
  /// inits
  ixFontStyle *prevFontStyle= _ix->pr.style;    /// save current style
  _ix->pr.style= &fontStyle;
  _ix->glo.draw.quad.useProgram();

  glDisable(GL_DEPTH);

  if(bgTex) {
    glEnable(GL_TEXTURE_2D);
    _ix->glo.draw.quad.setTexture(bgTex);
  } else {
    _ix->glo.draw.quad.disableTexture();
    glDisable(GL_TEXTURE_2D);
  }
  _ix->glo.draw.quad.setCamera(_ix->camera);
  _ix->glo.draw.quad.setColorv(bgColor);
  

  int32 charDy= ixPrint::getCharDy(fontStyle.selFont);

  _ix->glo.draw.quad.delClipPlane();
  _ix->glo.draw.quad.setCoordsDi(in_x0, in_y0, in_dx, in_dy, 0);
  _ix->glo.draw.quad.setTexCoords(0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
  _ix->glo.draw.quad.render();

  // selected text background
  if(_sel1 || _sel2) {
    /// start and end are the selection range (char units), but in visible range
    uint8 start= MIN(_sel1, _sel2);
    uint8 end= MAX(_sel1, _sel2);
    start= MAX(start, visibleX0);
    end= MIN(end, visibleXe);
      
    /// text pixel sizes
    uint8 *t= (uint8 *)getChar8(cmd.d, start);

    uint8 v= *t;
    *t= 0;
    float x0= (float)ixPrint::getTextLen(getChar8(cmd.d, visibleX0), 0, fontStyle.selFont);
    *t= v;
    t= (uint8 *)getChar8(cmd.d, end);
    v= *t;
    *t= 0;
    float dx= (float)ixPrint::getTextLen(getChar8(cmd.d, start), 0, fontStyle.selFont);
    *t= v;

    /// got all data - draw
    
    _ix->glo.draw.quad.setColor(0.1f, 0.2f, 0.9f, 1.0f);
    _ix->glo.draw.quad.setCoordsD(x0+ (float)in_x0, (float)in_y0, dx, (float)charDy);
    if(bgTex)
      if(bgTex->glData.id) {
        glDisable(GL_TEXTURE_2D);
        _ix->glo.draw.quad.disableTexture();
      }
    _ix->glo.draw.quad.render();
  }
    


  int nrLines= (in_dy/ charDy)- 3+ 1; /// 3= [commandLine] [reserved for scroll1] [reserved for scroll2]
  int y0= in_y0+ charDy* 2;

  if(nrLines> 0) {

    // buffer print
    for(int a= 0, l= 0; l< nrLines; a++) {
      uint32 id= a+ scrollY;
      TxtData *lin= (TxtData *)lines.get(id);
      uint8 *p= (uint8 *)lin->txt.d;

      /// count on how many lines this buffer fits
      lin->nlines= 0;

      uint16 bytes[100];
      for(int b= 0; b< 100; b++) {
        lin->nlines++;
        bytes[b]= ixPrint::getBytesMaxPixels((cchar *)p, in_dx, fontStyle.selFont);
        if(bytes[b]< Str::strlen8((char *)p)- 1)
          p+= bytes[b];
        else
          break;
      }

      /// print the buffer line
      p= (uint8 *)lin->txt.d;
      for(int b= 0; b< lin->nlines; b++) {
        int n= l+ b;
        if(n <= nrLines)
          _ix->pr.txt2i(in_x0, y0+ (n* charDy),
                            (cchar *)p, 0, 0xFFFFFFF, 0, bytes[b]);
        p+= bytes[b];
      }

      l+= lin->nlines;
    }
    
    if(scrollY> 0) 
      _ix->pr.txt2i((in_dx- ixPrint::getTextLen("vvv", 0, fontStyle.selFont))/ 2+ in_x0, y0- charDy, "vvv");
    if(scrollY< (int16)_maxLines- 2)
      _ix->pr.txt2i((in_dx- ixPrint::getTextLen("^^^", 0, fontStyle.selFont))/ 2+ in_x0, y0+ charDy* (nrLines), "^^^");
    if(visibleX0) 
      _ix->pr.txt2i(0, y0- ixPrint::getCharDy(fontStyle.selFont), "<<<");
    if(strchars8((char *)cmd.d)> visibleXe)
      _ix->pr.txt2i((in_dx- ixPrint::getTextLen(">>>", 0, fontStyle.selFont))+ in_x0, y0- charDy, ">>>");
  } /// buffer print

  

  
  // command line print
  _ix->pr.txt2i(in_x0, in_y0, cmd.d, visibleX0, cursorPos);
    
  // cursor draw
  static int alpha= 1;                        /// cursor alpha
  static uint64 time= 0, prevTime= 0;
  osi.getMillisecs(&time);
  if(time- prevTime> 200) {               /// toggle alpha each 200 milisecs
    alpha= (alpha? 0: 1);
    prevTime= time;
  }
    
  _ix->glo.draw.quad.useProgram();
  _ix->glo.draw.quad.setColor(1.0f, 1.0f, 1.0f, (GLfloat)alpha);
  _ix->glo.draw.quad.setCoordsDi((int32)_ix->pr.pos.x, in_y0, 2, charDy);
  _ix->glo.draw.quad.render();

  _ix->pr.txt(cmd.d, cursorPos, visibleXe);


  /// default values for VAO0
  glEnable(GL_DEPTH);
  glEnable(GL_TEXTURE_2D);
  //glEnableVertexAttribArray(0);

  _ix->pr.style= prevFontStyle;
  #endif /// IX_USE_OPENGL
}






void ixConsole::_vkDraw(int32 in_x0, int32 in_y0, int32 in_dx, int32 in_dy) {
  #ifdef IX_USE_VULKAN
  
  /// inits
  VkCommandBuffer cb= _ix->vki.ortho.cmd[_ix->vki.fi]->buffer;
  ixFontStyle *prevFontStyle= _ix->pr.style;    /// save current style
  _ix->pr.style= &fontStyle;
  int32 charDy= ixPrint::getCharDy(fontStyle.selFont);
  

  // background
  _ix->vk.CmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, _ix->vki.draw.quad.sl->vk->pipeline);
  _ix->vk.CmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, _ix->vki.draw.quad.sl->vk->pipelineLayout, 0, 1, &_ix->vki.glb[_ix->vki.fi]->set->set, 0, null);
  _ix->vk.CmdSetScissor(cb, 0, 1, &_ix->vki.render.scissor);
  
  _ix->vki.draw.quad.cmdTexture(cb, bgTex);   // if null, it handles it
  _ix->vki.draw.quad.flagTexture(bgTex? true: false);
  if(bgTex) _ix->vki.draw.quad.push.color.set(1, 1, 1, 1);
  else      _ix->vki.draw.quad.push.color= bgColor;
  _ix->vki.draw.quad.setPosDi(in_x0, in_y0, 0, in_dx, in_dy);
  _ix->vki.draw.quad.setTex(0, 1, 1, 0, 0);
  _ix->vki.draw.quad.push.hollow= -1;
  _ix->vki.draw.quad.cmdPushAll(cb);
  _ix->vki.draw.quad.cmdDraw(cb);


  // selected text background
  if(_sel1 || _sel2) {
    /// start and end are the selection range (char units), but in visible range
    uint8 start= MIN(_sel1, _sel2);
    uint8 end= MAX(_sel1, _sel2);
    start= MAX(start, visibleX0);
    end= MIN(end, visibleXe);
      
    /// text pixel sizes
    uint8 *t= (uint8 *)getChar8(cmd.d, start);

    uint8 v= *t;
    *t= 0;
    float x0= (float)ixPrint::getTextLen(getChar8(cmd.d, visibleX0), 0, fontStyle.selFont);
    *t= v;
    t= (uint8 *)getChar8(cmd.d, end);
    v= *t;
    *t= 0;
    float dx= (float)ixPrint::getTextLen(getChar8(cmd.d, start), 0, fontStyle.selFont);
    *t= v;

    /// got all data - draw
    _ix->vki.draw.quad.push.color.set(0.1f, 0.2f, 0.9f, 1.0f);
    _ix->vki.draw.quad.setPosD(x0+ (float)in_x0, (float)(in_y0+ in_dy- charDy), 0, dx, (float)charDy);
    _ix->vki.draw.quad.flagTexture(false);

    _ix->vki.draw.quad.cmdPushAll(cb);
    _ix->vki.draw.quad.cmdDraw(cb);
  }
    


  int nrLines= (in_dy/ charDy)- (3+ 1); /// 3= [commandLine] [reserved for scroll1] [reserved for scroll2]
  int y0= in_y0+ in_dy- (charDy* 3);

  // buffer print
  if(nrLines> 0) {
    for(int a= 0, l= 0; l< nrLines; a++) {
      uint32 id= a+ scrollY;
      TxtData *lin= (TxtData *)lines.get(id);
      uint8 *p= (uint8 *)lin->txt.d;

      /// count on how many lines this buffer fits
      lin->nlines= 0;

      uint16 bytes[100];
      for(int b= 0; b< 100; b++) {
        lin->nlines++;
        bytes[b]= ixPrint::getBytesMaxPixels((cchar *)p, in_dx, fontStyle.selFont);
        if(bytes[b]< Str::strlen8((char *)p)- 1)
          p+= bytes[b];
        else
          break;
      }

      /// print the buffer line
      p= (uint8 *)lin->txt.d;
      for(int b= 0; b< lin->nlines; b++) {
        int n= l+ b;
        if(n <= nrLines)
          _ix->pr.txt2i(in_x0, y0- (n* charDy),
                            (cchar *)p, 0, 0xFFFFFFF, 0, bytes[b]);
        p+= bytes[b];
      }

      l+= lin->nlines;
    }
    
    if(scrollY> 0) 
      _ix->pr.txt2i((in_dx- ixPrint::getTextLen("vvv", 0, fontStyle.selFont))/ 2+ in_x0, y0+ charDy, "vvv");
    if(scrollY< (int16)_maxLines- 2)
      _ix->pr.txt2i((in_dx- ixPrint::getTextLen("^^^", 0, fontStyle.selFont))/ 2+ in_x0, y0- (charDy* nrLines), "^^^");
    if(visibleX0) 
      _ix->pr.txt2i(in_x0, y0+ charDy, "<<<");
    if(strchars8((char *)cmd.d)> visibleXe)
      _ix->pr.txt2i((in_dx- ixPrint::getTextLen(">>>", 0, fontStyle.selFont))+ in_x0, y0+ charDy, ">>>");
  } /// buffer print

  
  // command line print
  _ix->pr.txt2i(in_x0, in_y0+ in_dy- charDy, cmd.d, visibleX0, cursorPos);
    


  // cursor draw
  static int alpha= 1;                        /// cursor alpha
  static uint64 time= 0, prevTime= 0;
  osi.getMillisecs(&time);
  if(time- prevTime> 200) {               /// toggle alpha each 200 milisecs
    alpha= (alpha? 0: 1);
    prevTime= time;
  }
    
  _ix->vk.CmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, _ix->vki.draw.quad.sl->vk->pipeline);
  _ix->vk.CmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, _ix->vki.draw.quad.sl->vk->pipelineLayout, 0, 1, &_ix->vki.glb[_ix->vki.fi]->set->set, 0, null);

  _ix->vki.draw.quad.cmdTexture(cb, null);
  _ix->vki.draw.quad.push.color.set(1.0f, 1.0f, 1.0f, (float)alpha);
  _ix->vki.draw.quad.setPosDi((int32)_ix->pr.pos.x, in_y0+ in_dy- charDy, 0, 2, charDy);
  _ix->vki.draw.quad.flagTexture(false);

  _ix->vki.draw.quad.cmdPushAll(cb);
  _ix->vki.draw.quad.cmdDraw(cb);

  _ix->pr.txt(cmd.d, cursorPos, visibleXe);
  _ix->pr.style= prevFontStyle;
  #endif /// IX_USE_VULKAN
}


void ixConsole::_draw(int32 in_x0, int32 in_y0, int32 in_dx, int32 in_dy) {
  #ifdef IX_USE_VULKAN
  if(_ix->ren->type== 1)
    _vkDraw(in_x0, in_y0, in_dx, in_dy);
  #endif
  #ifdef IX_USE_OPENGL
  if(_ix->ren->type== 0)
    _glDraw(in_x0, in_y0, in_dx, in_dy);
  #endif
}











void ixConsole::update() {
    // update() should not have many problems with speed... if you open the console, the program becomes secondary
  
  mutex.lock();
  
  if(!bShow) { mutex.unlock(); return; }

  _ix->osiMutex.lock();
  
  int32 conDx= _ix->win->dx;
  int32 conDy= (showPercentage* _ix->win->dy)/ 100;
  int32 conX0= _ix->win->x0;
  int32 conY0= _ix->win->dy- conDy+ _ix->win->y0;
  
  int32 charDy= ixPrint::getCharDy(fontStyle.selFont);

  int nrLines= (conDy/ charDy)- (3+ 1); /// 3= [commandLine] [reserved for scroll1] [reserved for scroll2]
  int y0= conY0+ conDy- (charDy* 3);



  // input handling
  /// keyboard unicodes -> going to cmd line
  while(in.k.charTyped.nrNodes) {
    uint32 c= in.k.getChar();

    // keyboard commands / key combinations
    ///====================================

    /// ctrl + pgup/ down = scroll all the way to start / end
    if((in.k.key[in.Kv.lctrl] || in.k.key[in.Kv.rctrl])&& c== Kch_pgUp) {
      scrollY= 254;

    } else if((in.k.key[in.Kv.lctrl] || in.k.key[in.Kv.rctrl])&& c== Kch_pgDown) {
      scrollY= 0;

    /// copy selected text
    } else if(c== Kch_copy) {
      if(_sel1 || _sel2) {
        uint8 start= MIN(_sel1, _sel2);
        uint8 end= MAX(_sel1, _sel2);

        uint8 *t= (uint8 *)getChar8(cmd, end);
        uint8 v= *t;
        *t= 0;

        osi.setClipboard(Str::getChar8(cmd, start));
        *t= v;
      }

    /// selection cut (copy+ delete)
    } else if(c== Kch_cut) {
        
      if(_sel1 || _sel2) {
        /// copy
        uint8 start= MIN(_sel1, _sel2);
        uint8 end= MAX(_sel1, _sel2);

        uint8 *t= (uint8 *)cmd.pointUnicode(end);
        uint8 v= *t;

        *t= 0;    /// temporary string terminator
        osi.setClipboard(Str::getChar8(cmd, start));
        *t= v;    /// restore string

        __delSelection();
      }

    /// paste text
    } else if(c== Kch_paste) {
      __delSelection();

      char *text;
      if(osi.getClipboard(&text)) {
          
        //int n= strlen8(text)- 1;
        //int len= cmd.len- 1;
        cmd.insertStr(text, -1, cursorPos);
        cursorPos+= strunicodes8(text);
        _adjustVisibleXe(cursorPos, conDx);
        delete[] text;
      }

    /// backspace handling
    } else if(c== Kch_backSpace) {
      /// selection delete
      if(_sel1 || _sel2) {
        __delSelection();

      /// single character delete
      } else if(cursorPos) {
        if(cmd.nrUnicodes) {
          cmd.del(1);
          cursorPos--;
        }
      }

    } else if(c== Kch_delete) {
      /// delete selected text
      if(_sel1 || _sel2) {
        __delSelection();

      } else if(cursorPos< cmd.nrUnicodes)
        cmd.del(1, cursorPos);

    /// enter handling
    } else if(c== Kch_enter) {
        
      if(((TxtData *)lines.last)->saved== 0) {
        _saveBuffers();
      }

      TxtData *lin= (TxtData *)lines.last;
      lines.release(lin);
      lines.addFirst(lin);

      lin->lineNr= nrLinesFromStart++;
      lin->saved= 0;
      lin->txt= cmd;

      // COMMAND WAITING TO BE PROCESSED HERE <<<<<<<<<<<<<<
      // before deleting it if possible

      cursorPos= 0;
      cmd.delData();
      _adjustVisibleX0(cursorPos, conDx);
      _sel1= _sel2= 0;

    } else if(c== Kch_pgUp) {
        
      // scrollY will point to the first line outside the screen to the up. not accurate but it will do for this

      if(scrollY>= (int16)lines.nrNodes) 
        scrollY= ((int16)lines.nrNodes)- 1;
      if(scrollY< 0)
        scrollY= 0;

      // advance line by line scrollY, thru nlines and buffer lines
      TxtData *lin= (TxtData *)lines.get(scrollY);
      for(int a= 0; ;)
        if(lin->next) {
          if((a+ lin->nlines<= nrLines) || (a== 0)) {   // a== 0 asurance at least one line goes up
            a+= lin->nlines;
            scrollY++;
            lin= (TxtData *)lin->next;
          } else break;
        } else break;
        
    } else if(c== Kch_pgDown) {
      if(scrollY>= (int16)lines.nrNodes)
        scrollY= (int16)lines.nrNodes- 1;
      if(scrollY< 0)
        scrollY= 0;

      TxtData *lin= (TxtData *)lines.get(scrollY);
      for(int a= 0; ;) {
        if(lin->prev) {
          if((a+ ((TxtData *)(lin->prev))->nlines<= nrLines) || (a== 0)) {    // a== 0 asurance at least one line goes down
            lin= (TxtData *)lin->prev;
            a+= lin->nlines;
            scrollY--;
          } else break;
        } else break;
      }
                
    } else if(c== Kch_left) {
      if(cursorPos) cursorPos--;
      _adjustVisibleX0(cursorPos, conDx);
      _sel1= _sel2= 0;

    } else if(c== Kch_right) {
      if(cursorPos< cmd.nrUnicodes)
        cursorPos++;
      _adjustVisibleXe(cursorPos, conDx);
      _sel1= _sel2= 0;

    } else if(c== Kch_home) {
      cursorPos= 0;
      _adjustVisibleX0(cursorPos, conDx);
      _sel1= _sel2= 0;

    } else if(c== Kch_end) {
      cursorPos= cmd.nrUnicodes;
      _adjustVisibleXe(cursorPos, conDx);
      _sel1= _sel2= 0;

    } else if(c== Kch_up) {
      if(scrollY< (int16)lines.nrNodes- 2)
        scrollY++;

    } else if(c== Kch_down) {
      if(scrollY> 0)
        scrollY--;

    } else if(c== Kch_selLeft) {
      if(cursorPos) {
        if(_sel2 || _sel1) {
          cursorPos--;
          _sel2--;
        } else {
          _sel1= cursorPos;
          _sel2= --cursorPos;
        }
        _adjustVisibleX0(cursorPos, conDx);
      }

    } else if(c== Kch_selRight) {
      if(cursorPos< cmd.nrUnicodes) {
        if(_sel2 || _sel1) {
          cursorPos++;
          _sel2++;
        } else {
          _sel1= cursorPos;
          _sel2= ++cursorPos;
        }
        _adjustVisibleXe(cursorPos, conDx);
      }

    } else if(c== Kch_selHome) {
      if(_sel2 || _sel1)
        _sel2= 0;
      else 
        _sel1= cursorPos;
      cursorPos= 0;
      _adjustVisibleX0(cursorPos, conDx);

    } else if(c== Kch_selEnd) {
      if(!(_sel2 || _sel1))
        _sel1= cursorPos;

      _sel2= cmd.nrUnicodes;

      cursorPos= cmd.nrUnicodes;
      _adjustVisibleXe(cursorPos, conDx);

    // unicode character
    } else if (c<= Str::UNICODE_MAX) {
      //if(c< 0x20) continue;
      if(Str::isComb(c)) continue;

      __delSelection();                     /// delete selection if anything is selected

      cmd.insert(c, cursorPos);
      cursorPos++;
      _adjustVisibleXe(cursorPos, conDx);

      // <<INSERT ON>> COULD BE MADE HERE, BUT LOW PRIORITY
    }
  }

    
  // mouse commands
  static bool waitB0up= false;

  if(in.m.but[0].down) {
    if(mBoundsD(conX0, y0- (nrLines+ 1)* charDy, conDx, (nrLines+ 1)* charDy) && !waitB0up) {

      // was in.m.y- y0
      int32 y= y0- in.m.y;
      int n= y/ charDy;         // n holds wich line is pressed

      //n-= 2;      // -1 cmd line, -1 vvv scroll show
      if(n>= 0) {
        TxtData *lin= (TxtData *)lines.get(scrollY);
        TxtData *target= null;
        while(1) {
          if(lin== null) break;
          if(n< lin->nlines- 1) {
            target= lin;
            break;
          }
          n-= lin->nlines;
          lin= (TxtData *)lin->next;
        }

        // found the line clicked
        if(target) {
          cmd= target->txt;
          cursorPos= 0;
          _sel1= _sel2= 0;
          _adjustVisibleX0(0, conDx);
        }
      }
    }
    waitB0up= true;
  } else
    waitB0up= false;

  // console draw
  _draw(conX0, conY0, conDx, conDy);

  _ix->osiMutex.unlock();
  mutex.unlock();
}








// makes sure that character number c is visible, scrolling to the left (ONLY to the left) if neccesary
void ixConsole::_adjustVisibleX0(uint8 c, int editSize) {
  if(!visibleXe)
    visibleXe= ixPrint::getCharsMaxPixels(cmd, editSize, fontStyle.selFont);

  if(visibleX0> c) {
    visibleX0= c;
    visibleXe= ixPrint::getCharsMaxPixels((cchar *)getChar8(cmd, c), editSize, fontStyle.selFont)+ c;
  }
}

// makes sure that character number c is visible, scrolling to the right if neccesary
void ixConsole::_adjustVisibleXe(uint8 c, int editSize) {
  if(!visibleXe)
    visibleXe= ixPrint::getCharsMaxPixels(cmd, editSize, fontStyle.selFont);

  if(visibleXe< c) {
    visibleXe= c;

    /// keep increasing x0 until character c is visible
    visibleX0= 0;
    while(ixPrint::getCharsMaxPixels(getChar8(cmd, visibleX0), editSize, fontStyle.selFont) < (c- visibleX0))
      visibleX0++;
  }
  visibleXe= visibleX0+ ixPrint::getCharsMaxPixels((cchar *)getChar8(cmd, visibleX0), editSize, fontStyle.selFont);
}

// internal++ - deletes selected text in command line edit
void ixConsole::__delSelection() {
  if(_sel1 || _sel2) {
    uint8 start= MIN(_sel1, _sel2), end= MAX(_sel1, _sel2);
    /*
    str8 s2= getChar8(cmd, end);
    *((uint8 *)Str::getChar8((char *)cmd, start))= 0;
    str8 s1= (char *)cmd;
        
    s1+= s2;
    strcpy8((char *)cmd, s1.d);
    cursorPos= start;
    _sel1= _sel2= 0;
    */

    str8 s1; s1.insertStr(cmd, start);
    str8 s2; s2.insertStr(cmd.pointUnicode(end));
    cmd= s1+ s2;
    _sel1= _sel2= 0;
    cursorPos= start;
  }
}




void ixConsole::print(cchar *in_txt) {
  if(!in_txt) return;
  mutex.lock();

  // parse in_txt, each line delimited by \n will be another line in buffers
  cuint8 *p= (cuint8 *)in_txt;
  cuint8 *start= p;
  int nchars= 0;
  while(1) {
    if(*p== 0) break;
    nchars++;

    /// print the line if \n found
    if(*p== '\n') {
      _rawPrint((const char *)start, nchars- 1);
      start= p+ 1;
      nchars= 0;
    }
    p++;
  }

  /// usually if it's only one line it comes to this
  if(nchars)
    _rawPrint((const char *)start, nchars);

  mutex.unlock();
}




void ixConsole::_rawPrint(cchar *in_txt, int in_n) {

  TxtData *p= (TxtData *)lines.last;

  /// save buffers if last line is not saved
  if(p->saved== 0)
    _saveBuffers();

  /// move last line to first
  lines.release(p);
  lines.addFirst(p);
  p->lineNr= nrLinesFromStart++;
  p->nlines= 0;
  p->saved= 0;
  p->txt.delData();
  p->txt.insertStr(in_txt, in_n);
}




void ixConsole::printf(cchar *txt, ...) {
  /// no mutex needed here, no internal vars changed
  uint len= (uint)Str::strlen8(txt)- 1;
  if(len> 511) {
    print("Line too large to use printf(512 bites max); use \'print\'");
    return;
  }
  va_list args;
  char b[512];
  va_start(args, txt);
  vsprintf(b, txt, args);
  va_end(args);
  
  print(b);
}


void ixConsole::_clearBuffers() {
  //selText.delData();
  for(TxtData *p= (TxtData *)lines.first; p; p= (TxtData *)p->next) {
    p->txt.delData();
    p->lineNr= 0;
    p->nlines= 1;
    p->saved= 1;
  }
  cursorPos= 0;
  _sel1= _sel2= 0;
  cmd.delData();
}


void ixConsole::_saveBuffers() {
  FILE *f= fopen(IX_CONSOLE_OUTFILE, "a+b");

  for(TxtData *p= (TxtData *)lines.last; p; p= (TxtData *)p->prev)
    if(p->saved== 0) {
      fprintf(f, "%d: %s\n", p->lineNr, p->txt.d);
      p->saved= 1;
    }

  fclose(f);
}














