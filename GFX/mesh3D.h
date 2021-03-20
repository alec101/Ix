#pragma once




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
    inline void linkToMesh(const ixMesh *in_m) { pos= (vec3 *)in_m->data; nrm= pos+ in_m->nrVert; tex1= (vec2 *)(nrm+ in_m->nrVert); }
    Data0(const ixMesh *in_m) { linkToMesh(in_m); }
  };
  
  struct Data1i { /// dataType 1 - interweaved
    struct Vert {
      vec3 pos;
      vec3 nrm;
      vec2 tex1;
    } *vert;
    inline static uint64 size() { return (sizeof(vec3)+ sizeof(vec3)+ sizeof(vec2)); }
    Data1i(const ixMesh *in_m): vert((Vert *)in_m->data) {}
  };

  // vertex data
  
  str8 fileName;      // [def:null] filename of the mesh, used for reloads
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

  /*
  inline bool upload()   { _ix->res.mesh.upload(this); }
  inline bool download() { _ix->res.mesh.download(this); }
  inline bool unload()   { _ix->res.mesh.unload(this); }
  */

  ixMesh(Ix *in_ix);
  ~ixMesh();
  void delData();
  //void delHostData(); maybe
  //void delDeviceData(); maybe

private:
  ixMaterial::Mesh *_matLink; // direct link to the mat::Mesh chainlist, for fast remove when needed

  //bool _load(cchar *in_fileName);

  bool _loadI3D(cchar *in_fname);
  bool _saveI3D(cchar *in_fname);
  bool _loadI3DdataV0_0_1(FILE *in_f);
  
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

  bool load(cchar *in_fileName, ixMesh *out_mesh);

  bool loadI3D(cchar *in_fname, ixMesh *out_mesh);
  bool saveI3D(cchar *in_fname, ixMesh *in_mesh);


  bool loadOBJ(cchar *in_fileName, uint32 nrOutMeshes, ixMesh *out_meshes, uint32 *in_OBJmeshNr= null);

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













