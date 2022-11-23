#include "ix/ix.h"
#include <stdarg.h>







// maybe just altering the glyph abit, the outline could happen way easier...
// https://blog.mapbox.com/drawing-text-with-signed-distance-fields-in-mapbox-gl-b0933af6f817
// for certain tex offset might not be any use (the func in the shader), you can just use texture func, i think, will see... it could be faster







/*

// Texture compression https://software.intel.com/en-us/articles/android-texture-compression < need to check this further

NOTE: fnt file will write floats; if problems arise the solution is to simply write the pixel locations as a integers

when a renderer will be made... unfortunately
print(text) -> search rendered texts by length first then by fast compare (do not unpack) -> if not found render it -> print it.
            -> on pre-rendered fonts, no problemoes
            database must hold string8's, so len var is used in compare

if a request for a specific unichar that is not loaded, loads that charset auto?

TODO:
 - a mutex would solve the thread safety

 - there's text orientation, but a simple rotation would be needed too (NEEDED FOR WINDOWS); this would have 0- 359 degrees, any degree

 - compressed texture or not? im tending towards not...

 - color for each vertex, for blending of colors, kewl effect!

 - check the list for missing characters (not 100% shure of that website that were copied from)
 - a loadFontForUnicodeChar(font name, size, unicode character) ?
*/

  /*
  ===============================================================
    PRINTING METHODS RESEARCH - EXPANDED INTO VBO USAGE METHODS
  ===============================================================

  !!! https://www.opengl.org/wiki/Buffer_Object_Streaming !!! very inportant

  so the colors are made to an array too. no more simple change of color.
  updating each text to the grcard is slow - but there are texts that are updated each frame
  there have to be a way to upload the text, and keep it until it is not needed

  so

  1. upload text - gen buffers

  n. delete buffers when is not needed.

  how do you know how much time that text is needed?
  when do you delete it, basically?

  maybe: create text, chainlist, delete text, from chain list?
  should this chainlist have a vbo ID (s) and maybe the whole text, in str8, even?       

  so there's gonna be a renderText() that returns an chainList object
  and print(object)

  render text should be advanced, with formatting, everything needed. renderNumber? renderText? renderFormatted? renderInt? renderDouble?
  renderInt would be nice for fast number texts... renderInt(v). print(coords, obj), print(obj) with print.x, print.y, print.z



  method 1. - render each text- per frame. PRO: easy to do / easy to print; CONS: slowest (vbo create+destroy= insane slow speed)
  method 2. - chainlist with rendered text. PRO: ttf compatible; CONS: clogier to print stuff (ids)/ creating VBOs is slow -mem allocs
  method 3. - 1 single id, there's 4 vertices anyway, modify before draw each vertex. STREAM_DRAW: updating the vbo for each char is the drag
  method 4. - 1 VBO, video mem alloc for 256 chars, if more than 256 chars, concat text, resend same text to print.
            pro: 1 update for each string
            cons: still update the vbo
  method 5. - 1 vbo per font page - created on font load - 1024 chars per vbo - 1024chars 4(vert) 4(color) 4(texcoord) 2(float)= 24576bytes
            - there has to be a translate for each draw
            - so it may be fast to select what to draw, but the translation severly kills any speed gains, i think
            - still changing vbo data multiple times per frame is bad


            !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!11
            UPDATE: there needs to be only tex coords, and a size of the character. the shader can do from this. and color, ofc

            1vbo        32768tex 32768vert= 65536 per Print page
            1vbo dxvert 32768tex 16386vert 
            1vbo+ 1ubo  32768tex 4096vert
            1ubo        16384tex 4096vert - TRYING THIS

            "
            gl_VertexID​
              the index of the vertex currently being processed. When using non-indexed rendering, it is the effective index
              of the current vertex (the number of vertices processed + the first​ value). For indexed rendering, it is the index
              used to fetch this vertex from the buffer.
              Note: gl_VertexID​ will have the base vertex applied to it.
            "

             layouts: 
            https://www.opengl.org/wiki/Layout_Qualifier_(GLSL)
            it's all play with the layouts

            so, something like vert1.xyz= Print::pos
            vert2= Print::pos+ 
            char wide must be found:
            checkout how wide the char is from the buffer should be vertID/4

            ogl 4.3 https://www.opengl.org/wiki/Shader_Storage_Buffer_Object - apple don't know this unfortunately


            !!!
            ogl 3.1 https://www.opengl.org/wiki/Uniform_Buffer_Object !!!!
            !!! FINNALY FOUND IT !!!

            try this:
            https://www.packtpub.com/books/content/opengl-40-using-uniform-blocks-and-uniform-buffer-objects
            hopefully (it IS possible) accessing arr[10].texX0 will work, the size of the array could be computed automatically from the size of the block


            uniform block{
              float x0[1024];
              flaot y0[];
              float xe[];
              float ye[];
              float size[];
            } block name

            or uniform block {
              struct {
                float x0, .... size;
              } bla[];
            } uniformName

            don't think blockname[1024] works, like in C

            check UNIFORM_ARRAY_STRIDE when creating the buffer https://www.opengl.org/registry/specs/ARB/uniform_buffer_object.txt

            dunno what to say anymore, unfortunately. this might not work, as glDrawElements


            this needs lots of thinking, it might work somehow, it really should


            vvv THIS MIGHT BE THE WINNER VVV
  method 6. - 2 or more buffers, like method 4, switch buffers. update 1 buffer data, draw with the other(s)

  method 7. - a pixel buffer object that sends data to an immutable object with server side commands (copySubData) - probly mutiple of these objects... 2-4-8
  method 8. - the invalidation technique. https://www.opengl.org/wiki/Buffer_Object_Streaming.
              
  
  best method utopia:
               -no vbo create
               -update only 1 time per frame
               -no matrix translates
               -

 all these methods must be done, tests to see exactly the speeds on each one. If only slight speed decrease on simple methods... so TEST!!!



CONCLUSIONS:
             - there should be a set number of VBO's. Create+destroy during a frame= kill the speed
             - 





  glGenBuffers(2, vertex+ color ids);
  glBindBuffer(GL_ARRAY_BUFFER, vboID[1]); // Bind our second Vertex Buffer Object  
  glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(GLfloat), colors, GL_STATIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW  
  glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0); // Set up our vertex attributes pointer  
  glEnableVertexAttribArray(1); // Enable the second vertex attribute array  



  shader storage blocks ogl4.3
   - variable namings, how to do querries - https://www.opengl.org/wiki/Program_Introspection#Interface_block_member_naming

  uniform blocks ogl3.2
   - variable namings, how to do querries - https://www.opengl.org/wiki/Program_Introspection#Interface_block_member_naming
   - array of blocks:
     uniform BlockName3 {
       int mem;
     } instanceName3[4];
     *Note: Block indices for arrays of blocks are not necessarily contiguous. So you have to ask for each one individually.*

*/





///=================================================================================================================///
// different printing methods - the best one must be choosen, the others are learing material - might not be deleted //
///=================================================================================================================///

/// start, end, startBytes, endBytes will be used if only a certain part of the string is needed to be printed
/// start and end can be used to print a certain number of unicode values
/// startBytes and endBytes will print a certain number of bytes from the string



#ifdef IX_USE_OPENGL

IF THIS WILL EVER BE DONE, IT NEEDS HEAVY WORK



//_ixM3 *_M3= null;
//_ixM5 *_M5= null;
//_ixM5U *_M5U= null;


// print private class
namespace _ixpr {
  


  static int glM;

  class glShader: public ixPrintShader {
  public:
    void txt(const void *, int, int, int, int, int);
    glShader(Ix *in_ix): ixPrintShader(in_ix) {}
  };


  class glM3: public glShader {
  public:
    uint32 VBOid;               /// 1 storage[20]: 12vert coords, 8 tex coords
    glVAO VAO;
    float bufferData[12+ 8];    /// VERTEX: 3float (x, y, z) * 4 vertices + TEXCOORDS 2float (x, y)* 4 vertices

    void initUniforms();
    void printChar(uint32 in_unicode);
    glM3(Ix *in_ix): glShader(in_ix), VBOid(0) {}
    ~glM3() { if(osi.glr) _delete(null); }
  protected:
    void _prePrintInit();
    void _afterPrint();
    void _create(_ixFPage *);
    void _delete(_ixFPage *);
    friend class ixPrint;
  };


  class glM5: public glShader {
  public:
    int u_pos, u_chDy;        // glsl uniform IDs
    //uint8 M5bufferData[];     /// keep data in memory too?

    // the VBO is on the font page struct - _ixFPage
    void printChar(uint32 in_unicode);
    void initUniforms();
    glM5(Ix *in_ix): glShader(in_ix), u_pos(-1), u_chDy(-1) {}
  protected:
    void _prePrintInit();
    void _afterPrint();
    void _create(_ixFPage *);
    void _delete(_ixFPage *);
    friend class ixPrint;
    friend class _ixFPage;
  };


  class glM5U: public glShader {
  public:
    int u_pos;        // glsl uniforms
    //uint8 M5bufferData[];       /// keep data in memory too?
    void printChar(uint32 in_unicode);
    void initUniforms();
    glM5U(Ix *in_ix): glShader(in_ix), u_pos(-1) {}
  protected:
    void _prePrintInit();
    void _afterPrint();
    void _create(_ixFPage *);
    void _delete(_ixFPage *);
    friend class ixPrint;
    friend class _ixFPage;
  };

} _ixPrint;



///===================================================================///
int _ixpr::glM= 55;     // SET THE METHOD HERE << 3= M3, 5= M5, 55=M5U //
///===================================================================///
/// connect with testChamber for speed tests:
int _ixPrintGlMethod= _ixpr::glM;





void glInit() {

  if(_ix->renOpenGL()) {
    // method 3
    if(_ixpr::glM== 3)  {
      if(_shader== null)
        _shader= new _ixpr::glM3(_ix);

      if(_shader) {
        if(_shader->gl->id== 0) {
          _shader->gl->load(Ix::Config::shaderDIR()+ "ixPrintM3v.glsl", Ix::Config::shaderDIR()+ "ixPrintM3f.glsl");
          if(!_shader->gl->build())
            error.console("ixPrint CRITICAL ERROR: could not load shader M3. aborting.", true);
          _shader->_print= this;
          _shader->initUniforms();
          _shader->_create(null);
        }
      }
    } /// method 3
  
    // method 5
    if(_ixpr::glM== 5) {
      if(_shader== null)
        _shader= new _ixpr::glM5(_ix);

      if(_shader) {
        if(_shader->gl->id== 0) {
          _shader->gl->load(Ix::Config::shaderDIR()+ "ixPrintM5v.glsl", Ix::Config::shaderDIR()+ "ixPrintM5f.glsl");
          if(!_shader->gl->build()) 
            error.console("ixPrint CRITICAL ERROR: could not load shader M5. aborting.", true);
          _shader->_print= this;
          _shader->initUniforms();
        }
      }
    } /// method 5

    // method 5Uniforms
    if(_ixpr::glM== 55) {
      if(_shader== null)
        _shader= new _ixpr::glM5U(_ix);

      if(_shader) {
        if(_shader->gl->id== 0) {
          _shader->gl->load(Ix::Config::shaderDIR()+ "ixPrintM5Uv.glsl", Ix::Config::shaderDIR()+ "ixPrintM5Uf.glsl");
          if(!_shader->gl->build())
            error.console("ixPrint CRITICAL ERROR: could not load shader M5U. aborting.", true);
          _shader->_print= this;
          _shader->initUniforms();
        }
      }
    } /// method 5Uniforms
  }
}



///================================================///
// -------======= PRINTING functions =======------- //
///================================================///

// print a unicode char, method 3 - only 1 stream VBO, 4 vert, update for each char
// PRO: no translate / small mem imprint / small and simple
// CONS: update a single VBO for each character - can (must test) be a big speed decrease

void _ixpr::glM3::initUniforms() {
  ixPrintShader::initUniforms();
}


void _ixpr::glM3::_create(_ixFPage *dummy) {
  if(!osi.glr) { error.console("Print::_M3::VBOidCreate() FAILED - no renderer active"); return; }

  // vertex array buffer create - VAO OBJECT MUST REPLACE THIS CODE
  if(!VAO.id)
    VAO.genArray();
  
  /// generate buffer
  glGenBuffers(1, &VBOid);
  glBindBuffer(GL_ARRAY_BUFFER, VBOid);
  //VAO.bindBuffer(GL_ARRAY_BUFFER, VBOid);
  glBufferData(GL_ARRAY_BUFFER, (4* 3+ 4* 2)* sizeof(GLfloat), null, GL_STREAM_DRAW); /// size of the vertex data and tex coods

  //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  VAO.bindAndVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  //glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)(4* 3* sizeof(GLfloat)));
  VAO.bindAndVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)(4* 3* sizeof(GLfloat)));
  VAO.enableVertexAttribArray(0);
  VAO.enableVertexAttribArray(1);

  // done
  glBindVertexArray(0);
  error.glError();
}


void _ixpr::glM3::_delete(_ixFPage *dummy) {
  if(VAO.id|| VBOid) {
    if(!osi.glr) { error.console("M3 data delete failed - no glr active"); return; }

    if(VAO.id)  VAO.delArray();
    if(VBOid)   glDeleteBuffers(1, &VBOid);

    VBOid= 0;        // seems ID0 is reserved, therefore it cannot be a valid ID
  }
}


void _ixpr::glM3::_prePrintInit() {
  //glEnable(GL_TEXTURE_2D);
  //glEnable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  VAO.bind();

  glUseProgram(gl->id);
  glBindBuffer(GL_ARRAY_BUFFER, VBOid);

  glUniformMatrix4fv(u_camera, 1, GL_FALSE, _ix->camera->cameraMat);

  error.glError();
}


void _ixpr::glM3::_afterPrint() {
  glEnable(GL_DEPTH_TEST);
  glBindVertexArray(0);
}


void _ixpr::glM3::printChar(uint32 in_c) {
  // glBufferStorage - this creates a new buffer i think, new stuff, it can be faster, because it's an imovable storage.
  // glBufferSubData - this is used to update parts or all the data of a buffer (send data only)
  // glMapBuffer- this accesses the buffer, you MUST specify that you want to write to it (or read or watever)
  // glBufferSubData or glMapBuffer? this is the question
  // !!! https://www.opengl.org/wiki/Buffer_Object_Streaming !!!

  // CLEANUP AFTER EVERYTHING ACTUALLY WORKS !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    
  _ixFSize *fs= (_ixFSize *)_print->style->selFont;
  _ixFPage *fp= (_ixFPage *)fs->pages.first;
  
  /// search for the page the unicode char belongs to
  for(; fp; fp= (_ixFPage *)fp->next)
    if(pagesList[fp->id].min<= in_c && in_c<= pagesList[fp->id].max)
      break;

  /// if no fp is found, there is no page that has this character
  if(!fp)
    fp= (_ixFPage *)fs->pages.first,
    in_c= '?';                                 /// this should be a character that is on all fonts for shure
    
  _ixFChar *ch= &fp->ch[in_c- pagesList[fp->id].min];    /// shortcut
  
  bool comb= Str::isComb(in_c);


  // character ---=== DRAW ===---
  if(fp->tex!= _prevTex && in_c!= ' ') {    /// avoid switching texture if texture used for previous char is the same or the char is a space
    glBindTexture(GL_TEXTURE_2D, fp->tex->glData.id);
    _prevTex= fp->tex;
  }


  vec2 s, e;            /// s= start e= end
  //s.z= p->pos.z;
  //e.z= p->pos.z;
  
  if(_print->style->orientation== IX_TXT_RIGHT) {           // >  to right orientation
    if(in_c== ' ') {
      _print->pos.x+= (_print->style->spaceSize== 0.0f? ch->end: _print->style->spaceSize);
      return;
    }
    s.x= _print->pos.x+ ch->start;
    s.y= _print->pos.y;
    e.x= s.x+ ch->dx;
    e.y= s.y+ ch->dy;
    if(!comb)_print->pos.x+= ch->end;

  } else if(_print->style->orientation== IX_TXT_UP) {     // ^  to up orientation
    if(in_c== ' ') {
      _print->pos.y+= (_print->style->spaceSize== 0.0f? ch->dy: _print->style->spaceSize);
      return;
    }
    s.x= _print->pos.x+ ch->start;
    s.y= _print->pos.y;
    e.x= s.x+ ch->dx;
    e.y= s.y+ ch->dy;
    if(!comb)_print->pos.y+= ch->dy;

  } else if(_print->style->orientation== IX_TXT_DOWN) {   // v  to down orientation
    if(in_c== ' ') {
      _print->pos.y-= (_print->style->spaceSize== 0.0f? ch->dy: _print->style->spaceSize);
      return;
    }
    if(!comb)_print->pos.y-= ch->dy;
    s.x= _print->pos.x+ ch->start;
    s.y= _print->pos.y;
    e.x= s.x+ ch->dx;
    e.y= s.y+ ch->dy;

  } else if(_print->style->orientation== IX_TXT_LEFT) {   // <  to left orientation
    if(in_c== ' ') {
      _print->pos.x-= (_print->style->spaceSize== 0.0f? ch->end: _print->style->spaceSize);
      return;
    }
    if(!comb)_print->pos.x-= ch->end;
    s.x= _print->pos.x+ ch->start;
    s.y= _print->pos.y;
    e.x= s.x+ ch->dx;
    e.y= s.y+ ch->dy;
  }

  s.x= (float)mlib::roundf(s.x); s.y= (float)mlib::roundf(s.y);
  e.x= (float)mlib::roundf(e.x); e.y= (float)mlib::roundf(e.y);

  /// vert coords
  bufferData[0]= s.x; bufferData[1]=  s.y; bufferData[2]=  _print->pos.z;
  bufferData[3]= s.x; bufferData[4]=  e.y; bufferData[5]=  _print->pos.z;
  bufferData[6]= e.x; bufferData[7]=  e.y; bufferData[8]=  _print->pos.z;
  bufferData[9]= e.x; bufferData[10]= s.y; bufferData[11]= _print->pos.z;

  /// vert tex coords
  bufferData[12]= ch->texX0; bufferData[13]= ch->texY0;
  bufferData[14]= ch->texX0; bufferData[15]= ch->texYe;
  bufferData[16]= ch->texXe; bufferData[17]= ch->texYe;
  bufferData[18]= ch->texXe; bufferData[19]= ch->texY0;

  // this updates the data. The map one you can read - write, and is slower i think(note _i think_). this just feeds ogl data
  glBufferSubData(GL_ARRAY_BUFFER, 0, 20* sizeof(float), bufferData);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}



///========///
// METHOD 5 //
///========///

void _ixpr::glM5::initUniforms() {
  ixPrintShader::initUniforms();
  u_pos= glGetUniformLocation(gl->id, "u_pos");
  u_chDy= glGetUniformLocation(gl->id, "u_chDy");

}


void _ixpr::glM5::_create(_ixFPage *page) {
  if(!osi.glr) { error.console("Print::_M5VBOidCreate() FAILED - no renderer active"); return; }

  /// used vars
  int nrChars= pagesList[page->id].max- pagesList[page->id].min+ 1;
  uint32 data1size= nrChars* sizeof(GLint)* 4;
  uint32 data2size= nrChars* sizeof(GLfloat)* 2* 4;

  /// data populate
  uint8 *data= new uint8[data1size+ data2size];
  if(!data) { error.simple("memory alloc failed"); return; }

  int32 *pi= (int32 *)data;
  float *pf= (float *)((uint8 *)data+ data1size);

  for(int a= 0; a< nrChars; a++) {
    *pi++= page->ch[a].start;                   // v1
    *pi++= page->ch[a].start;                   // v2
    *pi++= page->ch[a].start+ page->ch[a].dx;   // v3
    *pi++= page->ch[a].start+ page->ch[a].dx;   // v4
  }

  for(int a= 0; a< nrChars; a++) {
    *pf++= page->ch[a].texX0;     // v1
    *pf++= page->ch[a].texY0;
    *pf++= page->ch[a].texX0;     // v2
    *pf++= page->ch[a].texYe;
    *pf++= page->ch[a].texXe;     // v3
    *pf++= page->ch[a].texYe;
    *pf++= page->ch[a].texXe;     // v4
    *pf++= page->ch[a].texY0;
  }
  
  /// create the VBO buffer that holds all chars data
  glGenBuffers(1, &page->_M5VBOid);
  glBindBuffer(GL_ARRAY_BUFFER, page->_M5VBOid);

  glBufferData(GL_ARRAY_BUFFER, data1size+ data2size, data, GL_STATIC_DRAW); /// size of the vertex data and tex coods
  page->_M5texPointer= data1size;

  //glVertexAttribIPointer(0, 1, GL_INT, 0, 0);
  //glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)data1size);
  
  if(data) delete[] data;
  error.glError();
}


void _ixpr::glM5::_delete(_ixFPage *page) {
  if(page->_M5VBOid) {
    if(_ix->glIsActive()) { error.console("M5 buffer data delete failed - no glr active"); return; }
    glDeleteBuffers(1, &page->_M5VBOid);
    page->_M5VBOid= 0;
  }
}



void _ixpr::glM5::_prePrintInit() {
  glUseProgram(gl->id);
  glDisable(GL_DEPTH_TEST);

  /// camera matrix update - lots of optimisations can be made - if cam==ortho, translate should be highly optimized
  glUniformMatrix4fv(u_camera, 1, GL_FALSE, _ix->camera->cameraMat);

  glUniform1i(u_chDy, ((_ixFSize *)_print->style->selFont)->size);
  glBindVertexArray(0);
  glEnableVertexAttribArray(1);

  error.glError();
}


void _ixpr::glM5::_afterPrint() {
  glEnable(GL_DEPTH_TEST);
  glDisableVertexAttribArray(1);
}


inline void _ixpr::glM5::printChar(uint32 in_c) {
  _ixFSize *fs= (_ixFSize *)_print->style->selFont;
  _ixFPage *fp= (_ixFPage *)fs->pages.first;
  
  /// search for the page the unicode char belongs to
  for(; fp; fp= (_ixFPage *)fp->next)
    if(pagesList[fp->id].min<= in_c && in_c<= pagesList[fp->id].max)
      break;

  /// if no fp is found, there is no page that has this character
  if(!fp)
    fp= (_ixFPage *)fs->pages.first,
    in_c= '?';                                 /// this should be a character that is on all fonts for shure
    
  _ixFChar *ch= &fp->ch[in_c- pagesList[fp->id].min];    /// shortcut
  
  bool comb= Str::isComb(in_c);

  // character ---=== DRAW ===---
  if(_prevTex!= fp->tex && in_c!= ' ') {
  // if(!_prevTex)                 // WHY WAS THIS LIKE SO??? SHOULD FURTHER TEST, BUT IN ANY CASE, IT SHOULD AT LEAST CHANGE TEXTURE IF THE PREV IS NOT THE SAME
    glBindBuffer(GL_ARRAY_BUFFER, fp->_M5VBOid);
    glVertexAttribIPointer(0, 1, GL_INT, 0, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)(fp->_M5texPointer));

    glBindTexture(GL_TEXTURE_2D, fp->tex->glData.id);
    _prevTex= fp->tex;
  }

  /// advance cursor position based on text orientation
  if(_print->style->orientation== IX_TXT_RIGHT) {       // >  to right orientation
    if(in_c== ' ') {
      _print->pos.x+= (_print->style->spaceSize== 0.0f? ch->end: _print->style->spaceSize);
      return;
    }
    glUniform4f(u_pos, (float)mlib::roundf(_print->pos.x+ ch->start), (float)mlib::roundf(_print->pos.y), _print->pos.z, 1.0f);
    if(!comb)_print->pos.x+= ch->end;

  } else if(_print->style->orientation== IX_TXT_UP) {   // ^  to up orientation
    if(in_c== ' ') {
      _print->pos.y+= (_print->style->spaceSize== 0.0f? ch->dy: _print->style->spaceSize);
      return;
    }
    glUniform4f(u_pos, (float)mlib::roundf(_print->pos.x+ ch->start), (float)mlib::roundf(_print->pos.y), _print->pos.z, 1.0f);
    if(!comb)_print->pos.y+= ch->dy;

  } else if(_print->style->orientation== IX_TXT_DOWN) { // v  to down orientation
    if(in_c== ' ') {
      _print->pos.y-= (_print->style->spaceSize== 0.0f? ch->dy: _print->style->spaceSize);
      return;
    }
    if(!comb)_print->pos.y-= ch->dy;
    glUniform4f(u_pos, (float)mlib::roundf(_print->pos.x+ ch->start), (float)mlib::roundf(_print->pos.y), _print->pos.z, 1.0f);

  } else if(_print->style->orientation== IX_TXT_LEFT) { // <  to left orientation
    if(in_c== ' ') {
      _print->pos.x-= (_print->style->spaceSize== 0.0f? ch->end: _print->style->spaceSize);
      return;
    }
    if(!comb)_print->pos.x-= ch->end;
    glUniform4f(u_pos, (float)mlib::roundf(_print->pos.x+ ch->start), (float)mlib::roundf(_print->pos.y), _print->pos.z, 1.0f);
  }
  // THIS MUST BE BROKEN, it hs to be some id that is not being updated at start, or something, it was multiplying gl->id, that's not good
  //id*= 4;   /// 4 vertex per char
  glDrawArrays(GL_TRIANGLE_STRIP, 0 /*id*/, 4);
}




///======================///
// METHOD 5 with uniforms //
///======================///

void _ixpr::glM5U::initUniforms() {
  ixPrintShader::initUniforms();
  u_pos=   glGetUniformLocation(gl->id, "u_pos");
}


void _ixpr::glM5U::_create(_ixFPage *page) {
  if(!osi.glr) { error.console("Print::_M5UVBOidCreate() FAILED - no renderer active"); return; }
  if(!gl->id) { error.console("M5U shader not loaded"); return; }

  GLuint blockID= glGetUniformBlockIndex(gl->id, "U_data");
  
  // Query for the offsets of each block variable
  const GLchar *names[]= { "chDy", "data[].chDx", "data[].x0", "data[].y0", "data[].xe", "data[].ye", "data[1].chDx"};
  GLuint indices[7];
  glGetUniformIndices(gl->id, 7, names, indices);
  
  GLint offset[7];
  glGetActiveUniformsiv(gl->id, 7, indices, GL_UNIFORM_OFFSET, offset);
  error.glError();

  //stride, still not sure what this can help with
  //GLint stride;
  //glGetActiveUniformsiv(program, 1, &indices[6], GL_UNIFORM_ARRAY_STRIDE, &stride);
  int arrWide= offset[6]- offset[1];
  /// block size
  int nrChars= pagesList[page->id].max- pagesList[page->id].min+ 1;
  int bsize= (arrWide* nrChars)+ offset[1]- offset[0];  // array size+ chDy

  float *pf;
  int32 *pi;

  uint8 *data= new uint8[bsize];
  if(!data) { error.simple("mem alloc fail"); return; }
  uint8 *p= data;

  /// chDy
  p+= offset[0];
  pi= (int32 *)p;
  *pi= page->size->size;

  p+= offset[1];
  
  for(int a= 0; a< nrChars; a++) {
    pi= (int32 *)((uint8 *)p);                          /// dx
    *pi= page->ch[a].dx;
    pf= (float *)((uint8 *)p+ offset[2]- offset[1]);    /// tex x0
    *pf= page->ch[a].texX0;
    pf= (float *)((uint8 *)p+ offset[3]- offset[1]);    /// tex y0
    *pf= page->ch[a].texY0;
    pf= (float *)((uint8 *)p+ offset[4]- offset[1]);    /// tex xe
    *pf= page->ch[a].texXe;
    pf= (float *)((uint8 *)p+ offset[5]- offset[1]);    /// tex ye
    *pf= page->ch[a].texYe;

    p+= arrWide;
  }

  /// built all the data - create the buffer
  glGenBuffers(1, &page->_M5UUBOid);
  glBindBuffer(GL_UNIFORM_BUFFER, page->_M5UUBOid);
  glBufferData(GL_UNIFORM_BUFFER, bsize, data, GL_STATIC_DRAW); /// populate ubo with created data

  // the buffer point (0 in this case) i think is a constant that any buffer can use, but the program must set it,
  // so some kind of order in these binding points must be done somehow, it's pretty important
  // but these binding points are per program... so no major order is needed... actually there's no problem
  glUniformBlockBinding(gl->id, blockID, 0); // this must be set once
  error.glError();
  //glBindBufferBase(GL_UNIFORM_BUFFER, 0, page->_M5UUBOid); // - this can be used frequently

  delete[] data;
}


void _ixpr::glM5U::_delete(_ixFPage *page) {
  if(page->_M5UUBOid) {
    if(!osi.glr) { error.console("M5U buffer data delete failed - no glr active"); return; }
    else
    glDeleteBuffers(1, &page->_M5UUBOid);
    page->_M5UUBOid= 0;
  }
}




void _ixpr::glM5U::_prePrintInit() {
  glUseProgram(gl->id);
  glDisable(GL_DEPTH_TEST);

  /// camera matrix update - lots of optimisations can be made - if cam==ortho, translate should be highly optimized
  glUniformMatrix4fv(u_camera, 1, GL_FALSE, _ix->camera->cameraMat);

  //glUniform1i(p->_M5U.slChDy, ((_ixFSize *)p->selFont)->size);
  glBindVertexArray(0);
  glDisableVertexAttribArray(0);
  //glEnableVertexAttribArray(1);
  error.glError();
}


void _ixpr::glM5U::_afterPrint() {
  glEnable(GL_DEPTH_TEST);
  glEnableVertexAttribArray(0);
}


inline void _ixpr::glM5U::printChar(uint32 c) {
  
  _ixFSize *fs= (_ixFSize *)_print->style->selFont;
  _ixFPage *fp= (_ixFPage *)fs->pages.first;
  
  /* IT WOULD HAVE TO BE ON EVERY FUNC. this is too much i think
  if(c>= 9644 && c<= 9835) {
    if     (c== 9786) c= 1;
    else if(c== 9787) c= 2;
    else if(c== 9829) c= 3;
    else if(c== 9830) c= 4;
    else if(c== 9827) c= 5;
    else if(c== 9824) c= 6;
    else if(c== 9688) c= 8;
    else if(c== 9675) c= 9;
    else if(c== 9689) c= 10;
    else if(c== 9794) c= 11;
    else if(c== 9792) c= 12;
    else if(c== 9834) c= 13;
    else if(c== 9835) c= 14;
    else if(c== 9788) c= 15;
    else if(c== 9658) c= 16;
    else if(c== 9668) c= 17;
    else if(c== 9644) c= 22;
    else if(c== 9650) c= 30;
    else if(c== 9660) c= 31;
  } else if(c>= 8226 && c<= 8735) {
    if     (c== 8226) c= 7;
    else if(c== 8597) c= 18;
    else if(c== 8252) c= 19;
    else if(c== 8593) c= 24;
    else if(c== 8595) c= 25;
    else if(c== 8594) c= 26;
    else if(c== 8592) c= 27;
    else if(c== 8735) c= 28;
    else if(c== 8596) c= 29;
  } else if(c== 182) c= 20;
  else if(c== 167) c= 21;
  */
  //1= 9786;
  //2= 9787;
  //3= 9829;
  //4= 9830;
  //5= 9827;
  //6= 9824;
  //7= 8226;
  //8= 9688;
  //9= 9675;
  //10= 9689;
  //11= 9794;
  //12= 9792;
  //13= 9834;
  //14= 9835;
  //15= 9788;
  //16= 9658;
  //17= 9668;
  //18= 8597;
  //19= 8252;
  //20= 182;
  //21= 167;
  //22= 9644;
  //23= 8616;
  //24= 8593;
  //25= 8595;
  //26= 8594;
  //27= 8592;
  //28= 8735;
  //29= 8596;
  //30= 9650;
  //31= 9660;



  /// search for the page the unicode char belongs to
  for(; fp; fp= (_ixFPage *)fp->next)
    if(pagesList[fp->id].min<= c && c<= pagesList[fp->id].max)
      break;

  /// if no fp is found, there is no page that has this character
  if(!fp)
    fp= (_ixFPage *)fs->pages.first,
    c= '?';                                 /// this should be a character that is on all fonts for shure
    
  _ixFChar *ch= &fp->ch[c- pagesList[fp->id].min];    /// shortcut
  
  bool comb= Str::isComb(c);

  // character ---=== DRAW ===---
  if(_prevTex!= fp->tex && c!= ' ') {
  //if(!prevTex) {                 // WHY WAS THIS LIKE SO??? SHOULD FURTHER TEST, BUT IN ANY CASE, IT SHOULD AT LEAST CHANGE TEXTURE IF THE PREV IS NOT THE SAME
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, fp->_M5UUBOid); // - this can be used frequently
    glBindTexture(GL_TEXTURE_2D, fp->tex->glData.id);
    _prevTex= fp->tex;
  }

  /// advance cursor position based on text orientation
  if(_print->style->orientation== IX_TXT_RIGHT) {       // >  to right orientation
    if(c== ' ') {
      _print->pos.x+= (_print->style->spaceSize== 0.0f? ch->end: _print->style->spaceSize);
      return;
    }
    glUniform4f(u_pos, (float)mlib::roundf(_print->pos.x+ ch->start), (float)mlib::roundf(_print->pos.y), _print->pos.z, 1.0f);
    if(!comb)_print->pos.x+= ch->end;

  } else if(_print->style->orientation== IX_TXT_UP) {   // ^  to up orientation
    if(c== ' ') {
      _print->pos.y+= (_print->style->spaceSize== 0.0f? ch->dy: _print->style->spaceSize);
      return;
    }
    glUniform4f(u_pos, (float)mlib::roundf(_print->pos.x+ ch->start), (float)mlib::roundf(_print->pos.y), _print->pos.z, 1.0f);
    if(!comb)_print->pos.y+= ch->dy;

  } else if(_print->style->orientation== IX_TXT_DOWN) { // v  to down orientation
    if(c== ' ') {
      _print->pos.y-= (_print->style->spaceSize== 0.0f? ch->dy: _print->style->spaceSize);
      return;
    }
    if(!comb)_print->pos.y-= ch->dy;
    glUniform4f(u_pos, (float)mlib::roundf(_print->pos.x+ ch->start), (float)mlib::roundf(_print->pos.y), _print->pos.z, 1.0f);

  } else if(_print->style->orientation== IX_TXT_LEFT) { // <  to left orientation
    if(c== ' ') {
      _print->pos.x-= (_print->style->spaceSize== 0.0f? ch->end: _print->style->spaceSize);
      return;
    }
    if(!comb)_print->pos.x-= ch->end;
    glUniform4f(u_pos, (float)mlib::roundf(_print->pos.x+ ch->start), (float)mlib::roundf(_print->pos.y), _print->pos.z, 1.0f);
  }

  uint32 id= (c- pagesList[fp->id].min);
  id*= 4;   /// 4 vertex per char
  glDrawArrays(GL_TRIANGLE_STRIP, id, 4);
}





// common uniforms

void ixPrintShader::initUniforms() {
  glUseProgram(gl->id);
  u_camera=       glGetUniformLocation(gl->id, "u_camera");
  u_color=        glGetUniformLocation(gl->id, "u_color");
  u_viewportPos=  glGetUniformLocation(gl->id, "u_viewportPos");
  u_clip=         glGetUniformLocation(gl->id, "u_clip");
  u_clip0=        glGetUniformLocation(gl->id, "u_clip0");
  u_clipE=        glGetUniformLocation(gl->id, "u_clipE");

  glUniform4f(u_color, 1.0f, 1.0f, 1.0f, 1.0f);
  glUniform2f(u_viewportPos, 0.0f, 0.0f);
  glUniform1i(u_clip, 0);
}






// ------------------- //
// MAIN PRINT FUNCTION //  tied to the shader, will call specific method funcs depending of the currently selected method
// ------------------- //


void _ixpr::glShader::txt(const void *s, int type, int start, int end, int startBytes, int endBytes) {
  if(!s) return;
  // needs: textures on, blend on, depth test off, ch0, ch1
  // defaults should be: texture2 on, blend on, depth test on, ch0 on, ch1-n off
  // changes depth and ch1
  error.glFlushErrors();

  ixPrint *p= _print;
  if(p== null) return;
  if(!p->style->selFont) return;                    /// if no font is selected, just return
  if(!p->justDraw) {
    _prePrintInit();

    /// background draw - simple shadow
    if(p->style->drawMode== 4) { // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< drawmode got flagg-erized;
      vec2 oldPos(p->pos);
      p->justDraw= true;
      glUniform4f(u_color, p->style->color2.r, p->style->color2.g, p->style->color2.b, p->style->color2.a);
      p->pos+= p->style->shadowPos; txt(s, type, start, end, startBytes, endBytes);
      p->pos= oldPos;

    } else {
    /// background draw - 4 extra text draws, in a cross shape
      if(p->style->drawMode == 1|| p->style->drawMode== 3) {
        float x= p->pos.x, y= p->pos.y;
        p->justDraw= true;
        glUniform4f(u_color, p->style->color2.r, p->style->color2.g, p->style->color2.b, p->style->color2.a);
      
        p->pos.x= x- (float)p->style->outlineSize; p->pos.y= y;                               txt(s, type, start, end, startBytes, endBytes);
        p->pos.x= x+ (float)p->style->outlineSize; p->pos.y= y;                               txt(s, type, start, end, startBytes, endBytes);
        p->pos.x= x;                               p->pos.y= y- (float)p->style->outlineSize; txt(s, type, start, end, startBytes, endBytes);
        p->pos.x= x;                               p->pos.y= y+ (float)p->style->outlineSize; txt(s, type, start, end, startBytes, endBytes);

        p->pos.x= x;                          p->pos.y= y;
      }
      /// background draw - 4 extra text draws, in a x shape
      if(p->style->drawMode == 2|| p->style->drawMode== 3) {
        float x= p->pos.x, y= p->pos.y;
        p->justDraw= true;
        glUniform4f(u_color, p->style->color2.r, p->style->color2.g, p->style->color2.b, p->style->color2.a);

        p->pos.x= x- (float)p->style->outlineSize; p->pos.y= y- (float)p->style->outlineSize; txt(s, type, start, end, startBytes, endBytes);
        p->pos.x= x+ (float)p->style->outlineSize; p->pos.y= y- (float)p->style->outlineSize; txt(s, type, start, end, startBytes, endBytes);
        p->pos.x= x+ (float)p->style->outlineSize; p->pos.y= y+ (float)p->style->outlineSize; txt(s, type, start, end, startBytes, endBytes);
        p->pos.x= x- (float)p->style->outlineSize; p->pos.y= y+ (float)p->style->outlineSize; txt(s, type, start, end, startBytes, endBytes);

        p->pos.x= x;                          p->pos.y= y;
      }
    }
    /// vvv no effects basically normal code starts from here, just draw is used only for additional effects

    /// print color
    glUniform4f(u_color, p->style->color1.r, p->style->color1.g, p->style->color1.b, p->style->color1.a);

    p->justDraw= false; /// just draw must always be false, it's used to force a draw whitout checks for effects
  }

  _prevTex= 0;
  uint32 c;

  // UTF-8 print
  if(type== 8) {
    const uint8 *t= (const uint8*)s+ startBytes;
    endBytes-= startBytes;

    if(start)                         /// skip [start] characters from the string
      while(*t && start)
        if(((*t++)& 0xc0)!= 0x80)     /// 0xc0= 11000000, first two bits  0x80= 10000000, test for 10xxxxxx the last 6 bits wont matter, as they are not tested so test to 10000000
          start--, end--;
    
    // for each character
    for(; *t && end && (endBytes> 0); end--) {
      if(*t < 128)                    /// character uses 1 byte (ascii 0-127)
        c= *t++, endBytes--;
      else {
        int n= 0;
        if((*t& 0xe0) == 0xc0)        /// character uses 2 bytes
          c= *t++ & 0x1f, n= 1;
        else if((*t& 0xf0) == 0xe0)   /// character uses 3 bytes
          c= *t++ & 0x0f, n= 2;
        else if((*t& 0xf8) == 0xf0)   /// character uses 4 bytes
          c= *t++ & 0x07, n= 3;
        // the last 2 bytes are not used, but printed if in the future unicode will expand
        else if((*t& 0xfc) == 0xf8)   /// character uses 5 bytes
          c= *t++ & 0x03, n= 4;
        else if((*t& 0xfe) == 0xfc)   /// character uses 6 bytes
          c= *t++ & 0x01, n= 5; 
        else error.simple("invalid utf8 string");
        endBytes-= n+ 1;

        while(n--) {
          c<<= 6; c+= *t++ & 0x3f;
        }
      } /// unpack the unicode value

      printChar(c);
    }	/// for each character


  // UTF-16 print
  } else if(type== 16) {
    const uint16 *t16= (const uint16 *)((int8 *)s+ startBytes);
    endBytes-= startBytes;
    if(start)          /// skip [start] characters from the string
      while(*t16 && start)
        t16+= (Str::isHighSurrogate(*t16)? 2: 1), start--, end--;
    
    // for each character
    for(; *t16 && end && (endBytes> 0); end--) {
      if(Str::isHighSurrogate(*t16))
        c= (*t16<< 10)+ *(t16+ 1)+ Str::UTF16_SURROGATE_OFFSET,
        t16+= 2, endBytes-= 4;
      else 
        c= *t16++, endBytes-= 2;

      printChar(c);
    }	/// for each character


  // UTF-32 print
  } else if(type== 32) {
    const uint32 *t32= (uint32 *)((int8 *)s+ startBytes);
    endBytes-= startBytes;

    if(start)         /// skip [start] characters from the string
      while(*t32 && start)
        t32++, start--, end--;
    
    // for each character
    for(; *t32 && end && (endBytes> 0); end--, endBytes-= 4) {
      c= *t32++;
      printChar(c);
    }	/// for each character
  }

  error.glError();

  if(!p->justDraw)
    _afterPrint();
}


// WORK IN PROGRESS ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
/// ==========================================================================

#endif // IX_USE_OPENGL
































/*
// clipping funcs

void ixPrint::setScissor(int32 in_x0, int32 in_y0, int32 in_xe, int32 in_ye) {
  /// fragment shader works with viewport coords
  in_x0-= _ix->win->x0;
  in_y0-= _ix->win->y0;
  
  glUseProgram(_shader->gl->id);
  glUniform1i(_shader->u_clip, 1);
  glUniform2f(_shader->u_clip0, (float)in_x0, (float)in_y0);
  glUniform2f(_shader->u_clipE, (float)in_xe, (float)in_ye);
}


void ixPrint::setClipPlaneD(int32 in_x0,int32 in_y0,int32 in_dx, int32 in_dy) {
  /// fragment shader works with viewport coords
  in_x0-= _ix->win->x0;
  in_y0-= _ix->win->y0;
  
  glUseProgram(_shader->gl->id);
  glUniform1i(_shader->u_clip, 1);
  glUniform2f(_shader->u_clip0, (float)in_x0,          (float)in_y0);
  glUniform2f(_shader->u_clipE, (float)(in_x0+ in_dx), (float)(in_y0+ in_dy));
}


void ixPrint::setClipPlaneR(const recti *in_r) {
  /// fragment shader works with viewport coords
  int x0= in_r->x0- _ix->win->x0;
  int y0= in_r->y0- _ix->win->y0;
  
  glUseProgram(_shader->gl->id);
  glUniform1i(_shader->u_clip, 1);
  glUniform2f(_shader->u_clip0, (float)x0,             (float)y0);
  glUniform2f(_shader->u_clipE, (float)(x0+ in_r->dx), (float)(y0+ in_r->dy)); /// fragment shader works with viewport coords
}
*/























































































