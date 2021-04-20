#include "ix/ix.h"
#include "ix/winSys/_privateDef.h"


/*
- buttons must be able to be toggle buttons, not only something you press and imediatly depresses



*/



ixButton::ixButton() {
  ixBaseWindow();
  _type= _IX_BUTTON;
  textX= textY= 0;
  //font= null;
}


ixButton::~ixButton() {
  delData();
}


void ixButton::delData() {
  ixBaseWindow::delData();
  text.delData(); 
  is.delData();
  usage.delData();
  //font= null;
  textX= textY= 0;
}


/*
void ixButton::setText(cchar *s) {
  text= s;
}
*/



void ixButton::setTextCentered(cchar *in_text) {
  text= in_text;
  
  int32 dx= ixPrint::getTextLen(in_text, 0, font.selFont);
  int32 dy= ixPrint::getCharDy(font.selFont);

  textX= (pos.dx- dx)/ 2;
  textY= (pos.dy- dy)/ 2;
}







// DRAW func ========================------------------------------------
#ifdef IX_USE_OPENGL
void ixButton::_glDraw(Ix *in_ix, ixWSsubStyleBase *dummy) {
  /*
  // THE WINDOW COULD HAVE CHILDREN, THEY'RE NOT DRAWN. <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  ixBaseWindow::_glDraw(in_ix, (is.pressed? stylePressed: null));

  recti clp;
  getVDcoordsRecti(&clp);
  clp.intersectRect(_clip);

  in_ix->pr.style= &font;
  //in_ix->pr.setScissor(&clp);
  in_ix->pr.txt2i(clp.x0+ textX, clp.y0+ textY, text);
  */
}
#endif

#ifdef IX_USE_VULKAN
void ixButton::_vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *dummy) {
  ixBaseWindow::_vkDraw(in_cmd, in_ix, (is.pressed? stylePressed: null));

  if(!is.visible) return;
  if(!_clip.exists()) return;

  int32 _x, _y;
  getVDcoords2i(&_x, &_y);
  recti clp;
  getVDcoordsRecti(&clp);
  clp.intersectRect(_clip);
  in_ix->vki.cmdScissor(in_cmd, &clp);

  in_ix->pr.style= &font;
  in_ix->pr.txt2i(_x+ textX, _y+ textY, text);

  // THE WINDOW COULD HAVE CHILDREN, THEY'RE NOT DRAWN. <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
}
#endif





// UPDATE func =============================--------------------------------


// special actions that must be done the next time update is called
#define _SPECIAL_NONE         0x0000  
#define _SPECIAL_ACTIVATE_OFF 0x0001  // normal button -> reset activate next update() - a normal button stays activated one time, after the update is called
//#define _SPECIAL_BLABLA       0x0002
uint16 _specialAction= 0;

void _doSpecialAction(ixButton *out_but) {
  if(_specialAction== _SPECIAL_NONE) return;

  if(_specialAction== _SPECIAL_ACTIVATE_OFF) {
    out_but->is.activated= false;
    _specialAction= _SPECIAL_NONE;
    return;
  }
}

bool ixButton::_update(bool in_mIn, bool in_updateChildren) {
  _doSpecialAction(this);

  recti r; getVDcoordsRecti(&r);
  bool inside= r.inside(in.m.x, in.m.y);

  /// update it's children first
  if(in_updateChildren)
    if(_updateChildren((in_mIn? inside: false)))
      return true;

  //int32 x= hook.pos.x+ pos.x0, y= hook.pos.y+ pos.y0;



  // an operation is in progress
  if(Ix::wsys()._op.win) {
    if(Ix::wsys()._op.win != this) return false;

    // button activate / toggle
    if(Ix::wsys()._op.mLclick) {

      /// recheck the pressed state depending on the mouse position
      //if(!mPos(x, y, pos.dx, pos.dy))
      if(!r.inside(in.m.x, in.m.y))
        is.pressed= is.activated; //(is.activated? false: true);
      else
        is.pressed= !is.activated; //(is.activated? true: false);

      // the left mouse button is depressed -> action is needed
      if(!in.m.but[0].down) {

        /// activate (deactivate) button - if the mouse was depressed in the button's area
        //if(mPos(x, y, pos.dx, pos.dy)) {
        if(r.inside(in.m.x, in.m.y)) {

          // toggled button state switch
          if(usage.toggleable) {
            is.activated= !is.activated; //(is.activated? false: true);
            Ix::wsys()._op.delData();
            // AN ACTION WAS PERFORMED -> THIS MUST BE SENT SOMEWHERE
            return true;

          // normal button activation
          } else {
            is.activated= true;           // MUST STAY ACTIVATED ONLY UNTIL THE ACTION WAS PROCESSED
            is.pressed= false;
            Ix::wsys()._op.delData();
            _specialAction= _SPECIAL_ACTIVATE_OFF;
            // AN ACTION WAS PERFORMED -> THIS MUST BE SENT SOMEWHERE
            return true;
          }



        /// reset the operation, if the mouse was depressed not in the button area
        } else {
          is.pressed= is.activated; //(is.activated? false: true);
          Ix::wsys()._op.delData();
        }
      } /// mouse button depressed
    } /// an mouse left click operation is in progress



  // no operation is in progress
  } else if(in_mIn) {
    if(in.m.but[0].down) {
      //if(mPos(x, y, pos.dx, pos.dy)) {
      if(r.inside(in.m.x, in.m.y)) {
        is.pressed= !is.activated; //(is.activated? false: true);

        Ix::wsys()._op.mLclick= true;
        Ix::wsys()._op.win= this;
        Ix::wsys().bringToFront(this);
        return true;
      }
    } /// left mouse button is being pressed
    //_colorToUse= &colorHover;
  } /// operation in progress / no operation in progress

  if(is.activated) {
    _colorToUse= (inside? &colorHover: &colorFocus);;
    _colorBRDtoUse= &colorBRDfocus;
  } else {
    _colorToUse= (inside? &colorHover: &color);
    _colorBRDtoUse= &colorBRD;
  }

  return ixBaseWindow::_update(in_mIn, false);
}












