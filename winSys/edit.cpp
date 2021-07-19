#include "ix/ix.h"
#include "ix/winSys/_privateDef.h"

/*
WHEN A CHAR IS DELETED, ALL IT'S DIECRITICALS MUST BE DELETED!!!


TODO:

 -left->right right->left top->bottom, all should be basically done
 -bottom to top writing i don't think exists, but you never know...

IDEEAS:
 -sound for keys? another sound for when a char cannot be typed
*/

using namespace Str;


static int32 _ixEditClickDelta= 7;      // (default: 7x7 box) any mouse up that moved outisde this box is considered a drag instead
void ixEdit::_setClickLimits(int32 in_delta) {
  _ixEditClickDelta= in_delta;
}



ixEdit::ixEdit(): text(this) {
  ixBaseWindow();
  _type= ixeWinType::edit;
  
  enterPressed= false;
  
  usage._parent= this;

  //text._parent= this;
}


ixEdit::~ixEdit() {
  delData();
}

void ixEdit::delData() {
  text.delData();
  //text.font= null;
  enterPressed= false;
  text.cur.line= text.cur.pos= 0; text.cur.pLine= (ixTxtData::Line *)text.lines.first;
  text.sel.delData();

  ixBaseWindow::delData();
}



// funcs


// sets the editor in a special one-line, fixed buffer mode. the buffer is UTF-32 format (int32 per unicode value)
bool ixEdit::Usage::setOneLineFixed(int32 in_size) {
  if(in_size<= 0) return false;
  if(_parent->text.nrUnicodes) _parent->delData();

  /// set the vars
  oneLine= 1;
  fixedBuffer= 1;
  limitUnicodes= in_size;
  _parent->text._fixedBuffer= (char32 *)new uint32[limitUnicodes+ 1];
  
  // create the line in the text data
  ixTxtData::Line *p= new ixTxtData::Line;
  p->text.wrap(_parent->text._fixedBuffer, limitUnicodes+ 1);
  _parent->text.lines.add(p);
  _parent->text._updateWrapList();
  _parent->text.cur.updateWline();
  _parent->text.findTextDy();

  return true;
}


// whiteLists an unicode or a sequence of unicodes
void ixEdit::Usage::whiteList(int32 in_from, int32 in_to) {
  /// search if this sequence is already whitelisted
  _List *p= (_List *)_whiteList.first;
  for(; p; p= (_List *)p->next)
    if((p->from== in_from) && (p->to== in_to)) {
      error.console("IGNORING: unicode sequence already whitelisted [ixEdit::whiteList()]");
      return;
    }

  if(in_from< 0) { error.console("IGNORING: unicode sequence starts with negative number [ixEdit::whiteList()]"); return; }
 
  // add the unicode sequence
  p= new _List;
  p->from= in_from;
  p->to= (in_to< 0? in_from: in_to);
  _whiteList.add(p);
}

// removes the whole list if no params are passed, or searches for the unicode or unicodes to remove from the list
void ixEdit::Usage::whiteListDel(int32 in_from, int32 in_to) {
  /// delete whole list, case in_from is negative
  if(in_from< 0) {
    _whiteList.delData();
    return;
  }
  
  _List *p= (_List *)_whiteList.first;

  for(; p; p= (_List *)p->next)
    if((p->from== in_from) && (p->to== (in_to< 0? in_from: in_to))) {
      _whiteList.del(p);
      return;
    }

  error.console("IGNORING: unicode sequence to delete not found [ixEdit::whiteListDel()]");
}

// blackLists an unicode or a sequence of unicodes
void ixEdit::Usage::blackList(int32 in_from, int32 in_to) {
  /// search if this sequence is already whitelisted
  _List *p= (_List *)_blackList.first;
  for(; p; p= (_List *)p->next)
    if((p->from== in_from) && (p->to== in_to)) {
      error.console("IGNORING: unicode sequence already whitelisted [ixEdit::blackList()]");
      return;
    }

  if(in_from< 0) { error.console("IGNORING: unicode sequence starts with negative number [ixEdit::blackList()]"); return; }
 
  // add the unicode sequence
  p= new _List;
  p->from= in_from;
  p->to= (in_to< 0? in_from: in_to);
  _blackList.add(p);
}

// removes the whole list if no params are passed, or searches for the unicode or unicodes to remove from the list
void ixEdit::Usage::blackListDel(int32 in_from, int32 in_to) {
  /// delete whole list, case in_from is negative
  if(in_from< 0) {
    _blackList.delData();
    return;
  }
  
  _List *p= (_List *)_blackList.first;

  for(; p; p= (_List *)p->next)
    if((p->from== in_from) && (p->to== (in_to< 0? in_from: in_to))) {
      _blackList.del(p);
      return;
    }

  error.console("IGNORING: unicode sequence to delete not found [ixEdit::whiteListDel()]");
}



// if using the special fixed buffer, returns a pointer to it to be easily accessed
//char32 *ixEdit::Usage::getOneLineFixedBuffer() {
//  return _parent->text._fixedBuffer;
//}



// PRIVATE FUNCS
/*
void ixEdit::_paste(str32 *in_str) {
  if(!in_str) return;
  _delSelection();

  for(uint32 *p= (uint32 *)in_str->d; *p; p++) {  /// loop thru all unicodes in paste string
    
    // insert only if passes the _checkLimits() func
    if(_checkLimits(*p)) {
      text.cur.pLine->text.insert(*p, text.cur.pos);
      text.nrUnicodes++;
      ixPrint::getCharDy(text.font);

// THIS IS NOT WORKING WITH A FIXED BUFFER  

      // add a line
      if(*p== '\n') {                             /// this won't be a case when the editor is on oneline, it simply wont come up to this point
        ixLineData *p= new ixLineData;
        p->text= text.cur.pLine->text.d+ text.cur.pos+ 1; /// +1 due current \n char
        p->dx= ixPrint::getTextDx32(p->text, text.font);
        p->dy= ixPrint::getCharDy(text.font);
        text.lines.addAfter(p, text.cur.pLine);

        text.cur.pLine->text.del(text.cur.pLine->text.nrUnicodes- text.cur.pos- 1, text.cur.pos+ 1);
        text.cur.pLine->dx= ixPrint::getTextDx32(text.cur.pLine->text, text.font);
        
        text.cur.pos= 0;
        text.cur.line++;
        text.cur.pLine= p;
      }
 
      text.cur.pos++;
    } /// add a line

  } /// loop thru all unicodes in paste string

  text.cur.pLine->dx= ixPrint::getTextDx32(text.cur.pLine->text, text.font);     /// linesize in pixels
  text.findTextDx();
  text.findTextDy();
}
*/



bool ixEdit::_checkLimits(char32 unicode) {
  if(!usage.acceptCombs)  if(isComb(unicode)) return false;
  if(usage.limitUnicodes) if(text.nrUnicodes>= usage.limitUnicodes) return false;
  if(usage.oneLine)       if(unicode== '\n') return false;
  if(usage.onlyNumbers)   if(!(unicode>= '0' && unicode<= '9')) return false;
  
  if(usage._whiteList.nrNodes) {
    bool found= false;
    for(Usage::_List *p= (Usage::_List *)usage._whiteList.first; p; p= (Usage::_List *)p->next)
      if((unicode>= p->from) && (unicode <= p->to)) {
        found= true; break;
      }
    if(!found) return false;
  }

  if(usage._blackList.nrNodes)
    for(Usage::_List *p= (Usage::_List *)usage._blackList.first; p; p= (Usage::_List *)p->next)
      if((unicode>= p->from) && (unicode <= p->to))
        return false;

  return true;

  // REMOVED
  //if(usage.limitLines) if(text.lines.nrNodes>= usage.limitLines) return false;
  //if(usage.limitUnicodesPerLine) if(text.cur.pLine->text.nrUnicodes>= usage.limitUnicodesPerLine) return false;
  //if(usage.limitCharsPerLine) { if(!isComb(unicode)) if(text.cur.pLine->text.nrChars()>= usage.limitCharsPerLine) return false; }
}

/*
maybe a "check n unicodes that can be inserted"

still... when pasting happens... it's very hard not to check char by char...
well... a trimming of bad chars can happen before paste, so only insert line by line happens
*/


















//  ##    ##    ######      ######        ####      ########    ########
//  ##    ##    ##    ##    ##    ##    ##    ##       ##       ##
//  ##    ##    ######      ##    ##    ##    ##       ##       ######      func
//  ##    ##    ##          ##    ##    ########       ##       ##
//    ####      ##          ######      ##    ##       ##       ########


bool ixEdit::_update(bool in_mIn, bool updateChildren) {

  recti r; getVDcoordsRecti(&r);
  bool sendMIn= (in_mIn? r.inside(in.m.x, in.m.y): false);

  /// update it's children first
  if(updateChildren)
    if(_updateChildren(sendMIn))
      return true;

  
  // A THING TO CONSIDER: ret (return value), as it is now, is true either any command is procesed or not, when osi.in is processed
  //                      problem is, if the manip/char is not part of this, it should be put in osi again? cuz in.getManip() will erase it

  // think on the keyboard focus. an esc press would mean lost keyboard focus?

  // _view and cursor MUST WORK LIKE CLOCKWORK
  
  bool ret= false;        /// this will return true if any action to this window happened
  enterPressed= false;    /// reset it from last time
  //uint32 c;
  
  ret= text._update(sendMIn);

  // THE MOVEMENT IN txtShared.cpp OF THIS WHOLE MECHANISM IS NOT TESTED <<<<<<<<<<<<<<<<<<<<<<<
  // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  // static seems ok, if edit works too, this whole code can just go away

  /*


  // process all chars - can be unicodes, or special str manipulation codes
  if(Ix::wsys().focus== this)
  while((c= in.k.getChar())) {
    /// shortcuts
    str32 *s= &text.cur.pLine->text;  // AFTER A _delSelection() OR SIMILAR, THIS POINTER WILL BE BAD, CAREFUL TO UPDATE
    int32 *cpos= &text.cur.pos;

    // new line
    if(c== Kch_enter) {
      if(usage.oneLine) {
        enterPressed= true;

      } else {
        if(!_checkLimits('\n')) continue;


        /// if the cursor is in the middle of diacriticals, move it back, so the whole char is moved to the next line
        while(*cpos> 0) {
          if(!Str::isComb(s->d[*cpos- 1]))
            break;
          (*cpos)--;
        }
        
        s->insert('\n', *cpos);
        text.nrUnicodes++;
        int32 ipos= (*cpos)+ 1;
        

        ixTxtData::Line *p= new ixTxtData::Line;
        text.lines.addAfter(p, text.cur.pLine);
        
        /// everything after the cursor is inserted in next line
        p->text= s->d+ ipos;

        /// everything after cursor is deleted from current line
        s->del(s->nrUnicodes- ipos, ipos);

        text._updateWrapList(text.cur.pLine);
        text._updateWrapList(p);

        /// cursor update, move to next line
        text.cur.increaseUnicode();
        text.cur.makeSureInBounds();
        text.cur.makeSureVisible();
      }
      

    // backspace
    } else if(c== Kch_backSpace) {
      if(text.sel) {
        _delSelection();

      } else if(*cpos> 0) {
        /// all diacriticals will be erased, if a char is deleted, not a diacritical
        bool wipeCombs= false;
        if(isComb(s->d[*cpos- 1]))
          wipeCombs= true;

        s->del(1, (*cpos)- 1);
        text.cur.decreaseUnicode();
        text.nrUnicodes--;

        if(wipeCombs && *cpos)
          while(isComb(s->d[*cpos- 1])) {
            s->del(1, (*cpos)- 1);
            text.cur.decreaseUnicode();
            text.nrUnicodes--;
            if(!*cpos) { error.simple("ixEdit::update() - cursor position reached 0, shouldn't have"); break; }
          }

        text._updateWrapList(text.cur.pLine);
        text.cur.makeSureInBounds();
        text.cur.makeSureVisible();

      /// delete a \n
      } else if((*cpos== 0) && (text.cur.line> 0)) {
        text.cur.decreaseUnicode();
        text.cur.makeSureVisible();   /// asures _view is not on the deleted line
        ixTxtData::Line *p1= (ixTxtData::Line *)text.cur.pLine;
        ixTxtData::Line *p2= (ixTxtData::Line *)text.cur.pLine->next;
        text._delWrapForLine(p2);

        // delete '\n' if there is one
        if(p1->text.nrUnicodes)
          if(p1->text.d[p1->text.nrUnicodes- 1]== '\n') {
            //text.cur.decreaseUnicode(); first decrease unicode already does this
            p1->text.del();
            text.nrUnicodes--;
          }
        // copy line 2 to line 1's end
        p1->text+= p2->text;

        text.lines.del(p2);     /// delete the second line

        text._updateWrapList(p1);
        text.cur.makeSureInBounds();
        text.cur.makeSureVisible();
      }

    } else if(c== Kch_delete) {
      if(text.sel)
        _delSelection();

      else if(s->d[*cpos]== '\n') {
        ixTxtData::Line *p1= text.cur.pLine;
        ixTxtData::Line *p2= (ixTxtData::Line *)p1->next;

        s->del(1, *cpos);
        text.nrUnicodes--;

        if(p2) {
          text.cur.makeSureInBounds();
          text.cur.makeSureVisible();   /// asure _view is not on the deleted line
          text._delWrapForLine(p2);
          p1->text+= p2->text;
          text.lines.del(p2);
        }

        text._updateWrapList(p1);
        text.cur.makeSureInBounds();
        text.cur.makeSureVisible();

      } else if(s->d[*cpos]!= 0) {
        /// check if to wipe diacriticals. when deleting a char, all it's diacriticals are deleted
        bool wipeCombs= false;
        if(!isComb(s->d[*cpos]))
          wipeCombs= true;

        s->del(1, *cpos);
        text.nrUnicodes--;

        if(wipeCombs)
          if(*cpos> 0)
            while(isComb(s->d[(*cpos)- 1])) {
              s->del(1, (*cpos)- 1);
              text.nrUnicodes--;
              text.cur.decreaseUnicode();
              if(*cpos== 0) break;
            }

        text._updateWrapList(text.cur.pLine);
        text.cur.makeSureInBounds();
        text.cur.makeSureVisible();
      }
      
    } else if(c== Kch_cut) {
      if(text.sel) {
        str32 s;
        _copy(&s);
        osi.setClipboard(s.convert8());
        _delSelection();
      }

    } else if(c== Kch_copy) {
      if(text.sel) {
        str32 s;
        _copy(&s);
        osi.setClipboard(s.convert8());
      }

    } else if(c== Kch_paste) {
      str8 s;
      osi.getClipboard(&s.d);
      s.updateLen();
      str32 s32(s);
      _paste(&s32);
      text.cur.makeSureVisible();

    } else if(c== Kch_left) {
      text.sel.delData();
      text.cur.left();
      text.cur.makeSureVisible();

    } else if(c== Kch_right) {
      text.sel.delData();
      text.cur.right();
      text.cur.makeSureVisible();

    } else if(c== Kch_up) {
      text.sel.delData();
      text.cur.up();
      text.cur.makeSureVisible();

    } else if(c== Kch_down) {
      text.sel.delData();
      text.cur.down();
      text.cur.makeSureVisible();

    } else if(c== Kch_home) {
      text.sel.delData();
      text.cur.home();
      text.cur.makeSureVisible();

    } else if(c== Kch_end) {
      text.sel.delData();
      text.cur.end();
      text.cur.makeSureVisible();

    } else if(c== Kch_pgUp) {
      text.sel.delData();
      text.cur.pgUp();
      text.cur.makeSureVisible();

    } else if(c== Kch_pgDown) {
      text.sel.delData();
      text.cur.pgDown();
      text.cur.makeSureVisible();

    } else if(c== Kch_selLeft) {
      text.sel.addLeft();
      text.cur.makeSureVisible();

    } else if(c== Kch_selRight) {
      text.sel.addRight();
      text.cur.makeSureVisible();

    } else if(c== Kch_selUp) {
      text.sel.addUp();
      text.cur.makeSureVisible();

    } else if(c== Kch_selDown) {
      text.sel.addDown();
      text.cur.makeSureVisible();

    } else if(c== Kch_selHome) {
      text.sel.addHome();
      text.cur.makeSureVisible();

    } else if(c== Kch_selEnd) {
      text.sel.addEnd();
      text.cur.makeSureVisible();

    } else if(c== Kch_selPgUp) {
      text.sel.addPgUp();
      text.cur.makeSureVisible();

    } else if(c== Kch_selPgDown) {
      text.sel.addPgDown();
      text.cur.makeSureVisible();

    // unicode character
    } else {

      // ONELINES MUST BE FAST, A SPECIAL BRANCH SHOULD BE DONE FOR THEM

      if(text.sel) {
        _delSelection();
        s= &text.cur.pLine->text;
      }

      if(_checkLimits(c)) {
        s->insert(c, *cpos);
        text.nrUnicodes++;
      
        text._updateWrapList(text.cur.pLine);
        text.cur.increaseUnicode();
        text.cur.makeSureInBounds();
        text.cur.makeSureVisible();
      
      }
    }
    
    if(c!= 0) {
      Ix::wsys().flags.setUp((uint32)ixeWSflags::keyboardUsed);
      ret= true;
    }
  } /// process all manip chars




  // MOUSE events

  //static int32 imx, imy;        /// these will hold where the initial click happened
  //static int32 iUnicode, iLine; /// the line and unicode when the mouse was initially clicked

  // no event currently happening
  if(!Ix::wsys()._op.win && in_mIn) {
    /// a r-click event starts - can be a SELECTION DRAG or a CURSOR POS CHANGE
    if(in.m.but[0].down && r.inside(in.m.x, in.m.y)) {
      Ix::wsys()._op.win= this;
      Ix::wsys()._op.mRclick= true;
      text.sel.delData();

      Ix::wsys().bringToFront(this);
      Ix::wsys().focus= this;
      Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
      return true;
    }
  }


  // if there are problems with small fonts, and selecting while you don't want selecting,
  //   selection could happen only if mouse is being hold more than 1 second for example

  /// an event is happening with this window involved
  if(Ix::wsys()._op.win== this) {

    // R-Click event
    if(Ix::wsys()._op.mRclick) {
      static int32 lx= -1, ly= -1;

      // R-Click is being hold down - update cursor & selection
      if(in.m.but[0].down) {

        /// mouse moved while clicked
        if(in.m.x!= lx || in.m.y!= ly) {
          /// text coord y is from top to bottom, like a page of text
          //text.cur._setLineAndPosInPixels(in.m.x- (r.x0+ _viewArea.x0)+ hscroll->position, (r.y0+ _viewArea.ye)- in.m.y+ vscroll->position);
          text.cur._setLineAndPosInPixels(in.m.x- (r.x0+ _viewArea.x0)+ hscroll->position, in.m.y- (r.y0+ _viewArea.y0)+ vscroll->position);
          if(!text.sel)
            text.sel._startSelection();
          text.sel._updateEndFromCursor();

          lx= in.m.x, ly= in.m.y;
        }
        Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
        return true;

      // R-Click end - final cursor and selection positions
      } else if(!in.m.but[0].down) {     /// mouse r-button released
        
        /// text coord y is from top to bottom, like a page of text
        //text.cur._setLineAndPosInPixels(imx- (r.x0+ _viewArea.x0), (r.y0+ _viewArea.ye)- imy);
        //text.cur._setLineAndPosInPixels(in.m.x- (r.x0+ _viewArea.x0)+ hscroll->position, (r.y0+ _viewArea.ye)- in.m.y+ vscroll->position);
        text.cur._setLineAndPosInPixels(in.m.x- (r.x0+ _viewArea.x0)+ hscroll->position, in.m.y- (r.y0+ _viewArea.y0)+ vscroll->position);

        if(text.sel.start== text.sel.end && text.sel.startLine== text.sel.endLine)
          text.sel.delData();

        Ix::wsys()._op.delData();
        lx= ly= -1;
        Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
        return true;
      }
    } /// r-click event
  } /// an event happening with this window
  */


  if(ret)
    return ret;

  // base window update, at this point - no childrens tho, those were updated first
  return ixBaseWindow::_update(in_mIn, false);
}


void ixEdit::resize(int32 dx,int32 dy) {
  // identic func with ixStatic
  // any change here, must happen to the other one too
  ixBaseWindow::resize(dx, dy);
  text._computeWrapLen();
  text._updateWrapList();   // <<< _VIEW AND CUR WILL BE PLACED AT START. THAT IS NOT GOOD

}


void ixEdit::resizeDelta(int32 dx,int32 dy) {
  // identic func with ixStatic
  // any change here, must happen to the other one too

  ixBaseWindow::resizeDelta(dx, dy);
  text._computeWrapLen();
  text._updateWrapList();
}


void ixEdit::setPos(int32 x0,int32 y0,int32 dx,int32 dy) {
  // identic func with ixStatic
  // any change here, must happen to the other one too

  ixBaseWindow::setPos(x0, y0, dx, dy);
  text._computeWrapLen();
  text._updateWrapList();
}


void ixEdit::_computeChildArea() {
  // identic func with ixStatic
  // any change here, must happen to the other one too

  ixBaseWindow::_computeChildArea();

  if(_childArea.xe< text.textDx)
    _childArea.xe= text.textDx;
  if(_childArea.ye< text.textDy)
    _childArea.ye= text.textDy;
  _childArea.compDeltas();
}





















// DDDDDD      RRRRRR        AAAA      WW      WW
// DD    DD    RR    RR    AA    AA    WW      WW
// DD    DD    RRRRRR      AA    AA    WW  WW  WW     func
// DD    DD    RR    RR    AAAAAAAA    WW  WW  WW
// DDDDDD      RR    RR    AA    AA      WW  WW

#ifdef IX_USE_OPENGL
void ixEdit::_glDraw(Ix *in_ix, ixWSsubStyleBase *in_style) {
  ixBaseWindow::_glDraw(in_ix, in_style);

  /// vars init
  recti r;
  _getVDviewArea(&r);
  vec3i scr;
  scr.x= (hscroll? hscroll->position: 0);
  scr.y= (vscroll? vscroll->position: 0);
  scr.z= 0;

  text._glDraw(in_ix, r, scr);

  /// scrollbars draw
  if(usage.scrollbars || usage.autoScrollbars) {
    if(hscroll) hscroll->_glDraw(in_ix);
    if(vscroll) vscroll->_glDraw(in_ix);
  }

  /// childrens draw
  for(ixBaseWindow *p= (ixBaseWindow *)childrens.last; p; p= (ixBaseWindow *)p->prev)
    if(p!= hscroll && p!= vscroll)
      p->_glDraw(in_ix);
}
#endif /// IX_USE_OPENGL

#ifdef IX_USE_VULKAN
void ixEdit::_vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style) {
  ixBaseWindow::_vkDraw(in_cmd, in_ix, in_style);
  if(!_clip.exists()) return;
  if(!is.visible) return;

  /// vars init
  recti r;
  _getVDviewArea(&r);
  vec3i scr;
  scr.x= (hscroll? hscroll->position: 0);
  scr.y= (vscroll? vscroll->position: 0);
  scr.z= 0;

  text._vkDraw(in_cmd, in_ix, r, scr);

  /// scrollbars draw
  if(usage.scrollbars || usage.autoScrollbars) {
    if(hscroll) hscroll->_vkDraw(in_cmd, in_ix);
    if(vscroll) vscroll->_vkDraw(in_cmd, in_ix);
  }

  /// childrens draw
  for(ixBaseWindow *p= (ixBaseWindow *)childrens.last; p; p= (ixBaseWindow *)p->prev)
    if(p!= hscroll && p!= vscroll)
      p->_vkDraw(in_cmd, in_ix);
}
#endif
















