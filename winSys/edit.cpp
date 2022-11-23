#include "ix/ix.h"
#include "ix/winSys/_privateDef.h"

/*
TODO:
 -when a char is deleted, all it's diecriticals must be deleted!!!
 -_ixEditClickDelta put in effect, if the mouse drag happens when you don't need it

IDEEAS:
 -sound for keys? another sound for when a char cannot be typed

 -update(): A THING TO CONSIDER: ret (return value), as it is now, is true either any command is procesed or not, when osi.in is processed
  problem is, if the manip/char is not part of this, it should be put in osi again? cuz in.getManip() will erase it

 -think on the keyboard focus. an esc press would mean lost keyboard focus?

*/

using namespace Str;

static int32 _ixEditClickDelta= 7;      // (default: 7x7 box) any mouse up that moved outisde this box is considered a drag instead


void ixEdit::_setClickLimits(int32 in_delta) {
  _ixEditClickDelta= in_delta;
}



ixEdit::ixEdit(): usage(this), text(this), ixBaseWindow(&is, &usage) {
  _type= ixeWinType::edit;
  
  enterPressed= false;
}


ixEdit::~ixEdit() {
  delData();
}

void ixEdit::delData() {
  text.delData();
  enterPressed= false;
  text.cur.line= text.cur.pos= 0; text.cur.pLine= (ixTxtData::Line *)text.lines.first;
  text.sel.delData();

  ixBaseWindow::delData();
}



// funcs


// sets the editor in a special one-line, fixed buffer mode. the buffer is UTF-32 format (int32 per unicode value)
bool ixEdit::Usage::setOneLineFixed(int32 in_size) {
  if(in_size<= 0) return false;
  if(((ixEdit *)_win)->text.nrUnicodes) ((ixEdit *)_win)->delData();

  /// set the vars
  oneLine= 1;
  fixedBuffer= 1;
  limitUnicodes= in_size;
  ((ixEdit *)_win)->text._fixedBuffer= (char32 *)new uint32[limitUnicodes+ 1];
  
  // create the line in the text data
  ixTxtData::Line *p= new ixTxtData::Line;
  p->text.wrap(((ixEdit *)_win)->text._fixedBuffer, limitUnicodes+ 1);
  ((ixEdit *)_win)->text.lines.add(p);
  ((ixEdit *)_win)->text._updateWrapList();
  ((ixEdit *)_win)->text.cur.updateWline();
  ((ixEdit *)_win)->text.findTextDy();

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

}


















//  ##    ##    ######      ######        ####      ########    ########
//  ##    ##    ##    ##    ##    ##    ##    ##       ##       ##
//  ##    ##    ######      ##    ##    ##    ##       ##       ######      func
//  ##    ##    ##          ##    ##    ########       ##       ##
//    ####      ##          ######      ##    ##       ##       ########


bool ixEdit::_update(bool updateChildren) {
  bool ret= false;        /// this will return true if any action to this window happened

  if(!is.visible) return false;

  /// update it's children first
  if(updateChildren)
    if(_updateChildren())
      return true;
  
  enterPressed= false;    /// reset it from last time
  
  ret= text._update();

  if(ret)
    return ret;

  // base window update, at this point - no childrens tho, those were updated first
  return ixBaseWindow::_update(false);
}


void ixEdit::resize(float dx, float dy) {
  // identic func with ixStatic
  // any change here, must happen to the other one too
  ixBaseWindow::resize(dx, dy);
  text._computeWrapLen();
  text._updateWrapList();   // <<< _VIEW AND CUR WILL BE PLACED AT START. THAT IS NOT GOOD

}


void ixEdit::resizeDelta(float dx, float dy) {
  // identic func with ixStatic
  // any change here, must happen to the other one too

  ixBaseWindow::resizeDelta(dx, dy);
  text._computeWrapLen();
  text._updateWrapList();
}


void ixEdit::setPos(float x0, float y0, float dx, float dy) {
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
  error.makeme();
}
#endif /// IX_USE_OPENGL

#ifdef IX_USE_VULKAN
void ixEdit::_vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style) {
  ixBaseWindow::_vkDraw(in_cmd, in_ix, in_style);
  if(!_clip.exists()) return;
  if(!is.visible) return;

  /// vars init
  rectf r; _getVDviewArea(&r);
  vec3 scr;
  scr.x= (hscroll? hscroll->position: 0.0f);
  scr.y= (vscroll? vscroll->position: 0.0f);
  scr.z= 0.0f;

  text._vkDraw(in_cmd, in_ix, r, scr);

  /// scrollbars draw
  if(usage._scrollbars || usage._autoScrollbars) {
    if(hscroll) hscroll->_vkDraw(in_cmd, in_ix);
    if(vscroll) vscroll->_vkDraw(in_cmd, in_ix);
  }

  /// childrens draw
  for(ixBaseWindow *p= (ixBaseWindow *)childrens.last; p; p= (ixBaseWindow *)p->prev)
    if(p!= hscroll && p!= vscroll)
      p->_vkDraw(in_cmd, in_ix);
}
#endif
















