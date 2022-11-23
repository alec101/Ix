#include "ix/ix.h"
#include "ix/winSys/_privateDef.h"

using namespace ixUtil;

/*
Mouse drag:
 THIS CAN BE A MOVEMENT BIGGER THAN A CERTAIN AMOUNT OF PIXELS, SCALED FOR 720p, 1080p, 2160p >>>
 _ixEditClickDelta i already created this variable but never used it <<<<<<<<<<<<<<<<<<<<<<<<
 ATM the mouse must be pixel perfect like last update or no drag
 thinking on it, i think only very small fonts would have issues with this - NOT SURE IF REALLY NEEDED 
*/


/*
WHEN A CHAR IS DELETED, ALL IT'S DIECRITICALS MUST BE DELETED!!!

  so there's 3 methods that i can think of
  1, having the text in one str, winLines
  2, having the text in paragraphs, textData+winLines
  3, having the text in window lines, heavy winLines, cutting text non-stop between lines



  there should be a static/dynamic text window
  one would have allocs, one not
  the text chat of a multi-player window should not do any allocs, that's the thing
  but, either modifying the ixStatic
  or
  creating ixDynamic class, would solve things
  or
  even modifying somehow manually the ixStatic... alloc it's lintes and handle them, but... neah

  ISSUES:
  diacriticals - selection... do you move whole char left and right? when a char is deleted, all it's diecriticals must be deleted!!!


  
  
draw re-design
==============
  there's a startLine, start wrap line, of the drawing
  there's a pixel delta, based on scrolling
  when the line is out of the view, start wrap line advances, pixel delta is changed
  there has to be a check at start wrap line (or when there is scrolling) to know if a selection is ongoing, so full selection lines are drawn or not
  when cursor wrap line == current draw line, the cursor is drawn too
  there's no nr lines required, when the window is resized, wrap start must be re-found

drawbacks...
  re-do everything
  there is no known line numbers or anything, everything is done with advances

  the line numbering can be done, advanced, tho, with these startLine/startWrapLine... IF needed.

  maybe every line could have a count of nrWrapLines it has...
  dono if this helps for anything... i don't think it does



Other variant:
==============
there's a linePosPix - in pixels, where everything is
cursor has one, etc...
but this must be re-calculated with every wrap re-do
and this is calculated with adding the delta line height of every line to it...


with the first version tho...
even the wrap lines are not required to be computed for the whole text...
a huge optimisation is possible
but... atm i think computing it for every line is fine


GONNA GO WITH THE FASTEST METHOD TO DO, IT MIGHT STILL BE FASTER THAN ANY EDITOR ANYWAY

BUT, for the future, going with even on the go wLines, basically creating them only for the draw, might do the trick...
still... dono if that is possible tho ... the more i think the more see it's not possible

*/


using namespace Str;



///==================================================///
// TEXTDATA class *********************************** //
///==================================================///

ixTxtData::ixTxtData(ixBaseWindow *in_parent): _view(this) {
  _parent= in_parent;
  sel._parent= cur._parent= this;
  textDx= textDy= 0.0f;
  nrUnicodes= 0;

  _fixedBuffer= null;
  _wrapLen= 0;

  //font= null;

  //if(*Ix::glActiveRen())
  //  font= *(*Ix::glActiveRen())->pr.style;
  
  orientation= IX_TXT_RIGHT;
  alignment= IX_TXT_ALN_START_WRAP;
  _debug= false;

  // always a line in the editor
  Line *p= new Line;
  p->text= "";                  /// make sure there is some text, just a terminator is fine
  lines.add(p);

  _updateWrapList();            /// create the first wrap line
  _view.resetToStart();         /// _view struct start position
  cur.resetToStart();           /// cursor class start position
  computeAndSetChildArea();
}


void ixTxtData::delData() {
  //lines.delData();
  if(lines.first) {
    while(lines.first->next)
      lines.del(lines.first->next);
    ((Line *)lines.first)->text.delData();
  }
  
  _wrapLines.delData();

  if(_fixedBuffer) { delete[] _fixedBuffer; _fixedBuffer= null; }
  textDx= textDy= 0.0f;
  nrUnicodes= 0;
  
  sel.delData();
  cur.delData();
  //computeAndSetChildArea();
}


void ixTxtData::findTextDx() {
  textDx= 0.0f;
  if(orientation& IX_TXT_HORIZONTAL) {
    if(alignment& IX_TXT_ALN_FIXEDWIDTH)
      textDx= _wrapLen;
    else
      for(Wline *p= (Wline *)_wrapLines.first; p; p= (Wline *)p->next)
        if(textDx< p->dx)
          textDx= p->dx;
  } else
    for(Wline *p= (Wline *)_wrapLines.first; p; p= (Wline *)p->next)
      textDx+= p->dx;
}


void ixTxtData::findTextDy() {
  textDy= 0.0f;
  if(orientation& IX_TXT_HORIZONTAL)
    for(Wline *p= (Wline *)_wrapLines.first; p; p= (Wline *)p->next)
      textDy+= p->dy;
  else {
    if(alignment& IX_TXT_ALN_FIXEDWIDTH)
      textDy= _wrapLen;
    else
      for(Wline *p= (Wline *)_wrapLines.first; p; p= (Wline *)p->next)
        if(textDy< p->dy)
          textDy= p->dy;
  }
}


void ixTxtData::findTextDxDy() {
  /// there are extra loops for fewer instructions, in case text is very big
  textDx= textDy= 0.0f;
  // horizontal text
  if(orientation& IX_TXT_HORIZONTAL) {
    if(alignment& IX_TXT_ALN_FIXEDWIDTH) {
      textDx= _wrapLen;
      for(Wline *p= (Wline *)_wrapLines.first; p; p= (Wline *)p->next)
        textDy+= p->dy;
    } else
      for(Wline *p= (Wline *)_wrapLines.first; p; p= (Wline *)p->next) {
        if(textDx< p->dx)
          textDx= p->dx;
        textDy+= p->dy;
      }

  // vertical text
  } else {
    if(alignment& IX_TXT_ALN_FIXEDWIDTH) {
      textDx= _wrapLen;
      for(Wline *p= (Wline *)_wrapLines.first; p; p= (Wline *)p->next)
        textDx+= p->dx;
    } else
      for(Wline *p= (Wline *)_wrapLines.first; p; p= (Wline *)p->next) {
        textDx+= p->dx;
        if(textDy< p->dy)
          textDy= p->dy;
      }
  } /// text orientation
}

/*
void ixTxtData::updateAllWlinesDxDy() {
  cursor position? _view? if wlines change? but from here on down i think things must change for _view too
    this func in particular can go i think, it's not ever used?
    wline dx and dy is made in updateWlines, all of em

  if(orientation & IX_TXT_HORIZONTAL) {
    for(Wline *p= (Wline *)_wrapLines.first; p; p= (Wline *)p->next) {
      p->dy= ixPrint::getCharDy(font);
      if(p->line)
        if(p->line->text.d) {
          p->dx= ixPrint::getTextLen32(p->line->text.d+ p->startUnicode, p->nrUnicodes, font, p->spaceSize);
          continue;
        }
      p->dx= 0;
    }

  } else if(orientation & IX_TXT_VERTICAL) {
    for(Wline *p= (Wline *)_wrapLines.first; p; p= (Wline *)p->next) {
      int32 s= ixPrint::getCharDy(font);
      p->dx= s;
      p->dy= 0;
      int32 *p2= (int32 *)p->line->text.d+ p->startUnicode;
      for(int32 a= p->nrUnicodes; a> 0; a--, p2++) {
        if(Str::isComb(*p2) || (*p2== '\n') || (ixPrint::getCharDx(*p2, font)== 0))
          continue;
        p->dy+= s;
      }

    } /// for each window line
  } /// text orientation
}
*/



void ixTxtData::setFont(void *in_fnt) {
  font.selFont= in_fnt;

  computeAndSetChildArea();
  _updateWrapList();

  
  _view.moveToCursor();
  computeAndSetChildArea();
}



void ixTxtData::clearText() {
  Line *l= (Line *)lines.first;

  /// if not oneline+fixedbuffer, delete all lines but first
  if(l)
    while(l->next)
      lines.del(l->next);
  else
    l= new Line;

  l->text= "";            /// clear text in the first line

  nrUnicodes= 0;
  computeAndSetChildArea();
  _updateWrapList();
  if(_parent->hscroll) {
    _parent->hscroll->_computeDrgRect();
    _parent->hscroll->_computeScrLength();
    _parent->hscroll->setPositionMin();
  }

  if(_parent->vscroll) {
    _parent->vscroll->_computeDrgRect();
    _parent->vscroll->_computeScrLength();
    _parent->vscroll->setPositionMin();
  }

  sel.delData();
  cur.resetToStart();
  _view.resetToStart();
}



void ixTxtData::findUnicodeLineForCoords(float in_x, float in_y, int32 *out_unicode= null, int32 *out_line= null) {
  if(!lines.nrNodes) { if(out_unicode) *out_unicode= 0; if(out_line) *out_line= 0; return; }
  int32 line, unicode;
  Wline *p;

  // finding out the line
  if(orientation & IX_TXT_HORIZONTAL) {
    /// eiter start from view point (most likely) or from zero
    if(in_y>= _view.pos) line= _view.line, p= _view.pWline;
    else                 line= 0,          p= (Wline *)_wrapLines.first;

    for(float a= 0.0f; p; p= (Wline *)p->next) {
      if(in_y< a+ p->dy) break;   // found

      /// next iteration
      a+= p->dy;
      if(p->next) line++;                 // if there's not a next line, it's found also, and it's not increased anymore
      else        break;                 /// p will hold the line, so it's not needed to go thru and get the line pointer
    }

  } else {
    /// eiter start from view point (most likely) or from zero
    if(in_x>= _view.pos) line= _view.line, p= _view.pWline;
    else                 line= 0,          p= (Wline *)_wrapLines.first;

    for(float a= 0.0f; p; p= (Wline *)p->next) {
      if(in_x< a+ p->dx) break;           // found

      /// next iteration
      a+= p->dx;
      if(p->next) line++;                 // if there's not a next line, it's found also, and it's not increased anymore
      else break;                        /// p will hold the line, so it's not needed to go thru and get the line pointer
    }
  }

  // finding out the unicode nr
  if(out_unicode) {                     /// if it's smaller than 0, it's the first unicode anyway
    float coord= (orientation& IX_TXT_HORIZONTAL? in_x: in_y)- _getWlineX0orY0InPixels(p);
    unicode= font.getUnicodesMaxPixels32(p->line->text.d+ p->startUnicode, (float)coord, p->spaceSize, orientation);

    // MAAAAAYBE if the click is like more than 75% into the next char you can consider it's the next char
    /// if this works fine as it is, complicating things won't help
  }

  /// return values
  if(out_line) *out_line= line;
  if(out_unicode) *out_unicode= unicode;
}



bool ixTxtData::checkLimits(char32 unicode) {
  if(!_parent) return true;

  if(_parent->_type== ixeWinType::edit)
    return ((ixEdit *)_parent)->_checkLimits(unicode);

  if(_parent->_type== ixeWinType::staticText)
    return ((ixStaticText *)_parent)->_checkLimits(unicode);

  return true;
}


void ixTxtData::_computeWrapLen() {
  if(orientation & IX_TXT_HORIZONTAL)
    _wrapLen= _parent->_viewArea.dx;
  else if(orientation & IX_TXT_VERTICAL)
    _wrapLen= _parent->_viewArea.dy;
}



// THIS FUNC I DON'T THINK IT CAN UPDATE THE CURSOR OR THE VIEW
// they must be manually handled, case by case. it won't be called in many situations
void ixTxtData::_delWrapForLine(Line *in_line) {
  bool computeLineLen= false;
  bool checkLineLen= !((alignment& IX_TXT_ALN_WRAP) || (alignment& IX_TXT_ALN_WRAP));
  bool horiz= orientation& IX_TXT_HORIZONTAL;
  Wline *wl= in_line->wline;

  /* NOP. JUST DON'T TOUCH THE CURSOR OR THE _view. THEY MUST BE HANDLED ON A CASE-BY-CASE, WHEN THIS FUNC IS USED
  // place cursor on best possible position, outside the deleted wline
  if(cur.pLine== in_line) {
    error.simple("deleting a wline that has the cursor on!!!");
    /// try place cursor on prev line
    if(cur.pLine->prev) {
      while(cur.pLine== in_line)
        cur.decreaseLine();
      cur.pos= cur.pLine->text.nrUnicodes;
      if(horiz) cur.x0= cur._getPosInPixels();
      else      cur.y0= cur._getPosInPixels();
      
    /// try make the best of it if this is the only line
    } else {
      cur.resetToStart();
      cur.pWline= null;
    }
  } /// cursor is in deleted wlines

  // _view will have to move to cursor
  if(_view.pLine== in_line)
    _view.moveToCursor(this);
  */

  // delete all wlines pointing to this textLine


  /*
  // THIS WHILE LOOP COULD GO AWAY TBH, IF THE RULE IS, line ALWAYS POINTS TO FIRST wline IN IT
  /// delete previous line (should not exist)
  while(wl->prev)
    if(((Wline *)(wl->prev))->line== in_line) {
      Wline *p= (Wline *)wl->prev;
      /// text dimensions update
      if(horiz) {
        textDy-= p->dy;
        if(checkLineLen)
          if(p->dx== textDx)
            computeLineLen= true;

      } else {
        textDy-= p->dx;
        if(checkLineLen)
          if(p->dy== textDy)
            computeLineLen= true;
      }

      _wrapLines.del(p);
    } else
      break;
  */

  /// delete current, pointer jumps forward
  while(wl->line== in_line) {
    Wline *p= (Wline *)wl->next;

    /// adjust text size
    if(horiz) {
      textDy-= wl->dy;
      if(checkLineLen)
        if(wl->dx== textDx)
          computeLineLen= true;
    } else {
      textDx-= wl->dx;
      if(checkLineLen)
        if(wl->dy== textDy)
          computeLineLen= true;
    }

    // del the line
    _wrapLines.del(wl);

    wl= p;
    if(wl== null) break;
  }

  /// in this case there's no way but to check for the biggest line in the whole text
  if(computeLineLen) {
    if(horiz)   findTextDx();
    else        findTextDy();
  } else
    textDx= _wrapLen;

  // update child area + scrolls
  _parent->_computeChildArea();
  if(_parent->hscroll) {
    _parent->hscroll->_computeDrgRect();
    _parent->hscroll->_computeScrLength();
    _parent->hscroll->setPosition(_parent->hscroll->position); /// this will check the position is in bounds
  }

  if(_parent->vscroll) {
    _parent->vscroll->_computeDrgRect();
    _parent->vscroll->_computeScrLength();
    _parent->vscroll->setPosition(_parent->vscroll->position);
  }
}




float ixTxtData::_getWlineX0orY0InPixels(Wline *in_w) {
  if(orientation& IX_TXT_RIGHT) {
    if     (alignment& IX_TXT_ALN_START)  return 0.0f;
    else if(alignment& IX_TXT_ALN_END)    return textDx- in_w->dx;
    else if(alignment& IX_TXT_ALN_CENTER) return (textDx- in_w->dx)/ 2.0f;

  } else if(orientation& IX_TXT_LEFT) {
    if     (alignment& IX_TXT_ALN_START)  return textDx- in_w->dx;
    else if(alignment& IX_TXT_ALN_END)    return 0.0f;
    else if(alignment& IX_TXT_ALN_CENTER) return (textDx- in_w->dx)/ 2.0f;

  } else if(orientation& IX_TXT_DOWN) {
    if     (alignment& IX_TXT_ALN_START)  return 0.0f;
    else if(alignment& IX_TXT_ALN_END)    return textDy- in_w->dy;
    else if(alignment& IX_TXT_ALN_CENTER) return (textDy- in_w->dy)/ 2.0f;

  } else if(orientation& IX_TXT_UP) {
    if     (alignment& IX_TXT_ALN_START)  return textDy- in_w->dy;
    else if(alignment& IX_TXT_ALN_END)    return 0.0f;
    else if(alignment& IX_TXT_ALN_CENTER) return (textDy- in_w->dy)/ 2.0f;
  } /// text orientation
  return 0.0f;
}




struct _ixWlineWord {
  int32 nrUnicodes;
  int32 nrChars;
  char32 *start, *end;
};

/*
read on alignment
i think only on end line, spacesize could be a thing
the word wrap alignment, i think needs a total different loop, tho
without any word checks, nothing. watever unicode fits, it fits.
*/

#define IXWLINE_START wl= new Wline;\
                      if(aa)  _wrapLines.addAfter(wl, aa);\
                      else    _wrapLines.addFirst(wl);\
                      wl->line= l;\
                      /*wl->lineNr= lineNr;*/\
                      wl->startUnicode= u;\
                      wl->alignment= alignment;\
                      nrChars= 0;\
                      nrSpaces= 0;\
                      nrWords= 0;\
                      lineLen= 0.0f;

#define IXWLINE_END if(horiz) {\
                      if(wl->alignment& IX_TXT_ALN_JUSTIFIED)\
                        if((nrSpaces> 0) && (nrChars> 1) && (!newLineChar)) {\
                          if(endsInSpace) \
                            wl->spaceSize= defSpaceSize+ ((_wrapLen- (lineLen- defSpaceSize))/ ((float)(nrSpaces- 1)));\
                          else\
                            wl->spaceSize= defSpaceSize+ ((_wrapLen- lineLen)/ (float)nrSpaces);\
                        }\
                      wl->dx= lineLen;\
                      wl->dy= charDy;\
                      textDy+= charDy;\
                      if(wl->dx> textDx) textDx= wl->dx;\
                    } else {\
                      wl->dx= charDy;\
                      wl->dy= lineLen;\
                      textDx+= charDy;\
                      if(wl->dy> textDy) textDy= wl->dy;\
                    }\
                    aa= wl, newLineChar= false, endsInSpace= false;


// 'wrapLines' can skip some spaces, incorporate some spaces, so the whole text is not within these.
// 'lines' must be used for true text operations.
void ixTxtData::_updateWrapList(Line *in_l, bool in_updateCur) {
  
  bool horiz= orientation& IX_TXT_HORIZONTAL;
  bool recheckWidth= false;

  // cursor must be updated in case is on the requested line
  /// cursor is placed at the start of the line, wline will not count
  int32 curPos= cur.pos;
  if(in_updateCur)
    if((cur.pLine== in_l) && (in_l!= null))
      while(cur.pWline->prev? ((Wline *)cur.pWline->prev)->line== in_l: 0)
        cur.decreaseLine(1, false);
  
  /// _view is placed at the start of the line, at end of func, will advance where it was, or close
  float viewPos= _view.pos;
  if((_view.pLine== in_l) && in_l)
    while(_view.pWline->prev? ((Wline *)_view.pWline->prev)->line== in_l: 0)
      _view.pWline= (Wline *)_view.pWline->prev,
      _view.wline--,
      _view.pos-= (horiz? _view.pWline->dy: _view.pWline->dx);

  Wline *aa= null;
  if(in_l== null) {
    _wrapLines.delData();                   /// clear whole list
    textDx= textDy= 0;
    //recheckWidth= true; // NO NEED TO RECHECK THE TEXTDX, DUE IT'S MADE FROM EACH LINE BUILT

  } else {

    /// usually this is a newly added line, therefore it has no wrap at all
    if(!in_l->wline) {

      /// there is a previous line of text
      if(in_l->prev) {
        /// find out the previous line's last wrap line
        aa= ((Line *)in_l->prev)->wline;
        while(aa->next)
          if(((Wline *)aa->next)->line== in_l->prev)
            aa= (Wline *)aa->next;
          else
            break;

      /// no previous line of text, wlines start here
      } else 
        aa= null;

    /// this line already has a wrap, so it's modified - update to wraps need to happen
    } else {
      Wline *w= in_l->wline;
      /// move w to first wline of that line (rule should be that this is not needed)
      if(w->prev)
        while(((Wline *)w->prev)->line== in_l)
          w= (Wline *)w->prev;

      /// found aa - it's either null or pointing to the previous line
      aa= (Wline *)w->prev;

      while(w->line== in_l) {
        Wline *n= (Wline *)w->next;

        /// textDx/textDy
        if(horiz) {
          textDy-= w->dy;
          if(w->dx== textDx)        // <<<< float equal check, but this should be safe
            recheckWidth= true;
        } else {
          textDx-= w->dx;
          if(w->dy== textDy)        // <<<< float equal check, but this should be safe
            recheckWidth= true;
        }

        // line delete
        _wrapLines.del(w);
        w= n;
        if(w== null) break;
      }
    } /// newly added line / update a line that has a wrap
  } /// update all / update specific line
  
  _ixWlineWord w;                           /// tmp struct, will walk words
  
  Line *l= (in_l? in_l: (Line *)lines.first);
  if(l== null) { if(in_updateCur) cur.resetToStart(); return; }

  Wline *wl;
  uint32 *s= (uint32 *)l->text.d;
  int32 u= 0;
  //int32 lineNr= 0;
  float lineLen= 0.0f;
  int32 nrChars= 0;
  int32 nrSpaces= 0;
  int32 nrWords= 0;
  bool newLineChar= false;      /// signals that '\n' found. used for justify alignment, so the spacesize is not computed if newlinechar is present
  bool endsInSpace= false;

  float charDy= font.getCharDy();
  float defSpaceSize= horiz? font.getCharDx(' '): charDy;

  IXWLINE_START
  l->wline= wl;
  if(!s) { IXWLINE_END }

  while(s) {


    if(*s== 0) {        // end of line/whole text
      /// this is another thing that will happen eventually
      /// it can happen at start, when text is empty

      if(lineLen== 0.0f)
        lineLen= font.getTextLen32(wl->line->text.d+ wl->startUnicode, wl->nrUnicodes, wl->spaceSize);

      newLineChar= true;
      IXWLINE_END                   /// end current wline

      if(in_l) break;               // if function is asked to populate a specific line, then it's done

      /// the text continues
      if(l->next) {
        l= (Line *)l->next;
        u= 0;
        s= (uint32 *)l->text.d;
        //lineNr++;
        IXWLINE_START               /// create a new line

        l->wline= wl;

      // END OF TEXT
      } else
        break;

    } else if(*s== '\n') {    // new line, wrapping stops here
      // this ASUMES, that after each '\n' is a string terminator
      s++, u++, wl->nrUnicodes++, newLineChar= true;
     // parse the text, space by space, word by word


    

    } else {

      // ALIGNMENTS

      if(alignment& IX_TXT_ALN_JUSTIFIED || alignment& IX_TXT_ALN_WRAP) {

        //if(*s== '\t') {    // horizontal tab
          /// these have to have fixed sizes im guessing...
          /// maybe jump from point to point like in text editors? >>> i think so <<<
          // MAKEME
          //s++, u++;

        if(*s== ' ' || *s== '\t') {     // space      <- horizontal tab was fit in here until some special code is done for it
          /// sidenote: one single space char is treated like a \n when it's at the end of the line

          /*
          int32 n;
          if(horiz)
            n= defSpaceSize;
          else
            n= charDy;
            */

          // CASE 1: space fits inside the wline
          if(lineLen+ defSpaceSize<= _wrapLen) {
            lineLen+= defSpaceSize;
            u++;
            s++;
            nrChars++;
            nrSpaces++;
            wl->nrUnicodes++;

          } else {
            // CASE 2: not one char in the wline, space will forcefully fit
            if(nrChars< 1) {
              wl->nrUnicodes++;
              lineLen+= defSpaceSize;
              endsInSpace= true;

              IXWLINE_END
              u++, s++;
              IXWLINE_START

            // CASE 3: line has characters, it ends
            } else {
              //endsInSpace= 1.0f;
              IXWLINE_END          /// end current wline
              s++, u++;             // skip the space, it's at the end of the line
              IXWLINE_START        /// create a new wline

              // let it parse again, from this point, i think it's best

            } /// line has chars / don't have chars
          } /// space fits / don't fit

        } else {                  // it's a word / single char
          /// posibility 1: word don't fit, the line is empty
          /// posibility 2: word don't fit, line has at least one char in it
          /// posibility 3: word fits

          /// populating the word
          w.nrChars= 0;
          w.nrUnicodes= 0;
          w.start= (char32 *)s;
          uint32 *p= (uint32 *)s;

          if(p)
          while(*p) {
            if(isWhitespace(*p))
              break;

            if(!Str::isComb(*p))
              w.nrChars++;
            w.nrUnicodes++;

            p++;
          }
          w.end= (char32 *)p;

          /// size of the word
          float n;
          if(horiz) n= font.getTextLen32(w.start, w.nrChars);
          else      n= charDy* w.nrChars;


          // word don't fit ===========
          if(lineLen+ n> _wrapLen) {

            // CASE 1: not even one word fits in the line - the chars must be wrapped on next line
            if(!nrChars) {

              /// maximum number of unicodes and chars that will fit
              if(horiz) {
                wl->nrUnicodes= font.getUnicodesMaxPixels32(w.start, _wrapLen);
                if(wl->nrUnicodes< 1) wl->nrUnicodes= Str::getUnicodesInChar32(w.start); // at least 1 char, IT WILL EXCEED _wrapLen, THIS IS ONE OF THE SPECIAL CASES
                lineLen= font.getTextLenu32(w.start, wl->nrUnicodes);

              } else {
                nrChars= MAX((int32)(_wrapLen/ charDy), 1);
                wl->nrUnicodes= Str::getUnicodesInChars32(w.start, nrChars);
                lineLen= (float)nrChars* charDy;
              } /// text orientation

              u+= wl->nrUnicodes;
              s+= wl->nrUnicodes;
          
              IXWLINE_END          /// end current wline
              IXWLINE_START        /// create a new line


            // CASE 2: the wline has at least one char in it - start a new wline
            } else {
              endsInSpace= (nrSpaces> 1);
              IXWLINE_END          /// end current wline
              IXWLINE_START        /// create a new wline
              // let it parse again, from this point, i think it's best

            } /// not one word fits / start a new line



          // CASE 3: word fits =========
          } else {
            lineLen+= n;
            u+= w.nrUnicodes;
            s+= w.nrUnicodes;
            nrChars+= w.nrChars;
            nrWords++;
            wl->nrUnicodes+= w.nrUnicodes;
          } /// word fits / doesn't fit
        } /// it's a word


      // DIFFERENT ALINGMENTS HERE <--------------

      // no wrap/justification - the wrap will be the whole line
      } else {
        u++, s++, wl->nrUnicodes++;

      } /// different alignments
    } /// chars / spaces handling
  } /// forever loop

  // update _view position




  // update cursor's position on new wline
  if(in_updateCur)
    if((cur.pLine== in_l) && (in_l!= null)) {
      cur.pWline= cur.pLine->wline;
      cur.pos= curPos;

      /// y0, wline, pWline
      while(cur.pos> cur.pWline->startUnicode+ cur.pWline->nrUnicodes) {
        if(horiz) cur.y0+= cur.pWline->dy;
        else      cur.x0+= cur.pWline->dx;
      
        cur.pWline= (Wline *)cur.pWline->next;
        cur.wline++;
      }
      /// x0
      if(horiz) cur.x0= cur._getPosInPixels();
      else      cur.y0= cur._getPosInPixels();
    }

  // update view's position on new wline
  if((_view.pLine== in_l) && in_l) {
    _view.pWline= _view.pLine->wline;
    while(_view.pos< viewPos && (_view.pWline->next? ((Wline *)_view.pWline->next)->line== in_l: 0))
      _view.pWline= (Wline *)_view.pWline->next,
      _view.wline++,
      _view.pos+= (horiz? _view.pWline->dy: _view.pWline->dx);
  }

  // recheck text width if needed (when alignment is not just/wrap)
  if(recheckWidth) {
    if(horiz) findTextDx();
    else      findTextDy();
  }

  
  if(in_l== null) {
    cur.resetToStart();
    _view.resetToStart();
  }


  // update child area + scrolls
  _parent->_computeChildArea();
  if(_parent->hscroll) {
    _parent->hscroll->_computeDrgRect();
    _parent->hscroll->_computeScrLength();
    _parent->hscroll->setPosition(_parent->hscroll->position);
  }
  if(_parent->vscroll) {
    _parent->vscroll->_computeDrgRect();
    _parent->vscroll->_computeScrLength();
    _parent->vscroll->setPosition(_parent->vscroll->position);
  }
}


#undef IXWLINE_START
#undef IXWLINE_END 













// DDDDDD      RRRRRR        AAAA      WW      WW
// DD    DD    RR    RR    AA    AA    WW      WW
// DD    DD    RRRRRR      AA    AA    WW  WW  WW     funcs
// DD    DD    RR    RR    AAAAAAAA    WW  WW  WW
// DDDDDD      RR    RR    AA    AA      WW  WW









#ifdef IX_USE_OPENGL
void ixTxtData::Cursor::_glDraw(Ix *in_ix, const recti in_pos, const vec3i in_scroll) {
  /*
  // cursor blink, if it's in the non-drawing time, just return
  if(osi.present- _readTime> (blinkRate* 1000000))
    _readTime= osi.present,
    _show= !_show;
  if(!_show) return;
  if(!pWline) return;
  
  //ixWinSys::ixWSshader *sl= (ixWinSys::ixWSshader *)in_sl;//Ix::wsys.getShader(in_ix);
  //if(!sl) return;           // at this point, there's an error    
  //_parent->_parent->style->VBOindex


  /// tmp vars
  int32 x= in_pos.x0+ x0- in_scroll.x;
  int32 y= in_pos.ye- y0- pWline->dy+ in_scroll.y;
  int32 dx, dy;
  int32 charDy= (pWline->dy? pWline->dy: ixPrint::getCharDy(_parent->font.selFont)); <<<font.getChar

  /// cursor dimensions
  if(_parent->orientation& IX_TXT_HORIZONTAL)
    dx= drawWidth, dy= charDy;
  else
    dx= charDy,    dy= drawWidth;

  // out of bounds check
  if(x+ dx< in_pos.x0 || x> in_pos.xe || y+ dy< in_pos.y0 || y> in_pos.ye)
    return;

  // drawing
  
  glUseProgram(in_sl->gl->id);
  glUniform1ui(in_sl->u_customPos, 1);         /// enable custom quad position
  glUniform1ui(in_sl->u_useTexture, 0);
  glUniform3f(in_sl->u_origin, 0.0f, 0.0f, 0.0f);
  glUniform4fv(in_sl->u_color, 1, color.v);
  glUniform2f(in_sl->u_quadPos0, (float)x, (float)y);
  glUniform2f(in_sl->u_quadPosE, (float)(x+ dx), (float)(y+ dy));

  glDrawArrays(GL_TRIANGLE_STRIP, _parent->_parent->style->VBOindex, 4);

  glUniform1ui(in_sl->u_customPos, 0);         /// disable custom quad position
  

  //  ixDraw method
  //in_ix->draw.quad.texture= null;
  //in_ix->draw.quad.color= color;
  //in_ix->draw.quad.pos.setD((float)x, (float)y, (float)dx, (float)dy);
  //in_ix->draw.quad.render();
  */
}
#endif

#ifdef IX_USE_VULKAN
void ixTxtData::Cursor::_vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, const rectf in_pos, const vec3 in_scroll) {
  // cursor blink, if it's in the non-drawing time, just return
  if(osi.present- _readTime> (blinkRate* 1000000))
    _readTime= osi.present,
    _show= !_show;
  if(!_show) return;
  if(!pWline) return;
  
  /// tmp vars
  float x= in_pos.x0+ x0- in_scroll.x;
  float y= in_pos.y0+ y0- in_scroll.y;
  float dx, dy;
  float charDy= (pWline->dy? pWline->dy: _parent->font.getCharDy());

  /// cursor dimensions
  if(_parent->orientation& IX_TXT_HORIZONTAL)
    dx= drawWidth, dy= charDy;
  else
    dx= charDy,    dy= drawWidth;

  // out of bounds check
  if(x+ dx< in_pos.x0 || x> in_pos.xe || y+ dy< in_pos.y0 || y> in_pos.ye)
    return;

  // drawing
  in_ix->vki.draw.quad.flagTexture(false);

  in_ix->vki.draw.quad.push.color= color;
  in_ix->vki.draw.quad.setPosD(x, y, 0.0f, dx, dy);
  in_ix->vki.draw.quad.cmdPushAll(in_cmd);
  in_ix->vki.draw.quad.cmdDraw(in_cmd);
}
#endif


#ifdef IX_USE_OPENGL
void ixTxtData::Sel::_glDraw(Ix *in_ix, const recti in_pos, const vec3i in_scroll) {
  /*
  if(!this->operator bool()) return;

  //ixWinSys::ixWSshader *sl= (ixWinSys::ixWSshader *)in_sl;
  //if(!sl) return;

  int32 x0= in_pos.x0;
  int32 y0= in_pos.ye;
  int32 dx, dy;
  int32 charDy= ixPrint::getCharDy(_parent->font.selFont); fontStyle func,w/scale

  int32 sx0= getStartX0(), ex0= getEndX0(), sy0= getStartY0(), ey0= getEndY0();
  int32 s, e, lineNr;
  int32 boundS, boundE;
  bool inBounds;

  ixTxtData::Wline *w1, *w2, *p;

  /// selection inits
  glUseProgram(in_sl->gl->id);
  glUniform1ui(in_sl->u_customPos, 1);         /// enable custom quad position
  glUniform1ui(in_sl->u_useTexture, 0);
  glUniform4fv(in_sl->u_color, 1, color.v);
  glUniform3f(in_sl->u_origin, 0.0f, 0.0f, 0.0f);

  //glUniform2f(sl->u_quadPos0, (float)x, (float)y);
  //glUniform2f(sl->u_quadPosE, (float)(x+ dx), (float)(y+ dy));
  

  //in_ix->draw.quad.texture= null;
  //in_ix->draw.quad.color= color;

  w1= getStartWlinePointer();   /// starting window line
  w2= getEndWlinePointer();     /// ending window point
  s= getStart();
  e= getEnd();
  lineNr= getStartWline();
  p= w1;
  
  // start line (%possibly only line)
  if(_parent->orientation& IX_TXT_HORIZONTAL) {
    //int n= p->nrUnicodes- (s- p->startUnicode); // nr of unicodes the first
    x0+= sx0- in_scroll.x;
    y0-= sy0+ w1->dy- in_scroll.y;
    if(w1== w2) dx= ex0- sx0;
    else        dx= in_pos.dx- sx0+ in_scroll.x;;//ixPrint::getTextLenu32(p->line->text.d+ s, p->nrUnicodes- (s- p->startUnicode), _parent->font, p->spaceSize);

    dy= p->dy;

    boundS= in_pos.y0- charDy;
    boundE= in_pos.ye;
    inBounds= (y0>= boundS && y0<= boundE);

  } else {
    x0+= sx0;
    y0-= sy0;
    dx= p->dx;
    if(w1== w2) dy= ey0- sy0;
    else {
      uint32 *txt= (uint32 *)p->line->text.d+ p->startUnicode;
      dy= 0;
      for(int32 a= s; a< p->nrUnicodes; a++, txt++) 
        if(!Str::isComb(*txt))
          dy+= charDy;
    }

    boundS= in_pos.x0- charDy;
    boundE= in_pos.xe;
    inBounds= (x0>= boundS && x0<= boundE);
  }

  /// draw first line (maybe only one)
  if(inBounds) {
    glUniform2f(in_sl->u_quadPos0, (float)x0, (float)y0);
    glUniform2f(in_sl->u_quadPosE, (float)(x0+ dx), (float)(y0+ dy));
    glDrawArrays(GL_TRIANGLE_STRIP, _parent->_parent->style->VBOindex, 4);

    //in_ix->draw.quad.pos.setD((float)(x0), (float)(y0), (float)dx, (float)dy);
    //in_ix->draw.quad.render();
  }

  if(w1== w2) { glUniform1ui(in_sl->u_customPos, 1); return; } // one line? that's all there is to draw

  // draw in-between start and end lines
  if(_parent->orientation& IX_TXT_HORIZONTAL) {
    x0= in_pos.x0;
    dx= in_pos.dx;

    for(p= (ixTxtData::Wline *)w1->next; p!= w2; p= (ixTxtData::Wline *)p->next) {
      y0-= p->dy;
      if(y0>= boundS && y0<= boundE) {
        glUniform2f(in_sl->u_quadPos0, (float)x0, (float)y0);
        glUniform2f(in_sl->u_quadPosE, (float)(x0+ dx), (float)(y0+ p->dy));
        glDrawArrays(GL_TRIANGLE_STRIP, _parent->_parent->style->VBOindex, 4);
        //in_ix->draw.quad.pos.setD((float)x0, (float)y0, (float)dx, (float)p->dy);
        //in_ix->draw.quad.render();
      }
    }

  } else if(_parent->orientation& IX_TXT_VERTICAL) {
    for(p= (ixTxtData::Wline *)w1->next; p!= w2; p= (ixTxtData::Wline *)p->next) {
      x0+= p->dx;
      if(x0>= boundS && x0<= boundE) {
        glUniform2f(in_sl->u_quadPos0, (float)x0, (float)in_pos.y0);
        glUniform2f(in_sl->u_quadPosE, (float)(x0+ p->dx), (float)(in_pos.y0+ in_pos.dy));
        glDrawArrays(GL_TRIANGLE_STRIP, _parent->_parent->style->VBOindex, 4);
        //in_ix->draw.quad.pos.setD((float)x0, (float)in_pos.y0, (float)p->dx, (float)in_pos.dy);
        //in_ix->draw.quad.render();
      }
    }
  }

  // draw end line
  if(_parent->orientation& IX_TXT_HORIZONTAL) {
    x0= in_pos.x0- in_scroll.x;
    y0-= p->dy;
    dx= ex0;
    dy= p->dy;
    inBounds= (y0>= boundS && y0<= boundE);

  } else {
    x0+= p->dx;
    y0= in_pos.y0- in_scroll.y;
    dx= p->dx;
    dy= in_pos.ye- ey0;
    inBounds= (x0>= boundS && x0<= boundE);
  }

  if(inBounds) {
    glUniform2f(in_sl->u_quadPos0, (float)x0, (float)y0);
    glUniform2f(in_sl->u_quadPosE, (float)(x0+ dx), (float)(y0+ dy));
    glDrawArrays(GL_TRIANGLE_STRIP, _parent->_parent->style->VBOindex, 4);
    //in_ix->draw.quad.pos.setD((float)x0, (float)y0, (float)dx, (float)dy);
    //in_ix->draw.quad.render();
  }
  glUniform1ui(in_sl->u_customPos, 1);
  */
}
#endif /// IX_USE_OPENGL

#ifdef IX_USE_VULKAN
void ixTxtData::Sel::_vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, const rectf in_pos, const vec3 in_scroll) {
  
  if(!this->operator bool()) return;

  float x0= in_pos.x0;
  float y0= in_pos.y0;
  float dx, dy;
  float charDy= _parent->font.getCharDy();

  float sx0= getStartX0(), ex0= getEndX0(), sy0= getStartY0(), ey0= getEndY0();
  int32 s, e, lineNr;
  float boundS, boundE;
  bool inBounds;

  ixTxtData::Wline *w1, *w2, *p;



  w1= getStartWlinePointer();   /// starting window line
  w2= getEndWlinePointer();     /// ending window point
  s= getStart();
  e= getEnd();
  lineNr= getStartWline();
  p= w1;
  
  // start line (%possibly only line)
  if(_parent->orientation& IX_TXT_HORIZONTAL) {   // horizontal
    //int n= p->nrUnicodes- (s- p->startUnicode); // nr of unicodes the first
    x0+= sx0- in_scroll.x;
    //y0-= sy0+ w1->dy- in_scroll.y; // bottom origin
    y0+= sy0- in_scroll.y;
    if(w1== w2) dx= ex0- sx0;
    else        dx= in_pos.dx- sx0+ in_scroll.x; //_parent->font.getTextLenu32(p->line->text.d+ s, p->nrUnicodes- (s- p->startUnicode), p->spaceSize);

    dy= p->dy;

    //boundS= in_pos.y0- charDy;
    //boundE= in_pos.ye;
    boundS= in_pos.y0- charDy;
    boundE= in_pos.ye;

    inBounds= (y0>= boundS && y0<= boundE);

  } else {                                        // vertical
    x0+= sx0;
    y0+= sy0;
    dx= p->dx;
    if(w1== w2) dy= ey0- sy0;
    else {
      uint32 *txt= (uint32 *)p->line->text.d+ p->startUnicode;
      dy= 0.0f;
      for(int32 a= s; a< p->nrUnicodes; a++, txt++) 
        if(!Str::isComb(*txt))
          dy+= charDy;
    }

    boundS= in_pos.x0- charDy;
    boundE= in_pos.xe;
    inBounds= (x0>= boundS && x0<= boundE);
  }

  /// starting quad vals
  in_ix->vki.draw.quad.push.color= color;
  in_ix->vki.draw.quad.flagTexture(false);
  in_ix->vki.draw.quad.cmdPushColor(in_cmd);
  in_ix->vki.draw.quad.cmdPushFlags(in_cmd);

  /// draw first line (maybe only one)
  if(inBounds) {
    in_ix->vki.draw.quad.setPosD(x0, y0, 0.0f, dx, dy);
    in_ix->vki.draw.quad.cmdPushPos(in_cmd);
    in_ix->vki.draw.quad.cmdDraw(in_cmd);
  }

  if(w1== w2) return; // one line? that's all there is to draw

  // draw in-between start and end lines
  if(_parent->orientation& IX_TXT_HORIZONTAL) {
    x0= in_pos.x0;
    dx= in_pos.dx;

    for(p= (ixTxtData::Wline *)w1->next; p!= w2; p= (ixTxtData::Wline *)p->next) {
      y0+= p->dy;
      if(y0>= boundS && y0<= boundE) {
        in_ix->vki.draw.quad.setPosD(x0, y0, 0.0f, dx, p->dy);
        in_ix->vki.draw.quad.cmdPushPos(in_cmd);
        in_ix->vki.draw.quad.cmdDraw(in_cmd);
      }
    }

  } else if(_parent->orientation& IX_TXT_VERTICAL) {
    for(p= (ixTxtData::Wline *)w1->next; p!= w2; p= (ixTxtData::Wline *)p->next) {
      x0+= p->dx;
      if(x0>= boundS && x0<= boundE) {
        in_ix->vki.draw.quad.setPosD(x0, in_pos.y0, 0.0f, p->dx, in_pos.dy);
        in_ix->vki.draw.quad.cmdPushPos(in_cmd);
        in_ix->vki.draw.quad.cmdDraw(in_cmd);
      }
    }
  }

  // draw end line
  if(_parent->orientation& IX_TXT_HORIZONTAL) {
    x0= in_pos.x0- in_scroll.x;
    y0+= p->dy;
    dx= ex0;
    dy= p->dy;
    inBounds= (y0>= boundS && y0<= boundE);

  } else {
    x0+= p->dx;
    y0= in_pos.y0- in_scroll.y;
    dx= p->dx;
    dy= in_pos.ye- ey0;
    inBounds= (x0>= boundS && x0<= boundE);
  }

  if(inBounds) {
    in_ix->vki.draw.quad.setPosD(x0, y0, 0.0f, dx, dy);
    in_ix->vki.draw.quad.cmdPushPos(in_cmd);
    in_ix->vki.draw.quad.cmdDraw(in_cmd);
  }
}
#endif /// IX_USE_VULKAN






















inline void _debugIxTxtDataDraw1(VkCommandBuffer in_cmd, Ix *in_ix, ixTxtData::Wline *p, float x, float y, int32 lineNr, bool horiz, rectf *in_clip) {
  char buf[256];          // DEBUG print buffer
  ixFontStyle *save= in_ix->pr.style;
  in_ix->pr.style= &in_ix->debugStyle;

  in_ix->vki.cmdScissorDefault(in_cmd);

  sprintf(buf, "spc(%.1f) s(%d) n(%d) wLine(%d)", p->spaceSize, p->startUnicode, p->nrUnicodes, lineNr);

  if(horiz) {
    in_ix->pr.style->setOrientation(IX_TXT_RIGHT);
    in_ix->pr.txt2f(x- in_ix->pr.style->getTextLenu(buf), y, buf);
  } else {
    in_ix->pr.style->setOrientation(IX_TXT_DOWN);
    in_ix->pr.txt2f(x, y, buf);
  }

  in_ix->vki.cmdScissor(in_cmd, in_clip);
  in_ix->pr.style= save;
}


#ifdef IX_USE_OPENGL
void ixTxtData::_glDraw(Ix *in_ix, const recti in_pos, const vec3i in_scroll) {
  /*
  //ixWinSys::ixWSshader *sl= Ix::wsys.getShader(in_ix);
  //if(!sl) return;

  

  int32 charDy= ixPrint::getCharDy(font.selFont);
  // int32 boundS,
  int32 boundE;
  int32 x, y;
  //char buf[256];  // DEBUG print buffer
  

  // NO CLIPPING UNTIL I AM SURE THINGS ARE DRAWN OK
  // -----------------------------------------------
  //glUseProgram(sl->id);
  //Ix::wsys._addClipPlaneR(sl, &in_pos);
  //in_ix->pr.addClipPlaneR(&in_pos);

  in_ix->pr.setClipPlaneR(&_parent->_clip);
  in_ix->pr.setFont(font.selFont);
  glUseProgram(in_sl->gl->id);
  in_sl->setClipPlaneR(_parent->_clip);

 
  sel._draw(in_sl, in_pos, in_scroll);
  
  
  //LEFT AND RiGHT ORIENTATIONS MIGHT BE IDENTICAL, SAME WITH UP AND DOWN, BUT MUST FIRST DO THEM

  int32 lineNr= 0;
  
  // DEBUG
  if(_debug) {
    char buf[256];  // DEBUG print buffer
    int cy= ixPrint::getCharDy(in_ix->fnt5x6)+ 1;
    ixFontStyle *saveStyle= in_ix->pr.style;
    in_ix->pr.style= &in_ix->debugStyle;
    in_ix->pr.delClipPlane();

    uint32 unicode= 0xFDFDFDFD;
    if(cur.pWline)
      if(cur.pWline->line)
        if(cur.pWline->line->text.d)
          if(cur.pos< cur.pWline->line->text.nrUnicodes)
            unicode= cur.pWline->line->text.d[cur.pos];

    sprintf(buf, "wlines:%d lines:%d cur[%d,%d,%d %dx %dy unicode[%d] %p,%p]", _wrapLines.nrNodes, lines.nrNodes, cur.pos, cur.wline, cur.line, cur.x0, cur.y0, unicode, cur.pWline, cur.pLine);
    in_ix->pr.txt2i(in_pos.x0, in_pos.ye+ (cy* 0), buf);
    sprintf(buf, "_view: %d,%d,%d %p,%p", _view.pos, _view.wline, _view.line, _view.pWline, _view.pLine);
    in_ix->pr.txt2i(in_pos.x0, in_pos.ye+ (cy* 1), buf);
    sprintf(buf, "scroll: vert[%d/%d] horiz[%d/%d]", _parent->vscroll->position, _parent->vscroll->_scroll, _parent->hscroll->position, _parent->hscroll->_scroll);
    in_ix->pr.txt2i(in_pos.x0, in_pos.ye+ (cy* 2), buf);

    in_ix->pr.restoreClipping();
    in_ix->pr.style= saveStyle;
  } /// DEBUG

  in_ix->pr.setOrientation(orientation);


  if(orientation== IX_TXT_RIGHT) {
    x= in_pos.x0- in_scroll.x;
    y= in_pos.ye- charDy- _view.pos+ in_scroll.y;
    //boundS= in_pos.y0- in_scroll.y- charDy; // y+ charDy>= boundS
    boundE= in_pos.y0;
    lineNr= _view.wline;

    if(alignment& IX_TXT_ALN_START) {
      for(Wline *p= _view.pWline; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(y+ p->dy< boundE) break;                    /// out of drawing bounds check
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2i(x, y, p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_ix, p, x+ in_scroll.x, y, lineNr++, true);  // DEBUG
        y-= p->dy;
      }

    } else if(alignment& IX_TXT_ALN_END) {
      x+= textDx;
      for(Wline *p= _view.pWline; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(y+ p->dy< boundE) break;                    /// out of drawing bounds check
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2i(x- p->dx, y, p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_ix, p, x, y, lineNr++, true);  // DEBUG
        y-= p->dy;
      }
      
    } else if(alignment& IX_TXT_ALN_CENTER) {
      // x+= textDx/ 2; there's a int divide precision loss, it is doubled if using this shortcut
      for(Wline *p= _view.pWline; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(y+ p->dy< boundE) break;                    /// out of drawing bounds check
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2i(x+ ((textDx- p->dx)/ 2), y, p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_ix, p, x, y, lineNr++, true);  // DEBUG
        y-= p->dy;
      }
    }

  } else if(orientation== IX_TXT_LEFT) {
    x= in_pos.x0+ textDx;
    y= in_pos.ye- charDy- _view.pos;
    boundE= in_pos.y0;

    if(alignment& IX_TXT_ALN_START) {
      for(Wline *p= _view.pWline; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(y+ p->dy< boundE) break;    /// out of drawing bounds check
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2i(x, y, p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_ix, p, x, y, lineNr++, true);  // DEBUG
        y-= p->dy;
      }

    } else if(alignment& IX_TXT_ALN_END) {
      x-= textDx;
      for(Wline *p= _view.pWline; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(y+ p->dy< boundE) break;    /// out of drawing bounds check
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2i(x+ p->dx, y, p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_ix, p, x, y, lineNr++, true);  // DEBUG
        y-= p->dy;
      }

    } else if(alignment& IX_TXT_ALN_CENTER) {
      for(Wline *p= _view.pWline; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(y+ p->dy< boundE) break;    /// out of drawing bounds check
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2i(x- ((textDx- p->dx)/ 2), y, p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_ix, p, x, y, lineNr++, true);  // DEBUG
        y-= p->dy;
      }
    }


  } else if(orientation== IX_TXT_DOWN) {
    x= in_pos.x0+ in_scroll.x;
    y= in_pos.ye+ in_scroll.y;
    //boundS= in_pos.x0- in_scroll.x- charDy;
    boundE= in_pos.xe;

    if(alignment& IX_TXT_ALN_START) {
      for(Wline *p= (Wline *)_wrapLines.first; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(x- p->dx> boundE) break;
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2i(x, y, p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_ix, p, x, y, lineNr++, false); // DEBUG
        x+= p->dx;
      }

    } else if(alignment& IX_TXT_ALN_END) {
      y-= textDy;
      for(Wline *p= (Wline *)_wrapLines.first; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(x- p->dx> boundE) break;
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2i(x, y+ p->dy, p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_ix, p, x, y, lineNr++, false); // DEBUG
        x+= p->dx;
      }

    } else if(alignment& IX_TXT_ALN_CENTER) {
      for(Wline *p= (Wline *)_wrapLines.first; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(x- p->dx> boundE) break;
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2i(x, y- ((textDy- p->dy)/ 2), p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_ix, p, x, y, lineNr++, false); // DEBUG
        x+= p->dx;
      }
    }

  } else if(orientation== IX_TXT_UP) {
    x= in_pos.x0+ in_scroll.x;
    y= in_pos.y0+ in_scroll.y;
    //boundS= in_pos.x0- in_scroll.x- charDy;
    boundE= in_pos.xe- in_scroll.x;

    if(alignment& IX_TXT_ALN_START) {

      for(Wline *p= (Wline *)_wrapLines.first; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(x- p->dx> boundE) break;
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2i(x, y, p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_ix, p, x, y, lineNr++, false); // DEBUG
        x+= p->dx;
      }

    } else if(alignment& IX_TXT_ALN_END) {
      y+= textDy;
      for(Wline *p= (Wline *)_wrapLines.first; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(x- p->dx> boundE) break;
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2i(x, y- p->dy, p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_ix, p, x, y, lineNr++, false); // DEBUG
        x+= p->dx;
      }

    } else if(alignment& IX_TXT_ALN_CENTER) {
      for(Wline *p= (Wline *)_wrapLines.first; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(x- p->dx> boundE) break;
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2i(x, y+ ((textDy- p->dy)/ 2), p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_ix, p, x, y, lineNr++, false); // DEBUG
        x+= p->dx;
      }
    } /// alignment
  } /// orientation

  if(Ix::wsys.focus== _parent)
    cur._draw(in_sl, in_pos, in_scroll);


  //in_ix->pr.restoreStyle();
  //in_ix->pr.delClipPlane();
  */
}
#endif /// IX_USE_OPENGL



#ifdef IX_USE_VULKAN
//#define IX_TXTDATA_DEBUG(a, b, c, d, e, f) if(_debug) { in_ix->vki.cmdScissorDefault(in_cmd); _debugIxTxtDataDraw1(a, b, c, d, e, f); in_ix->vki.cmdScissor(in_cmd, &_parent->_clip); }


void ixTxtData::_vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, const rectf in_pos, const vec3 in_scroll) {
  in_ix->pr.style= &font;
  float charDy= font.getCharDy();
  float boundE;
  float x, y;
  ixTexture *t= in_ix->vki.noTexture;
  
  in_ix->vk.CmdBindPipeline(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipeline);
  in_ix->vki.cmdScissor(in_cmd, &_parent->_clip);
  in_ix->vki.draw.quad.cmdTexture(in_cmd, t);

  sel._vkDraw(in_cmd, in_ix, in_pos, in_scroll);
  
  //LEFT AND RiGHT ORIENTATIONS MIGHT BE IDENTICAL, SAME WITH UP AND DOWN, BUT MUST FIRST DO THEM

  int32 lineNr= 0;
  
  // DEBUG
  if(_debug) {
    char buf[256];  // DEBUG print buffer
    float cy= in_ix->debugStyle.getCharDy()+ 1.0f;
    ixFontStyle *saveStyle= in_ix->pr.style;
    in_ix->vki.cmdScissorDefault(in_cmd);
    in_ix->pr.style= &in_ix->debugStyle;

    uint32 unicode= 0xFDFDFDFD;
    if(cur.pWline)
      if(cur.pWline->line)
        if(cur.pWline->line->text.d)
          if(cur.pos< cur.pWline->line->text.nrUnicodes)
            unicode= cur.pWline->line->text.d[cur.pos];

    sprintf(buf, "wlines:%d lines:%d cur[%d,%d,%d %.1fx %.1fy unicode[%d] %p,%p]", _wrapLines.nrNodes, lines.nrNodes, cur.pos, cur.wline, cur.line, cur.x0, cur.y0, unicode, cur.pWline, cur.pLine);
    in_ix->pr.txt2f(in_pos.x0, in_pos.y0- (cy* 1.0f), buf);
    sprintf(buf, "_view: %.1f,%d,%d %p,%p", _view.pos, _view.wline, _view.line, _view.pWline, _view.pLine);
    in_ix->pr.txt2f(in_pos.x0, in_pos.y0- (cy* 2.0f), buf);
    sprintf(buf, "scroll: vert[%.1f/%.1f] horiz[%.1f/%.1f]", _parent->vscroll->position, _parent->vscroll->_scroll, _parent->hscroll->position, _parent->hscroll->_scroll);
    in_ix->pr.txt2f(in_pos.x0, in_pos.y0- (cy* 3.0f), buf);

    in_ix->pr.style= saveStyle;
    in_ix->vki.cmdScissor(in_cmd, &_parent->_clip);
  } /// DEBUG

  in_ix->pr.style->setOrientation(orientation);

  //in_ix->vki.cmdScissorDefault(in_cmd);     // <<< why here??? DEBUG???<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


  
  if(orientation== IX_TXT_RIGHT) {
    x= in_pos.x0- in_scroll.x;
    //y= in_pos.y0+ _view.pos+ in_scroll.y;
    y= in_pos.y0- in_scroll.y+ _view.pos;
    //boundS= in_pos.y0- in_scroll.y- charDy; // y+ charDy>= boundS
    boundE= in_pos.ye;
    lineNr= _view.wline;

    if(alignment& IX_TXT_ALN_START) {
      for(Wline *p= _view.pWline; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(y> boundE) break;                    /// out of drawing bounds check
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2f(x, y, p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_cmd, in_ix, p, x+ in_scroll.x, y, lineNr++, true, &_parent->_clip);  // DEBUG
        y+= p->dy;
      }

    } else if(alignment& IX_TXT_ALN_END) {
      x+= textDx;
      for(Wline *p= _view.pWline; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(y+ p->dy< boundE) break;                    /// out of drawing bounds check
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2f(x- p->dx, y, p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_cmd, in_ix, p, x, y, lineNr++, true, &_parent->_clip);  // DEBUG
        y+= p->dy;
      }
      
    } else if(alignment& IX_TXT_ALN_CENTER) {
      // x+= textDx/ 2; there's a int divide precision loss, it is doubled if using this shortcut
      for(Wline *p= _view.pWline; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(y+ p->dy< boundE) break;                    /// out of drawing bounds check
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2f(x+ ((textDx- p->dx)/ 2), y, p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);

        if(_debug) _debugIxTxtDataDraw1(in_cmd, in_ix, p, x, y, lineNr++, true, &_parent->_clip);  // DEBUG
        y+= p->dy;
      }
    }

  } else if(orientation== IX_TXT_LEFT) {
    x= in_pos.x0+ textDx;
    y= in_pos.ye- charDy- _view.pos;
    boundE= in_pos.y0;

    if(alignment& IX_TXT_ALN_START) {
      for(Wline *p= _view.pWline; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(y+ p->dy< boundE) break;    /// out of drawing bounds check
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2f(x, y, p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_cmd, in_ix, p, x, y, lineNr++, true, &_parent->_clip);  // DEBUG
        y-= p->dy;
      }

    } else if(alignment& IX_TXT_ALN_END) {
      x-= textDx;
      for(Wline *p= _view.pWline; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(y+ p->dy< boundE) break;    /// out of drawing bounds check
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2f(x+ p->dx, y, p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_cmd, in_ix, p, x, y, lineNr++, true, &_parent->_clip);  // DEBUG
        y-= p->dy;
      }

    } else if(alignment& IX_TXT_ALN_CENTER) {
      for(Wline *p= _view.pWline; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(y+ p->dy< boundE) break;    /// out of drawing bounds check
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2f(x- ((textDx- p->dx)/ 2), y, p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_cmd, in_ix, p, x, y, lineNr++, true, &_parent->_clip);  // DEBUG
        y-= p->dy;
      }
    }


  } else if(orientation== IX_TXT_DOWN) {
    x= in_pos.x0+ in_scroll.x;
    y= in_pos.ye+ in_scroll.y;
    //boundS= in_pos.x0- in_scroll.x- charDy;
    boundE= in_pos.xe;

    if(alignment& IX_TXT_ALN_START) {
      for(Wline *p= (Wline *)_wrapLines.first; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(x- p->dx> boundE) break;
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2f(x, y, p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_cmd, in_ix, p, x, y, lineNr++, false, &_parent->_clip); // DEBUG
        x+= p->dx;
      }

    } else if(alignment& IX_TXT_ALN_END) {
      y-= textDy;
      for(Wline *p= (Wline *)_wrapLines.first; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(x- p->dx> boundE) break;
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2f(x, y+ p->dy, p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_cmd, in_ix, p, x, y, lineNr++, false, &_parent->_clip); // DEBUG
        x+= p->dx;
      }

    } else if(alignment& IX_TXT_ALN_CENTER) {
      for(Wline *p= (Wline *)_wrapLines.first; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(x- p->dx> boundE) break;
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2f(x, y- ((textDy- p->dy)/ 2), p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_cmd, in_ix, p, x, y, lineNr++, false, &_parent->_clip); // DEBUG
        x+= p->dx;
      }
    }

  } else if(orientation== IX_TXT_UP) {
    x= in_pos.x0+ in_scroll.x;
    y= in_pos.y0+ in_scroll.y;
    //boundS= in_pos.x0- in_scroll.x- charDy;
    boundE= in_pos.xe- in_scroll.x;

    if(alignment& IX_TXT_ALN_START) {

      for(Wline *p= (Wline *)_wrapLines.first; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(x- p->dx> boundE) break;
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2f(x, y, p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_cmd, in_ix, p, x, y, lineNr++, false, &_parent->_clip); // DEBUG
        x+= p->dx;
      }

    } else if(alignment& IX_TXT_ALN_END) {
      y+= textDy;
      for(Wline *p= (Wline *)_wrapLines.first; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(x- p->dx> boundE) break;
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2f(x, y- p->dy, p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_cmd, in_ix, p, x, y, lineNr++, false, &_parent->_clip); // DEBUG
        x+= p->dx;
      }

    } else if(alignment& IX_TXT_ALN_CENTER) {
      for(Wline *p= (Wline *)_wrapLines.first; p; p= (Wline *)p->next) {    /// for each wrapLine
        if(x- p->dx> boundE) break;
        in_ix->pr.style->spaceSize= p->spaceSize;
        in_ix->pr.txt32_2f(x, y+ ((textDy- p->dy)/ 2), p->line->text.d, p->startUnicode, p->startUnicode+ p->nrUnicodes);
        if(_debug) _debugIxTxtDataDraw1(in_cmd, in_ix, p, x, y, lineNr++, false, &_parent->_clip); // DEBUG
        x+= p->dx;
      }
    } /// alignment
  } /// orientation

  if(Ix::wsys().focus== _parent) {
    in_ix->vk.CmdBindPipeline(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipeline);
    in_ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipelineLayout, 0, 1, &in_ix->vki.glb[in_ix->vki.fi]->set->set, 0, 0);
    in_ix->vki.draw.quad.cmdTexture(in_cmd, t);
    cur._vkDraw(in_cmd, in_ix, in_pos, in_scroll);
  }
}
#endif /// IX_USE_VULKAN



// handles keyboard input, text selection, depending if _parent has flags that allow any of such operations
bool ixTxtData::_update() {
  bool ret= false;
  uint32 c;
  bool selPossible, cursorUsable, readOnly;

  if(_parent->_type== ixeWinType::staticText) {
    readOnly= true;
    cursorUsable=      ((ixStaticText *)_parent)->usage.hasCursor;
    selPossible= ((ixStaticText *)_parent)->usage.selection;
  } else if(_parent->_type== ixeWinType::edit) {
    readOnly=          ((ixEdit *)_parent)->usage.readOnly;
    cursorUsable=      ((ixEdit *)_parent)->usage.hasCursor;
    selPossible= ((ixEdit *)_parent)->usage.selection;
  }
  
  // process all chars - can be unicodes, or special str manipulation codes
  if(Ix::wsys().focus== _parent)
  while((c= in.k.getChar())) {
    /// shortcuts
    str32 *s= &cur.pLine->text;  // AFTER A _delSelection() OR SIMILAR, THIS POINTER WILL BE BAD, CAREFUL TO UPDATE
    int32 *cpos= &cur.pos;

    // new line
    if(c== Kch_enter) {
      
      /// ixEdit one line usage
      if(_parent->_type== ixeWinType::edit) {
        if(((ixEdit *)_parent)->usage.oneLine) {
          ((ixEdit *)_parent)->enterPressed= true;
          ret= true;
          continue;
        }
      }

      /// ixStatic one line usage
      if(_parent->_type== ixeWinType::staticText)
        if(((ixStaticText *)_parent)->usage.oneLine)
          continue;

      if(readOnly) continue;
      
      if(!checkLimits('\n')) continue;

      if(selPossible) {
        sel.delSelection();
        s= &cur.pLine->text;
      }

      /// if the cursor is in the middle of diacriticals, move it back, so the whole char is moved to the next line
      while(*cpos> 0) {
        if(!Str::isComb(s->d[*cpos- 1]))
          break;
        (*cpos)--;
      }
        
      s->insert('\n', *cpos);
      nrUnicodes++;
      int32 ipos= (*cpos)+ 1;
        

      ixTxtData::Line *p= new ixTxtData::Line;
      lines.addAfter(p, cur.pLine);
        
      /// everything after the cursor is inserted in next line
      p->text= s->d+ ipos;

      /// everything after cursor is deleted from current line
      s->del(s->nrUnicodes- ipos, ipos);

      _updateWrapList(cur.pLine);
      _updateWrapList(p);

      /// cursor update, move to next line
      cur.increaseUnicode();
      cur.makeSureInBounds();
      cur.makeSureVisible();

      ret= true;
      

    // backspace
    } else if(c== Kch_backSpace) {
      if(readOnly) continue;

      if(selPossible && sel) {
        sel.delSelection();
        ret= true;

      } else if(*cpos> 0) {
        /// all diacriticals will be erased, if a char is deleted, not a diacritical
        bool wipeCombs= false;
        if(isComb(s->d[*cpos- 1]))
          wipeCombs= true;

        s->del(1, (*cpos)- 1);
        cur.decreaseUnicode();
        nrUnicodes--;

        if(wipeCombs && *cpos)
          while(isComb(s->d[*cpos- 1])) {
            s->del(1, (*cpos)- 1);
            cur.decreaseUnicode();
            nrUnicodes--;
            if(!*cpos) { error.simple("ixEdit::update() - cursor position reached 0, shouldn't have"); break; }
          }

        _updateWrapList(cur.pLine);
        cur.makeSureInBounds();
        cur.makeSureVisible();
        ret= true;

      /// delete a \n
      } else if((*cpos== 0) && (cur.line> 0)) {
        cur.decreaseUnicode();
        cur.makeSureVisible();   /// asures _view is not on the deleted line
        ixTxtData::Line *p1= (ixTxtData::Line *)cur.pLine;
        ixTxtData::Line *p2= (ixTxtData::Line *)cur.pLine->next;
        _delWrapForLine(p2);

        // delete '\n' if there is one
        if(p1->text.nrUnicodes)
          if(p1->text.d[p1->text.nrUnicodes- 1]== '\n') {
            //text.cur.decreaseUnicode(); first decrease unicode already does this
            p1->text.del();
            nrUnicodes--;
          }
        // copy line 2 to line 1's end
        p1->text+= p2->text;

        lines.del(p2);     /// delete the second line

        _updateWrapList(p1);
        cur.makeSureInBounds();
        cur.makeSureVisible();
        ret= true;
      }

    } else if(c== Kch_delete) {
      if(readOnly) continue;

      if(selPossible && sel) {
        sel.delSelection();
        ret= true;

      } else if(s->d[*cpos]== '\n') {
        ixTxtData::Line *p1= cur.pLine;
        ixTxtData::Line *p2= (ixTxtData::Line *)p1->next;

        s->del(1, *cpos);
        nrUnicodes--;

        if(p2) {
          cur.makeSureInBounds();
          cur.makeSureVisible();   /// asure _view is not on the deleted line
          _delWrapForLine(p2);
          p1->text+= p2->text;
          lines.del(p2);
        }

        _updateWrapList(p1);
        cur.makeSureInBounds();
        cur.makeSureVisible();
        ret= true;

      } else if(s->d[*cpos]!= 0) {
        /// check if to wipe diacriticals. when deleting a char, all it's diacriticals are deleted
        bool wipeCombs= false;
        if(!isComb(s->d[*cpos]))
          wipeCombs= true;

        s->del(1, *cpos);
        nrUnicodes--;

        if(wipeCombs)
          if(*cpos> 0)
            while(isComb(s->d[(*cpos)- 1])) {
              s->del(1, (*cpos)- 1);
              nrUnicodes--;
              cur.decreaseUnicode();
              if(*cpos== 0) break;
            }

        _updateWrapList(cur.pLine);
        cur.makeSureInBounds();
        cur.makeSureVisible();
        ret= true;
      }
      
    } else if(c== Kch_cut) {
      if(selPossible)
        if(sel) {
          str32 s32;
          sel.copy(&s32);
          osi.setClipboard(s32.convert8());
          if(!readOnly)
            sel.delSelection();
          ret= true;
        }

    } else if(c== Kch_copy) {
      if(selPossible)
        if(sel) {
          str32 s32;
          sel.copy(&s32);
          osi.setClipboard(s32.convert8());
          ret= true;
        }

    } else if(c== Kch_paste) {
      if(readOnly) continue;
      str8 s8;
      osi.getClipboard(&s8.d);
      s8.updateLen();
      str32 s32(s8);
      sel.paste(&s32);
      cur.makeSureVisible();
      ret= true;

    } else if(c== Kch_left) {
      if(selPossible) if(sel) { sel.delData(); ret= true; }
      if(cursorUsable) {
        cur.left();
        cur.makeSureVisible();
        ret= true;
      }

    } else if(c== Kch_right) {
      if(selPossible) if(sel) { sel.delData(); ret= true; }
      if(cursorUsable) {
        cur.right();
        cur.makeSureVisible();
        ret= true;
      }

    } else if(c== Kch_up) {
      if(selPossible) if(sel) { sel.delData(); ret= true; }
      if(cursorUsable) {
        cur.up();
        cur.makeSureVisible();
        ret= true;
      }

    } else if(c== Kch_down) {
      if(selPossible) if(sel) { sel.delData(); ret= true; }
      if(cursorUsable) {
        cur.down();
        cur.makeSureVisible();
        ret= true;
      }

    } else if(c== Kch_home) {
      if(selPossible) if(sel) { sel.delData(); ret= true; }
      if(cursorUsable) {
        cur.home();
        cur.makeSureVisible();
        ret= true;
      }

    } else if(c== Kch_end) {
      if(selPossible) if(sel) { sel.delData(); ret= true; }
      if(cursorUsable) {
        cur.end();
        cur.makeSureVisible();
        ret= true;
      }

    } else if(c== Kch_pgUp) {
      if(selPossible) if(sel) { sel.delData(); ret= true; }
      if(cursorUsable) {
        cur.pgUp();
        cur.makeSureVisible();
        ret= true;
      }

    } else if(c== Kch_pgDown) {
      if(selPossible) if(sel) { sel.delData(); ret= true; }
      if(cursorUsable) {
        cur.pgDown();
        cur.makeSureVisible();
        ret= true;
      }

    } else if(c== Kch_selLeft) {
      if(selPossible) {
        sel.addLeft();
        cur.makeSureVisible();
        ret= true;
      }

    } else if(c== Kch_selRight) {
      if(selPossible) {
        sel.addRight();
        cur.makeSureVisible();
        ret= true;
      }

    } else if(c== Kch_selUp) {
      if(selPossible) {
        sel.addUp();
        cur.makeSureVisible();
        ret= true;
      }

    } else if(c== Kch_selDown) {
      if(selPossible) {
        sel.addDown();
        cur.makeSureVisible();
        ret= true;
      }

    } else if(c== Kch_selHome) {
      if(selPossible) {
        sel.addHome();
        cur.makeSureVisible();
        ret= true;
      }

    } else if(c== Kch_selEnd) {
      if(selPossible) {
        sel.addEnd();
        cur.makeSureVisible();
        ret= true;
      }

    } else if(c== Kch_selPgUp) {
      if(selPossible) {
        sel.addPgUp();
        cur.makeSureVisible();
        ret= true;
      }

    } else if(c== Kch_selPgDown) {
      if(selPossible) {
        sel.addPgDown();
        cur.makeSureVisible();
        ret= true;
      }

    // unicode character
    } else {

      // ONELINES MUST BE FAST, A SPECIAL BRANCH SHOULD BE DONE FOR THEM
      if(!readOnly) {
        if(selPossible && sel) {
          sel.delSelection();
          s= &cur.pLine->text;
          ret= true;
        }

        if(checkLimits(c)) {
          s->insert(c, *cpos);
          nrUnicodes++;
      
          _updateWrapList(cur.pLine);
          cur.increaseUnicode();
          cur.makeSureInBounds();
          cur.makeSureVisible();
          ret= true;
        }
      } /// !readOnly
    }
  } /// process all manip chars

  /// any action is done with keyboard at this point
  if(ret)
    Ix::wsys().flags.setUp((uint32)ixeWSflags::keyboardUsed);


  // MOUSE events

  //static int32 imx, imy;        /// these will hold where the initial click happened
  //static int32 iUnicode, iLine; /// the line and unicode when the mouse was initially clicked

  rectf r; _parent->getPosVD(&r);
  float mx= _parent->_scaleDiv(in.m.x), my= _parent->_scaleDiv(in.m.y);
  float mdx= _parent->_scaleDiv(in.m.dx), mdy= _parent->_scaleDiv(in.m.dy);
  bool inside= r.inside(mx, my);

  // no event currently happening
  if(!Ix::wsys()._op.win && inside) {
    /// a r-click event starts - can be a SELECTION DRAG or a CURSOR POS CHANGE
    if(in.m.but[0].down) {
      Ix::wsys()._op.win= _parent;
      Ix::wsys()._op.mRclick= true;
      if(selPossible) sel.delData();

      Ix::wsys().bringToFront(_parent);
      Ix::wsys().focus= _parent;
      Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
      return true;
    }
  }


  // if there are problems with small fonts, and selecting while you don't want selecting,
  //   selection could happen only if mouse is being hold more than 1 second for example, or moved more than unit* 5 or something like that

  /// an event is happening with this window involved
  if(Ix::wsys()._op.win== _parent) {

    // R-Click event
    if(Ix::wsys()._op.mRclick) {
      static float lx= -1, ly= -1;    // tricky, but hoping the focus can happen only on one window... multiple texts will use the same mem location

      // R-Click is being hold down - update cursor & selection
      if(in.m.but[0].down) {

        // THIS CAN BE A MOVEMENT BIGGER THAN A CERTAIN AMOUNT OF PIXELS, SCALED FOR 720p, 1080p, 2160p >>>
        // _ixEditClickDelta i already created this variable but never used it <<<<<<<<<<<<<<<<<<<<<<<<

        /// mouse moved while clicked
        if(mx!= lx || my!= ly) {
          /// text coord y is from top to bottom, like a page of text
          //text.cur._setLineAndPosInPixels(in.m.x- (r.x0+ _viewArea.x0)+ hscroll->position, (r.y0+ _viewArea.ye)- in.m.y+ vscroll->position);

          if(cursorUsable)
            cur._setLineAndPosInPixels(mx- (r.x0+ _parent->_viewArea.x0)+ _parent->hscroll->position, my- (r.y0+ _parent->_viewArea.y0)+ _parent->vscroll->position);
          if(selPossible) {
            if(!sel)
              sel._startSelection();
            sel._updateEndFromCursor();
          }
          lx= mx, ly= my;
        }
        Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
        return true;

      // R-Click end - final cursor and selection positions
      } else if(!in.m.but[0].down) {     /// mouse r-button released
        
        /// text coord y is from top to bottom, like a page of text
        //text.cur._setLineAndPosInPixels(imx- (r.x0+ _viewArea.x0), (r.y0+ _viewArea.ye)- imy);
        //text.cur._setLineAndPosInPixels(in.m.x- (r.x0+ _viewArea.x0)+ hscroll->position, (r.y0+ _viewArea.ye)- in.m.y+ vscroll->position);
        if(cursorUsable)
          cur._setLineAndPosInPixels(mx- (r.x0+ _parent->_viewArea.x0)+ (float)_parent->hscroll->position,
                                     my- (r.y0+ _parent->_viewArea.y0)+ (float)_parent->vscroll->position);

        if(selPossible)
          if(sel.start== sel.end && sel.startLine== sel.endLine)
            sel.delData();

        Ix::wsys()._op.delData();
        lx= ly= -1;
        Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
        return true;
      }
    } /// r-click event
  } /// an event happening with this window

  return ret;
}










///==================================================///
// SELECTION class ********************************** //
///==================================================///


inline void ixTxtData::Sel::_startSelection() {
  start=       end=       _parent->cur.pos;
  startLine=   endLine=   _parent->cur.line;
  pStartLine=  pEndLine=  _parent->cur.pLine;
  pStartWline= pEndWline= _parent->cur.pWline;
  startWline=  endWline=  _parent->cur.wline;
  startX0=     endX0=     _parent->cur.x0;
  startY0=     endY0=     _parent->cur.y0;
}


inline void ixTxtData::Sel::_updateEndFromCursor() {
  end=       _parent->cur.pos;
  endLine=   _parent->cur.line;
  pEndLine=  _parent->cur.pLine;
  pEndWline= _parent->cur.pWline;
  endWline=  _parent->cur.wline;
  endX0=     _parent->cur.x0;
  endY0=     _parent->cur.y0;
}


void ixTxtData::Sel::addLeft() {
  /// start of a new selection
  if(!(start|| end|| startLine|| endLine))
    _startSelection();

  _parent->cur.left();
  _updateEndFromCursor();
  
  checkValid();
}


void ixTxtData::Sel::addRight() {
  /// start of a new selection
  if(!(start|| end|| startLine|| endLine))
    _startSelection();

  _parent->cur.right();
  _updateEndFromCursor();

  checkValid();
}


void ixTxtData::Sel::addUp() {
  /// start of a new selection
  if(!(start|| end|| startLine|| endLine))
    _startSelection();

  _parent->cur.up();
  _updateEndFromCursor();

  checkValid();
}


void ixTxtData::Sel::addDown() {
  /// start of a new selection
  if(!(start|| end|| startLine|| endLine))
    _startSelection();

  _parent->cur.down();
  _updateEndFromCursor();

  checkValid();
}


void ixTxtData::Sel::addHome() {
 /// start of a new selection
  if(!(start|| end|| startLine|| endLine))
    _startSelection();

  _parent->cur.home();
  _updateEndFromCursor();

  checkValid();
}


void ixTxtData::Sel::addEnd() {
 /// start of a new selection
  if(!(start|| end|| startLine|| endLine))
    _startSelection();

  _parent->cur.end();
  _updateEndFromCursor();

  checkValid();
}


void ixTxtData::Sel::addPgUp() {
 /// start of a new selection
  if(!(start|| end|| startLine|| endLine))
    _startSelection();

  _parent->cur.pgUp();
  _updateEndFromCursor();

  checkValid();
}


void ixTxtData::Sel::addPgDown() {
 /// start of a new selection
  if(!(start|| end|| startLine|| endLine))
    _startSelection();

  _parent->cur.pgDown();
  _updateEndFromCursor();

  checkValid();
}


void ixTxtData::Sel::addCtrlHome() {
 /// start of a new selection
  if(!(start|| end|| startLine|| endLine))
    _startSelection();

  _parent->cur.ctrlHome();
  _updateEndFromCursor();

  checkValid();
}


void ixTxtData::Sel::addCtrlEnd() {
 /// start of a new selection
  if(!(start|| end|| startLine|| endLine))
    _startSelection();

  _parent->cur.ctrlEnd();
  _updateEndFromCursor();

  checkValid();
}


void ixTxtData::Sel::addCtrlPgUp() {
 /// start of a new selection
  if(!(start|| end|| startLine|| endLine))
    _startSelection();

  _parent->cur.ctrlPgUp();
  _updateEndFromCursor();

  checkValid();
}


void ixTxtData::Sel::addCtrlPgDown() {
 /// start of a new selection
  if(!(start|| end|| startLine|| endLine))
    _startSelection();

  _parent->cur.ctrlPgDown();
  _updateEndFromCursor();

  checkValid();
}



void ixTxtData::Sel::delSelection() {
  if(!(*this)) return;                            /// if there's no selection there's nothing to delete

  int32 _start= getStart(),
        _end= getEnd(),
        _startLine= getStartLine(),
        _endLine= getEndLine();

  ixTxtData::Line *p1= getStartLinePointer();
  ixTxtData::Line *p2= getEndLinePointer();
  
  // adjust cursor position - cursor will be placed on the start selection position
  _parent->cur.pos= _start;
  _parent->cur.line= _startLine;
  _parent->cur.pLine= p1;
  _parent->cur.x0= getStartX0();
  _parent->cur.y0= getStartY0();
  _parent->cur.wline= getStartWline();
  _parent->cur.pWline= getStartWlinePointer();
  _parent->cur.makeSureVisible();       // move the _view. this will make sure, too, that the _updateWrapList will update both cursor and _view
  

  // NORMAL EDITOR, WITH FLEXIBLE LINES
  if(!_parent->_fixedBuffer) {

    /// delete something in the same line
    if(_startLine== _endLine) {
      _parent->nrUnicodes-= _end- _start;           /// number of unicodes remaining in line
      p1->text.del(_end- _start, _start);             // deletion
      _parent->_updateWrapList(p1);
      _parent->findTextDx();
      //if(_parent->_view.pLine== p1) moveView= true;

    /// delete from multiple lines
    } else {
      /// delete whole lines between start and end line
      if(_endLine- _startLine> 1)
        for(int a= _endLine- _startLine- 1; a> 0; a--) {
          _parent->_delWrapForLine((ixTxtData::Line *)p1->next);
          _parent->nrUnicodes-= ((ixTxtData::Line *)p1->next)->text.nrUnicodes,
          _parent->lines.del(p1->next);
          //if(_parent->_view.pLine== (Line *)p1->next) moveView= true;
        }

      /// delete from first and last lines
      //_parent->_delWrapForLine(p1);  THE WRAP WILL ONLY BE UPDATED - THIS INCLUDES A DELETE OF CURRENT WRAP
      //if(_parent->_view.pLine== p1) moveView= true;
      //if(_parent->_view.pLine== p2) moveView= true;
      _parent->nrUnicodes-= p1->text.nrUnicodes- _start;
      p1->text.del(p1->text.nrUnicodes- _start, _start);  /// delete unicodes in start line after first mark
      _parent->_delWrapForLine(p2);
      _parent->nrUnicodes-= _end;
      p2->text.del(_end, 0);                             /// delete unicodes in end line before last mark
      
      // combine the two lines
      p1->text+= p2->text;                     // startline incorporate the end line
      _parent->lines.del(p2);                 /// end line dissapears as it is within start line now
      _parent->_updateWrapList(p1);
    }

  // ONE LINE EDITOR
  } else {
    _parent->nrUnicodes-= _end- _start;           /// number of unicodes remaining in line
    p1->text.del(_end- _start, _start);             // deletion
    //p1->dx= i->pr.getTextDx32(p1->text, _parent->font);     /// linesize in pixels
    _parent->_updateWrapList();
    //moveView= true;
  }

  _parent->cur.makeSureInBounds();
  _parent->cur.makeSureVisible();
  
  delData();                     /// selection is no more
}


void ixTxtData::Sel::copy(str32 *out_str) {
  if(!out_str) return;

  int32 _start= getStart(), _end= getEnd(), _startLine= getStartLine(), _endLine= getEndLine();
  ixTxtData::Line *p1= (ixTxtData::Line *)_parent->lines.get(_startLine);
  ixTxtData::Line *p2= (ixTxtData::Line *)_parent->lines.get(_endLine);
  out_str->delData();

  // copy from the same line
  if(_startLine== _endLine) {
    out_str->insertStr(p1->text.d+ _start, _end- _start);

  // copy from multiple lines
  } else {
    /// copy from start line
    out_str->insertStr(p1->text.d+ _start);

    /// copy all lines between start and end
    ixTxtData::Line *p= (ixTxtData::Line *)p1->next;
    for(int a= 0; a< _endLine- _startLine- 1; a++) {
      (*out_str)+= p->text;
      p= (ixTxtData::Line *)p->next;
    }

    /// copy from end line
    out_str->insertStr(p2->text, _end);
  }
}



void ixTxtData::Sel::paste(str32 *in_str) {
  if(!in_str) return;
  delSelection();
  bool horiz= _parent->orientation& IX_TXT_HORIZONTAL;

  // if the view is on the line that is being pasted upon, it has to be updated
  bool moveView= false;
  float moveDelta;
  if(_parent->_view.pos>= (horiz? _parent->cur.y0: _parent->cur.x0))
    moveView= true;
  if(_parent->_view.pLine== _parent->cur.pLine)
    moveView= true;

  /// note: fixed buffer editor is only oneline (so i don't think of wild things anymore)

  // METHOD 1: char by char
  /*
  for(uint32 *p= (uint32 *)in_str->d; *p; p++) {  /// loop thru all unicodes in paste string
    
    // insert only if passes the _checkLimits() func
    if(!_parent->checkLimits(*p))
       continue;

    // unicode check passed, insert
    cur.pLine->text.insert(*p, cur.pos);
    _parent->nrUnicodes++;

    // add a line
    if(*p== '\n') {                               /// this won't be a case when the editor is on oneline, it simply wont come up to this point
      ixTxtLine *l= new ixTxtLine;
      l->text= cur.pLine->text.d+ cur.pos+ 1; /// +1 due current \n char
      l->dx= ixPrint::getTextDx32(l->text, _parent->font);
      l->dy= ixPrint::getCharDy(_parent->font);
      _parent->lines.addAfter(l, cur.pLine);

      cur.pLine->text.del(cur.pLine->text.nrUnicodes- cur.pos- 1, cur.pos+ 1);
      cur.pLine->dx= ixPrint::getTextDx32(cur.pLine->text, _parent->font);
        
      cur.pos= 0;
      cur.line++;
      cur.pLine= l;
    } /// add a line
    else 
      cur.pos++;
  } /// loop thru all unicodes in paste string
  */
  
  // METHOD 2: line by line
  for(uint32 *p= (uint32 *)in_str->d; *p; ) {  /// loop thru all lines in the paste string
    
    int32 lineLen;
    char32 *lstart= (char32 *)p;                  /// points to each line start in paste string

    /// find out the line length
    for(lineLen= 0; *p; lineLen++, p++)
      if(*p == '\n') {
        lineLen++;
        break;
      }
    
    // line insert
    _parent->cur.pLine->text.insertStr(lstart, lineLen, _parent->cur.pos);

    // char-by-char check
    //uint32 *p2= (uint32 *)_parent->cur.pLine->text.pointUnicode(_parent->cur.pos);
    uint32 *p2= (uint32 *)_parent->cur.pLine->text[(uint32)_parent->cur.pos];

    bool deleted= false;
    for(int32 a= lineLen; a> 0; a--)
      if(!_parent->checkLimits(*p2))
        Str::del32static((char32 *)p2), deleted= true;
      else
        _parent->nrUnicodes++, _parent->cur.pos++, p2++;
    
    if(deleted)                                   /// updateLen only something didn't pass the checkLimits()
      _parent->cur.pLine->text.updateLen();

    // add a line
    if(*(p2- 1)== '\n') {                         /// this won't be a case when the editor is on oneline, it simply wont come up to this point
      ixTxtData::Line *l= new ixTxtData::Line;
      l->text= _parent->cur.pLine->text.d+ _parent->cur.pos;                
      _parent->lines.addAfter(l, _parent->cur.pLine);

      _parent->cur.pLine->text.del(_parent->cur.pLine->text.nrUnicodes- _parent->cur.pos, _parent->cur.pos);
      _parent->_updateWrapList(_parent->cur.pLine, false);

      _parent->cur.pos= 0;
      _parent->cur.line++;
      _parent->cur.pLine= l;
    } /// add a line
    
    if(*p== '\n') p++;
  }
 
  _parent->cur.pWline= null;
  _parent->_updateWrapList(_parent->cur.pLine, false);
  _parent->cur.updateWlineAndCoords();
  _parent->cur.makeSureInBounds();

  if(moveView) {
    moveDelta= _parent->_view.pos- (horiz? _parent->cur.y0: _parent->cur.x0);
    _parent->_view.moveRelativeToCursor(moveDelta);
  }

  _parent->cur.makeSureVisible();
}














///===========================================///
// CURSOR class ****************************** //
///===========================================///



void ixTxtData::Cursor::decreaseLine(int32 in_n, bool in_computePosInPixels) {
  bool horiz= _parent->orientation& IX_TXT_HORIZONTAL;  /// shortcut
  ixTxtData::Wline *w= pWline;
  if(!w || in_n<= 0) return;

  while(in_n--)
    if(w->prev) {
      int32 wpos= pos- w->startUnicode;
      pWline= (ixTxtData::Wline *)w->prev;
      wline--;

      if(pWline->line!= w->line)
        line--;

      w= pWline;                   /// shortcut purposes
      pLine= w->line;

      pos= w->startUnicode+ wpos;
      if(pos> w->startUnicode+ w->nrUnicodes)
        pos= w->startUnicode+ w->nrUnicodes;

      /// avoid position to pass the \n
      if(pos> 0) {
        uint32 c= pLine->text.d[pos- 1];
        if(c== '\n')
          pos--;
        else if(c== ' ')
          if(pWline->next && pos== w->startUnicode+ w->nrUnicodes)
            pos--;
      }

      if(horiz) y0-= w->dy;
      else      x0-= w->dx;
    }

  if(in_computePosInPixels) {
    // heh would this work?   (horiz? x0: y0)= _getPosInPixels();
    if(horiz) x0= _getPosInPixels();
    else      y0= _getPosInPixels();
  }
}


void ixTxtData::Cursor::increaseLine(int32 in_n, bool in_computePosInPixels) {
  bool horiz= _parent->orientation& IX_TXT_HORIZONTAL;  /// shortcut
  ixTxtData::Wline *w= pWline;
  if(!w || in_n<= 0) return;

  while(in_n--)
    if(w->next) {
      int32 wpos= pos- w->startUnicode;
      pWline= (ixTxtData::Wline *)w->next;
      wline++;

      if(pWline->line!= w->line)
        line++;

      w= pWline;                   /// shortcut purposes
      pLine= w->line;

      pos= w->startUnicode+ wpos;
      if(pos> w->startUnicode+ w->nrUnicodes)
        pos= w->startUnicode+ w->nrUnicodes;

      /// avoid position to pass the \n
      if(pos> 0) {
        uint32 c= pLine->text.d[pos- 1];
        if(c== '\n')
          pos--;
        else if(c== ' ')
          if(pWline->next && pos== w->startUnicode+ w->nrUnicodes)
            pos--;
      }

      if(horiz) y0+= w->dy;
      else      x0+= w->dx;
    }

  if(in_computePosInPixels) {
    if(horiz) x0= _getPosInPixels();
    else      y0= _getPosInPixels();
  }
}


inline void ixTxtData::Cursor::decreaseUnicode(int32 in_n) {
  bool horiz= _parent->orientation& IX_TXT_HORIZONTAL;  /// shortcut

  while(in_n--)
    // decrease within the line
    if(pos> 0) {
      pos--;
      // decrease to the prev wline
      if(pos< pWline->startUnicode) {
        pWline= (ixTxtData::Wline *)pWline->prev;
        wline--;

        if(horiz) y0-= pWline->dy;
        else      x0-= pWline->dx;
      }
    // decrease to the prev line
    } else if((pos== 0) && (line> 0)) {
      pLine= (ixTxtData::Line *)pLine->prev;
      line--;
      pWline= (ixTxtData::Wline *)pWline->prev;
      wline--;

      pos= pLine->text.nrUnicodes;
      /// avoid position to pass the \n
      if(pos> 0)
        if(pLine->text.d[pos- 1]== '\n')
          pos--;

      if(horiz) y0-= pWline->dy;
      else      x0-= pWline->dx;
    }

  if(horiz) x0= _getPosInPixels();
  else      y0= _getPosInPixels();
}


inline void ixTxtData::Cursor::increaseUnicode(int32 in_n) {
  bool nextLine= false, nchar= false;
  bool horiz= _parent->orientation& IX_TXT_HORIZONTAL;  /// shortcut

  while(in_n--) {
    pos++;

    if(pos- pWline->startUnicode> pWline->nrUnicodes)   // next wline
      nextLine= true;
    else if(pLine->text.d[pos- 1]== '\n')               // next line
      nextLine= true, nchar= true;
    else if((pos- pWline->startUnicode== pWline->nrUnicodes) && (pLine->text.d[pos- 1]== ' ')) {
      if(pWline->next)
        if(((Wline *)pWline->next)->line== pWline->line)
        nextLine= true;
        
    }
      

    if(nextLine) {
      if(pos> pLine->text.nrUnicodes || nchar) {
        if(pLine->next) {
          pLine= (ixTxtData::Line *)pLine->next;
          line++;
          pos= 0;

        // end of text, reverse unicode increase
        } else {
          pos--;
          nextLine= false, nchar= false;
          continue;
        }
      }

      pWline= (ixTxtData::Wline *)pWline->next;
      wline++;

      if(horiz) y0+= pWline->dy;
      else      x0+= pWline->dx;

      nextLine= false;
    }
  }

  if(horiz) x0= _getPosInPixels();
  else      y0= _getPosInPixels();
}


void ixTxtData::Cursor::advanceLineForPixels(float in_pixels) {
  //_setLineAndPosInPixels() and _setPosInPixels() are done
    //i think it's better to use those, as there's fewer funcs to debug if something is wrong
    //so this will just change the in_pixels to be exact pixels to jump to for _setLineAndPosInPixels() i think, and it will be fine
    //but, keep the current func, don't delete, comment, due who knows...

  if(in_pixels== 0.0f) return;

  // have to use getPosForPixel() func, that is not done yet i think

  float saveX0= x0, saveY0= y0;   /// will put pos as closely as possible to current pos
  bool horiz= _parent->orientation& IX_TXT_HORIZONTAL;

  // JUST ADVANCE LINE BY LINE, PUT POS WHERE IT WAS, CUZ IT CHANGES WITH EVERY LINE ADVANCE

  while(1) {

    // condition to break the loop - cursor is as close as possible to the target
    if((in_pixels>= 0.0f) && (in_pixels< (horiz? pWline->dy: pWline->dx)))
      break;
    
    /// decrease
    if(in_pixels< 0) {
      if(pWline->prev== null)
        break;

      in_pixels+= (horiz? pWline->dy: pWline->dx);
      decreaseLine(1, false);

    /// increase
    } else {
      if(pWline->next== null)
        break;

      in_pixels-= (horiz? pWline->dy: pWline->dx);
      increaseLine(1, false);
    } /// decrease or increase
  } /// infinite loop

  _setPosInPixels(horiz? saveX0: saveY0);
}




void ixTxtData::Cursor::resetToStart() {
  bool horiz= _parent->orientation& IX_TXT_HORIZONTAL;
  pos= line= wline= 0;
  pLine= (ixTxtData::Line *)_parent->lines.first;
  pWline= (ixTxtData::Wline *)_parent->_wrapLines.first;

  if(horiz) x0= _getPosInPixels(), y0= _getWlineInPixels();
  else      y0= _getPosInPixels(), x0= _getWlineInPixels();

  makeSureInBounds();
}


void ixTxtData::Cursor::resetToEnd() {
  bool horiz= _parent->orientation& IX_TXT_HORIZONTAL;
  pLine= (ixTxtData::Line *)_parent->lines.last;
  pWline= (ixTxtData::Wline *)_parent->_wrapLines.last;
  line= _parent->lines.nrNodes- 1;
  wline= _parent->_wrapLines.nrNodes- 1;
  pos= pLine->text.nrUnicodes;
  
  if(horiz) x0= _getPosInPixels(), y0= _parent->textDy- pWline->dy;
  else      y0= _getPosInPixels(), x0= _parent->textDx- pWline->dx;

  makeSureInBounds();
}
















void ixTxtData::Cursor::up() {
  makeSureInBounds();

  // left & right orientation
  if(_parent->orientation& IX_TXT_HORIZONTAL)
    decreaseLine();

  // up orientation
  else if(_parent->orientation== IX_TXT_DOWN)
    increaseUnicode();

  // down orientation
  else if(_parent->orientation== IX_TXT_UP)
    decreaseUnicode();
}


void ixTxtData::Cursor::down() {
  makeSureInBounds();
  
  // left & right orientation
  if(_parent->orientation& IX_TXT_HORIZONTAL)
    increaseLine();

  // up orientation
  else if(_parent->orientation== IX_TXT_DOWN)
    decreaseUnicode();

  // down orientation
  else if(_parent->orientation== IX_TXT_UP)
    increaseUnicode();
}


void ixTxtData::Cursor::left() {
  makeSureInBounds();

  // right text orientation
  if(_parent->orientation== IX_TXT_RIGHT)
    decreaseUnicode();
  // left text orientation
  else if(_parent->orientation== IX_TXT_LEFT)
    increaseUnicode();
  // vertical text (up or down)
  else if(_parent->orientation& IX_TXT_VERTICAL)
    decreaseLine();
}


void ixTxtData::Cursor::right() {
  makeSureInBounds();

  // right text orientation
  if(_parent->orientation== IX_TXT_RIGHT)
    increaseUnicode();
  // left text orientation
  else if(_parent->orientation== IX_TXT_LEFT)
    decreaseUnicode();
  // vertical text (up or down)
  else if(_parent->orientation& IX_TXT_VERTICAL)
    decreaseLine();
}


void ixTxtData::Cursor::home() {
  makeSureInBounds();

  pos= pWline->startUnicode;

  if(_parent->orientation& IX_TXT_HORIZONTAL)
    x0= _getPosInPixels();
  else if(_parent->orientation& IX_TXT_VERTICAL)
    y0= _getPosInPixels();
}


void ixTxtData::Cursor::end() {
  makeSureInBounds();
  pos= pWline->startUnicode+ pWline->nrUnicodes;
  if(pos> 0)
    if(pWline->line->text.d) {
      uint32 c= pWline->line->text.d[pos- 1];
      if(c== '\n')
        pos--;
      else if(c== ' ' && pWline->next)
        pos--;
    }

  if(_parent->orientation& IX_TXT_HORIZONTAL)
    x0= _getPosInPixels();
  else if(_parent->orientation& IX_TXT_VERTICAL)
    y0= _getPosInPixels();
}







void ixTxtData::Cursor::pgUp() {
  bool horiz= _parent->orientation& IX_TXT_HORIZONTAL;  /// shortcut
  ixBaseWindow *p= _parent->_parent;                    /// shortcut
  //int32 advance= (horiz? p->pos.dy: p->pos.dx);
  float delta= (horiz? p->_viewArea.dy: p->_viewArea.dx);

  if(horiz) { if(p->hscroll) p->hscroll->setPositionD(-delta); }
  else      { if(p->vscroll) p->vscroll->setPositionD(-delta); }
  advanceLineForPixels(-delta); // was -advance, but really i dono what's the point of that
}


void ixTxtData::Cursor::pgDown() {
  bool horiz= _parent->orientation& IX_TXT_HORIZONTAL;  /// shortcut
  ixBaseWindow *p= _parent->_parent;                    /// shortcut
  //int32 advance= (horiz? p->pos.dy:            p->pos.dx);
  float delta=   (horiz? p->_viewArea.dy: p->_viewArea.dx);

  if(horiz) { if(p->hscroll) p->hscroll->setPositionD(delta); }
  else      { if(p->vscroll) p->vscroll->setPositionD(delta); }
  advanceLineForPixels(delta);    // was advance, but it has no sense
}


void ixTxtData::Cursor::ctrlHome() {
  bool horiz= _parent->orientation& IX_TXT_HORIZONTAL;  /// shortcut
  ixBaseWindow *p= _parent->_parent;                    /// shortcut
  if(horiz) { if(p->hscroll) p->hscroll->setPositionMin(); }
  else      { if(p->vscroll) p->vscroll->setPositionMin(); }

  resetToStart();
}


void ixTxtData::Cursor::ctrlEnd() {
  bool horiz= _parent->orientation& IX_TXT_HORIZONTAL;  /// shortcut
  ixBaseWindow *p= _parent->_parent;                    /// shortcut
  if(horiz) { if(p->hscroll) p->hscroll->setPositionMax(); }
  else      { if(p->vscroll) p->vscroll->setPositionMax(); }
  
  resetToEnd();
}


void ixTxtData::Cursor::ctrlPgUp() {
  bool horiz= _parent->orientation& IX_TXT_HORIZONTAL;  /// shortcut
  if(horiz && !_parent->_parent->vscroll) return;
  if((!horiz) && !_parent->_parent->hscroll) return;
  float saveX0= x0, saveY0= y0;

  wline= _parent->_view.wline;
  line= _parent->_view.line;
  pWline= _parent->_view.pWline;
  pLine= _parent->_view.pLine;
  pos= pWline->startUnicode;

  // horizontal text
  if(horiz) {
    y0= _parent->_view.pos;

    if((pWline->next) && (y0< _parent->_parent->vscroll->position))
      increaseLine(1, false);

    _setPosInPixels(saveX0);

  // vertical text
  } else {
    x0= _parent->_view.pos;
    if((pWline->next) && (x0< _parent->_parent->hscroll->position))
      increaseLine(1, false);

    _setPosInPixels(saveY0);
  }
}


void ixTxtData::Cursor::ctrlPgDown() {
  bool horiz= _parent->orientation& IX_TXT_HORIZONTAL;  /// shortcut
  if(horiz && !_parent->_parent->vscroll) return;
  if((!horiz) && !_parent->_parent->hscroll) return;
  float saveX0= x0, saveY0= y0;

  wline= _parent->_view.wline;
  line= _parent->_view.line;
  pWline= _parent->_view.pWline;
  pLine= _parent->_view.pLine;
  pos= pWline->startUnicode;

  // horizontal text
  if(horiz) {
    y0= _parent->_view.pos;

    while(1) {
      Wline *p= (Wline *)pWline->next;
      if(!p)
        break;
      if(y0+ pWline->dy+ p->dy> _parent->_parent->vscroll->position+ _parent->_parent->_viewArea.dy)
        break;
      increaseLine(1, false);
    }

    _setPosInPixels(saveX0);

  // vertical text
  } else {
    x0= _parent->_view.pos;

    while(1) {
      Wline *p= (Wline *)pWline->next;
      if(!p)
        break;
      if(x0+ pWline->dx+ p->dx> _parent->_parent->hscroll->position+ _parent->_parent->_viewArea.dx)
        break;
      increaseLine(1, false);
    }

    _setPosInPixels(saveY0);
  }

}


inline ixTxtData::Wline *ixTxtData::Cursor::getWline(int32 *out_wline) {
  if(!pLine) { if(out_wline) *out_wline= 0; return null; }

  ixTxtData::Wline *w= pLine->wline;
  
  for(; w; w= (ixTxtData::Wline *)w->next)
    if((pos>= w->startUnicode) && (pos<= w->startUnicode + w->nrUnicodes))
      break;
  
  if(out_wline) {
    *out_wline= 0;
    for(ixTxtData::Wline *p= (ixTxtData::Wline *)_parent->_wrapLines.first; p; p= (ixTxtData::Wline *)p->next, (*out_wline)++)
      if(p== w) break;
  }

  return w;

  /* NO out_wline VER
  if(!pLine) return null; 

  ixTxtData::Wline *w= pLine->wline;

  for(; w; w= (ixTxtData::Wline *)w->next)
    if((w->startUnicode>= pos) && (pos<= w->startUnicode+ w->nrUnicodes))
      break;

  return w;
  */
}


void ixTxtData::Cursor::updateWlineAndCoords() {
  x0= y0= 0;
  pWline= 0;
  wline= 0;
  if(!pLine) return;
  bool horiz= _parent->orientation& IX_TXT_HORIZONTAL;
  ixTxtData::Wline *p= (ixTxtData::Wline *)_parent->_wrapLines.first;

  // pass thru all wlines
  while(p) {
    if(p->line== pLine)
      if((pos>= p->startUnicode) && (pos<= p->startUnicode+ p->nrUnicodes)) break; // found
    
    if(horiz) y0+= p->dy;                 // 1. line in pixels 
    else      x0+= p->dx;
    wline++;                              // 2. wline
    p= (ixTxtData::Wline *)p->next;
  }

  pWline= p;                              // 3. pWline
  if(!pWline) { wline= 0; x0= y0= 0; }
  
  if(horiz) x0= _getPosInPixels();        // 4. position in pixels
  else      y0= _getPosInPixels();

}


float ixTxtData::Cursor::_getPosInPixels() {
  if(!pLine) return 0.0f;
  if(!pLine->text.d) return 0.0f;

  float ret= 0.0f;
  //recti *p= &_parent->_parent->_childArea; this won't work due the window could have actual children
  float charDy= _parent->font.getCharDy();

  // return value depending on text orientation
  if(_parent->orientation& IX_TXT_RIGHT) {
    if(pos- pWline->startUnicode> 0)
      ret= _parent->font.getTextLenu32(pLine->text.d+ pWline->startUnicode, pos- pWline->startUnicode, pWline->spaceSize);

    if(_parent->alignment& IX_TXT_ALN_CENTER)
      //ret+= (p->dx- pWline->dx)/ 2;
      ret+= (_parent->textDx- pWline->dx)/ 2.0f;

    else if(_parent->alignment& IX_TXT_ALN_END)
      ret+= _parent->textDx- pWline->dx;

  } else if(_parent->orientation& IX_TXT_LEFT) {
    if(pos- pWline->startUnicode> 0)
      ret= _parent->textDx- _parent->font.getTextLenu32(pLine->text.d+ pWline->startUnicode, pos- pWline->startUnicode, pWline->spaceSize);

    if(_parent->alignment& IX_TXT_ALN_CENTER)
      ret-= (_parent->textDx- pWline->dx)/ 2.0f;

    else if(_parent->alignment& IX_TXT_ALN_END)
      ret-= _parent->textDx- pWline->dx;

  } else if(_parent->orientation& IX_TXT_DOWN) {
    ret= _parent->textDy- charDy;
    for(int a= pWline->startUnicode; a< pos; a++)
      if(!Str::isComb(*(pLine->text.d+ a)))
        ret-= charDy;                             // <<< MULTIPLE FONTS CHANGE HERE

    if(_parent->alignment& IX_TXT_ALN_CENTER)
      ret-= (_parent->textDy- pWline->dy)/ 2.0f;
    else if(_parent->alignment& IX_TXT_ALN_END)
      ret-= _parent->textDy- pWline->dy;

  } else if(_parent->orientation& IX_TXT_UP) {
    for(int a= pWline->startUnicode; a< pos; a++)
      if(!Str::isComb(*(pLine->text.d+ a)))
        ret+= charDy;                             // <<< MULTIPLE FONTS CHANGE HERE

    if(_parent->alignment& IX_TXT_ALN_CENTER)
      ret-= (_parent->textDy- pWline->dy)/ 2.0f;
    else if(_parent->alignment& IX_TXT_ALN_END)
      ret-= _parent->textDy- pWline->dy;
  }

  return ret;
}


// slow func if there's many lines
// this func recalculates, basically, y0 or x0, so not sure if needed
float ixTxtData::Cursor::_getWlineInPixels() {
  float ret= 0.0f;
  Wline *p= (Wline *)_parent->_wrapLines.first;

  if(!p || !pWline) return 0;

  // return value depending on text orientation
  if(_parent->orientation& IX_TXT_HORIZONTAL)
    for(; p!= pWline; p= (Wline *)p->next)
      ret+= p->dy;

  else if(_parent->orientation& IX_TXT_VERTICAL)
    for(; p!= pWline; p= (Wline *)p->next)
      ret+= p->dx;

  return ret;
}


void ixTxtData::Cursor::_setPosInPixels(float in_pixels) {
  float lineStart;

  /// line start (wline start) in pixels, from origin
  if     (_parent->alignment& IX_TXT_ALN_START)   lineStart= 0.0f;
  else if(_parent->alignment& IX_TXT_ALN_CENTER)  lineStart= (_parent->textDx- pWline->dx)/ 2.0f;
  else if(_parent->alignment& IX_TXT_ALN_END)     lineStart= _parent->textDx- pWline->dx;

  // cursor pos depending on text orientation
  if(_parent->orientation& IX_TXT_RIGHT) {
    if(in_pixels< lineStart)
      pos= pWline->startUnicode;
    else if(in_pixels> lineStart+ pWline->dx)
      pos= pWline->startUnicode+ pWline->nrUnicodes;
    else
      pos= pWline->startUnicode+ _parent->font.getUnicodesMaxPixels32(pLine->text.d+ pWline->startUnicode, in_pixels- lineStart, pWline->spaceSize);

  } else if(_parent->orientation& IX_TXT_LEFT) {
    if(in_pixels< _parent->textDx- lineStart- pWline->dx)
      pos= pWline->startUnicode+ pWline->nrUnicodes;
    else if(in_pixels> _parent->textDx- lineStart)
      pos= pWline->startUnicode;
    else
      pos= pWline->startUnicode+ _parent->font.getUnicodesMaxPixels32(pLine->text.d+ pWline->startUnicode, _parent->textDx- in_pixels- lineStart, pWline->spaceSize, IX_TXT_LEFT);

  } else if(_parent->orientation& IX_TXT_DOWN) {
    if(in_pixels< lineStart)
      pos= pWline->startUnicode;
    else if(in_pixels> lineStart+ pWline->dy)
      pos= pWline->startUnicode+ pWline->nrUnicodes;
    else
      pos= pWline->startUnicode+ _parent->font.getUnicodesMaxPixels32(pLine->text.d+ pWline->startUnicode, in_pixels- lineStart, pWline->spaceSize, IX_TXT_DOWN);

  } else if(_parent->orientation& IX_TXT_UP) {
    if(in_pixels< _parent->textDy- lineStart- pWline->dy)
      pos= pWline->startUnicode+ pWline->nrUnicodes;
    else if(in_pixels> _parent->textDy- lineStart)
      pos= pWline->startUnicode;
    else
      pos= pWline->startUnicode+ _parent->font.getUnicodesMaxPixels32(pLine->text.d+ pWline->startUnicode, _parent->textDx- in_pixels- lineStart, pWline->spaceSize, IX_TXT_UP);
  } /// text orientation


  if(pos> 0)
    if(pLine->text.d[pos- 1]== '\n')
      pos--;

  if(_parent->orientation& IX_TXT_HORIZONTAL) x0= _getPosInPixels();
  else                                        y0= _getPosInPixels();
}



// pixel coords are from 0 to textDx/Dy
// they can be negative or over the bounds, in that case the cursor will set to the closest position possible
void ixTxtData::Cursor::_setLineAndPosInPixels(float in_x, float in_y) {
  bool horiz= _parent->orientation& IX_TXT_HORIZONTAL;
  
  // cursor line (wline)
  if(in_y< 0.0f)
    resetToStart();
  else if(in_y> _parent->textDy)
    resetToEnd();
  else 
    if(horiz) {
      while(!((in_y>= y0) && (in_y<= y0+ pWline->dy)))
        if(y0> in_y) decreaseLine(1, false);
        else         increaseLine(1, false);
    } else {
      while(!((in_y>= x0) && (in_x<= x0+ pWline->dx)))
        if(x0> in_x) decreaseLine(1, false);
        else         increaseLine(1, false);
    }

  _setPosInPixels(horiz? in_x: in_y);
  makeSureInBounds();
  makeSureVisible();
}


void ixTxtData::Cursor::makeSureInBounds() {
  bool wlineChange= false;
  
  if(!pLine) {
    error.detail("pLine is null", __FUNCTION__);
    resetToStart();
  }

  if(!pWline) {
    error.detail("pWline is null", __FUNCTION__);
    resetToStart();
  }
  
  if(line>= (int32)_parent->_wrapLines.nrNodes) {
    error.detail("line>= wrapLines.nrNodes", __FUNCTION__);
    resetToEnd();
  }

  if(line< 0) {
    error.detail("line < 0", __FUNCTION__);
    resetToStart();
  }

  if(pos> pLine->text.nrUnicodes) {
    error.detail("pos is out of line bounds (positive)", __FUNCTION__);
    pos= pLine->text.nrUnicodes;
    updateWlineAndCoords();
  }

  if(pos< 0) {
    error.detail("pos is negative", __FUNCTION__);
    pos= 0;
    updateWlineAndCoords();
  }


  if(pos> pWline->startUnicode+ pWline->nrUnicodes) {
    error.detail("pLine is out of pWline bounds (positive)", __FUNCTION__);
    pos= pWline->startUnicode+ pWline->nrUnicodes;
    if(_parent->orientation& IX_TXT_HORIZONTAL)
      x0= _getPosInPixels();
    else
      y0= _getPosInPixels();
  }

  if(pos< pWline->startUnicode) {
    error.detail("pos is out of pWline bounds (negative)", __FUNCTION__);
    pos= pWline->startUnicode;
    if(_parent->orientation& IX_TXT_HORIZONTAL)
      x0= _getPosInPixels();
    else
      y0= _getPosInPixels();
  }

  /*
  /// if the line doesn't end with newline, maximum of 'pos' is +1
  int m= pLine->text.nrUnicodes;
  if(*pLine->text[m- 1]!= '\n')
    m++;
  if(pos> m) pos= m;
  */


  if(wlineChange || !pWline)
    pWline= getWline(&wline);
}





void ixTxtData::Cursor::makeSureVisible() {
  bool horiz= _parent->orientation& IX_TXT_HORIZONTAL;
  float viewLen;
  float newPos;
  ixScroll *scr;

  // text orientation = horizontal
  if(horiz) {
    // vertical check
    scr= _parent->_parent->vscroll;
    if(scr) {

      viewLen= _parent->_parent->_viewArea.dy;
      if(viewLen< 1.0f) return;

      // cursor is out of view, to the top
      if(y0< scr->position) {
        scr->setPosition(y0);
        _parent->_view.moveToCursor();

      // cursor is out of view, to the bottom
      } else if(y0+ pWline->dy> scr->position+ viewLen) {
        /// this method puts _view to cur, and scrolls up until it reaches newPos
        /// it is slower for 1 line scrolls, or small texts, but it is the same speed for huge texts, and that's why it's best to use
        newPos= y0- viewLen+ pWline->dy;       // _view.pos must reach this point, from the cursor position
        if(newPos< 0) newPos= 0;
        _parent->_view.moveToCursor();

        /// scroll the view cursor until the first line that's out of view
        while(_parent->_view.pos> newPos)
          _parent->_view.decreaseLine();

        _parent->_parent->vscroll->setPosition(newPos);
        //_parent->_view.pos= newPos;

      } /// cursor out of view
    } /// vertical check

    // horizontal check
    scr= _parent->_parent->hscroll;
    if(scr) {
      viewLen= _parent->_parent->_viewArea.dx;
      if(x0< scr->position)
        scr->setPosition(x0);
      if(x0+ drawWidth> scr->position+ viewLen)
        scr->setPosition(x0+ drawWidth- viewLen);
    } /// horizontal check


  // text orientation = vertical
  } else {

    // horizontal check
    scr= _parent->_parent->hscroll;
    if(scr) {
      viewLen= _parent->_parent->pos.dx;
      if(_parent->_parent->vscroll)
        viewLen-= _parent->_parent->vscroll->pos.dx;
    
      /// cursor is out of view, to the top
      if(x0< scr->position) {
        _parent->_parent->hscroll->setPosition(x0);
        _parent->_view.moveToCursor();

      /// cursor is out of view, to the bottom
      } else if(x0> scr->position+ viewLen) {
        newPos= x0- (viewLen- pWline->dx);
        if(newPos< 0) newPos= 0;
        _parent->_view.moveToCursor();

        /// scroll the view cursor until the first line that is partially visible
        while(_parent->_view.pos> newPos)
          _parent->_view.decreaseLine();
        _parent->_parent->hscroll->setPosition(newPos);
      } /// over cursor / before cursor
    } /// horizontal check

    // vertical check
    scr= _parent->_parent->vscroll;
    if(scr) {
      viewLen= _parent->_parent->_viewArea.dx;
      if(y0< scr->position)
        scr->setPosition(y0);
      if(y0+ drawWidth> scr->position+ viewLen)
        scr->setPosition(x0+ drawWidth- viewLen);
    } /// vertical check
  } /// horizontal / vertical text
}
  





///==============///
// ViewPos struct //
///==============///
  
inline void ixTxtData::ViewPos::advanceLine() {
  if(pWline->next) {
    pos+= (_parent->orientation& IX_TXT_HORIZONTAL? pWline->dy: pWline->dx);
    pWline= (Wline *)pWline->next;
    wline++;
    if(pWline->line!= pLine) {
      if(pLine->next) {
        pLine= (Line *)pLine->next;
        line++;
      } else
        error.detail("next in pLine is null", __FUNCTION__);
    }
  } else
    error.detail("next in pWline is null", __FUNCTION__);
}


inline void ixTxtData::ViewPos::decreaseLine() {
  if(pWline->prev) {
    pos-= (_parent->orientation& IX_TXT_HORIZONTAL? pWline->dy: pWline->dx);
    pWline= (Wline *)pWline->prev;
    wline--;
    if(pWline->line!= pLine) {
      if(pLine->prev) {
        pLine= (Line *)pLine->prev;
        line--;
      } else
        error.detail("prev in pLine is null", __FUNCTION__);
    }
  } else
    error.detail("prev in pWline is null", __FUNCTION__);
}


void ixTxtData::ViewPos::resetToStart() {
  delData();
  pWline= (Wline *)_parent->_wrapLines.first;
  pLine= (Line *)_parent->lines.first;
  /*if(_parent->_parent->hscroll)
    _parent->_parent->hscroll->setPositionMin();
  if(_parent->_parent->vscroll)
    _parent->_parent->vscroll->setPositionMin();*/
}



void ixTxtData::ViewPos::moveToCursor() {
  line= _parent->cur.line;
  pLine= _parent->cur.pLine;
  wline= _parent->cur.wline;
  pWline= _parent->cur.pWline;
  if(_parent->orientation& IX_TXT_HORIZONTAL) {
    pos= _parent->cur.y0;
    //if(_parent->_parent->vscroll)
      //_parent->_parent->vscroll->setPosition(pos);
  } else {
    pos= _parent->cur.x0;
    //if(_parent->_parent->hscroll)
      //_parent->_parent->hscroll->setPosition(pos);
  }
}


void ixTxtData::ViewPos::moveToScrollPosition() {
  bool horiz= _parent->orientation& IX_TXT_HORIZONTAL;
  ixScroll *scr= (horiz? _parent->_parent->vscroll: _parent->_parent->hscroll);
  if(!scr) return;

  // 2 possibilities, pos is lower, pos is higher (3rd is equal, nothing needs be done)
  if(pos> scr->position)
    while(pos> scr->position)
      decreaseLine();

  else if(pos+ (horiz? pWline->dy: pWline->dx)< scr->position)
    while(pos+ (horiz? pWline->dy: pWline->dx)< scr->position)
      advanceLine();
}



void ixTxtData::ViewPos::moveRelativeToCursor(float in_pixels) {
  bool horiz= _parent->orientation& IX_TXT_HORIZONTAL;
  moveToCursor();

  // move before the cursor
  if(in_pixels< 0) {
    in_pixels= -in_pixels;
    while(in_pixels>= (horiz? pWline->dy: pWline->dx)) {
      in_pixels-= (horiz? pWline->dy: pWline->dx);
      decreaseLine();
    }

  // move after the cursor
  } else if(in_pixels> 0) {
    while(in_pixels>= (horiz? pWline->dy: pWline->dx)) {
      in_pixels-= (horiz? pWline->dy: pWline->dx);
      advanceLine();
    }
  }
}












