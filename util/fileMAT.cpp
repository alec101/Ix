#include "ix/ix.h"

#include "osi/include/util/fileOp.h"


// private stuff
namespace _ixMAT {
  const char id[]= "ixMT";
  const uint32 version= IX_VER_MAKE(0, 0, 1);
}




bool ixMatSys::saveMAT(cchar *in_fname) {
  cchar *err= null;
  int errL;
  ixFlags8 maps;
  int32 ti32;
  FILE *f= null;
  int64 matPos;
  uint32 nmat;

  if(in_fname== null)
    in_fname= fileMaterialDB;

  if(in_fname== null) IXERR("filename is null")

  f= fopen(in_fname, "wb");
  if(f== null) IXERR("Cannot open file");

  IXFWRITE(_ixMAT::id, 1, Str::strlen8(_ixMAT::id), f);   // file ID
  IXFWRITE(&_ixMAT::version, 4, 1, f);       // file version

  IXFWRITE(&mats.nrNodes, 4, 1, f);
  if(mats.nrNodes== 0) goto Exit;           // exit on zero mats to write

  /// reserve mat locations
  matPos= osi.ftell64(f);
  ti32= 0;
  IXFWRITE(&ti32, 4, mats.nrNodes, f);

  nmat= 0;
  for(ixMaterial *p= (ixMaterial *)mats.first; p; ++nmat, p= (ixMaterial *)p->next) {
    int64 pos= osi.ftell64(f);
    osi.fseek64(f, matPos+ (nmat* 4), SEEK_SET);
    IXFWRITE(&pos, 4, 1, f);
    osi.fseek64(f, pos, SEEK_SET);

    IXFWRITE(&p->diffuse, 16, 1, f);        // vec4
    IXFWRITE(&p->ambient, 12, 1, f);        // vec3
    IXFWRITE(&p->specular, 12, 1, f);       // vec3
    IXFWRITE(&p->emission, 12, 1, f);       // vec3

    /// number of maps, a flag for each if it's up or not
    for(uint a= 0; a< 4; ++a)
      if(p->map[a]) maps.setUp(0x01<< a);
    IXFWRITE(&maps, 1, 1, f);

    for(uint a= 0; a< 4; ++a)
      if(p->map[a]) {
        IXFWRITE(&p->map[a]->texLibID, 4, 1, f);
        IXFWRITE(&p->map[a]->fileName.len, 4, 1, f);
        IXFWRITE(&p->map[a]->fileName.d, 1, p->map[a]->fileName.len, f);
        
        if((p->map[a]->fileName.nrUnicodes== 0) && (p->map[a]->texLibID== 0))   // warning
          error.detail("no fileName and no texLibID for texture", __FUNCTION__, __LINE__);
      } /// there is a map
  } /// for each material
  
Exit:
  if(f) fclose(f);

  if(err) {
    error.detail(str8().f("[%s] %s", in_fname, err), __FUNCTION__, errL);

    return false;
  } else return true; // no error== success
}





bool ixMatSys::loadMAT(cchar *in_fname) {
  bool ret= false;
  cchar *err= null;
  int errL;
  FILE *f= null;
  char idBuffer[16];
  int32 l= Str::strlen8(_ixMAT::id);
  uint32 ui32;

  delData();

  if(in_fname)
    fileMaterialDB= in_fname;


  /// file open
  
  f= fopen(fileMaterialDB, "rb");
  if(f== null) IXERR("Cannot open file");

  /// header + ver load
  IXFREAD(idBuffer, 1, l, f);
  idBuffer[l- 1]= 0;               /// force terminator
  if(Str::strcmp8(_ixMAT::id, idBuffer)) IXERR("no header ID match");
  IXFREAD(&ui32, sizeof(uint32), 1, f);     // I3D file version
  
  // load data
  if     (ui32== IX_VER_MAKE(0, 0, 1)) ret= _loadMATdataV0_0_1(f); // load older save here
  else if(ui32== IX_VER_MAKE(0, 0, 0)) ret= false; // load older save here
  else IXERR("Unknown file version");

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



bool ixMatSys::_loadMATdataV0_0_1(FILE *in_f) {
  cchar *err= null;
  int errL;

  int32 nmats, ti32;
  ixFlags8 maps;
  uint8 textBuffer[256];
  uint8 *textBuffer2= null;
  const char *textPointer;

  IXFREAD(&nmats, 4, 1, in_f);
  if(nmats== 0) goto Exit;                          // exit on zero mats in file

  IXFREAD(&ti32, 4, 1, in_f);
  osi.fseek64(in_f, ti32, SEEK_SET);                // seek first material

  for(int32 a= 0; a< nmats; ++a) {
    ixMaterial *p= new ixMaterial(_ix);
    mats.add(p);

    IXFREAD(&p->diffuse, 16, 1, in_f);              // vec4
    IXFREAD(&p->ambient, 12, 1, in_f);              // vec3
    IXFREAD(&p->specular, 12, 1, in_f);             // vec3
    IXFREAD(&p->emission, 12, 1, in_f);             // vec3

    /// number of maps, a flag for each if it's up or not
    IXFREAD(&maps, 1, 1, in_f);

    for(uint a= 0; a< 4; ++a)
      if(maps.isUp(1<< a)) {
        IXFREAD(&ti32, 4, 1, in_f);                 // texture id, in library

        /// texture library match
        if(ti32> 0) {
          p->map[a]= _ix->res.tex.search(null, ti32);
          if(p->map[a]== null)
            p->map[a]= _ix->res.tex.add.unconfiguredTexture();

          /// skip texture filename
          IXFREAD(&ti32, 4, 1, in_f);               // tex filename len
          if(ti32)
            osi.fseek64(in_f, ti32, SEEK_CUR);

        /// texture filename match
        } else {

          IXFREAD(&ti32, 4, 1, in_f);               // tex filename len

          if(ti32> 0) {
            if(ti32< 256)
              textPointer= (const char *)textBuffer;
            else {
              if(textBuffer2) { delete[] textBuffer2; textBuffer2= null; }
              textBuffer2= new uint8[ti32];
              textPointer= (const char *)textBuffer2;
            }

            IXFREAD(&textPointer, 1, ti32, in_f);   // texture filename

            p->map[a]= _ix->res.tex.search((const char *)textPointer, 0);
            if(p->map[a]== null) {
              p->map[a]= _ix->res.tex.add.unconfiguredTexture();
              p->map[a]->fileName= textPointer;
            }
          }
        } /// texture match/add method
        if(p->map[a]== null) {
          p->map[a]= _ix->res.tex.add.unconfiguredTexture();
          error.detail("both texLibID and filename are null for texture map", __FUNCTION__, __LINE__);
        }
      } /// there is a map
  } /// for each material

Exit:

  if(textBuffer2) delete[] textBuffer2;

  if(err) {
    error.detail(str8().f("[%s] %s", fileMaterialDB, err), __FUNCTION__, errL);

    return false;
  } else return true;
}









