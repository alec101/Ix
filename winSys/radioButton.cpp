#include "ix/ix.h"
#include "ix/winSys/_privateDef.h"


// TODO:
/*
  - if no button Dx/ Dy is set, use the first button's font size, text length, <<< MAYBE ... OR JUST CLEAR THAT




*/




ixRadioButton::ixRadioButton(): usage(this), ixBaseWindow(&is, &usage) {
  _type= ixeWinType::radioButton;

  usage.setTextHeading(RIGHT);
  usage.setListHeading(DOWN);
  usage.radio= 1;

  _unit= 1.0f;
  selNr= -1;
  sel= null;
  buttonDx= buttonDy= 0.0f;
}


ixRadioButton::~ixRadioButton() {
  delData();
}


void ixRadioButton::delData() {
  ixBaseWindow::delData();
}







///=======================///
// radioButton USAGE class //
///=======================///

void ixRadioButton::Usage::setTextHeading(ixOrientation in_ori) {
  if(in_ori== HORIZONTAL) in_ori= RIGHT;
  if(in_ori== VERTICAL) in_ori= DOWN;
  textHeading= in_ori;
  ((ixRadioButton *)_win)->font.orientation= in_ori;
}


void ixRadioButton::Usage::setListHeading(ixOrientation in_ori) {
  if(in_ori== HORIZONTAL) in_ori= RIGHT;
  if(in_ori== VERTICAL) in_ori= DOWN;

  float ix, iy;
  ((ixRadioButton *)_win)->_computeInitialX0Y0(&ix, &iy);

  listHeading= in_ori;
  
  ((ixRadioButton *)_win)->_computeRects(&ix, &iy);
}


///=================///
// RADIOBUTTON funcs //
///=================///



// adds a button to the radio control
void ixRadioButton::addRadioButton(cchar *in_name, bool in_implicitSelect) {
  ixWinRadioData *b= new ixWinRadioData;
  b->text= in_name;
  buttonList.add(b);

  _computeRects();

  if(in_implicitSelect)
    sel= b,
    selNr= buttonList.nrNodes- 1;
}


// deletes button named <name> from the radio control
void ixRadioButton::delRadioButtonTxt(cchar *in_name) {
  ixWinRadioData *p= (ixWinRadioData *)buttonList.first;
  for(int a= buttonList.nrNodes; a> 0; a--, p= (ixWinRadioData *)p->next)
    if(p->text== in_name) {
      
      if(sel== p)     /// adjust selection
        selectRadioButtonn(-1);

      buttonList.del(p);

      if(sel)         /// adjust selection
        selNr= buttonList.search(sel);

      _computeRects();

      return;
    }
}

// deletes button number <n> from the radio control
void ixRadioButton::delRadioButtonn(int in_n) {
  if((in_n> (int)buttonList.nrNodes- 1) || (in_n< 0))
    return;

  ixWinRadioData *p= (ixWinRadioData *)buttonList.get(in_n);
  if(p== null) return;

  if(sel== p)     /// adjust selection
    selectRadioButtonn(-1);

  buttonList.del(p);
  _computeRects();

  if(sel)
    selNr= buttonList.search(sel);
}

// sets the button with <in_buttonText> as activated
void ixRadioButton::selectRadioButtonTxt(cchar *in_buttonText) {
  // special case, select none / null
  if(in_buttonText== null) {
    sel= null, selNr= -1;
    return;
  }

  ixWinRadioData *p= (ixWinRadioData *)buttonList.first;
  for(uint a= 0; a< buttonList.nrNodes; a++, p= (ixWinRadioData *)p->next)
    if(p->text== in_buttonText) {
      sel= p;
      selNr= a;
      return;
    }
}

// sets the button number <in_number> as activated
void ixRadioButton::selectRadioButtonn(int in_n) {
  if((in_n> (int)buttonList.nrNodes- 1) || (in_n< 0)) {
    sel= null, selNr= -1;
    return;
  }

  ixWinRadioData *p= (ixWinRadioData *)buttonList.get(in_n);
  sel= p;
  selNr= in_n;
}



void ixRadioButton::changeRadioButtoni(int in_n, cchar *newName) {
  if((in_n> (int)buttonList.nrNodes- 1) || (in_n< 0))
    return;

  ixWinRadioData *p= (ixWinRadioData *)buttonList.get(in_n);
  p->text= newName;
}


void ixRadioButton::changeRadioButton(cchar *in_oldName, cchar *in_newName) {
  ixWinRadioData *p= (ixWinRadioData *)buttonList.first;
  for(int a= buttonList.nrNodes; a> 0; a--, p= (ixWinRadioData *)p->next)
    if(p->text== in_oldName) {
      p->text= in_newName;
      return;
    }
}





void ixRadioButton::_computeAll() {
  ixBaseWindow::_computeAll();
  _computeRects();
  _unit= (float)_getUnit();
}

void ixRadioButton::_computeAllDelta(float x, float y) {
  ixBaseWindow::_computeAllDelta(x, y);
  _computeRects();
  _unit= (float)_getUnit();
}


void ixRadioButton::_computeInitialX0Y0(float *out_x0, float *out_y0) {
  if(usage.listHeading== DOWN) {
    if(out_x0) *out_x0= pos.x0;
    if(out_y0) *out_y0= pos.y0;
  } else if(usage.listHeading== UP) {
    if(out_x0) *out_x0= pos.x0;
    if(out_y0) *out_y0= pos.y0+ pos.dy;
  } else if(usage.listHeading== RIGHT) {
    if(out_x0) *out_x0= pos.x0;
    if(out_y0) *out_y0= pos.y0;
  } else if(usage.listHeading== LEFT) {
    if(out_x0) *out_x0= pos.x0+ pos.dx;
    if(out_y0) *out_y0= pos.y0;
  }
}


void ixRadioButton::_computeRects(float *in_initialX0, float *in_initialY0) {
  float _x, _y;
  rectf r;
  if(in_initialX0== null && in_initialY0== null)
    _computeInitialX0Y0(&_x, &_y);
  else
    _x= *in_initialX0, _y= *in_initialY0;

  r.set(_x, _y, _x, _y);  /// start from zero size, expand it with every button

  ixWinRadioData *p= (ixWinRadioData *)buttonList.first;
  for(; p; p= (ixWinRadioData *)p->next) {
    if(usage.listHeading== DOWN) {
      r.xe= r.x0+ buttonDx;     // << useless for every iteration but i guess it fits here so i don't make another if/elseif
      r.ye+= buttonDy;
      p->pos.set(r.x0, r.ye- buttonDy, r.xe, r.ye);
    } else if(usage.listHeading== UP) {
      r.xe= r.x0+ buttonDx;
      r.y0-= buttonDy;
      p->pos.set(r.x0, r.y0, r.xe, r.y0+ buttonDy);
    } else if(usage.listHeading== RIGHT) {
      r.xe+= buttonDx;
      r.ye= r.y0+ buttonDy;
      p->pos.set(r.xe- buttonDx, r.y0, r.xe, r.ye);
    } else if(usage.listHeading== LEFT) {
      r.x0-= buttonDx;
      r.ye= r.y0+ buttonDy;
      p->pos.set(r.x0, r.y0, r.x0+ buttonDx, r.ye);
    } /// list heading
  } /// for each button in radio list

  pos.setD(pos.x0, pos.y0, r.dx, r.dy);
}









//  ##    ##    ######      ######        ####      ########    ########
//  ##    ##    ##    ##    ##    ##    ##    ##       ##       ##
//  ##    ##    ######      ##    ##    ##    ##       ##       ######      func
//  ##    ##    ##          ##    ##    ########       ##       ##
//    ####      ##          ######      ##    ##       ##       ########

bool ixRadioButton::_update(bool updateChildren) {
  if(!is.visible) return false;

  rectf posvd, b;
  bool insideThis;
  float mx, my;
  // update it's children first
  if(updateChildren)
    if(_updateChildren())
      return true;

  mx= _scaleDiv(in.m.x), my= _scaleDiv(in.m.y);
  //mdx= _scaleDiv(in.m.dx), mdy= _scaleDiv(in.m.dy);
  getPosVD(&posvd);
  insideThis= posvd.inside(mx, my);

  // FUNCTION CORE

  // an operation is in progress
  if(Ix::wsys()._op.win) {
    if(Ix::wsys()._op.win != this) return false;

    // button activate / toggle
    if(Ix::wsys()._op.mLclick) {

      // the left mouse button is depressed -> action is needed
      if(!in.m.but[0].down) {
        int a= 0;
        for(ixWinRadioData *p= (ixWinRadioData *)buttonList.first; p; p= (ixWinRadioData *)p->next, a++) {
          b= p->pos;
          b.moveD(hook.pos.x, hook.pos.y);

          if(b.inside(mx, my)) {
            sel= p;
            selNr= a;
            Ix::wsys()._op.delData();
            Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
            return true;
          }
        }

      } /// left click depressed
    } /// left click operation

  // no operation is in progress
  } else if(insideThis) {
    if(in.m.but[0].down) {
      //if(insideThis) {
        //is.pressed= !is.activated; //(is.activated? false: true);

        Ix::wsys()._op.mLclick= true;
        Ix::wsys()._op.win= this;
        Ix::wsys().bringToFront(this);
        Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
        return true;
      //}
    } /// left mouse button is being pressed
  } /// operation in progress / no operation in progress

  
  // base window update, at this point - no childrens tho, those were updated first
  return ixBaseWindow::_update(false);
}












// DDDDDD      RRRRRR        AAAA      WW      WW
// DD    DD    RR    RR    AA    AA    WW      WW
// DD    DD    RRRRRR      AA    AA    WW  WW  WW     func
// DD    DD    RR    RR    AAAAAAAA    WW  WW  WW
// DDDDDD      RR    RR    AA    AA      WW  WW



#ifdef IX_USE_VULKAN
void ixRadioButton::_vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style) {
  if(!_clip.exists()) return;
  if(!_inBounds(in_ix)) return;     // nothing to draw, the ix+gpu don't draw this window

  rectf posvd; getPosVD(&posvd);
  in_ix->pr.style= &font;
  float charDy= font.getCharDy();
  float prx= 0, pry= 0;
  ixWinRadioData *p= (ixWinRadioData *)buttonList.first;
  ixWSgenericStyle *s= (ixWSgenericStyle *)(in_style? in_style: parent->style);
  ixTexture *t= s->parent->getTexture(in_ix);


  in_ix->vki.cmdScissor(in_cmd, &_clip);

  // radio control with circle
  if(usage.radio) {
    in_ix->vki.draw.circle.flagTexture(false);
    in_ix->vki.draw.circle.flagDisabled(is.disabled);
    in_ix->vki.draw.circle.push.color= font.color1; // >>> (is.disabled? _getDisableColor(font.color): font.color);

    float cDiameter= MIN(buttonDx, buttonDy)- _unit;  /// circle diameter
    rectf c;                                      /// circle rectangle

    if(p) {
      if(usage.textHeading== DOWN) {
        c.set(p->pos.x0+ 1.0f,            p->pos.y0+ 1.0f,            p->pos.xe- 1.0f,            p->pos.y0+ 1.0f+ cDiameter);
        prx= (buttonDx- charDy)/ 2.0f;
        if(prx< 0.0f) prx= 0.0f;
        prx+= p->pos.x0;
        pry= c.y0- 1.0f;
        
      } else if(usage.textHeading== UP) {
        c.set(p->pos.x0+ 1.0f,            p->pos.ye- 1.0f- cDiameter, p->pos.xe- 1.0f,            p->pos.ye- 1.0f);
        prx= (buttonDx- charDy)/ 2.0f;
        if(prx< 0.0f) prx= 0.0f;
        prx+= p->pos.x0;
        pry= c.ye+ 1.0f;

      } else if(usage.textHeading== LEFT) {
        c.set(p->pos.xe- 1.0f- cDiameter, p->pos.y0+ 1.0f,            p->pos.xe- 1.0f,            p->pos.ye- 1.0f);
        prx= c.x0- 1.0f;
        pry= (buttonDy- charDy)/ 2.0f;
        if(pry< 0.0f) pry= 0.0f;
        pry+= p->pos.y0;

      } else if(usage.textHeading== RIGHT) {
        c.set(p->pos.x0+ 1.0f,            p->pos.y0+ 1.0f,            p->pos.x0+ 1.0f+ cDiameter, p->pos.ye- 1.0f);
        prx= c.xe+ 1.0f;
        pry= (buttonDy- charDy)/ 2.0f;
        if(pry< 0.0f) pry= 0.0f;
        pry+= p->pos.y0;

      } else error.detail("Unknown text heading", __FUNCTION__, __LINE__);

      c.moveD(hook.pos.x, hook.pos.y);
      //c.moveD(pos.x0, pos.y0);
      prx+= hook.pos.x;
      pry+= hook.pos.y;
    }

    while(p) {
      in_ix->vki.draw.circle.setPosR(c);
      float cWidth= c.dx/ 10.0f;
      if(cWidth< 1.0f) cWidth= 1.0f;
      in_ix->vki.draw.circle.push.hollow= (sel== p? -1.0f: cWidth);

      in_ix->vk.CmdBindPipeline(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.circle.sl->vk->pipeline);
      in_ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.circle.sl->vk->pipelineLayout, 0, 1, &in_ix->vki.glb[in_ix->vki.fi]->set->set, 0, nullptr);
      in_ix->vki.draw.circle.cmdTexture(in_cmd, t);
      in_ix->vki.draw.circle.cmdPushAll(in_cmd);
      in_ix->vki.draw.circle.cmdDraw(in_cmd);
      
      in_ix->pr.txt32_2f(prx, pry, p->text);

      if(usage.listHeading== DOWN) {
        c.moveD(0.0f, buttonDy);
        pry+= buttonDy;
      } else if(usage.listHeading== UP) {
        c.moveD(0.0f, -buttonDy);
        pry-= buttonDy;
      } else if(usage.listHeading== LEFT) {
        c.moveD(-buttonDx, 0.0f);
        prx-= buttonDx;
      } else if(usage.listHeading== RIGHT) {
        c.moveD(buttonDx, 0.0f);
        prx+= buttonDx;
      }

      p= (ixWinRadioData *)p->next;
    }

  // radio control with whole buttons, no circles
  } else {

    if(p) {
      if(usage.textHeading== DOWN) {
        prx= (buttonDx- charDy)/ 2.0f;
        if(prx< 0.0f) prx= 0.0f;
        prx+= p->pos.x0;
        pry= p->pos.y0;

      } else if(usage.textHeading== UP) {
        prx= (buttonDx- charDy)/ 2.0f;
        if(prx< 0.0f) prx= 0.0f;
        prx+= p->pos.x0;
        pry= p->pos.ye;

      } else if(usage.textHeading== LEFT) {
        prx= p->pos.xe;
        pry= (buttonDy- charDy)/ 2.0f;
        if(pry< 0.0f) pry= 0.0f;
        pry+= p->pos.y0;

      } else if(usage.textHeading== RIGHT) {
        prx= p->pos.x0;
        pry= (buttonDy- charDy)/ 2.0f;
        if(pry< 0.0f) pry= 0.0f;
        pry+= p->pos.y0;
      }

      
      while(p) {
        in_ix->pr.txt32_2f(prx, pry, p->text);

        if(usage.listHeading== DOWN) {
          pry+= buttonDy;
        } else if(usage.listHeading== UP) {
          pry-= buttonDy;
        } else if(usage.listHeading== LEFT) {
          prx+= buttonDx;
        } else if(usage.listHeading== RIGHT) {
          prx-= buttonDx;
        }
        p= (ixWinRadioData *)p->next;
      }
    } /// p exists
  } /// radioCircle or not
}
#endif /// IX_USE_VULKAN


#ifdef IX_USE_OPENGL
void ixRadioButton::_glDraw(Ix *in_ix, ixWSsubStyleBase *in_style) {
  error.makeme();
}
#endif /// IX_USE_OPENGL


















