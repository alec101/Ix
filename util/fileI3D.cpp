#include "ix/ix.h"

#include "osi/include/util/fileOp.h"


// private stuff
namespace _ixI3D {
  const char id[]= "ix3D";
  const uint32 version= IX_VER_MAKE(0, 0, 1);
}










bool ixMesh::_saveI3D(cchar *in_fname) {
  cchar *err= null;
  int errL;
  FILE *f= null;

  if(in_fname== null) IXERR("filename is null")

  f= fopen(in_fname, "wb");
  if(f== null) IXERR("Cannot open file");

  IXFWRITE(_ixI3D::id, 1, Str::strlen8(_ixI3D::id), f);   // file ID
  IXFWRITE(&_ixI3D::version, 4, 1, f);       // file version


  IXFWRITE(&nrVert,   4, 1, f);
  IXFWRITE(&dataType, 4, 1, f);
  IXFWRITE(&flags,    4, 1, f);

  // data 0
  if(dataType== 0) {
    Data0 p(this);

    IXFWRITE(p.pos,  12, nrVert, f);
    IXFWRITE(p.nrm,  12, nrVert, f);
    IXFWRITE(p.tex1, 8,  nrVert, f);

  // data 0 interweaved
  }else if(dataType== 1) {
     Data1i p(this);
     
     IXFWRITE(p.vert, p.size(), nrVert, f);

  } else IXERR("Unknown data type");





  // MATERIAL
  mat;  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





  

Exit:
  if(f) fclose(f);

  if(err) {
    error.detail(str8().f("[%s] %s", in_fname, err), __FUNCTION__, errL);

    return false;
  } else return true; // no error== success
}





bool ixMesh::_loadI3D(cchar *in_fname) {
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
  if     (ui32== IX_VER_MAKE(0, 0, 1)) ret= _loadI3DdataV0_0_1(f); // load older save here
  else if(ui32== IX_VER_MAKE(0, 0, 0)) ret= false; // load older save here
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



bool ixMesh::_loadI3DdataV0_0_1(FILE *in_f) {
  cchar *err= null;
  int errL;

  IXFREAD(&nrVert,   4, 1, in_f);
  IXFREAD(&dataType, 4, 1, in_f);
  IXFREAD(&flags,    4, 1, in_f);

  if(nrVert== 0) IXERR("mesh::nrVert> from file is 0");

  if(dataType== 0) {
    data= new uint8[Data0::size()* nrVert];
    Data0 p(this);

    IXFREAD(p.pos, 12, nrVert, in_f);
    IXFREAD(p.nrm, 12, nrVert, in_f);
    IXFREAD(p.tex1, 8, nrVert, in_f);

  } else if(dataType== 1) {
    data= new uint8[Data1i::size()* nrVert];
    Data1i p(this);

    IXFREAD(p.vert, p.size(), nrVert, in_f);

  } else IXERR("Unknown data type");



Exit:
  if(err) {
    error.detail(str8().f("[%s] %s", fileName.d, err), __FUNCTION__, errL);

    return false;
  } else return true;
}









