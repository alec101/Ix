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
  usage.setRadioCircle(true);

  selNr= -1;
  sel= null;
  buttonDx= buttonDy= 0;
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

  int32 ix, iy;
  ((ixRadioButton *)_win)->_computeInitialX0Y0(&ix, &iy);

  listHeading= in_ori;
  
  ((ixRadioButton *)_win)->_computeRects(ix, iy);
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


void ixRadioButton::_computeInitialX0Y0(int32 *out_x0, int32 *out_y0) {
  if(usage.listHeading== DOWN) {
    if(out_x0) *out_x0= pos.x0;
    if(out_y0) *out_y0= pos.y0;
  } else if(usage.listHeading== UP) {
    if(out_x0) *out_x0= pos.x0;
    if(out_y0) *out_y0= pos.ye;
  } else if(usage.listHeading== RIGHT) {
    if(out_x0) *out_x0= pos.x0;
    if(out_y0) *out_y0= pos.y0;
  } else if(usage.listHeading== LEFT) {
    if(out_x0) *out_x0= pos.xe;
    if(out_y0) *out_y0= pos.y0;
  }
}


void ixRadioButton::_computeRects(int32 in_initialX0, int32 in_initialY0) {
  if(in_initialX0== INT32_MIN && in_initialY0== INT32_MIN)
    _computeInitialX0Y0(&in_initialX0, &in_initialY0);

  ixWinRadioData *p= (ixWinRadioData *)buttonList.first;
  pos.x0= pos.xe= in_initialX0, pos.y0= pos.ye= in_initialY0;

  for(; p; p= (ixWinRadioData *)p->next) {
    if(usage.listHeading== DOWN) {
      pos.xe= pos.x0+ buttonDx;     // << useless for every iteration but i guess it fits here so i don't make another if/elseif
      pos.ye+= buttonDy;
      p->pos.set(pos.x0, pos.ye- buttonDy, pos.xe, pos.ye);
    } else if(usage.listHeading== UP) {
      pos.xe= pos.x0+ buttonDx;
      pos.y0-= buttonDy;
      p->pos.set(pos.x0, pos.y0, pos.xe, pos.y0+ buttonDy);
    } else if(usage.listHeading== RIGHT) {
      pos.xe+= buttonDx;
      pos.ye= pos.y0+ buttonDy;
      p->pos.set(pos.xe- buttonDx, pos.y0, pos.xe, pos.ye);
    } else if(usage.listHeading== LEFT) {
      pos.x0-= buttonDx;
      pos.ye= pos.y0+ buttonDy;
      p->pos.set(pos.x0, pos.y0, pos.x0+ buttonDx, pos.ye);
    } /// list heading
  } /// for each button in radio list
  pos.compDeltas();
}









//  ##    ##    ######      ######        ####      ########    ########
//  ##    ##    ##    ##    ##    ##    ##    ##       ##       ##
//  ##    ##    ######      ##    ##    ##    ##       ##       ######      func
//  ##    ##    ##          ##    ##    ########       ##       ##
//    ####      ##          ######      ##    ##       ##       ########

bool ixRadioButton::_update(bool in_mIn,bool updateChildren) {
  if(!is.visible) return false;

  recti r; getVDcoordsRecti(&r);
  // update it's children first
  if(updateChildren)
    if(_updateChildren((in_mIn? r.inside(in.m.x, in.m.y): false)))
      return true;


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
          recti b(p->pos);
          b.moveD(hook.pos.x, hook.pos.y);

          if(b.inside(in.m.x, in.m.y)) {
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
  } else if(in_mIn) {
    if(in.m.but[0].down) {
      if(r.inside(in.m.x, in.m.y)) {
        //is.pressed= !is.activated; //(is.activated? false: true);

        Ix::wsys()._op.mLclick= true;
        Ix::wsys()._op.win= this;
        Ix::wsys().bringToFront(this);
        Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
        return true;
      }
    } /// left mouse button is being pressed
  } /// operation in progress / no operation in progress

  
  // base window update, at this point - no childrens tho, those were updated first
  return ixBaseWindow::_update(in_mIn, false);
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

  in_ix->pr.style= &font;
  int32 charDy= ixPrint::getCharDy(font.selFont);
  int32 prx= 0, pry= 0;
  ixWinRadioData *p= (ixWinRadioData *)buttonList.first;
  ixWSgenericStyle *s= (ixWSgenericStyle *)(in_style? in_style: parent->style);
  ixTexture *t= s->parent->getTexture(in_ix);


  in_ix->vki.cmdScissor(in_cmd, &_clip);

  // radio control with circle
  if(usage.radio) {
    in_ix->vki.draw.circle.flagTexture(false);
    in_ix->vki.draw.circle.flagDisabled(is.disabled);
    in_ix->vki.draw.circle.push.color= font.color1; // >>> (is.disabled? _getDisableColor(font.color): font.color);

    int32 cDiameter= MIN(buttonDx, buttonDy)- 2;  /// circle diameter
    recti c;                                      /// circle rectangle

    if(p) {
      if(usage.textHeading== DOWN) {
        c.set(p->pos.x0+ 1,            p->pos.y0+ 1,            p->pos.xe- 1,            p->pos.y0+ 1+ cDiameter);
        prx= (buttonDx- charDy)/ 2;
        if(prx< 0) prx= 0;
        prx+= p->pos.x0;
        pry= c.y0- 1;
        
      } else if(usage.textHeading== UP) {
        c.set(p->pos.x0+ 1,            p->pos.ye- 1- cDiameter, p->pos.xe- 1,            p->pos.ye- 1);
        prx= (buttonDx- charDy)/ 2;
        if(prx< 0) prx= 0;
        prx+= p->pos.x0;
        pry= c.ye+ 1;

      } else if(usage.textHeading== LEFT) {
        c.set(p->pos.xe- 1- cDiameter, p->pos.y0+ 1,            p->pos.xe- 1,            p->pos.ye- 1);
        prx= c.x0- 1;
        pry= (buttonDy- charDy)/ 2;
        if(pry< 0) pry= 0;
        pry+= p->pos.y0;

      } else if(usage.textHeading== RIGHT) {
        c.set(p->pos.x0+ 1,            p->pos.y0+ 1,            p->pos.x0+ 1+ cDiameter, p->pos.ye- 1);
        prx= c.xe+ 1;
        pry= (buttonDy- charDy)/ 2;
        if(pry< 0) pry= 0;
        pry+= p->pos.y0;

      } else error.detail("Unknown text heading", __FUNCTION__, __LINE__);

      c.moveD(hook.pos.x, hook.pos.y);
      //c.moveD(pos.x0, pos.y0);
      prx+= hook.pos.x;
      pry+= hook.pos.y;
    }

    while(p) {
      in_ix->vki.draw.circle.setPosR(c);
      float cWidth= (float)c.dx/ 10;
      if(cWidth< 1) cWidth= 1;
      in_ix->vki.draw.circle.push.hollow= (sel== p? -1.0f: cWidth);

      in_ix->vk.CmdBindPipeline(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.circle.sl->vk->pipeline);
      in_ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.circle.sl->vk->pipelineLayout, 0, 1, &in_ix->vki.glb[in_ix->vki.fi]->set->set, 0, nullptr);
      in_ix->vki.draw.circle.cmdTexture(in_cmd, t);
      in_ix->vki.draw.circle.cmdPushAll(in_cmd);
      in_ix->vki.draw.circle.cmdDraw(in_cmd);
      
      in_ix->pr.txt32_2i(prx, pry, p->text);

      if(usage.listHeading== DOWN) {
        c.moveD(0, buttonDy);
        pry+= buttonDy;
      } else if(usage.listHeading== UP) {
        c.moveD(0, -buttonDy);
        pry-= buttonDy;
      } else if(usage.listHeading== LEFT) {
        c.moveD(-buttonDx, 0);
        prx-= buttonDx;
      } else if(usage.listHeading== RIGHT) {
        c.moveD(buttonDx, 0);
        prx+= buttonDx;
      }

      p= (ixWinRadioData *)p->next;
    }

  // radio control with whole buttons, no circles
  } else {

    if(p) {
      if(usage.textHeading== DOWN) {
        prx= (buttonDx- charDy)/ 2;
        if(prx< 0) prx= 0;
        prx+= p->pos.x0;
        pry= p->pos.y0;

      } else if(usage.textHeading== UP) {
        prx= (buttonDx- charDy)/ 2;
        if(prx< 0) prx= 0;
        prx+= p->pos.x0;
        pry= p->pos.ye;

      } else if(usage.textHeading== LEFT) {
        prx= p->pos.xe;
        pry= (buttonDy- charDy)/ 2;
        if(pry< 0) pry= 0;
        pry+= p->pos.y0;

      } else if(usage.textHeading== RIGHT) {
        prx= p->pos.x0;
        pry= (buttonDy- charDy)/ 2;
        if(pry< 0) pry= 0;
        pry+= p->pos.y0;
      }

      
      while(p) {
        in_ix->pr.txt32_2i(prx, pry, p->text);

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
  /*
  int32 charDy= ixPrint::getCharDy(font.selFont);
  int32 prx, pry;
  ixWinRadioData *p= (ixWinRadioData *)buttonList.first;

  in_ix->pr.style= &font;
  //in_ix->pr.setScissor(&_clip);

  // radio control with circle
  if(usage.radio) {
    in_ix->glo.draw.circle.useProgram();
    in_ix->glo.draw.circle.disableTexture();
    in_ix->glo.draw.circle.delClipPlane();
    in_ix->glo.draw.circle.setClipPlaneR(_clip);
    //in_ix->glo.draw.circle.setColor(1.0f, 1.0f, 1.0f, 1.0f);
    in_ix->glo.draw.circle.setColorv(is.disabled? _getDisableColor(font.color1): font.color1);
    

    int32 cDiameter= MIN(buttonDx, buttonDy)- 2;  /// circle diameter
    recti c;                                      /// circle rectangle

    if(p) {
      if(usage.textHeading== DOWN) {
        c.set(p->x0+ 1, p->xe- 1, p->ye- 1- cDiameter, p->ye- 1);
        prx= (buttonDx- charDy)/ 2;
        if(prx< 0) prx= 0;
        prx+= p->x0;
        pry= c.y0- 1;
        
      } else if(usage.textHeading== UP) {
        c.set(p->x0+ 1, p->xe- 1, p->y0+ 1, p->y0+ 1+ cDiameter);
        prx= (buttonDx- charDy)/ 2;
        if(prx< 0) prx= 0;
        prx+= p->x0;
        pry= c.ye+ 1;

      } else if(usage.textHeading== LEFT) {
        c.set(p->xe- 1- cDiameter, p->xe- 1, p->y0+ 1, p->ye- 1);
        prx= c.x0- 1;
        pry= (buttonDy- charDy)/ 2;
        if(pry< 0) pry= 0;
        pry+= p->y0;

      } else if(usage.textHeading== RIGHT) {
        c.set(p->x0+ 1, p->x0+ 1+ cDiameter, p->y0+ 1, p->ye- 1);
        prx= c.xe+ 1;
        pry= (buttonDy- charDy)/ 2;
        if(pry< 0) pry= 0;
        pry+= p->y0;

      }
      c.moveD(hook.pos.x, hook.pos.y);
      //c.moveD(pos.x0, pos.y0);
      prx+= hook.pos.x;
      pry+= hook.pos.y;
    }



    while(p) {
      
      //glUseProgram(sl->id);
      //sl->circle2(c.x0, c.y0, c.xe, c.ye, (sel== p? true: false));

      in_ix->glo.draw.circle.useProgram();
      in_ix->glo.draw.circle.setCoordsD((float)c.x0, (float)c.y0, (float)c.dx, (float)c.dy);
      in_ix->glo.draw.circle.setFilled(sel== p);
      in_ix->glo.draw.circle.render();
      
      in_ix->pr.txt32_2i(prx, pry, p->text);


      if(usage.listHeading== DOWN) {
        c.moveD(0, -buttonDy);
        pry-= buttonDy;
      } else if(usage.listHeading== UP) {
        c.moveD(0, buttonDy);
        pry+= buttonDy;
      } else if(usage.listHeading== LEFT) {
        c.moveD(-buttonDx, 0);
        prx-= buttonDx;
      } else if(usage.listHeading== RIGHT) {
        c.moveD(buttonDx, 0);
        prx+= buttonDx;
      }


      p= (ixWinRadioData *)p->next;
    }

  // radio control with whole buttons, no circles
  } else {

    //recti r;
    if(p) {
      if(usage.textHeading== DOWN) {
        prx= (buttonDx- charDy)/ 2;
        if(prx< 0) prx= 0;
        prx+= p->x0;
        pry= p->ye;

      } else if(usage.textHeading== UP) {
        prx= (buttonDx- charDy)/ 2;
        if(prx< 0) prx= 0;
        prx+= p->x0;
        pry= p->y0;

      } else if(usage.textHeading== LEFT) {
        prx= p->xe;
        pry= (buttonDy- charDy)/ 2;
        if(pry< 0) pry= 0;
        pry+= p->y0;

      } else if(usage.textHeading== RIGHT) {
        prx= p->x0;
        pry= (buttonDy- charDy)/ 2;
        if(pry< 0) pry= 0;
        pry+= p->y0;

      }
    }

    while(p) {
      in_ix->pr.txt32_2i(prx, pry, p->text);
      

      if(usage.listHeading== DOWN) {
        pry-= buttonDy;
      } else if(usage.listHeading== UP) {
        pry+= buttonDy;
      } else if(usage.listHeading== LEFT) {
        prx+= buttonDx;
      } else if(usage.listHeading== RIGHT) {
        prx-= buttonDx;
      }
      p= (ixWinRadioData *)p->next;
    }
  } /// radioCircle or not
  */
}
#endif /// IX_USE_OPENGL


















