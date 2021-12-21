#pragma once



#define IX_MESH_NAME_LEN 64

// maybe more types of meshes... this might be the best solution
// a generic mesh would be slow, filled with conditionals to be able to work in any condition



class ixMaterial;
class ixvkBuffer;

class ixMesh: public ixClass {
  Ix *_ix;
public:
  
  static std::mutex mtx;      // if using multiple threads to handle threads, this will be the mutex to the chainList with all the ixMeshes

  // data types

  struct Data0 {  /// dataType 0
    vec3 *pos;        // array[nr vertices]
    vec3 *nrm;        // array[nr vertices]
    vec2 *tex1;       // array[nr vertices]
    inline static uint64 size() { return (sizeof(vec3)+ sizeof(vec3)+ sizeof(vec2)); }
    inline void linkToMesh(const ixMesh *in_m)            { pos= (vec3 *)in_m->data; nrm= pos+ in_m->nrVert; tex1= (vec2 *)(nrm+ in_m->nrVert); }
    inline void linkToData(cvoid *in_d, uint32 in_nrVert) { pos= (vec3 *)in_d;       nrm= pos+ in_nrVert;    tex1= (vec2 *)(nrm+ in_nrVert); }
    Data0(const ixMesh *in_m)            { linkToMesh(in_m); }
    Data0(cvoid *in_d, uint32 in_nrVert) { linkToData(in_d, in_nrVert); }
  };
  
  struct Data1i { /// dataType 1 - interweaved
    struct Vert {
      vec3 pos;
      vec3 nrm;
      vec2 tex1;
    } *vert;
    inline static uint64 size() { return (sizeof(vec3)+ sizeof(vec3)+ sizeof(vec2)); }
    Data1i(const ixMesh *in_m): vert((Vert *)in_m->data) {}
    Data1i(cvoid *in_d):        vert((Vert *)in_d) {}
  };

  // vertex data
  
  str8 fileName;      // [def:""] filename of the mesh, used for reloads
  str8 fileInputName; // [def:""] import file name (.dae .blend .3ds .obj) - used for fast updates from source
  char name[IX_MESH_NAME_LEN]; // [def:""] name of the object (same as in the input file)
  uint32 fileIndex;   // [def:0] mesh number in the <fileName> to load
  uint32 nrVert;      // number of vertices in mesh
  uint32 dataType;    // [def:1] internal data format (Data1 struct, Data1i struct, etc)
  uint8 *data;        // [def:null] data of the mesh, it can be null
  uint64 size;        // [def:0] data size in bytes;

  // [def:0] mesh affinity, defining how the mesh will work, internally.
  //  0: own buffer in device memory
  //  1: own buffer in host visible memory
  //  2: shared buffer - it will not handle the buffer in any way, it is given a place in one
  // 64: uber buffer system
  uint8 affinity;

  ixMaterial *mat;    // material used

  ixvkBuffer *buf;    // buffer
  uint64 bufOffset;   // buffer offset

  //0x01: interweaved data
  //0x02: keep host data (*data var)
  ixFlags8 flags;

  void setDataType(uint32 in_type); // [def:1] internal data format (Data0 struct, Data1i struct, etc)

  // useful inlines, ix.res.mesh has the real funcs
  inline bool load(cchar *, ixFlags32 in_flags= 0);
  inline bool save(cchar *);
  inline bool loadOBJ(cchar *in_f, uint32 *in_OBJmeshNr= null, const char **in_OBJmeshNames= null);
  inline bool upload();
  inline bool download();
  inline bool unload();
  
  bool changeDataType(uint32 in_newType);


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













