#include "ix/ix.h"
#include "ix/winSys/_privateDef.h"


/* TODO:
 - maybe more text position options
 - unsigned manualOff: 1;      // toggled button: when pressed again, can it be toggled off, or it will stay on <<< possible


*/



ixButton::ixButton(): usage(this), ixBaseWindow(&is, &usage) {
  _type= ixeWinType::button;
  textX= textY= 0;
  _specialAction= 0;
}


ixButton::~ixButton() {
  delData();
}


void ixButton::delData() {
  ixBaseWindow::delData();
  text.delData(); 
  is.delData();
  usage.delData();
  textX= textY= 0;
}






void ixButton::setTextCentered(cchar *in_text) {
  text= in_text;
  
  float dx= font.getTextLen(in_text, 0);  // ixPrint::getTextLen(in_text, 0, font.selFont);
  float dy= font.getCharDy();             // ixPrint::getCharDy(font.selFont)* font;

  textX= (float)((int)(pos.dx- dx)/ 2);
  textY= (float)((int)(pos.dy- dy)/ 2);
}







// DRAW func ========================------------------------------------
#ifdef IX_USE_OPENGL
void ixButton::_glDraw(Ix *in_ix, ixWSsubStyleBase *dummy) {
  error.makeme();
}
#endif

#ifdef IX_USE_VULKAN
void ixButton::_vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *dummy) {
  ixBaseWindow::_vkDraw(in_cmd, in_ix, (is.pressed? stylePressed: null));

  if(!is.visible) return;
  if(!_clip.exists()) return;

  rectf r; getPosVD(&r);

  rectf clp(r);
  clp.intersectRect(_clip);
  in_ix->vki.cmdScissor(in_cmd, &clp);

  in_ix->pr.style= &font;
  in_ix->pr.txt2f(r.x0+ textX, r.y0+ textY, text);

  // THE WINDOW COULD HAVE CHILDREN, THEY'RE NOT DRAWN. <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
}
#endif





// UPDATE func =============================--------------------------------


// special actions that must be done the next time update is called
#define _SPECIAL_NONE         0x0000  
#define _SPECIAL_ACTIVATE_OFF 0x0001  // normal button -> reset activate next update() - a normal button stays activated one time, after the update is called
//#define _SPECIAL_BLABLA       0x0002


void ixButton::_doSpecialAction() {
  if(_specialAction== _SPECIAL_NONE) return;

  if(_specialAction== _SPECIAL_ACTIVATE_OFF) {
    is.activated= false;
    _specialAction= _SPECIAL_NONE;
    return;
  }
}


void ixButton::setActivate(bool in_b) {
  is.activated= in_b;
  is.pressed= in_b;
}














// UPDATE func ==========================---------------------------

bool ixButton::_update(bool in_updateChildren) {
  _doSpecialAction();
  if(!is.visible) return false;
  rectf r;
  bool inside;

  /// update it's children first
  if(in_updateChildren)
    if(_updateChildren())
      return true;
  
  float mx= _scaleDiv(in.m.x), my= _scaleDiv(in.m.y);
  getPosVD(&r);
  inside= r.inside(mx, my);

  // an operation is in progress
  if(Ix::wsys()._op.win) {
    if(Ix::wsys()._op.win != this) return false;

    // button activate / toggle
    if(Ix::wsys()._op.mLclick) {

      /// recheck the pressed state depending on the mouse position
      if(!inside)
        is.pressed= is.activated; //(is.activated? false: true);
      else
        is.pressed= !is.activated; //(is.activated? true: false);

      Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);

      // the left mouse button is depressed -> action is needed
      if(!in.m.but[0].down) {

        /// activate (deactivate) button - if the mouse was depressed in the button's area
        if(inside) {

          // toggled button state switch
          if(usage.toggleable) {
            is.activated= !is.activated; //(is.activated? false: true);
            Ix::wsys()._op.delData();
            // AN ACTION WAS PERFORMED -> THIS MUST BE SENT SOMEWHERE

            if(onActivate&& is.activated) 
              onActivate(this);

            return true;
            
          // normal button activation
          } else {
            is.activated= true;           // MUST STAY ACTIVATED ONLY UNTIL THE ACTION WAS PROCESSED
            is.pressed= false;

            if(onActivate&& is.activated) 
              onActivate(this);

            Ix::wsys()._op.delData();
            _specialAction= _SPECIAL_ACTIVATE_OFF;
            // AN ACTION WAS PERFORMED -> THIS MUST BE SENT SOMEWHERE
            return true;
          }



        /// reset the operation, if the mouse was depressed not in the button area
        } else {
          is.pressed= is.activated; //(is.activated? false: true);
          Ix::wsys()._op.delData();

          /// still the mouse was used with this window, in this case
          return true;
        }
      } /// mouse button depressed

      return true;  /// mouse button hold
    } /// an mouse left click operation is in progress



  // no operation is in progress
  } else if(inside) {
    if(in.m.but[0].down) {
      //if(inside) {
        is.pressed= !is.activated; //(is.activated? false: true);

        Ix::wsys()._op.mLclick= true;
        Ix::wsys()._op.win= this;
        Ix::wsys().bringToFront(this);

        Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
        return true;
      //}
    } /// left mouse button is being pressed
  } /// operation in progress / no operation in progress

  if(is.activated) {
    _colorToUse= (inside? &colorHover: &colorFocus);;
    _colorBRDtoUse= &colorBRDfocus;
  } else {
    _colorToUse= (inside? &colorHover: &color);
    _colorBRDtoUse= &colorBRD;
  }

  return ixBaseWindow::_update(false);
}












