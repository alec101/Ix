#include "ix/ix.h"

#include "osi/include/util/fileOp.h"

using namespace ixUtil;

#define loadOBJbufferSize 1024

/// -objects must be exported with faces triangulated
///  i ain't gonna transform from a triangle stripped face, maybe quad, to triangles
///  there's a simple option in blender under export, to triangulate faces, so no big deal.
/// -there's 1 pass that must count how many positions/tex coords/normals are in the file, and 1 pass to read them
///  so the loading is not that fast, but i don't see another faster way. it's how obj's are
/// <in_file>: filename
/// <in_nrOutMeshes>: number or <out_meshes> to populate
/// <in_OBJmeshNr>: [optional] for each <out_meshes>, what respective object(mesh) number to load in the OBJ file
///                 if left null, each mesh will load it's coresponding number in the obj file (first mesh will load first mesh in file, etc)
bool ixMeshSys::loadOBJ(cchar *in_file, uint32 in_nrOutMeshes, ixMesh **out_meshes, uint32 *in_OBJmeshNr, const char **in_OBJmeshNames) {
  cchar *err= null;
  int errL;
  
  uint32 *p1, *p2;
  uint32 offset1, offset2;
  uint32 p2size;

  int64 startOfLine;
  bool start;
  
  uint8 line[loadOBJbufferSize]; str8 s;     s.wrap((char *)line,    loadOBJbufferSize);
  uint8 command[80];             str8 cmd; cmd.wrap((char *)command, 80);    // a single word, 80 should be enough
  uint8 word1[40];               str8 w1;   w1.wrap((char *)word1,   40);
  uint8 word2[40];               str8 w2;   w2.wrap((char *)word2,   40);
  uint8 word3[40];               str8 w3;   w3.wrap((char *)word3,   40);
  uint8 word4[40];               str8 w4;   w4.wrap((char *)word4,   40);

  struct OBJdata: public chainData {
    vec4 *pos;
    vec3 *nrm, *tex1;
    vec3i *ind;           // triangle index data

    uint32 nrPos, nrNrm, nrTex1, nrInd, startPos, startNrm, startTex1;
    str8 name;
    str8 materialName;
    OBJdata(chainList *list): pos(null), nrm(null), tex1(null), ind(null) {
      nrPos= nrNrm= nrTex1= nrInd= 0;
      OBJdata *l= (OBJdata *)list->last;
      if(l) { startPos= l->startPos+ l->nrPos; startNrm= l->startNrm+ l->nrNrm; startTex1= l->startTex1+ l->nrTex1; } 
      else    startPos= startNrm= startTex1= 1;
      list->add(this);
    }
    ~OBJdata() { if(pos)  delete[] pos;
                 if(nrm)  delete[] nrm;
                 if(tex1) delete[] tex1;
                 if(ind)  delete[] ind;
    }
    //void start(chainList *list) { OBJdata *l= (OBJdata *)list->last;
    //                              if(l) { startPos= l->startPos+ l->nrPos; startNrm= l->startNrm+ l->nrNrm; startTex1= l->startTex1+ l->nrTex1; } 
    //                              list->add(this); }
  };

  str8 matLibFile;
  chainList objMeshes;
  OBJdata *d= null;         // current mesh in the chainlist

  // file read start
  FILE *f= fopen(in_file, "rb");
  if(f== null) IXERR("file open failed");
  
  while(1) {
    startOfLine= osi.ftell64(f);
    if(!readLine8(f, &s)) break;

    // an object starts
    if((s.d[0]== 'o') && (s.d[1]== ' ')) {
      parseGenericTxtCommand(&s, &cmd, &w1);

      /// check to add or not to add another object
      start= false;
      if(d== null)  start= true;
      if(d) if(d->name.d) start= true;    // already a name in this obj? then it's another obj
      //if(d) if((!d->pos) && (!d->nrm) && (!d->tex1) && (!d->ind)) start= true;
      if(start) d= new OBJdata(&objMeshes);

      d->name= w1;

    // vertex positions
    } else if((s.d[0]== 'v') && (s.d[1]== ' ')) {
      // there's nothing in the book that states where a new mesh starts
      // blender starts with "o objName", but it could directly start with "v coord coord coord"

      /// check to add or not to add another object
      start= false;
      if(d== null)     start= true;
      if(d) if(d->pos) start= true;     // there is data already loaded current node, then a new node must happen
      if(start) d= new OBJdata(&objMeshes);

      /// count the number of vertex positions in the file
      d->nrPos= 1;
      while(1) {
        if(!readLine8(f, &s)) break;
        if((s.d[0]== 's') && (s.d[1]== ' ')) continue;      // ignore smoothing groups
        if((s.d[0]== 'v') && (s.d[1]== ' ')) d->nrPos++;
        else break;
      }
      osi.fseek64(f, startOfLine, SEEK_SET);
      
      /// alloc mem for the vertex positions
      d->pos= new vec4[d->nrPos];

      /// read all vertex positions
      for(uint32 a= 0; a< d->nrPos;) {
        readLine8(f, &s);
        if((s.d[0]== 's') && (s.d[1]== ' ')) continue;      // ignore smoothing groups

        parseGenericTxtCommand(&s, &cmd, &w1, &w2, &w3, &w4);
        d->pos[a].x= w1.toFloat();
        d->pos[a].y= w2.toFloat();
        d->pos[a].z= w3.toFloat();
        if(w4.nrUnicodes) d->pos[a].w= w4.toFloat();
        else              d->pos[a].w= 1.0f;           // default value
        a++;
      }

    // vertex tex coords;
    } else if(s.d[0]== 'v' && s.d[1]== 't') {
      /// check to add or not to add another object
      start= false;
      if(d== null)      start= true;
      if(d) if(d->tex1) start= true;     // there is data already loaded current node, then a new node must happen
      if(start) d= new OBJdata(&objMeshes);
     
      /// count the number of vertex tex coords in the file
      d->nrTex1= 1;
      while(1) {
        if(!readLine8(f, &s)) break;
        if((s.d[0]== 's') && (s.d[1]== ' ')) continue;      // ignore smoothing groups
        if(s.d[0]== 'v' && s.d[1]== 't') d->nrTex1++;
        else break;
      }
      osi.fseek64(f, startOfLine, SEEK_SET);
      
      /// alloc mem for the vertex positions
      d->tex1= new vec3[d->nrTex1];

      /// read all vertex positions
      for(uint32 a= 0; a< d->nrTex1;) {
        readLine8(f, &s);
        if((s.d[0]== 's') && (s.d[1]== ' ')) continue;      // ignore smoothing groups
        parseGenericTxtCommand(&s, &cmd, &w1, &w2, &w3);
        d->tex1[a].x= w1.toFloat();
        d->tex1[a].y= w2.toFloat();
        if(w3.nrUnicodes) d->tex1[a].z= w3.toFloat();
        else              d->tex1[a].z= 0.0f;           // default value
        a++;
      }

    // vertex normals
    } else if(s.d[0]== 'v' && s.d[1]== 'n') {
      /// check to add or not to add another object
      start= false;
      if(d== null)     start= true;
      if(d) if(d->nrm) start= true;     // there is data already loaded current node, then a new node must happen
      if(start) d= new OBJdata(&objMeshes);

      /// count the number of vertex normals in the file
      d->nrNrm= 1;
      while(1) {
        if(!readLine8(f, &s)) break;
        if((s.d[0]== 's') && (s.d[1]== ' ')) continue;      // ignore smoothing groups
        if(s.d[0]== 'v' && s.d[1]== 'n') d->nrNrm++;
        else break;
      }
      osi.fseek64(f, startOfLine, SEEK_SET);
      
      /// alloc mem for the vertex positions
      d->nrm= new vec3[d->nrNrm];

      /// read all vertex positions
      for(uint32 a= 0; a< d->nrNrm;) {
        readLine8(f, &s);
        if((s.d[0]== 's') && (s.d[1]== ' ')) continue;      // ignore smoothing groups
        parseGenericTxtCommand(&s, &cmd, &w1, &w2, &w3);
        d->nrm[a].x= w1.toFloat();
        d->nrm[a].y= w2.toFloat();
        d->nrm[a].z= w3.toFloat();
        a++;
      }

    } else if(s.d[0]== 'f' && s.d[1]== ' ') {
      // f v/vt/vn v/vt/vn v/vt/vn  - MUST be triangles, not gonna mess with unpacking quads or other polygons
      if(d== null) IXERR("no object previously created. aborting.");
      if(d->ind) IXERR("object already have face/triangle data. aborting");

      /// count the number of triangle indexes in the file
      d->nrInd= 1;
      while(1) {
        if(!readLine8(f, &s)) break;
        if((s.d[0]== 's') && (s.d[1]== ' ')) continue;      // ignore smoothing groups
        if(s.d[0]== 'f' && s.d[1]== ' ') d->nrInd++;
        else break;
      }
      osi.fseek64(f, startOfLine, SEEK_SET);
      d->nrInd*= 3;   // assuming a triangle. ALWAYS save with triangulate faces option. if it's not it will fail!!!

      /// alloc mem for the vertex positions
      d->ind= new vec3i[d->nrInd];
      
      /// read all vertex positions
      for(uint32 a= 0; a< d->nrInd;) {
        readLine8(f, &s);
        if((s.d[0]== 's') && (s.d[1]== ' ')) continue;      // ignore smoothing groups
        if(s.s("f %d/%d/%d %d/%d/%d %d/%d/%d\n", &d->ind[a].x,    &d->ind[a].y,    &d->ind[a].z,
                                                 &d->ind[a+ 1].x, &d->ind[a+ 1].y, &d->ind[a+ 1].z,
                                                 &d->ind[a+ 2].x, &d->ind[a+ 2].y, &d->ind[a+ 2].z)!= 9) {
          IXERR("parsing text line failed (faces indexes)");
        }
        a+= 3;
      }

    // longer commands
    } else {
      parseGenericTxtCommand(&s, &cmd, &w1, &w2, &w3);
      if(cmd== "usemtl") {
        d->materialName= w1;

      } else if(cmd== "mtllib") {
        matLibFile= w1;
      }
    } /// parse text line
  } /// infinite reading loop

  fclose(f);

  
  
  for(uint32 a= 0; a< in_nrOutMeshes; ++a) {
    ixMesh *m= out_meshes[a];
    

    OBJdata *o;
    if(in_OBJmeshNr) {
      if(in_OBJmeshNr[a]>= objMeshes.nrNodes) {
        error.detail(str8().f("[%s] WARNING: requested mesh #%d in obj file not found, skipping.", in_file, in_OBJmeshNr[a]), __FUNCTION__);
        continue;
      }
      o= (OBJdata *)objMeshes.get(in_OBJmeshNr[a]);

    } else if(in_OBJmeshNames) {
      for(o= (OBJdata *)objMeshes.first; o; o= (OBJdata *)o->next)
        if(o->name== in_OBJmeshNames[a])
          break;

      if(o== null) {
        error.detail(str8().f("[%s] WARNING: requested mesh [%s] in obj file not found, skipping", in_file, (in_OBJmeshNames[a]? in_OBJmeshNames[a]: "null")), __FUNCTION__);
        continue;
      }

    } else {
      if(a>= objMeshes.nrNodes) {
        error.detail(str8().f("[%s] WARNING: requested mesh #%d in obj file not found, skipping.", in_file, a), __FUNCTION__);
        continue;
      }
      o= (OBJdata *)objMeshes.get(a);
    }

    // populate ixMesh data
    

    m->fileName= in_file;
    m->fileIndex= (in_OBJmeshNr? in_OBJmeshNr[a]: a);
    m->nrVert= o->nrInd;

    if(m->data) { delete[] m->data; m->data= null; m->size= null; }
    m->alloc_data(m->nrVert);



    for(uint c= 0; c< IXMESH_MAX_CHANNELS; c++) {
      if(m->format.ch[c].size== 0)
        continue;

      p1= (uint32 *)m->data+ m->getChannel(&m->format, m->nrVert, c),
      offset1= (uint32)m->getChannelStride(&m->format, c);

      p2= null, offset2= 0, p2size= 0;

      if(c== (uint)ixMesh::channelType::POS && o->pos) {
        p2= (uint32 *)(&o->pos[o->ind[c].x- o->startPos]);
        p2size= 4;              // vec4
        offset2= p2size* 4;

      } else if(c== (uint)ixMesh::channelType::TEX1 && o->tex1) {
        p2= (uint32 *)(&o->tex1[o->ind[c].y- o->startTex1]);
        p2size= 3;              // vec3
        offset2= p2size* 4;

      } else if(c== (uint)ixMesh::channelType::NRM && o->nrm) {
        p2= (uint32 *)(&o->nrm[o->ind[c].z- o->startNrm]);
        p2size= 3;              // vec3
        offset2= p2size* 4;
      }

      // copy operation
      for(uint v= 0; v< o->nrInd; v++) {                /// for each vertex
        for(uint d= 0; d< m->format.ch[c].size; d++)    /// for each component
          if(d< p2size)
            p1[d]= p2[d];
          else
            p1[d]= 0;
      
        if(offset2) p2+= offset2;
        p1+= offset1;
      } /// for each vertex

    } /// for each channel in the mesh
  } /// for each output mesh
  
  // success

Exit:
  if(err) {
    if(in_nrOutMeshes && out_meshes)
      for(uint32 a= 0; a< in_nrOutMeshes; ++a)
        out_meshes[a]->delData();

    error.detail(str8("[")+ in_file+ "] "+ err, __FUNCTION__, errL);

    return false;
  } else {


    return true;
  }
}

#undef loadOBJbufferSize

