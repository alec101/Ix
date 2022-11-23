#include "ix/ix.h"

#include "osi/include/util/fileOp.h"

std::mutex ixMesh::mtx;     // no constructors loading no mesh, or this must be put in a inline func

/* ALIGNMENT:
 https://vulkan-tutorial.com/Uniform_buffers/Descriptor_pool_and_sets#page_Alignment-requirements
  vec2:    8
  vec3/4: 16
  nested (inside structures): 16
  float/int: 4
  following struct will have garbage of 8 between a and b:
  struct {
    alignas(8)  vec2 a;
    alignas(16) vec4 b;
    float c, d;
  }; 
  therefore, careful with the positioning
  c and d could be placed after a, and you would not have any garbage;
*/


/*
* RESEARCH:
  - index buffers' main thing is the reusability of vertexes... if you can use indexes even with more mem, it's worth it
    a vertex in cache, won't need to be passed thru the shader anymore
  - ofc, if you can use triangle strip, that will use the 2 other vertexes also, so that's also good


  BLENDER exporter:
  - https://docs.blender.org/api/2.93/info_quickstart.html <<< so every action every thing can be accesed in python, bpy.data.objects are all objects, etc
  - scripting view: templates->python->[operator file export] i bet all those templates would help to put up an exporter fast




*/
  
// skinning
// https://webglfundamentals.org/webgl/lessons/webgl-skinning.html



#define IX_VERTEX_DATA_ALIGNMENT 16



/*
MESHES

  vertex buffers have no patterned length
  i am guessing... chunks of memory get allocated

  static vertex buffers should be known to be static, and kinda there, all the time, and destroyed when needed

  but there should be dynamic vertex buffers that come and go too...
  this sounds very familliar with what open gl had, probably... with the hints of what the vertex buffer will be used for...
  i am rebuilding opengl here


  there is a way...
  i make 3 types of clusters, each different size, maybe more for very small meshes

  each big object would be comprised of many clusters
  the challenge would be to properly cut it into clusters
  that would happen with the obj/3ds/max to my mesh file
  it can be a triangle fan/strip/blabla
  so for each type of mesh composition, the mesh object must know to cut it right

  ok all this could be awesome

  the problem i foresee... memory segment 1 is 10% filled

                           memory segment 2 is 10% filled too, and that darn mesh is heavily used
                          
  so...
   there has to be some type of defragmentation...
  right?
  but the defragmentation knows how big to have a 3rd "glass" to be filled for the copy
  well... why a 3rd glass... direct copy... into the empty segment 1


  i am guessing the main issues would be...
  -some garbage , can be reduced with many segment sizes
  -the big question... how slow would it be to draw from a ton of places, everywhere... randomly...
   would having a nice organized stuff be drawned faster?!
  but such organisation can be created by the user, in a way.........


  */




// ##    ##  ########    ####    ##    ##
// ###  ###  ##        ##        ##    ##
// ## ## ##  ######      ####    ########
// ##    ##  ##              ##  ##    ##
// ##    ##  ########   #####    ##    ##

///======================================///

ixMesh::ixMesh(Ix *in_ix): ixClass(ixClassT::MESH), _ix(in_ix) {
  name[0]= 0;

  nrVert= 0;
  fileIndex= 0;
  data= null;
  size= 0;

  affinity= 0;

  mat= null;
  buf= null;
  bufOffset= 0;

  boneRoot= null;
  nrBones= 0;
  
  flags.setUp(0x0002);    // keep host data by default?????
  format.flags.setUp(0x0001);   // interweaved data by default?????

  _matLink= null;

  delData();
}


ixMesh::~ixMesh() {
  _ix->res.mat.cleanup_job= true;
  delData();
}


void ixMesh::delData() {
  nrVert= 0;
  size= 0;

  if(data) { delete[] data; data= null; }
  if(buf) { _ix->res.mesh.unload(this); buf= null; }
}





bool ixMesh::alloc_data(uint32 in_nrVert, const DataFormat *in_newFormat) {
  const char *err= null;
  int errL;

  if(in_nrVert== 0) IXERR("<in_nrVert> is zero");
  if(format.size== 0 && in_newFormat== null) IXERR("current data format is not setup prior to calling this func (format.size==0), new format not provided in <in_newFormat>");
  if(in_newFormat)
    if(in_newFormat->size== 0) IXERR("<in_newFormat->size> specified format's size is 0");

  if(data) dealloc_data();

  if(in_newFormat)
    format= *in_newFormat;
  nrVert= in_nrVert;
  size= (format.size* 4)* nrVert;

  data= new uint8[size];

Exit:
  if(err) {
    error.detail(str8(fileName)+ "["+ name+ "]"+ " "+ err, __FUNCTION__, errL);
    return false;
  } else
    return true;
}

void ixMesh::dealloc_data() {
  if(data) { delete[] data; data= null; }
  size= 0;
}







bool ixMesh::changeDataFormat(const DataFormat *in_newFormat) {
  cchar *err= null; int errL;
  uint8 *oldData= null;
  uint32 *p1, *p2;
  //DataFormat oldFormat;
  uint64 offset1, offset2;

  if(in_newFormat== null)    IXERR("<in_newType> is null");
  if(in_newFormat->size== 0) IXERR("in_newType->size is 0 (populate it first?)");
  if(nrVert== 0)             IXERR("current number of vertices is 0, so current data is bad/null; this func must change something already functional");
  if(data== null)            IXERR("current data is null; this func must change something that is already functional");
  if(format.size== 0)        IXERR("current format size is 0; something is wrong with current format setup; this func must act on something already functional");


  oldData= data;
  
  size= (in_newFormat->size* 4)* nrVert;
  data= new uint8[size];

  for(uint a= 0; a< IXMESH_MAX_CHANNELS; a++) {
    if(in_newFormat->ch[a].size== 0)
      continue;

    p1= (uint32 *)data+ getChannel(in_newFormat, nrVert, a),
    offset1= getChannelStride(in_newFormat, a);

    if(format.ch[a].size> 0)
      p2= (uint32 *)oldData+ getChannel(&format, nrVert, a),
      offset2= getChannelStride(&format, a);
    else
      p2= null, offset2= 0;

    for(uint v= 0; v< nrVert; v++) {                     /// for each vertex
      for(uint d= 0; d< in_newFormat->ch[a].size; d++)   /// for each component
        if(d< format.ch[a].size&& offset2)
          p1[d]= p2[d];                                   // copy from old data
        else
          *((uint32*)p1)= 0;                              // fill in zeroes if old data has no such channel OR channel is smaller

      if(offset2)
        p2+= offset2;
      p1+= offset1;
    } /// for each vertex
  } /// for each channel

  format= *in_newFormat;    /// new format adoption
  delete[] oldData;         /// old data buffer delete

Exit:
  if(err) {


    error.detail(str8(fileName)+ "["+ name+ "]"+ " "+ err, __FUNCTION__, errL);
    return false;
  } else return true;
}


uint64 ixMesh::getChannel(const DataFormat *in_f, uint64 in_totalVert, uint32 in_chan) {
  if(in_f->flags.isUp(0x0001)) return (uint64)in_f->ch[in_chan].loc;
  else                         return (uint64)in_f->ch[in_chan].loc* in_totalVert* (uint64)in_f->size;
}


uint64 ixMesh::getChannelStride(const DataFormat *in_f, uint32 in_chan) {
  if(in_f->flags.isUp(0x0001)) return (uint64)in_f->size;
  else                         return (uint64)in_f->ch[in_chan].size;
}


uint32 *ixMesh::getVertexData(uint8 *in_data, const DataFormat *in_f, uint64 in_totalVert, uint64 in_vert, uint32 in_chan) {
  if(in_f->flags.isUp(0x0001))
    return (uint32 *)in_data+ (uint64)in_f->ch[in_chan].loc+    // channel loc start
           (in_vert* (uint64)in_f->size);                       // vertex number
  else 
    return (uint32 *)in_data+ (uint64)in_f->ch[in_chan].loc* in_totalVert* (uint64)in_f->size+ // channel location start
           (in_vert* (uint64)in_f->ch[in_chan].size);
}









// mesh data format - SETUP funcs

void ixMesh::DataFormat::setupChannel(channelType in_type, uint8 in_size) {
  ch[(int)in_type].size= in_size;
  compute_locations();
  compute_size();
}

void ixMesh::DataFormat::setupChannelNr(uint32 in_nr, uint8 in_size) {
  ch[in_nr].size= in_size;
  compute_locations();
  compute_size();
}

void ixMesh::DataFormat::setupAll(const DataFormat *in_format) {
  for(uint a= 0; a< IXMESH_MAX_CHANNELS; a++)
    ch[a]= in_format->ch[a];
  compute_locations();
  compute_size();
}

uint32 ixMesh::DataFormat::nrInUse() const {
  uint32 ret= 0;
  for(uint a= 0; a< IXMESH_MAX_CHANNELS; a++)
    if(ch[a].size> 0)
      ret++;
  return ret;
}

void ixMesh::DataFormat::compute_locations() {
  uint8 currentLoc= 0;
  for(uint a= 0; a< IXMESH_MAX_CHANNELS; a++) {
    ch[a].loc= currentLoc;
    currentLoc+= ch[a].size;
  }
}

uint32 ixMesh::DataFormat::compute_size() {
  size= 0;
  for(uint a= 0; a< IXMESH_MAX_CHANNELS; a++)
    size+= ch[a].size;
  return size;
}

void ixMesh::DataFormat::clear() {
  for(uint a= 0; a< IXMESH_MAX_CHANNELS; a++)
    ch[a].loc= 0,
    ch[a].size= 0;
  size= 0;
}





















// MESH

//   ####    ##    ##    ####    ########  ########  ##    ##
// ##         ##  ##   ##           ##     ##        ###  ###
//   ####      ####      ####       ##     ######    ## ## ##
//       ##     ##           ##     ##     ##        ##    ##
//  #####       ##      #####       ##     ########  ##    ##

///==========================================================///

ixMeshSys::ixMeshSys(Ix *in_ix): _ix(in_ix), add(this) {
}


ixMeshSys::~ixMeshSys() {
}


void ixMeshSys::delData() {
  staticMeshes.delData();
}




ixMesh *ixMeshSys::Add::staticDevice() {
  ixMesh *m= new ixMesh(_ix);
  m->affinity= 0;

  m->mtx.lock();
  _ix->res.mesh.staticMeshes.add(m);
  m->mtx.unlock();

  return m;
}


ixMesh *ixMeshSys::Add::staticHost() {
  ixMesh *m= new ixMesh(_ix);
  m->affinity= 1;

  m->mtx.lock();
  _ix->res.mesh.staticMeshes.add(m);
  m->mtx.unlock();

  return m;
}

ixMesh *ixMeshSys::Add::staticCustom() {
  ixMesh *m= new ixMesh(_ix);
  m->affinity= 2;

  m->mtx.lock();
  _ix->res.mesh.staticMeshes.add(m);
  m->mtx.unlock();

  return m;
  
}

ixMesh *ixMeshSys::Add::staticUber() {
  // affinity 64
  error.makeme(__FUNCTION__); // MAKEMEEE <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  return null;
}







bool ixMeshSys::load(cchar *in_file, ixMesh *out_m, ixFlags32 in_flags) {
  str8 ext= pointFileExt(in_file);
  ext.lower();

  out_m->fileName= in_file;

  if(ext== "i3d")
    return out_m->_loadI3D(in_file, in_flags);
  else if(ext== "obj")
    return loadOBJ(in_file, 1, &out_m, &out_m->fileIndex);

  return false;
}


bool ixMeshSys::save(cchar *in_file, ixMesh *out_m) {
  str8 ext= pointFileExt(in_file);
  ext.lower();

  out_m->fileName= in_file;

  if(ext== "i3d")
    return out_m->_saveI3D(in_file);
  else
    return false;
}


bool ixMeshSys::upload(ixMesh *in_mesh) {
  const char *err= null;
  int errL;

  if(in_mesh== null) IXERR("<mesh> is null");
  if(in_mesh->data== null) IXERR("<mesh::data> is null");
  if(in_mesh->buf)
    if(*in_mesh->buf->size()< in_mesh->size)
      IXERR("buffer size too small to fit mesh size");


  // create a buffer if it has none
  if(in_mesh->buf== null) {
    /// own buffer in device/host memory
    if(in_mesh->affinity< 2) {
      if     (in_mesh->affinity== 0) in_mesh->buf= new ixvkBuffer(_ix->vki.clusterDevice);
      else if(in_mesh->affinity== 1) in_mesh->buf= new ixvkBuffer(_ix->vki.clusterHost);
      else IXERR("unknown mesh affinity");

      /// usage setup based on data type - but i don't see other usage for a mesh
      in_mesh->buf->handle->cfgUsage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT| VK_BUFFER_USAGE_TRANSFER_DST_BIT| VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

      in_mesh->buf->handle->cfgSize(in_mesh->size);

      in_mesh->buf->build();

    /// specific buffer - setup first
    } else if(in_mesh->affinity== 2) {
      if(in_mesh->buf== null) IXERR("<mesh::buffer> is null");
      if(in_mesh->buf->handle->buffer== VK_NULL_HANDLE) IXERR("<mesh::buffer>'s vulkan handle is not built");



    // uber buffer sys
    } else if(in_mesh->affinity== 64) {
      error.makeme(__FUNCTION__);
      
      
      // MAKEME <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<







    } else IXERR("unknown mesh affinity");
  }

  // the upload
  if(!in_mesh->buf->upload(in_mesh->data, in_mesh->bufOffset, in_mesh->size)) IXERR("ixvkBuffer upload failed. aborting");

  if(!(in_mesh->flags.isUp(0x02))) {    /// keep host data flag
    delete[] in_mesh->data;
    in_mesh->data= null;
  }

Exit:
  if(err) {
    if(in_mesh) error.detail(str8(in_mesh->fileName)+ " "+ err, __FUNCTION__, errL);
    else        error.detail(err, __FUNCTION__, errL);
    return false;
  } else return true;
}





bool ixMeshSys::download(ixMesh *in_m) {
  cchar *err= null;
  int errL;

  if(!in_m) IXERR("<in_mesh> is null");
  if(in_m->buf== null) IXERR("populate <in_mesh::buf> first");
  if(in_m->size== 0) IXERR("<in_mesh>\'s size must be populated first");

  // it wouldn't change in size, would it?
  if(in_m->data== null)
    in_m->data= new uint8[in_m->size];

  if(!in_m->buf->download(in_m->data, in_m->bufOffset, in_m->size)) IXERR("buffer download failed");
    
Exit:
  if(err) {
    error.detail(str8((in_m? in_m->fileName.d: ""))+ err, __FUNCTION__, errL);
    return false;
  } else return true;
}





bool ixMeshSys::unload(ixMesh *in_m) {
  cchar *err= null;
  int errL;
  if(in_m== null)               IXERR("<in_mesh> is null");
  if(in_m->buf== null)          IXERR("<in_mesh>::buf is null");
  if(in_m->buf->segment== null) IXERR("<in_mesh>::buf::segment is null");
  
  /// own buffer, device/host mem
  if(in_m->affinity< 2) {
    in_m->buf->cluster->delResource(in_m->buf);
    in_m->buf= null;

  /// not handling the buffer
  } else if(in_m->affinity== 2) {
    in_m->buf= null;

  /// uber buffer sys
  } else if(in_m->affinity== 64) {
    error.makeme(__FUNCTION__);
    // uber buffer SYS MUST HAPPEN <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

    // works with defrag::moveInside

  }




Exit:
  if(err) {
    error.detail(str8((in_m? in_m->fileName.d: ""))+ err, __FUNCTION__, errL);
    return false;
  } else return true;

}














