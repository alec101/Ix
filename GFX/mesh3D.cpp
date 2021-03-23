#include "ix/ix.h"

#include "osi/include/util/fileOp.h"

std::mutex ixMesh::mtx;     // no constructors loading no mesh, or this must be put in a inline func

/*
* RESEARCH:
  - index buffers' main thing is the reusability of vertexes... if you can use indexes even with more mem, it's worth it
    a vertex in cache, won't need to be passed thru the shader anymore
  - ofc, if you can use triangle strip, that will use the 2 other vertexes also, so that's also good



*/
  
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
  nrVert= 0;
  fileIndex= 0;
  dataType= 1;
  data= null;
  size= 0;

  affinity= 0;

  mat= null;
  buf= null;
  bufOffset= 0;

  flags.setUp(0x01);    // interweaved data
  flags.setUp(0x02);    // keep host data

  _matLink= null;

  delData();
}


ixMesh::~ixMesh() {
  _ix->res.mat.cleanup_job= true;
  delData();
}


void ixMesh::delData() {
  if(buf) _ix->res.mesh.unload(this);
  nrVert= 0;
  if(data) { delete[] data; data= null; }
  size= 0;
}





void ixMesh::setDataType(uint32 in_t) {
  dataType= in_t;
  if(in_t== 0)
    flags.setDown(0x01);
  else if(in_t== 1)
    flags.setUp(0x01);
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







bool ixMeshSys::load(cchar *in_file, ixMesh *out_m) {
  str8 ext= pointFileExt(in_file);
  ext.lower();

  out_m->fileName= in_file;

  if(ext== "i3d")
    return out_m->_loadI3D(in_file);  
  else if(ext== "obj")
    return loadOBJ(in_file, 1, out_m, &out_m->fileIndex);

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

      /// usage setup based on data type
      if(in_mesh->dataType< 2) in_mesh->buf->handle->cfgUsage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT| VK_BUFFER_USAGE_TRANSFER_DST_BIT| VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
      else 
        IXERR("unknown data type - makeme"); // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<< ADD DATA TYPES HERE

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














