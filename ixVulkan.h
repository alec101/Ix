#pragma once
#ifdef IX_USE_VULKAN
//#include "ix/draw/vkDraw.h"

class ixVulkan;
class ixvkResource;
class ixvkBuffer;
class ixvkImage;
class ixvkResCluster;
class vkDraw;


class ixvkDescPool;
class ixvkDescSet;



///========================================///
// ixvkResCluster class - Resources Cluster //
///========================================///
// -standard type of alocation for any buffer/texture (standard, as in, dumb, good for few static resources, that you know should always be there)
// -use for static resources, of any size, not part of any arrayLayer/atlas


// ############################################################################################################################
// >>> in any case, a huge cleanup must happen but after i see how objects and textures and meshes work with each other <<<
// >>> it is too early for a cleanup, when you dono what objects are important and what to change <<<
// ############################################################################################################################

class ixvkResClusterSegment;

class ixvkResCluster: public ixClass {
public:
  Ix *_ix;                             // parent ix engine
  
  uint64 segmentSize;                 // [def:0] the size of one cluster
  VkMemoryPropertyFlags memReq;       // [def:VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT] required memory property
  VkMemoryPropertyFlags memPref;      // [def:0] prefered memory property
  uint32_t memTypeBits;               // [def:~0] you can specify a direct memory to use; if left default, memReq+memPref will be used to search for one

  chainList segments;                 // [chainData:ixvkResClusterSegment] list with all segments

  /* DEFRAG brainstorm:
  - on each delete, you can mark all resources past the deleted one, to move a certain number of bytes.
  - alignments must be recalculated for ALL movements
  */
  uint32 defragBehaviour;             // [def:0] 0: no automatic defrag; 1: auto-defrag on each delete; 

  // in_size: [def:0] set the memory segment size; when running out of memory, another segment will be alocated
  // in_memReq: [def:VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT] set the memory requirement
  // in_memPref: [def:0] set another prefered memory atribute
  inline void cfg(uint64 in_size, VkMemoryPropertyFlags in_memReq, VkMemoryPropertyFlags in_memPref= 0, uint32 in_memType= ~0) { segmentSize= in_size, memReq= in_memReq, memPref= in_memPref, memTypeBits= in_memType; }
  
  bool allocRes(ixvkResource *out_res);
  void delResource(ixvkResource *out_res, bool in_delete= true);    // in_delete: delete the chain node or release it

  ixvkResCluster(Ix *in_ix);
  ~ixvkResCluster();
  void delData();

private:
  ixvkResClusterSegment *_getSegment(VkMemoryRequirements *in_memReq);    // returns the first segment with requrested free size
  

  // returns difference between aligned offset and offset
  inline static uint32 _getAlignOffsetDelta(VkDeviceSize in_offset, VkDeviceSize in_alignment) {
    VkDeviceSize m= in_offset% in_alignment;
    return (uint32)(m? in_alignment- m: 0);
  }

  // aligns <in_offset>
  inline static VkDeviceSize _getAlignedOffset(VkDeviceSize in_offset, VkDeviceSize in_alignment) {
    VkDeviceSize m= in_offset% in_alignment;
    return (in_offset+ (m? in_alignment- m: 0));
  }

  friend class ixVulkan;
  friend class ixvkBuffer;
  friend class ixvkImage;
};



class ixvkResClusterSegment: public ixClass {
public:
  inline Ix *_ix() { return cluster->_ix; }
  ixvkResCluster *cluster;   // parent ResCluster

  VkoMemory *memory;        // memory of this segment
  chainList resources;      // [ixvkBuffer/ixvkTexture:chainData] ixvkBuffers/images in this segment
  VkDeviceSize freeSize;    // remaining memory freespace
  VkDeviceSize freeOffset;  // offset start of the freespace

  // funcs

  void updateFreespace();
  bool build();

  ixvkResClusterSegment(ixvkResCluster *);
  ~ixvkResClusterSegment();
  void delData();
}; /// ixvkResCluster









// ixvk RESOURCE class
///===================

class ixvkResource: public ixClass {
public:
  inline Ix *_ix() { return cluster->_ix; }
  ixvkResCluster *cluster;            // parent cluster
  ixvkResClusterSegment *segment;     // parent segment - populated on build

  uint32 defragDelta;                 // amount of movement for the whole buffer
  uint32 defragDeltaInside;           // amount of movement for the inside movement defrag
  uint32 defragOffset;                // [~0: whole handle move in memory] / [offset of the defrag - INSIDE defrag only]
  uint32 defragSize;                  // size of the INSIDE defrag only




  //std::atomic_flag defragFlag;        // fast raise flag
  //std::atomic_uint32_t defragDelta;   // [oldoffset - defragDelta = new offset] - if not zero, defrag must happen
  //std::atomic_uint32_t defragOffset;  // ~0 = means whole buffer is moved in memory ELSE defrag happens within the buffer
  //std::atomic_uint32_t defragSize;    // the amount to be copied for a within buffer defrag
  //uint32_t defragSize;    // the amount to be copied for a within buffer defrag

  struct Access {
    uint32 qFamily;
    VkImageLayout layout;
    Access(): qFamily(~0), layout(VK_IMAGE_LAYOUT_UNDEFINED) {}
    /*
    VkAccessFlags firstAccess;          // [def:VK_ACCESS_MEMORY_READ_BIT| VK_ACCESS_MEMORY_WRITE_BIT] the first way the resource is accessed; making this more accurate, is better
    VkAccessFlags lastAccess;           // [def:VK_ACCESS_MEMORY_READ_BIT| VK_ACCESS_MEMORY_WRITE_BIT] last way the resource will be accessed;
    VkPipelineStageFlags firstStage;    // [def:TOP_OF_PIPE] first stage that the resource uses; moving this lower in the pipeline (acurate), will make tool activities (defrag/copy/etc) halt the pipeline for a shorter time
    VkPipelineStageFlags lastStage;     // [def:BOTTOM_OF_PIPE] last stage the resource will be used in; making this accurate is better
    Access(): currentQueue(null),
              firstAccess(VK_ACCESS_MEMORY_READ_BIT| VK_ACCESS_MEMORY_WRITE_BIT),
              lastAccess(VK_ACCESS_MEMORY_READ_BIT| VK_ACCESS_MEMORY_WRITE_BIT),
              firstStage(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT),
              lastStage(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT),
              currentLayout(VK_IMAGE_LAYOUT_UNDEFINED) {}
              */
  };

  virtual VkMemoryRequirements *mem()= 0; // memRequirements of the resource
  virtual VkDeviceSize *offset()= 0;       // memory offset (memory start is ref point)

  virtual bool build()= 0;
  virtual bool rebuild()= 0;
  virtual void destroy()= 0;

  void updateSets();

  ixvkResource(ixvkResCluster *in_p): ixClass(ixClassT::VKRESOURCE), cluster(in_p), segment(null), defragDelta(0), defragDeltaInside(0), defragOffset(0), defragSize(0) {}
  inline virtual ~ixvkResource() {}

private:
  //class Set: public onewayData { public: ixvkDescSet *set; Set(ixvkDescSet *s, ixvkResource *r): set(s) { r->_sets.add(this); } };
  //onewayList _sets;                    // sets that use this resource
  
  class Set: public chainData { public: ixvkDescSet *set; Set(ixvkDescSet *s, ixvkResource *r): set(s) { r->_sets.add(this); } };
  chainList _sets;                    // sets that use this resource

  friend class ixvkDescSet;
};



// ixvk BUFFER class
///=================

class ixvkBuffer: public ixvkResource {
public:
  VkoBuffer *handle;
  operator VkBuffer() { return handle->buffer; }

  Access access;



  // set the first pipeline stage + first access where the buffer will be used- ATM USED FOR DEFRAG/TOOL ACTIONS
  // these can be changed at any point in the program.
  //void cfg(VkPipelineStageFlags in_firstStage, VkAccessFlags in_firstAccess) { firstStage= in_firstStage; firstAccess= in_firstAccess; }

  // build will build the handle, and alloc memory from the cluster
  // create steps: 1. new(cluster); 2.configure; 3.build()
  virtual bool build();
  virtual inline bool rebuild() { destroy(); return build(); }
  virtual void destroy();

  // usage funcs

  inline VkDeviceSize         *offset() { return &handle->offset; }
  inline VkMemoryRequirements *mem()    { return &handle->memRequirements; }
  inline VkDeviceSize         *size()   { return &handle->createInfo.size; }

  void barrier(Ix *ix, VkCommandBuffer in_cmd, VkAccessFlags in_srcAccess, VkAccessFlags in_dstAccess, VkPipelineStageFlags in_srcStage, VkPipelineStageFlags in_dstStage, uint32 in_oldFamily= VK_QUEUE_FAMILY_IGNORED, uint32 in_newFamily= VK_QUEUE_FAMILY_IGNORED);
  void barrierRange(Ix *ix, VkCommandBuffer in_cmd, VkDeviceSize in_offset, VkDeviceSize in_size, VkAccessFlags in_srcAccess, VkAccessFlags in_dstAccess, VkPipelineStageFlags in_srcStage, VkPipelineStageFlags in_dstStage, uint32 in_oldFamily= VK_QUEUE_FAMILY_IGNORED, uint32 in_newFamily= VK_QUEUE_FAMILY_IGNORED);

  bool upload(void *in_data, VkDeviceSize in_offset, VkDeviceSize in_size);
  bool download(void *out_data, VkDeviceSize in_offset, VkDeviceSize in_size);

  // constructor / destructor

  ixvkBuffer(ixvkResCluster *in_cluster);
  virtual ~ixvkBuffer();
}; /// ixvkBuffer




// ixvk IMAGE class
///================

class ixvkImage: public ixvkResource {

public:
  VkoImage              *handle;
  VkImageViewCreateInfo viewInfo;
  VkImageView           view;
  Access                *access;   // [def:null] array of access for each image layer - created AFTER setting the size

  operator VkImage() { return handle->image; }

  // CONCEPT - LIKING THIS

  void cfgSize(VkExtent3D in_size, uint32 in_layers, uint32 in_mips= ~0u);
  inline void cfgFormat(VkFormat in_format) { handle->createInfo.format= in_format; }
  inline void cfgUsage(VkImageUsageFlags in_u) { handle->createInfo.usage= in_u; }

  // build will build the handle, and alloc memory from the cluster
  // create steps: 1. new(cluster); 2.configure; 3.build()
  // LEAVE CLUSTER UNBUILT, with size= 0, and the ixvkImage will fill in the size of the segment and build the cluster. Do configure mem requirements tho
  virtual bool build();          
  virtual void destroy();
  virtual inline bool rebuild() { destroy(); return build(); }

  inline VkDeviceSize         *offset() { return &handle->offset; }
  inline VkMemoryRequirements *mem()    { return &handle->memRequirements; }

  void barrier(Ix *ix, VkCommandBuffer in_cmd, uint32 in_layer, VkImageLayout in_newLayout, VkAccessFlags in_srcAccess, VkAccessFlags in_dstAccess, VkPipelineStageFlags in_srcStage, VkPipelineStageFlags in_dstStage, uint32 in_oldFamily= VK_QUEUE_FAMILY_IGNORED, uint32 in_newFamily= VK_QUEUE_FAMILY_IGNORED);
  void barrierRange(Ix *ix, VkCommandBuffer in_cmd, uint32 in_fromLayer, uint32 in_nrLayers, VkImageLayout in_newLayout, VkAccessFlags in_srcAccess, VkAccessFlags in_dstAccess, VkPipelineStageFlags in_srcStage, VkPipelineStageFlags in_dstStage, uint32 in_newFamily= VK_QUEUE_FAMILY_IGNORED);
  void barrierRange2(Ix *ix, VkCommandBuffer in_cmd, VkImageSubresourceRange *in_range, VkImageLayout in_newLayout, VkAccessFlags in_srcAccess, VkAccessFlags in_dstAccess, VkPipelineStageFlags in_srcStage, VkPipelineStageFlags in_dstStage, uint32 in_newFamily= VK_QUEUE_FAMILY_IGNORED);
  void createView(Tex *in_tex= null);       // leave null to use current viewInfo

  bool upload(Tex *in_tex, uint32 in_layer, VkImageLayout in_layout, uint32 in_level= 0, uint32 in_nlevels= 0);  // nlevels=0 means all levels; in_layout= final layout of the image
  bool download(Tex *out_tex, uint32 in_layer, uint32 in_level= 0, uint32 in_nlevels= 0);  // nlevels=0 means all levels

  ixvkImage(ixvkResCluster *in_parent);
  virtual ~ixvkImage();
};

/// inlines for ixvkResource
//VkMemoryRequirements *ixvkResource::mem() { return (type== 0? &((ixvkBuffer *)this)->handle->memRequirements: &((ixvkImage *)this)->handle->memRequirements); }
//VkDeviceSize *ixvkResource::offset()      { return (type== 0? &((ixvkBuffer *)this)->handle->offset:          &((ixvkImage *)this)->handle->offset); }


class ixvkDescPool: public VkoDynamicSetPool {
  Ix *_ix;
public:
  
  void addSet(ixvkDescSet **out_set);

  ixvkDescPool(Ix *in_ix);
  ~ixvkDescPool();

private:
  friend class ixvkDescSet;
};




class ixvkDescSet: public VkoDynamicSet {
public:
  // set[this], binding[binds]
  ixClass **binds;        // the number of binds is taken from the layout
  inline uint32 nbinds() { return _pool->layout->descriptors.nrNodes; }

  void bind(uint32 in_nr, ixClass *in_res); // can bind ixvkBuffers / ixTextures atm, but can be easily expanded.
  void unbind(uint32 in_nr);

  void update();
  void updateStandardMat();     // updates the 4 maps in the material - must be a material set

  ixvkDescSet();
  ~ixvkDescSet();

private:
  ixvkDescPool *_pool;
  inline Ix *_ix() { return _pool->_ix; }
  friend class ixvkDescPool;

  void _link(uint32 bindNr);
  void _unlink(uint32 bindNr);
};



















///===============----------------------------------------///
// IX VULKAN class ======================================= //
///===============----------------------------------------///

class ixVulkan {
  Ix *ix;
public:
  uint32 fi;            // IX FRAME INDEX - 0 or 1 render::endDrawing updates it

  VkoQueue *q1, *q2;    // primary/secondary queues that will be used for rendering
  VkoQueue *qTool;      // tool queue, used for single time commands, texture building, etc (q2 if none avaible)
  VkoQueue *qTransfer;  // transfer queue (fallback is an universal queue #4, then q2, then q1)
  VkoQueue *qCompute;   // compute queue (fallback is an universal queue #5, then q2, then q1)

  bool switchFamily;              // it will be auto-populated, on queue creation; if resources are not sharing queue families, it is neccesary to do queue switches with barriers

  VkoCommandPool *poolTool;       // universal family pool
  VkoCommandPool *poolTrn;        // transfer family pool
  VkoCommandPool *poolCompute;    // compute family pool
  VkoCommandBuffer *cmdTool;      // the command buffer used by ixVulkan for various operations
  VkoCommandBuffer *cmdTrn;       // transfer queue command buffer
  VkoCommandBuffer *cmdCompute;   // compute queue command buffer
  
  vkDraw draw;            // drawing primitives object

  ixTexture *noTexture;   // special texture that can be binded

  struct Swap {
    Ix *_ix;
    VkoSwapchain *handle;             // [def:2 images] the swapchain (the index in VkoSwapchain is not certain, logic clusterf)
    VkoFramebuffer *framebuffer[2];   // the framebuffers, one for each swapchain image
    ixvkImage *depthStencil;

    /// [0x01] - rebuildDepthStencil needed
    ixFlags32 flags;

    void init();                      // init after window is done, renderpass must happen first
    void rebuild();
    void rebuildDepthStencil();       

    Swap(Ix *in_ix): _ix(in_ix), handle(null), framebuffer{null, null}, depthStencil(null) {}
  } swap;


  struct RenderPass {
    Ix *_ix;
    VkoRenderPass *handle;        // the (handle) renderpass that will be used
    VkoCommandPool *pool[2];
    VkoCommandBuffer *cmdMain[2]; // MAIN command buffers [signal[1]+wait[1] semaphores are for defrag]
    VkoFence *fenStart[2];        // signaled by cmdEnd so cmdStat can start
    bool cmdBuild[2];             // THIS MIGHT WORK, BUT YOU NEVER KNOW IF THEY REALLY LET THIS HAPPEN... BASICALLY THE CMD BUFFERS COULD BE BUILT ONLY IF NEW SECONDARIES ARE ADDED
    bool clrImage;
    VkViewport viewport;          // viewport for the whole window, updated on render start
    VkRect2D scissor;             // scissor for the whole window, updated on render start

    VkoCommandBuffer *cmdPrev;    // points to previous frame command buffer

    inline void updateViewportAndScissor(); /// [body in ix.h] update the viewport / scissor
    
    void init();

    // METHOD 1
    bool startRender();   // START render

    bool startOrtho();    // ortho start
    void endOrtho();      // ortho end

    void endRender();     // END render

    // constr / destr

    RenderPass(Ix *in_ix): _ix(in_ix), handle(null), pool{null, null}, cmdMain{null, null}, fenStart{null, null},
                           cmdBuild{true, true}, cmdPrev(null)
                           {}
    inline ~RenderPass() { delData(); }
    void delData();
  } render;




  struct Ortho {
    Ix *_ix;

    VkoCommandPool *pool[2];      // orthographic drawing pools
    VkoCommandBuffer *cmd[2];     // orthographic drawing huge cmd buffer

    VkPipeline currentPipeline;   // current bound pipeline in ortho mode

    inline void cmdBindPipeline(VkPipeline in_p); // [body in ix.h] binds the pipeline only if current pipeline differs
    void init();

    Ortho(Ix *in_ix): _ix(in_ix), pool{null, null}, cmd{null, null}, currentPipeline(VK_NULL_HANDLE) {}
  } ortho;


    
  struct Defrag {
    Ix *_ix;

    VkoCommandPool *poolGfx[2];            /// release + aquire
    VkoCommandPool *poolCompute[2];        /// release + aquire
    VkoCommandPool *poolTrn[2];            /// transfer queue

    VkoCommandBuffer *cmdGfx[2];        /// release + aquire
    VkoCommandBuffer *cmdCompute[2];    /// release + aquire
    VkoCommandBuffer *cmdTrn[2];           /// transfer queue, actually making the 

    VkoCommandBuffer *cmdToSubmit;          /// used only on the 1 defrag per frame; if there's something to do, this will point to what to be submited
    VkQueue qToSubmit;
    ixvkResource *resToUpdate;

    /*
    yes, the new buffer vals are computed imediatly, it can happen, cuz the handle updates happen instant
      but... you need to wait for the prev buffer to happen, and the current one ... or... you do the defrag at the end, and you let it as it is
      it is important to have an order... defrag at start or end... one or the other
      */

    VkoSemaphore *semWait[2];   // pointer to current cmdMain's signal semaphore
    VkoSemaphore *semSignal[2]; // semaphore signaled after defrag is done, next frame's cmdMain will wait for it
    VkoFence *fenceSignal[2];   // fence after job's done - [destroy buffers]

    //VkPipelineStageFlags stageWait; // wait sem stages

    uint64 job;                 // number of jobs avaible
    ixvkBuffer *_buf;           // used to move images; binds over the image's memory

    void recountJobs();

    void init();                // init after window is created


    void doJobs(uint32 n= 1);   // <n>: number of jobs to do - 1 job: fits between frames ; multiple jobs: stop everything and do multiple jobs first.

    void afterSubmitJobs(bool useFence= true);

    void getQueueCmd(uint32 in_qfamily, VkoQueue **out_q, VkoCommandBuffer **out_c);

    void moveBuffer(VkCommandBuffer in_cmd, ixvkBuffer *out_buffer); // , uint32 in_delta);   // moves the buffer lower in mem by [delta]
    void moveImage(VkCommandBuffer in_cmd, ixvkImage *out_image); //, uint32 in_delta);      // moves the image lower in mem by [delta]
    void moveInside(VkCommandBuffer in_cmd, ixvkBuffer *out_buffer); // inside the buffer move operation
    

    //void destroyBuffers(VkoFence *in_fence= null);
    class DBuffer: public segData { public: VkBuffer buf; VkImage img; };
    segList destroyBuffersList[2]; // [10 segment size] list with buffers that need destroying


    Defrag(Ix *in_ix);
    ~Defrag();
  } defrag;


















  // when starting to record a secondary buffer, this will fill in the necesary inheritance info
  // NOT SURE IF NEEDED, THIS CAN JUST BE FILLED IN EASY. void writeInheritanceInfo(VkCommandBufferInheritanceInfo *out_inheritance);

  ixvkBuffer *stageHost;            // stage buffer in host memory, used for copy operations
  ixvkBuffer *stageDevice;          // stage buffer in device memory, used for copy operations

  ixvkResCluster *clusterIxDevice;  // [cfg::size_clusterIxDevice]        global uniform buffer / ix specific stuff
  ixvkResCluster *clusterIxHost;    // [cfg::size_clusterIxHost]          staging buffers
  ixvkResCluster *clusterDevice;    // [cfg::size_resourcesClusterDevice] single textures / single buffers - device memory
  ixvkResCluster *clusterHost;      // [cfg::size_resourcesClusterHost]   single textures / single buffers (UNIFORMS) - host visible memory

  ixvkDescPool *ixStaticSetPool;   // global set, static stuff, has fixed size

  // global set 0, binding 0, 

  
  // MAYBE bool defragAll;   // flag: signal to do all defrag jobs in one go


  void getToolCmdAndQueue(uint32 in_family, VkoQueue **out_queue, VkoCommandBuffer **out_cmd);   // returns the tool queue that has the same family as <in_queue>
  
  void cmdScissor(VkCommandBuffer in_cmd, recti *in_scissor);


  // ix global uniform buffer, placed first, in cluster 1

  // THIS SHOULD BE THE STANDARD FOR EVERY SHADER IN IX <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<,,
  // set 0, binding 0, a big buffer that can be as big as needed, for every shader
  // this way you basically work with pointers, in a way.

  // there could be [set 0, binding 1], but i can't think of it now

  struct GlbBuffer: public ixvkBuffer {
    struct Data {
      mat4 cameraPersp;       // perspective camera matrix
      mat4 cameraOrtho;       // orthographic camera matrix
      vec2 vp;                // viewport position on the virtual desktop
    } data;


    inline uint32 getSize() { return sizeof(Data); }

    VkoDescriptorSetLayout *layout;     // [points to ixStaticPool] layout of global buffer [set 0, binding 0]
    ixvkDescSet *set;                   // [pool: ixVulkan::ixStaticPool]

    GlbBuffer(ixvkResCluster *in_c): ixvkBuffer(in_c), layout(null), set(null) {}
  } *glb[2];


  // list with all uniform buffer clusters;
  // a cluster shares one vkMemory;
  // ix has one by default, add to that before calling Ix::initWindow(), if desired
  chainList resClusters;
  void addCluster(ixvkResCluster **out_cluster);
  void delCluster(ixvkResCluster **out_cluster);


  void initAfterWindow();    // ix vulkan init, called after the window is created

  void shutdown();

  // constructor / destructor

  ixVulkan(Ix *in_ix);
  ~ixVulkan();
  
private:

  //ixvkBuffer *_defragBuf;    // used to move images; binds over the image's memory

  VkoQueue *_getQueue(uint32 n, VkQueueFlags in_type, VkQueueFlags in_mustNotHave= 0); // queue number that has those flags, or universal; if not enough queues, fallback is q2, then q1
  void _handleQueues();

  friend class Ix;
}; /// ixVulkan

#include "ix/draw/vkDraw.h"

#endif // IX_USE_VULKAN
