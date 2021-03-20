#include "ix/ix.h"
//#include <stdio.h>

// IX.exe app, must be an all-knowing thing, a tex viewer/mat lib editor, i3d viewer, all-in one. not 100 things.
// even the cmd-line stuff it should do, like convert and everything.


// online shader compiler & preview http://shdr.bkcore.com

/*
* on LOADING:
* there can be a massive stage buffer for host memory, instead of tex's small allocs of data
* and no alloc would happen, data would be always streaming from one place to another...
* this can be made easy, i think, in Tex class
* >>> there could be 2 of them, for double buffer work, on 2 threads... this would rock on speed i think <<<
* 

YE, but with Vulkan, it's another story.... so there can be more threads for graphics
+======================================================================================================================+
|THREADS and OPENGL:                                                                                                   |
| there is no way to make 1 ix per thread. when you do a makeCurrent, all funcs will be sent to that last make current |
| with 2 threads, you cannot send funcs to different renderers, NOT WITH OPENGL                                        |
| SO                                                                                                                   |
| all ix engines will nicely draw all stuff, ONE AFTER THE OTHER, there is no other way around it.                     |
|======================================================================================================================+
*/


/* TODO:
  - adopt uints more often as a general guideline - they work faster
    errors can have ~0 as a value  uint a= ~0; ( = max uint, basically)
    i was using errors as          int a= -1;
    
  - all global objects should be in ix.cpp, not spread over diff cpp's
    because: the order of linking can differ, so objects sometimes get constructed before other objects and viceversa
    the only ones would be the objects that are not part of ix, i guess, but even in osi, i think all globals should be in osinteraction.cpp

 - a major coordinate redesign for ortographic? every coord would be in virtual desktop position maybe
   the ortographic camera matrix has to have an origin, so things can be easily done

  // THE PROGRAM WOULD USE HOOKS TO THE PRIMARY WINDOW, SO 0,0 WOULD BE THE WINDOW BOTTOM LEFT CORNER
  // IF THE WINDOW WOULD BE ON THE VIRTUAL DESKTOP, THAT WOULD BE AN OPTION ALSO
  BUT AS A GUIDELINE, EVERYTHING MUST BE TIED TO THE PRIMARY WINDOW, IF THERE IS SOMETHING TO BE DRAWN ON THE ORTHOGRAPHIC SPACE, I AM THINKING
  THERE COULD BE A HOOK IN PRINT ALSO, SO YOU SET IT UP AT START, TO THE MAIN WINDOW FOR EXAMPLE, SO THINGS ARE BIT SIMPLER




   it would be more to the hand of the program to be sure where to draw? i think
   THERE NEEDS TO BE MORE THINKING INTO THIS
   atm coords when drawing in opengl are substracted the opengl viewport window coords





 - Ix::getTargetPos() NEEDS UPDATE IN CASE OF RENDER TO TEXTURE OR WHATEVER WILL BE DONE IN THE FUTURE

 - isActive() - another thing must happen here... some 'active' flag... what's happening on alt-tab? that's another problem

 - every bynary file should have a header of 4 chars. 
 - maybe a simple security too, a crc? but doing a crc is too slow... maybe a file size? jump to file end and see if that's the last cher?
 - both crc/filesizes i think. crc is only when verifying data, if you wanna crc, why not have the option?
 - this must be implemented with the big file / packed file / the file that holds many files...

 - i think there is the need for the Ix cursor, that can be moved with either mouse or joystick or even keyboard.
   this might be left for a later date tho


*/

/* MEMORY:
==========
https://gpuopen.com/vulkan-device-memory/


Vulkan Device Memory
Posted on July 21, 2016 by Timothy Lottes
 Device Memory, driver, GCN, Heap, Memory Type, Vulkan
EDIT: 2016/08/08 � Added section on Targeting Low-Memory GPUs

This post serves as a guide on how to best use the various Memory Heaps and Memory Types exposed in Vulkan on AMD drivers,
starting with some high-level tips.

GPU Bulk Data
Place GPU-side allocations in DEVICE_LOCAL without HOST_VISIBLE. Make sure to allocate the highest priority resources first like Render Targets
and resources which get accessed more often. Once DEVICE_LOCAL fills up and allocations fail, have the lower priority allocations fall back
to CPU-side memory if required via HOST_VISIBLE with HOST_COHERENT but without HOST_CACHED. When doing in-game reallocations (say for display
resolution changes), make sure to fully free all allocations involved before attempting to make any new allocations. This can minimize
the possibility that an allocation can fail to fit in the GPU-side heap.

CPU-to-GPU Data Flow
For relatively small total allocation size (under 256 MB) the DEVICE_LOCAL with HOST_VISIBLE is the perfect Memory Type for
CPU upload to GPU cases: the CPU can directly write into GPU memory which the GPU can then access without reading across the PCIe bus.
This is great for upload of constant data, etc.

GPU-to-CPU Data Flow
Use HOST_VISIBLE with HOST_COHERENT and HOST_CACHED. This is the only Memory Type which supports cached reads by the CPU.
Great for cases like recording screen-captures, feeding back Hierarchical Z-Buffer occlusion tests, etc.

Pooling Allocations
EDIT: Great reminder from Axel Gneiting (leading Vulkan implementation in DOOM� at id Software), make sure to pool a group of resources,
like textures and buffers, into a single memory allocation. On Windows� 7 for example, Vulkan memory allocations map to
WDDM Allocations (the same lists seen in GPUView), and there is a relatively high cost associated for a WDDM Allocation as command buffers
flow through the WDDM based driver stack. Having 256 MB per DEVICE_LOCAL allocation can be a good target, takes only 16 allocations to fill 4 GB.

Hidden Paging
When an application starts over-subscribing GPU-side memory, DEVICE_LOCAL memory allocations will fail.
It is also possible that later during application execution, another application in the system increases its usage of GPU-side memory,
resulting in dynamic over-subscribing of GPU-side memory. This case can result in an OS (for instance Windows� 7)
to silently migrate or page GPU-side allocations to/from CPU-side as it time-slices execution of each application on the GPU.
This can result in visible �hitching�. There is currently no method to directly query if the OS is migrating allocations in Vulkan.
One possible workaround is for the app to detect hitching by looking at time-stamps, and then actively attempting to reduce DEVICE_LOCAL
memory consumption when hitching is detected. For example, the application could manually move around resources to fully empty DEVICE_LOCAL
allocations which can then be freed.

EDIT: Targeting Low-Memory GPUs
When targeting a memory surplus, using DEVICE_LOCAL+HOST_VISIBLE for CPU-write cases can bypass the need to schedule an extra copy.
However in memory constrained situations it is much better to use DEVICE_LOCAL+HOST_VISIBLE as an extension to the DEVICE_LOCAL heap
and use it for GPU Resources like Textures and Buffers. CPU-write cases can switch to HOST_VISIBLE+COHERENT. The number one priority
for performance is keeping the high bandwidth access resources in GPU-side memory.

Memory Heap and  Memory Type � Technical Details
Driver Device Memory Heaps and Memory Types can be inspected using the Vulkan Hardware Database. For Windows AMD drivers,
below is a breakdown of the characteristics and best usage models for all the Memory Types. Heap and Memory Type numbering is not guaranteed
by the Vulkan Spec, so make sure to work from the Property Flags directly. Also note memory sizes reported in Vulkan represent
the maximum amount which is shared across applications and driver.

Heap 0
VK_MEMORY_HEAP_DEVICE_LOCAL_BIT
Represents memory on the GPU device which can not be mapped into Host system memory
Using 256 MB per vkAllocateMemory() allocation is a good starting point for collections of buffers and images
Suggest using separate allocations for large allocations which might need to be resized (freed and reallocated) at run-time
Memory Type 0
VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
Full speed read/write/atomic by GPU
No ability to use vkMapMemory() to map into Host system address space
Use for standard GPU-side data
Heap 1
VK_MEMORY_HEAP_DEVICE_LOCAL_BIT
Represents memory on the GPU device which can be mapped into Host system memory
Limited on Windows to 256 MB
Best to allocate at most 64 MB per vkAllocateMemory() allocation
Fall back to smaller allocations if necessary
Memory Type 1
VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
Full speed read/write/atomic by GPU
Ability to use vkMapMemory() to map into Host system address space
CPU writes are write-combined and write directly into GPU memory
Best to write full aligned cacheline sized chunks
CPU reads are uncached
Best to use Memory Type 3 instead for GPU write and CPU read cases
Use for dynamic buffer data to avoid an extra Host to Device copy
Use for a fall-back when Heap 0 runs out of space before resorting to Heap 2
Heap 2
Represents memory on the Host system which can be accessed by the GPU
Suggest using similar allocation size strategy as Heap 0
Ability to use vkMapMemory()
GPU reads for textures and buffers are cached in GPU L2
GPU L2 misses read across the PCIe bus to Host system memory
Higher latency and lower throughput on an L2 miss
GPU reads for index buffers are cached in GPU L2 in Tonga and later GPUs like FuryX
Memory Type 2
VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
CPU writes are write-combined
CPU reads are uncached
Use for staging for upload to GPU device
Can use as a fall-back when GPU device runs out of memory in Heap 0 and Heap 1
Memory Type 3
VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
VK_MEMORY_PROPERTY_HOST_CACHED_BIT
CPU reads and writes go through CPU cache hierarchy
GPU reads snoop CPU cache
Use for staging for download from GPU device

Choosing the correct Memory Heap and Memory Type is a critical task in optimization. A GPU like Radeon� Fury X for instance has 512 GB/s
of DEVICE_LOCAL bandwidth (sum of any ratio of read and write) but the PCIe bus supports at most 16 GB/s read and at most 16 GB/s write
for a sum of 32 GB/s in both directions.

Timothy Lottes is a member of the Developer Technology Group at AMD. Links to third party sites are provided for convenience and unless explicitly
stated, AMD is not responsible for the contents of such linked sites and no endorsement is implied.







VULKAN gpu features

  VkPhysicalDeviceFeatures , you can simply get the features of the GPU and when creating the device you can pass the structure back, enabling everything

-from registry:
  "Some features, such as robustBufferAccess, may incur a run-time performance cost.
  Application writers should carefully consider the implications of enabling all supported features."
  

  
i think this is exactly for a windowing system:
VK_NV_corner_sampled_image #051 [device]   [new image organization <corner-sampled> (ints, best for texture dictionary i guess)] https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VK_NV_corner_sampled_image
VK_NV_fill_rectangle       #154 [device]   [draw rectangles with only one triangle] https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VK_NV_fill_rectangle






another way to make osi link vkDevices to windows... see if they work or not i guess

https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#_querying_for_wsi_support
i think this is the answer ^^^


vkGetPhysicalDeviceSurfaceSupportKHR  <<<


maybe create mini-windows on all monitors and use  vkGetPhysicalDeviceSurfaceSupportKHR ??? this might be the answer
in any case this func is the answer i think, there's no other option that i see
and it's the only thing it's even listed


+
there are specific funcs too, for every os, to see the queuefamily support
vkGetPhysicalDeviceWaylandPresentationSupportKHR
vkGetPhysicalDeviceWin32PresentationSupportKHR
vkGetPhysicalDeviceXcbPresentationSupportKHR
vkGetPhysicalDeviceXlibPresentationSupportKHR
^^^^^^^


create tmp windows on each monitor to see what physical device works on them?
or, when you choose your physical device you just see what works on the monitor you whant, i think



loosing a surface:
"Several WSI functions return VK_ERROR_SURFACE_LOST_KHR if the surface becomes no longer available.
After such an error, the surface (and any child swapchain, if one exists) should be destroyed,
  as there is no way to restore them to a not-lost state.
Applications may attempt to create a new VkSurfaceKHR using the same native platform window object,
  but whether such re-creation will succeed is platform-dependent and may depend on the reason the surface became unavailable.
A lost surface does not otherwise cause devices to be lost."



VkDisplayPowerStateEXT
you can see the power state of the display if you wish (Suspend/off/on)
you can set it too i think






*/


/*

DESIGNING STUFF, MEMORY FRAGMENTATION, MEMORY ALOCATION THINKING
================================================================

- for any blocks of memory that need alocation / dealocation and are the same size, the segList is the answer, and there won't be any fragmentation
- textures would use seglists, for sure
- there are multiple texture sizes tho, so you'd need mutliple seglists, where would you put them in memory...
- the fixed stuff could be at start
- there could be no difference between allocationg at the end and backwards, and allocating after the fixed stuff
  so there will be 2 fixed points, after fixed stuff and at the end.

  or a system that would know where to fit other textures, when there are gaps, due you know the slot sizes...
  so slots would appear at certain points hmm

  -maybe a system with slots, the minimum slot would be 128x128 texture? and you go forward from that. a 512x512 would need 16 slots
   a 1024x1024 texture would need 16x16 slots... hmmm

   or, for each texture size, you'd have a vkMemory, fixed number, and deal if there's not enough by clearing some older stuff?

   i think the latter method would be better

   M1: 1 contigious vkMemory for all texture sizes, all sizes will have a fixed chunk from that vkMemory
       
   M2: 1 vkMemory for each texture size

   M3: 1 contigious vkMemory for all texture sizes, each texture size will have "slots". the lowest level slot would be 128x128 or higher
       a 512x512 texture would occupy 16x16 slots if i compute right
       but you'd need a complex and clever system to know where slots are free
       so you'd have a list of slots for the whole memory. not that bad, and not that hard to implement
       you could have a rule that big textures would be alocated at the end
       and 128x128 + 256x256 would be alocated at start?

       atlases ...
       yeah...

       i dono what to say... vulkan is already 3 years in and nothing is changing
       so nvidia cannot even benefit from vulkan atm, there's like 3% improvements
       on the amd cards you see huge speeds, from all the multiple threadings


       to be or not to be, this is the question
       it... seems they wanna go forward with it... i dono , but it's extreem fuzy if they continue vulkan or not
       cuz nobody gives a damn about it atm, i mean, the amount of time you have to invest only to make it work is just too big

       i'm already 4-5 months into this
       in this time i could've started the darn game, and i am pretty sure something nice would've been done by now
       this vulkan thing is really starting to drag me down



       OK SECOND DAY OF THINKING:
       - everything must be perfectly ordered, so the need for a defragmentor disapears
       - the memory will be extreem good managed to the byte
       - the problem will be vertex attribute buffers. they will vary in length, but they are way smaller, at least.
       - can you make block of vertex attribute buffers? then what? you'd still have to put other buffer objects in these blocks and still need defrag
       - ofc you can make slots and know how many slots the buffer would take, but still there will be wasted space appearing between buffers

        you need to eliminate the wasted space between buffers, that's the problem
        can you do this without a defragmentor?
        could this problem not exist with a good(perfect) buffer ordering?

        neah, there's no way, you need defragmenter

        soo... a defragmenter too
        this is going too far


        ===================================================
        the main question is:
        would you not need a defragmenter in a system that adds and substracts vertex attrib blocks?
        ===================================================

        so im not sure that vulkan would limit this
        i am at the point of creating all these objects easy anyway, so i dono what would opengl gain me atm


        i think i should start doing it. i went too far anyway
       

       vvvvvvvvvvvvvv
        so... the defragger would copy 1 object per frame, maybe? depends how many objects there are
        so you lock your buffer, and copy 1-2-3 per frame, unlock
        ^^^^^^^^^^^^^^^^^^^^^^
        you'd have more than 1 deffraging buffer, probly i will have this functionality in the vkoBuffer object, with a buffer that can hold the copied thing
        maybe the lock could happen only per item, somehow, not the whole buffer, i dono



        but i am really thinking all the way here
        the memory/buffers are next


*/

/* vulkan
  Command Pools:
  - vk->TrimCommandPool(device, commandPool, 0);
    Trimming may be an expensive operation, and should not be called frequently.
    Trimming should be treated as a way to relieve memory pressure after application-known points
    when there exists enough unused memory that the cost of trimming is “worth” it.

  - vk->ResetCommandPool(device, commandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT); // that flag will release all memory
    Resetting the command pool will remove all allocated memory for command buffers. all recording buffers will be marked invalid
  

  Swap Chain:
  - the swapchain is an extension that has to be enabled, unless i'm gonna do with these imageViews and manually draw them,
      that will probly be a magnitude slower, unless i know the optimal way to show for every OS

  Descriptor Pools:
    -the free func is optional, and must be treated with care, can create fragmentation problems.
       i am thinking whatever is freed, must be alocated with the same block size
       if you use only alloc + reset, it is guaranteed to work.

    -also, descriptor pools can be created on the fly, they are not really tied to anything

    -Vulkan is semaphore city. EVERY submit must have event wait or semaphores or fences. The only ordered instructions are those sent in a single batch.
     if you send 2 batches you dono who finishes first.

*/









// static objects

//std::mutex _ixListMutex;              // mutex for the ixList

/* ADIOS STATIC OBJECTS. ALL ARE DONE WITH THOSE INLINE FUNCTIONS
ixWinSys Ix::wsys;                    // window system
ixConsole Ix::console;                // console

ixWSstyle def1Style, def2Style;       // default windows styles
str8 Ix::Config::shaderDIR;           // shader DIRECTORY
*/

///============================///
// ix construnctors/destructors //
///============================///

void Ix::printIxList() {
  ::
  printf("IX engines list:\n");
  int a= 0;
  for(Ix *i= (Ix *)Ix::ixList().first; i; i= (Ix *)i->next, a++) {
    printf("Ix[%d]: address[%llu]\n", a, (uint64)i);
  }
  printf("end of list.\n");
}


Ix::Ix(): ixClass(ixClassT::IX), res(this)
  #ifdef IX_USE_VULKAN
  , vki(this)
  #endif
{
  cameraPersp._parent= this;
  cameraOrtho._parent= this;
  camera= &cameraPersp;
  pr._ix= this;
  
  _lastFrameTime= _frameTime= 0;
  FPS= 0;

  // configuration defaults

  cfg.engine= 1;                      // Vulkan
  cfg.errorUseConsoleFlag= true;

  cfg.vk.physicalDeviceIndex= ~0u;
  #ifdef NDEBUG
  cfg.vk.enableValidationLayers= false;
  #else
  cfg.vk.enableValidationLayers= true;        // <<< TURN THIS OFF IF YOU WANT VALIDATION LAYERS OFF
  #endif

  #ifdef IX_USE_VULKAN
  cfg.vk.queueRequestUniversal= 3;
  cfg.vk.queueRequestGraphics=  0;
  cfg.vk.queueRequestCompute=   1;
  cfg.vk.queueRequestTransfer=  1;
  cfg.vk.gpuFeatures= {};

  cfg.vk.resourcesUseSharedQueueFamilies= false;

  cfg.vk.staticTexturesDynamicSetSegmentSize= 20;   // room for 20 textures per segment
  cfg.vk.printDynamicSetSegmentSize= 10;

  // default mem / cluster sizes    totals - 185MB
  cfg.vk.size_clusterIxDevice=   33554432; // 32MB
  cfg.vk.size_clusterIxHost=     33554432; // 32MB
  cfg.vk.size_clusterResDevice=  67108864; // 64MB
  cfg.vk.size_clusterResHost=    33554432; // 32MB
  cfg.vk.size_stageBufferDevice=  1048576; //  1MB
  cfg.vk.size_stageBufferHost=   25165824; // 24MB - will fit a 2048x2048 texture (including mipmaps) (CAN BE 8MB FOR JUST 1024x1024)
  #endif

  ren= null;
  gpu= null;
  win= null;

  fnt5x6= null;
  
  debugStyle.drawMode= 0x0004;
  debugStyle.outlineSize= 1;
  debugStyle.dblPrecision= 4;


  _ixListMutex().lock();
  ixList().add(this);
  //if(this== getMain())
  //  console.setParent(this);
  _ixListMutex().unlock();


  //delData();
}


Ix::~Ix() {
  if(ixList().isMember(this)) // shutdown could've already be called
    shutdown();
}





void Ix::delData() {
  tgtPos= vec3(0.0f, 0.0f, 0.0f); 
  clipboard.delData();

  /// debug font
  pr.delData();
  //pr.delFont(fnt5x6);
  fnt5x6= null;
  
  //wsys.delShader(this);
}


void Ix::shutdown() {
  _ixListMutex().lock();
  ixList().release(this);
  _ixListMutex().unlock();

  if(ren) {
    #ifdef IX_USE_VULKAN
    if(renVulkan())
      vk.DeviceWaitIdle(vk);
    #endif
    #ifdef IX_USE_OPENGL
    if(renOpenGL())
      glMakeCurrent();
    #endif
  }

  pr.delData();
  res.tex.delData(); //  <<<<<<<  texSys must unload everything before vulkan systems shutdown
  res.mesh.delData();
  res.shader.delData();

  delData();

  #ifdef IX_USE_VULKAN
  vki.shutdown();
  #endif

  #ifdef IX_USE_OPENGL
  if(ren)
    if(renOpenGL())
      osi.glMakeCurrent(null, null);
  #endif

  if(!ixList().nrNodes)
    exitCleanup();
}


void Ix::exitCleanup() {
  console().delData();
  glb.def1Style().delData();
  glb.def2Style().delData();
  wsys().delData();
}



/*
// NEEDS UPDATE IN CASE OF RENDER TO TEXTURE OR WHATEVER WILL BE DONE IN THE FUTURE
void Ix::getTargetPos(int32 *out_x0, int32 *out_y0) {
  // THIS->GLR THIS->WIN SINCE WERE ADDED. THERE ARE MULTIPLE IX ENGINES NOW.
  if(osi.glr && osi.glrWin) {
    if(out_x0) *out_x0= osi.glrWin->x0;
    if(out_y0) *out_y0= osi.glrWin->y0;
  } else {
    if(out_x0) *out_x0= 0;
    if(out_y0) *out_y0= 0;
  }
}
*/

#ifdef IX_USE_OPENGL
void Ix::glMakeCurrent() {
  osi.glMakeCurrent((osiRenderer *)ren, win);
  *glActiveRen()= this;
}
#endif




// main Ix init

void Ix::init() {
  error.useConsoleFlag= cfg.errorUseConsoleFlag;
  //bool chatty= true;
  #ifdef IX_USE_VULKAN
  if(cfg.engine== 1) {
    // vulkan instance build
    if(cfg.vk.enableValidationLayers)
      vk.cfg.addValidationLayer("VK_LAYER_KHRONOS_validation");

    vk.cfg.queueRequestUniversal= cfg.vk.queueRequestUniversal;
    vk.cfg.queueRequestCompute=   cfg.vk.queueRequestCompute;
    vk.cfg.queueRequestGraphics=  cfg.vk.queueRequestGraphics;
    vk.cfg.queueRequestTransfer=  cfg.vk.queueRequestTransfer;
    
    #ifdef OS_WIN
    vk.cfg.extensions.instance.vk_KHR_win32_surface.enable= 1;
    #endif
    #ifdef OS_LINUX
    
    vk.cfg.extensions.instance.vk_KHR_xlib_surface.enable= 1;
    vk.cfg.extensions.instance.vk_KHR_display.enable= 1;
    vk.cfg.extensions.instance.vk_EXT_direct_mode_display.enable= 1;
    vk.cfg.extensions.instance.vk_EXT_acquire_xlib_display.enable= 1;
    //vk.cfg.extensions.instance.vk_EXT_acquire_xlib_display.enable= 1;
    #endif
    #ifdef OS_MAC
    makeme;
    error.makeme();
    #endif

    vk.cfg.extensions.instance.vk_KHR_surface.enable= 1;
    if(cfg.vk.enableValidationLayers)
      vk.cfg.extensions.instance.vk_EXT_debug_utils.enable= 1;
    if(!vk.buildInstance())
      error.vkWindow("Ix::init(): Vulkan instance failed to build.", vk.errorText, vk.result, true);

  }
  #endif
}


// secondary Ix init (after window creation)
void Ix::initWindow(osiWindow *in_w) {
  /// simple checks
  if(in_w== null) { error.simple("Ix init failed: in_w is null"); return; }
  if(in_w->renderer== null) { error.simple("Ix init faild: window's renderer is null"); return; }
  if(in_w->renderer->type> 1) { error.simple("Ix init failed: unknown renderer type"); return; }
  
  //if(in_w->renderer->type!= cfg.engine) { error.simple("Ix init failed: requested engine and window's renderer do not match"); return; }
  
  #ifdef IX_USE_VULKAN
  if(cfg.vk.physicalDeviceIndex!= ~0u)    // physical device out of bounds check
    if(cfg.vk.physicalDeviceIndex> vk.info.nrPhysicalDevices()) { error.simple("Ix init failed: requested physical device index out of bounds"); return; }
  #endif

  win= in_w;
  ren= in_w->renderer;
  gpu= ren->monitor->GPU;

  #ifdef IX_USE_OPENGL  // opengl renderer
  if(ren->type== 0) {
    glo.init((osiGlRenderer *)ren);
  }
  #endif /// IX_USE_OPENGL

  #ifdef IX_USE_VULKAN  // vulkan renderer
  if(ren->type== 1) {
    vki.initAfterWindow();
    res.tex.vkd.init();
    vki.draw.init();        // draw has a no-texture ixTexture. texSys must be working at init
  }
  #endif /// IX_USE_VULKAN
  
  //_FUNCconsole= &console.ixErrorPrint;
  
  // ALL CLASSES INIT HERE

  res.mat._init();

  pr.init();

  if(this== getMain())
    console().init();

  //wsys.loadAssets(this);

  /// debug font
  fnt5x6= pr.loadFont("5x6.fnt", 6);
  debugStyle.selFont= fnt5x6;
}








void Ix::update() {

  #ifdef IX_USE_OPENGL
  if(osi.flags.windowMoved || osi.flags.windowResized) {
  
    if(ren)
      if(renOpenGL())
        if(*glActiveRen()!= this)
          glMakeCurrent();
    
    
    // shaders.updateAllViewportPos(); <<<<<<<<<<<<<<<<

    // CAMERA CAN BE AND SHOULD BE UPDATED HERE, I THINK
    // this func shuold be the main ix update per frame, it should take care of everything
    // almost like a osi.checkMSG()
  }
  #endif

  #ifdef IX_USE_VULKAN
  if(osi.flags.minimized)
    if(vki.swap.handle->swapchain) {
      vk.QueueWaitIdle(*vki.q1);
      vki.swap.handle->destroy();
    }

  if(osi.flags.windowResized) {
    vki.swap.rebuild();
  }

  #endif

  //cleanup();
}








bool Ix::startRender() {
  if(!win) return false;
  if(!win->isCreated) return false;

  #ifdef IX_USE_VULKAN
  if(renVulkan()) {
    return vki.render.startRender();
  }
  #endif /// IX_USE_VULKAN

  return false;
}


void Ix::endRender() {
  if(!win) return;
  if(!win->isCreated) return;

  #ifdef IX_USE_VULKAN
  if(renVulkan()) {
    vki.render.endRender();
  }
  #endif /// IX_USE_VULKAN
}


bool Ix::startPerspective() {
  if(!win) return false;
  if(!win->isCreated) return false;

  cameraPersp.setPerspective(vec3(0.0f), vec3(0.0f, 0.0f, 1.0f), 80.0f, (float)win->dx/ (float)(win->dy? win->dy: 1), 0, 1200);
  camera= &cameraPersp;

  #ifdef IX_USE_VULKAN
  if(renVulkan()) {
    return true; // nothing atm
  }
  #endif /// IX_USE_VULKAN
  
  return false;
}

void Ix::endPerspective() {
}


bool Ix::startOrtho() {
  /*
  option 1: one huge command buffer
  1 big command buffer for the whole ortho:
   - you have to have an order thru all
   - 1 queue that draws the whole big buffer (+his secondaries) is enough imho


   >>> if the queue must draw this then this then this then this, there cannot be paralelism, from what i gather
      - having 2 queues pump data would only make more code, to wait for stuff, imho
      - the 2 queues would be waiting anyway for the other to finish, so there's only one job happening anyway
      
     
   
  option 2: command buffer that waits for himself to finish the job
    - the CPU would have to wait for the GPU to finish the job, then record
    - there will be lag between the cpu and gpu jobs

  option 3: 2 command buffers and switch between them
    - in theory it can be possible
    - there is added complexion to the whole buisness
    - in the end, would it be faster than just one cmd buffer?
    
  - i see no way to do this with any paralelism, when you need an order of things

     CONCLUSION:
   >>>ONE BIG CMD BUFFER<<<, with secondaries where you can, for ortho.
  */
  if(!win) return false;
  if(!win->isCreated) return false;

  // virtual desktop coords
  cameraOrtho.setOrtho(rectf((float)win->x0, (float)win->y0, (float)(win->x0+ win->dx), (float)(win->y0+ win->dy)), -1000.5f, 100.5f);
  // viewport coords
  //cameraOrtho.setOrtho(rectf((float)(win->dx), (float)(win->dy)), -1000.5f, 100.5f);

  camera= &cameraOrtho;
  
  #ifdef IX_USE_VULKAN
  if(renVulkan()) {
    return vki.render.startOrtho();
  }
  #endif // IX_USE_VULKAN

  return false;     /// failed if reached this point
}


void Ix::endOrtho() {
  if(!win) return;
  if(!win->isCreated) return;

  #ifdef IX_USE_VULKAN
  if(renVulkan()) {
    vki.render.endOrtho();
  }
  #endif
}



void Ix::measureFPS() {
  osi.getNanosecs(&_frameTime);
  FPS= (_lastFrameTime?     (1000000000/ (float)(_frameTime- _lastFrameTime)):    0);
  _lastFrameTime= _frameTime;

  // MAYBE AN AVERAGE CAN HAPPEN, EITHER JUST WITH LAST FPS, OR OVER 1 SECOND
}



// VULKAN specific

#ifdef IX_USE_VULKAN

#endif





/*
bool Ix::ResourceSystem::cleanup_ONE() {
  if(mesh.cleanup_ONE()) return true;
  if(mat.cleanup_ONE()) return true;
  if(tex.cleanup_ONE()) return true;
  if(shader.cleanup_ONE()) return true;
  return false;
}


void Ix::ResourceSystem::cleanup_ALL() {
  mesh.cleanup_ALL();
  mat.cleanup_ALL();
  tex.cleanup_ALL();
  shader.cleanup_ALL();
}
*/











