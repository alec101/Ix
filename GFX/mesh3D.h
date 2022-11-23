#pragma once



#define IX_MESH_NAME_LEN 64
#define IXMESH_MAX_CHANNELS 16
#define IXMESH_MAX_TEX_CHANNELS 4

// maybe more types of meshes... this might be the best solution
// a generic mesh would be slow, filled with conditionals to be able to work in any condition


// bone data mk1, 3 bones max:
// vec3i + vec3, 6 components total

// bone data mk2, 4 bones max:
// vec4i + vec4, 8 components total

// bone data mk3, 4 bones max (i16+i16+i16+i16 for ID):
// vec2i + vec4, 6 components total                       <<< I THINK I WILL STICK TO THIS



class ixMaterial;
class ixvkBuffer;
class ixBone;

class ixMesh: public ixClass {
  Ix *_ix;
public:
  static std::mutex mtx;      // if using multiple threads to handle threads, this will be the mutex to the chainList with all the ixMeshes

  // 1. data format must be setup before anything is done with this class (ixMesh::format) - there are multiple ways to set it up
  // 2. Vertex data seems to be ok without any alignas, it CAN BE PACKED; it seems there are no rules for this particular buffer/data type in vulkan

  enum class channelType: uint8 { POS= 0, COL, NRM, TEX1, TEX2, TEX3, TEX4, BONEID, BONEWG, UNDEFINED= 255 };
  
  struct Channel {          // a channel has multiple components, each component has 4 bytes
    uint8 loc, size;        // location of first component and size of the channel (1component= 4bytes, unit is component)
    inline Channel(): loc(0), size(0) {}
  };

  struct DataFormat {
    union {
      struct {
        Channel pos, col, nrm, tex1, tex2, tex3, tex4, boneid, bonewg;    // standard channels - ix will follow this channel order, but it's not mandatory, can use ch[]
      };
      Channel ch[IXMESH_MAX_CHANNELS];          // array with all the possible channels
    };
    uint32 size;                                // [def:0] vertex size (total number of components, 1component= 4bytes)
    ixFlags32 flags;                            // [def:0] 0x0001 - interweaved

    // setup helper funcs- MUST EITHER MANUALLY HANDLING ch[] ARRAY, or add each channel with setChannel
    // if you setup yourself, set each channel size, then call compute_size() and compute_locations() and you won't need to handle anything else
    void setupChannel(channelType in_type, uint8 in_size);      // unit - component (4bytes)
    void setupChannelNr(uint32 in_nr, uint8 in_size);           // unit - component (4bytes)
    void setupAll(const DataFormat *in_format);                 // make your own DataFormat and provide it here

    uint32 compute_size();                                      // computes 'size' variable, the total number of components a vertex has
    void compute_locations();                                   // recomputes all locations; after all sizes are done

    void clear();                                               // sets everything to 0

    uint32 nrInUse() const;                                     // returns the number of channels that are used
    
    inline DataFormat(): size(0) {}
  } format;

  str8 fileName;              // [def:""] filename of the mesh, used for reloads
  str8 fileInputName;         // [def:""] import file name (.dae .blend .3ds .obj) - used for fast updates from source
  char name[IX_MESH_NAME_LEN];// [def:""] name of the object (same as in the input file)
  uint32 fileIndex;           // [def:0] mesh number in the <fileName> to load
  uint32 nrVert;              // number of vertices in mesh
  uint8 *data;                // [def:null] data of the mesh, it can be null
  uint64 size;                // [def:0] data size in bytes;

  // [def:0] mesh affinity, defining how the mesh will work, internally.
  //  0: own buffer in device memory
  //  1: own buffer in host visible memory
  //  2: shared buffer - it will not handle the buffer in any way, it is given a place in one
  // 64: uber buffer system
  uint8 affinity;

  ixMaterial *mat;    // material used

  ixvkBuffer *buf;    // buffer
  uint64 bufOffset;   // buffer offset

  bool alloc_data(uint32 in_nrVert, const DataFormat *in_newFormat= null); // allocs memory based on current format, or provide new format in <in_newFormat>
  void dealloc_data();                                        // dealocates memory for 'data' pointer

  // data manipulation funcs

  uint32 *getVertexData(uint8 *in_data, const DataFormat *in_format, uint64 in_meshTotalVertices, uint64 in_vert, uint32 in_chan);  // used in assimp import, slow, but everything will be slow with imports
  uint64 getChannel(const DataFormat *in_f, uint64 in_totalVert, uint32 in_chan);       // returns channel start location, unit is channel component (4bytes)
  uint64 getChannelStride(const DataFormat *in_f, uint32 in_chan); // returns number of components between each vertex, unit is channel component (4bytes)


  // >>>>>>>>>>>>>>>>> bones have to happen; <<<<<<<<<<<<<<<<<<<<<<
 // THIS IS CRAP, AN ANIMATOR MUST HAPPEN
  // A UNIT WILL HAVE IT'S OWN SKELLY, A MESH. YOU CAN'T PUT SKELLYS HERE OR ANIMS;
  // MESHES ARE SIMPLE DATA, CANNOT DO ANYTHING HERE WITH THOSE

  ixBone *boneRoot;   // [def:null] if it has a skeleton, this is non-null
  int32 nrBones;      // [def:0]    if it has a skeleton, this is the number of bones

  //ixAnimatorBones boneAnimator;

  //void setBoneAnim(ixBone *in_root, int32 in_nrBones) {
  //  boneRoot= in_root;
  //  nrBones= in_nrBones;
  //}





  //0x01: interweaved data MOVED TO FORMAT <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  //0x02: keep host data (*data var)
  ixFlags8 flags;

  // void setDataType(uint32 in_type); // [def:1] internal data format (Data0 struct, Data1i struct, etc)

  // useful inlines, ix.res.mesh has the real funcs
  inline bool load(cchar *, ixFlags32 in_flags= 0);
  inline bool save(cchar *);

  // each mesh format must be pre-defined, will try to fill everything possible inside
  inline bool loadOBJ(cchar *in_f, uint32 *in_OBJmeshNr= null, const char **in_OBJmeshNames= null);
  inline bool upload();
  inline bool download();
  inline bool unload();

  // this will:
  // 1. rearrange all data and format
  // 2. realloc mem for new data
  // 3. try to copy from the old data to new what fits inside, based on the old vs new format
  //    components that don't fit anymore (vec3 to a vec2), will still copy the components that fit
  //    channels that are enlarged (vec3 to vec4) will have last components bytes filled with 0 (hopely this will mean 0.0 in a float)
  // VERY SLOW STUFF, it should be mainly used in ixEditor
  bool changeDataFormat(const DataFormat *in_newType);


  ixMesh(Ix *in_ix);
  ~ixMesh();
  void delData();
  //void delHostData(); maybe
  //void delDeviceData(); maybe

private:
  ixMaterial::Mesh *_matLink; // direct link to the mat::Mesh chainlist, for fast remove when needed

  //bool _load(cchar *in_fileName);

  // flags:
  // 0x0001 - load the input filename that this mesh is based on (.dao .blend .3ds etc)
  // IT MIGHT BE THE fileName CAN BE IGNORED TOO, IF NEEDED, IT CAN BE ANOTHER OPTIMIZATION...
  bool _loadI3D(cchar *in_fname, ixFlags32 in_flags= 0);

  // flags unused atm
  bool _saveI3D(cchar *in_fname, ixFlags32 in_flags= 0);
  bool _loadI3DdataV0_0_1(FILE *in_f, ixFlags32 in_flags);
  
  friend class ixMaterial;
  friend class ixMeshSys;
};















class ixMeshSys {
  Ix *_ix;
public:

  struct Add {
    ixMesh *staticDevice();   // affinity 0
    ixMesh *staticHost();     // affinity 1
    ixMesh *staticCustom();   // affinity 2

    //THERE HAS TO BE A STREM SYS FOR BUFFERS, THAT TRIMS THEM OFTEN; YOU ASK FOR A SPACE, A ixvkBuffer IS GIVEN
    //  YOU RELEASE THE SPACE, IT'S MARKED AS FREE
    // THIS WILL BE MORE COMLPICATED... BUT MAYBE... IF YOU TRIM EVERYTHING ASAP YOU RELEASE THE RESOURCE... THE FREE SPACE WILL BE CONSOLIDATED...
    ixMesh *staticUber();     // affinity 64

    
  private:
    friend class ixMeshSys;
    inline Add(ixMeshSys *in_parent): _ix(in_parent->_ix) {}
    Ix *_ix;
  } add;


  chainList staticMeshes; // [chainData:ixMesh]


  // loading / saving funcs

  bool load(cchar *in_fileName, ixMesh *out_mesh, ixFlags32 in_flags= 0);
  bool save(cchar *in_fileName, ixMesh *in_mesh);

  bool loadI3D(cchar *in_fname, ixMesh *out_mesh, ixFlags32 in_flags= 0);
  bool saveI3D(cchar *in_fname, ixMesh *in_mesh);


  bool loadOBJ(cchar *in_fileName, uint32 nrOutMeshes, ixMesh **out_meshes, uint32 *in_OBJmeshNr= null, const char **in_OBJmeshNames= null);

  // device data handling

  bool upload(ixMesh *in_mesh);     // [host]->[device] copy
  bool download(ixMesh *in_mesh);   // [device]->[host] copy  size/nrVertex, must be populated beforehand, data can be null
  bool unload(ixMesh *in_mesh);     // unload from device - destroy vulkan buffer / mark uber buffer location as free

  // constructor / destructor

  ixMeshSys(Ix *in_ix);
  ~ixMeshSys();
  void delData();

private:


};













