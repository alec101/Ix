#include "ix/ix.h"

#include "osi/include/util/fileOp.h"


// private stuff
namespace _ixI3D {
  const char id[]= "ix3D";
  const uint32 version= IX_VER_MAKE(0, 0, 2);
}

// 0.0.1 initial working, total scrapped, cannot load from such files




bool ixMesh::_saveI3D(cchar *in_fname, ixFlags32 in_flags) {
  cchar *err= null;
  int errL;
  FILE *f= null;
  int32 l;

  if(in_fname== null) IXERR("filename is null")

  f= fopen(in_fname, "wb");
  if(f== null) IXERR("Cannot open file");

  IXFWRITE(_ixI3D::id, 1, Str::strlen8(_ixI3D::id), f);   // file ID
  IXFWRITE(&_ixI3D::version, 4, 1, f);                    // file version

  /// object name, same as in the .blend file
  l= Str::strlen8(name);
  IXFWRITE(&l, 4, 1, f);
  if(l)
    IXFWRITE(name, 1, l, f);

  /// input file (directly .blend, if assimp will ever work, or .dae .obj .3ds etc)
  IXFWRITE(&fileInputName.len, 4, 1, f);
  if(fileInputName.len)
    IXFWRITE(fileInputName.d, 1, fileInputName.len, f);

  IXFWRITE(&nrVert, 4, 1,    f);
  IXFWRITE(&format, sizeof(DataFormat), 1, f);    // IXMESH_MAX_CHANNELS is 16 for v0.0.2
  IXFWRITE(&flags,  4, 1,    f);
  
  IXFWRITE(&size,   8, 1,    f);
  IXFWRITE(data,    1, size, f);


Exit:
  if(f) fclose(f);

  if(err) {
    error.detail(str8().f("[%s] %s", in_fname, err), __FUNCTION__, errL);

    return false;
  } else return true; // no error== success
}





bool ixMesh::_loadI3D(cchar *in_fname, ixFlags32 in_flags) {
  bool ret= false;
  cchar *err= null;
  int errL;
  FILE *f= null;
  char idBuffer[16];
  int32 l= Str::strlen8(_ixI3D::id);
  uint32 ui32;

  delData();

  /// file open
  if(in_fname== null) IXERR("filename is null");
  f= fopen(in_fname, "rb");
  if(f== null) IXERR("Cannot open file");
  fileName= in_fname;

  /// header + ver load
  IXFREAD(idBuffer, 1, l, f);
  idBuffer[l- 1]= 0;               /// force terminator
  if(Str::strcmp8(_ixI3D::id, idBuffer)) IXERR("no header ID match");
  IXFREAD(&ui32, sizeof(uint32), 1, f);     // I3D file version
  
  // load data
  if     (ui32== IX_VER_MAKE(0, 0, 1)) ret= _loadI3DdataV0_0_1(f, in_flags); // loads current ver
  // load older save from here
  else if(ui32== IX_VER_MAKE(0, 0, 0)) ret= false;
  else IXERR("Unknown file version");



  
  // MATERIAL
  mat; // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



Exit:
  if(f) fclose(f);
  /// fail
  if(ret== false) {
    if(err) error.detail(str8().f("[%s] %s", in_fname, err), __FUNCTION__, errL);
    delData();
    return false;
  // success
  } else return true;
}



bool ixMesh::_loadI3DdataV0_0_1(FILE *in_f, ixFlags32 in_flags) {
  cchar *err= null;
  int errL;
  int32 l= 0;

  /// object name, same as in the .blend file
  name[0]= 0;
  IXFREAD(&l, 4, 1, in_f);
  if(l> IX_MESH_NAME_LEN) IXERR(_IXFILEREADERROR());
  if(l)
    IXFREAD(name, 1, l, in_f);

  /// input file (directly .blend, if assimp will ever work, or .dae .obj .3ds etc)
  IXFREAD(&l, 4, 1, in_f);
  if(in_flags.isUp(0x0001)) {
    fileInputName.delData();
    if(l) {
      fileInputName.d= (char *)new uint8[l];
      IXFREAD(fileInputName.d, 1, l, in_f);
      fileInputName.updateLen();
    }
  } else {
    osi.fseek64(in_f, l, SEEK_CUR);
  }

  IXFREAD(&nrVert, 4,                  1,    in_f);
  IXFREAD(&format, sizeof(DataFormat), 1,    in_f);
  IXFREAD(&flags,  4,                  1,    in_f);
  
  if(nrVert== 0) IXERR("mesh::nrVert> from file is 0");

  IXFREAD(&size,   8,                  1,    in_f);
  if(size== 0) IXERR("mesh::size from file is 0");

  data= new uint8[size];
  IXFREAD(data,    1,                  size, in_f);
  

Exit:
  if(err) {
    error.detail(str8().f("[%s] %s", fileName.d, err), __FUNCTION__, errL);

    return false;
  } else return true;
}









