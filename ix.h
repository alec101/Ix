#pragma once

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif


// if a func has err as char *, errL as int, Exit as exit label, this macro will work
#define IXERR(x) { err= x; errL= __LINE__; goto Exit; }

#if (defined IX_USE_VULKAN) && (!defined OSI_USE_VKO)
#define OSI_USE_VKO 1
//#define VKO_USE_VULKAN_INCLUDE_DIR "Vulkan-Headers/include/vulkan/vulkan.h"
#endif

#if (defined IX_USE_OPENGL) && (!defined OSI_USE_OPENGL)
#define OSI_USE_OPENGL
#endif

#if !defined NDEBUG
#define IX_BE_CHATTY
#endif

//#include <atomic>
#include <mutex>

//#define OSI_USE_VULKAN 1
//#define OSI_USE_OPENGL_EXOTIC_EXT 1
#include "osi/include/osinteraction.h"

#include "osi/include/util/typeShortcuts.h"
#include "osi/include/util/str8.h"
#include "osi/include/util/mlib.hpp"
#include "osi/include/util/rgb.hpp"
#include "osi/include/util/chainList.hpp"
#include "osi/include/util/circleList.hpp"
#include "ix/util/onewayList.hpp"

inline void ixMemcpy(void *dst, const void *src, uint64_t n) { return Str::memcpy(dst, src, n); }


enum class ixClassT: uint16 {
  IX, MATERIAL, MESH, RESOURCE, SHADER, TEXSTREAM, TEXTURE, VKCLUSTER, VKCLUSTERSEGMENT, VKRESOURCE, VKBUFFER, VKIMAGE
};

// base class for all ix objects
class ixClass: public chainData {
public:
  ixClassT classT;
  ixClass(ixClassT in_t): classT(in_t) {}

  // THIS COULD HAPPEN....... ALSO MUST THINK ABOUT THE allocArray(..) func, if it should be here somehow, maybe =0, to force create it in derived
  // Ix IS a ixClass ... so it can't happen? unless you just ignore _ix, or just put this... i mean... i don't even wanna know what happens, maybe end the universe?
  //Ix *_ix;
  //ixClass(ixClassT in_t, Ix *in_ix): classT(in_t), _ix(in_ix) {}
};



#include "util/common.hpp"
#include "osi/include/util/imgClass.h"
#include "util/fileTEX.h"

#include "GFX/camera.h"
#include "GFX/shaderSys.h"
#include "GFX/resource.h"
//#include "GFX/texture.h"
#include "GFX/texSys.h"
#include "GFX/texStream.h"
#include "GFX/material.h"
#include "GFX/mesh3D.h"
#include "GFX/object.h"

#ifdef IX_USE_OPENGL
#include "GFX/glo/gloShader.h"
#include "ix/draw/glDraw.h"
#include "GFX/glo/glObject.h"
#endif

#ifdef IX_USE_VULKAN
#include "ix/draw/vkDraw.h"
#include "ix/ixVulkan.h"
#endif




#include "GFX/print.h"
#include "util/ixConsole.h"
#include "ix/util/orientation.h"
#include "winSys/winSys.h"


// VERSIONING: (uint32) / binary(10.10.12) / hex max(3ff.3ff.fff) / decimal max(1023.1023.4095)

#define IX_VER_MAKE(major, minor, patch) ((((uint32_t)(major)) << 22) | (((uint32_t)(minor)) << 12) | ((uint32_t)(patch)))
#define IX_VER_MAJOR(version)  ((uint32_t)(version)>> 22)
#define IX_VER_MINOR(version) (((uint32_t)(version)>> 12)& 0x3ff)
#define IX_VER_PATCH(version)  ((uint32_t)(version)      & 0xfff)



/*
THIS MUST STAY UNTIL IT'S FULL LEARNED IN MY HEAD:
#define IX_TXT_RIGHT      0x01
#define IX_TXT_LEFT       0x02
#define IX_TXT_DOWN       0x04
#define IX_TXT_UP         0x08
#define IX_TXT_HORIZONTAL 0x03      // IX_TXT_RIGHT and IX_TXT_LEFT bytes form IX_TXT_HORIZONTAL - used to know if text is horizontal in orientation
#define IX_TXT_VERTICAL   0x0C      // IX_TXT_DOWN  and IX_TXT_UP bytes   form IX_TXT_VERTICAL   - used to know if text is vertical in orientation

INSTEAD OF HAVING A switch-case() or an if/else if/else if/ for 20 posibilities,
you can have a simple variable& IX_COMBO_OF_POSIBILITIES_HERE
there are 
*/


// THINGS TO CONFIGURE ON EVERY PROGRAM THAT USES IX:
/*
 shaderDIR - where the ix shaders are located

*/

/*

major redesign
there gonna be an ix class for every renderer you wanna use in your program
therefore, static functions for every class must disapear or use the mutex system
you must be able to create an ix on a thread and an ix on another thread

the window system has the only problem
the number of windows created must be static
each class, therefore must know of each other
either that, or major parts must be static
you cannot have 2 wsys updates, i think

the draw... hmmm

each widnow must draw on multiple rendereres
draw must be for the part that shows on the current monitor, maybe...
so draw can be for each ix class

the update... nomatter what ix class you want wsystem updates - window input/output from, it must be static

there gonna be static/non-static stuff here, lots of thinking
==============

there are still static stuff, that would work on every ix class



====================
====================

to have only 1 ix class... each shader, each texture, must have id's for every renderer
   you must define how many renderers you gonna use
   the internal 'guts' of it all, is gonna be just UGLY, TOO COMPLEX, VERY HARD.

  with multiple ix classes, only the window system is different (static)



  ----------------------------------------------------------------------------------------
  ========================================================================================
  you can have multiple ix classes on multiple threads, else you cannot with only one.....      <-- THE DEFINITIVE ANSWER HERE
  ========================================================================================
  ----------------------------------------------------------------------------------------
   ... you can, with mutex checks everywhere, ugly as hell



  ix.print.txt
  ix.pr.txt(
    ^
    it is not that big...


    ix.wsys.bla
    
    seems this is the definitive answer to the multiple renderers issue
    and it seems the final structure of the ix engine is set




    further think: you'd link ix classes, how many you want, and when you load a texture in one, you load it in the other too
    basically, for 2 gr cards rendering a different perspective of the same thing

*/

class vkDraw;
class ixVulkan;
class ixDraw;
class ixResourceSys;
class ixTexSys;
class ixShaderSys;
class ixPrint;
class ixConsole;
class ixWinSys;
class ixCamera;
class ixTexSys;
class ixShaderSys;

#define ixMemcpy(_dst_, _src_, _nr_) Str::memcpy(_dst_, _src_, _nr_)
//#define offsetof(s, m) ((size_t)&(((s*)0)->m))



class Ix: public ixClass {
public:

  // chainlist with all created Ix engines
  inline static chainList &ixList() { static chainList ___ixList; return ___ixList; }

  struct {
    inline static ixWSstyle &def1Style() { static ixWSstyle _def1Style; return _def1Style; } // default windows style 1
    inline static ixWSstyle &def2Style() { static ixWSstyle _def2Style; return _def2Style; } // default windows style 2
  } static glb;

  //int32 id;
  // THOSE 2 INLINE BOOLS... WHY ADD THIS? uint32 engine;  // [def:0] 0= Vulkan, 1= OpenGL; all funcs will use this engine;


  struct ResourceSystem {
    ixShaderSys shader;
    ixTexSys    tex;
    ixMatSys    mat;      // always destroy meshes before mats
    ixMeshSys   mesh;     // always destroy meshes before mats

    // mat+tex sys, might have an ID file, for fast loading
    // tex would have ID+file, in a nice order
    // and a game would use these files, somehow, but it must be automated, these must be auto-created
    // this would be a way to optimize load times tho...


    // DISABLED ATM, MATERIALS MUST EXIST, ALL SYSTEMS MUST BE FUNCTIONAL
    //bool cleanup_ONE(); // to be called once per frame, for a small amount of cleanup
    //void cleanup_ALL(); // can stagger app for a long time

    ResourceSystem(Ix *in_ix): tex(in_ix), mat(in_ix), mesh(in_ix), shader(in_ix) {}
  } res;

  ixPrint pr;
  inline static ixWinSys &wsys()     { static ixWinSys _wsys;     return _wsys; }    // window system
  inline static ixConsole &console() { static ixConsole _console; return _console; } // console


  struct Config {
    // set this directory before initializing anything, so the engine knows where the shaders are located
    inline static str8 &shaderDIR() { static str8 _shaderDIR; return _shaderDIR; }

    uint8 engine;                 // [def:1] 0= OpenGL, 1= Vulkan
    bool errorUseConsoleFlag;     // [def:true] - Error class will use the console
    #ifdef IX_USE_VULKAN
    struct Vulkan {
      bool enableValidationLayers;    // [def:depends on project setup:NDEBUG] enable/disable validation layers
      uint32 physicalDeviceIndex;     // [def:~0] by def, it will try to get it from OSI's window. If not default, specify a physical device's index
      /// the queues Ix will work with
      int32 queueRequestUniversal;    // [def:3] Ix::Vulkan::q1 + q2 + qTool
      int32 queueRequestGraphics;     // [def:0] pure graphics queues don't exist, i don't think
      int32 queueRequestCompute;      // [def:1] Ix::Vulkan::qCompute
      int32 queueRequestTransfer;     // [def:1] Ix::Vulkan::qTransfer

      VkPhysicalDeviceFeatures gpuFeatures; // the gpu features that ix will enable. Add more here, when needed

      bool resourcesUseSharedQueueFamilies; // [def:false] when true, all resources are created shared families option, so no queue transfer is needed.

      // static textures will have their own dynamic set, not part of the stream sys
      uint32 staticTexturesDynamicSetSegmentSize; // [def:20] <<< ATM
      uint32 printDynamicSetSegmentSize;          // [def:10] how many pages of any size/font

      uint64 size_clusterIxDevice;        // 32MB - defrag staging buffer(24MB) / print pages / global uniform buffer(MOVE TO HOST VISIBLE?! - only if it's device+host visible)
      uint64 size_clusterIxHost;          // 32MB - staging buffer
      uint64 size_clusterResDevice;       // 64MB - single textures/buffers
      uint64 size_clusterResHost;         // 32MB - single textures/buffers (UNIFORM?) - in host visible
      uint64 size_stageBufferDevice;      //  1MB
      uint64 size_stageBufferHost;        // 24MB - can hold 2048x2048x4xmips textures

      // add data to the glb buffer used by all shaders in here
      ixVulkan::GlbBuffer::Data *derivedData[2]; // [def:null] derive from ixVulkan::GlbBuffer::Data then pass it here - data is NOT DESTROYED by ix, if you create it
      uint32 derivedDataSize;                 // [def:0]    derived data size (including base)
    } vk;
    #endif
  } cfg;

  #ifdef IX_USE_OPENGL
  glObject glo;
  #endif
  
  #ifdef IX_USE_VULKAN
  vkObject vk;            // vulkan object
  ixVulkan vki;           // ix vulkan specialized object
  #endif


  ixCamera cameraPersp, cameraOrtho;
  ixCamera *camera;                     // currently active camera class
  const osiRenderer *ren;               // osi renderer tied to this ix engine (Stimpy would be proud)
  const osiGPU *gpu;
  osiWindow *win;

  str8 clipboard;

  vec3 tgtPos;            //NOT TO BE USED; USE getTargetPos position of the surface the renderer draws on

  void getTargetPos(int32 *out_x0, int32 *out_y0);  // renderer's target position in virtual desktop.

  #ifdef IX_USE_OPENGL
  void glMakeCurrent(); // SHOULD BE IN THE IxGl class
  #endif

  
  // drawing cycle - THIS MUST BE EXPANDED WITH AS MANY STEPS AS NEEDED

  void setCameras();



  bool startRender();        // handles swapchain image aquisition, starts the first renderpass

    bool startPerspective();
    void endPerspective();

    bool startOrtho();
    void endOrtho();

  void endRender();

  float FPS;
  void measureFPS();

  void update();      // basic checks/updates per frame

  // renderer funcs

  inline bool renVulkan() { return ren->type== 1; }
  inline bool renOpenGL() { return ren->type== 0; }

  //inline void createShader(ixShader **out_shader) { if(renVulkan()) vk.createShader(out_shader); else if(renOpenGL()) glo.createShader(out_shader); }



  Ix();
  ~Ix();
  void delData();
  void shutdown();
  void exitCleanup();   // this fun will be called if the last ix engine is called. static objects should be destroyed here


  void init(void *glbData[2]= null, uint32 size= 0); // before window creation - sets up main program stuff; <glbData & size>: global shader uniform buffer extra data (glb)
  void initWindow(osiWindow *renderer);           // after window creation  - sets up the renderer / monitor / window it will use


  std::mutex osiMutex;


  //int32 getId() { return id; }
  inline static Ix *getMain() { return (ixList().first? ((Ix *)(ixList().first)): null); }

  #ifdef IX_USE_OPENGL
  // if the renderer is opengl, it checks if it's the current renderer; vulkan renderer should always be active
  inline bool glIsActive() { return ((osi.glr!= null) && (osi.glr== ren)); }
  inline static Ix **glActiveRen() { static Ix* ___glActiveRen= null; return &___glActiveRen; }
  #endif

  void *fnt5x6;                   // very small fnt, used for debugs, mainly
  ixFontStyle debugStyle;         // uses 5x6 font
  
  void printIxList();
  
private:
  //float _lastFPS;

  uint64 _frameTime;
  uint64 _lastFrameTime;

  inline static std::mutex &_ixListMutex() { static std::mutex _ixListMutexStatic; return _ixListMutexStatic; }
  
  friend class ixWSstyle;
  friend class ixWinSys;
  friend class ixVulkan;
};



/*
#ifdef IX_USE_VULKAN
#include "ix/draw/vkDraw.h"
#include "ix/ixVulkan.h"
#endif
*/


// INLINE FUNCS OF VARIOUS OBJECTS
///===============================


//#define IX_LOAD_INLINES 1

#include "ix/draw/vkDraw.inl"
//#include "ix/ixVulkan.h"


void ixVulkan::Ortho::cmdBindPipeline(VkPipeline in_p) { /*if(currentPipeline!= in_p) {*/ _ix->vk.CmdBindPipeline(*cmd[_ix->vki.fi], VK_PIPELINE_BIND_POINT_GRAPHICS, in_p); /*currentPipeline= in_p; }*/ }

void ixVulkan::RenderPass::updateViewportAndScissor() {
//  viewport= { 0.0f,                            ((float)_ix->vki.swap.handle->dy),   // x, y    >>> viewport hack; y0+= dy; <<<
//              (float)_ix->vki.swap.handle->dx, -((float)_ix->vki.swap.handle->dy),  // dx, dy  >>> viewport hack: dy= -dy; <<<
//              0.0f, 1.0f };                                                         // minDepth, maxDepth

  viewport= { 0.0f, 0.0f, (float)_ix->vki.swap.handle->dx, (float)_ix->vki.swap.handle->dy, 0.0f, 1.0f }; // minDepth, maxDepth
  scissor= {{ 0, 0 }, { _ix->vki.swap.handle->dx, _ix->vki.swap.handle->dy } };
}



inline bool ixTexture::load(cchar *in_fn)    { return _ix->res.tex.load(this, in_fn); }
inline bool ixTexture::loadMem(cchar *in_fn) { return _ix->res.tex.loadMem(this, in_fn); }
inline bool ixTexture::reload()   { return _ix->res.tex.reload(this); }        // reloads texture from disk (uses fileName as the file)
inline bool ixTexture::upload()   { return _ix->res.tex.upload(this); }
inline bool ixTexture::download() { return _ix->res.tex.download(this); }
inline bool ixTexture::unload()   { return _ix->res.tex.unload(this); }


inline bool ixMesh::load(cchar *in_f, ixFlags32 in_flags) { return _ix->res.mesh.load(in_f, this, in_flags); }
inline bool ixMesh::save(cchar *in_f) { return _ix->res.mesh.save(in_f, this); }
inline bool ixMesh::upload()   { return _ix->res.mesh.upload(this); }
inline bool ixMesh::download() { return _ix->res.mesh.download(this); }
inline bool ixMesh::unload()   { return _ix->res.mesh.unload(this); }
inline bool ixMesh::loadOBJ(cchar *in_file, uint32 *in_OBJmeshNr, const char **in_OBJmeshNames) { return _ix->res.mesh.loadOBJ(in_file, 1, (ixMesh **)this, in_OBJmeshNr, in_OBJmeshNames); }





