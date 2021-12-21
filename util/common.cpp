#include "ix/ix.h"
//#include "osi/include/util/typeShortcuts.h"
//#include "osi/include/util/str8.h"
//#include "common.hpp"

using namespace ixUtil;

bool ixUtil::isWhitespace(uint32 c) {
  if     (c== ' ')  return true; // (0x20) (SPC) space 
  else if(c== '\t') return true; // (0x09) (TAB) horizontal tab 
  else if(c== '\n') return true; // (0x0a) (LF)  newline 
  else if(c== '\v') return true; // (0x0b) (VT)  vertical tab 
  else if(c== '\f') return true; // (0x0c) (FF)  feed 
  else if(c== '\r') return true; // (0x0d) (CR)  carriage return 
  else return false;
}


char *ixUtil::skipWhitespace(char *in_string) {
  uint8 *p= (uint8 *)in_string;
  while(p)
    if     (*p== ' ')  p++; // (0x20) (SPC) space 
    else if(*p== '\t') p++; // (0x09) (TAB) horizontal tab 
    else if(*p== '\n') p++; // (0x0a) (LF)  newline 
    else if(*p== '\v') p++; // (0x0b) (VT)  vertical tab 
    else if(*p== '\f') p++; // (0x0c) (FF)  feed 
    else if(*p== '\r') p++; // (0x0d) (CR)  carriage return 
    else return (char *)p;
  return null;
}


char *ixUtil::readWordOrWordsInQuotes(char *in_string, str8 *out_string) {
  in_string= skipWhitespace(in_string);   /// skip whitespaces
  uint8 *p= (uint8 *)in_string;           /// points to start
  uint8 until= 0;                         /// read the text until a whitespace or string end
  if(*p== '\"') { until= '\"'; p++; }     /// read the text until another " is found
  if(*p== '\'') { until= '\''; p++; }     /// read the text until another ' is found

  uint8 *s= p;                            /// will walk src str

  // wrapping output str - NO MEM ALLOCS
  if(out_string->wrapping) {
    int a= 0;
    for(; a< out_string->wrapSize- 1; ++a, ++s) {
      if((!*s) || (*s== until))
        break;

      if(until== 0)
        if(isWhitespace(*s) || *s== ',')
          break;

      out_string->d[a]= *s;
    }
    out_string->d[a]= 0;
    out_string->updateLen();

  // non-wrapping output str - MEM ALLOCS
  } else {
    uint8 backup= 0;

    for(; *s; ++s) {
      if(until== 0) {                       /// there are no quotes
        if(isWhitespace(*s) || *s== ',') {  /// if a whitespace or comma is found, an end of a word is found
          backup= *s; *s= 0; break;
        } 
      } else if(*s== until) {               /// read until another quote is found
        backup= *s; *s= 0; break;
      }
    }

    *out_string= (char *)p;   // <- alloc happens
    *s= backup;
  }
  
  if(*s== '\"' || *s== '\'') s++;
  return (char *)s;
}



/*
/// reads first word in the <in_string>
/// works on both wrapped/normal str8 class
char *readFirstWord(char *in_string, str8 *out_string) {
  in_string= skipWhitespace(in_string);
  uint8 *s= (uint8 *)in_string;

  // wrapping output - NO MEM ALLOCS
  if(out_string->wrapping) {
    uint8 *d= (uint8 *)out_string->d;

    for(int a= 0; a< out_string->wrapSize- 1; ++a, ++s, ++d) {
      if(isWhitespace(*s) || *s== ',' || *s== '.')     /// if a whitespace or comma is found, an end of a word is found
        break;
      *d= *s;
    }
    
    *d= 0;
    out_string->updateLen();

  // non-wrapping output - MEM ALLOCS
  } else {
    out_string->delData();

    for(; *s; ++s)
      if(isWhitespace(*s) || *s== ',' || *s== '.') {
        uint8 backup= *s;
        *s= 0;
        *out_string= in_string; // <- alloc
        *s= backup;
        break;
      }
  }
  return (char *)s;
}
*/

char *ixUtil::readWordsInBrackets(char *in_string, str8 *out_string) {
  in_string= skipWhitespace(in_string);
  uint8 *s= (uint8 *)in_string;
  if(*s != '[') return in_string;
  ++s;
  // wrapping variant
  if(out_string->wrapping) {
    uint8 *d= (uint8 *)out_string->d;
    for(int a= 0; a< out_string->wrapSize- 1; ++s, ++d, ++a) {
      if(*s== ']' || *s== 0) { *d= 0; out_string->updateLen(); return (char *)++s; }
      *d= *s;
    }

  // non-wrapping variant - MEM ALLOCS
  } else {
    uint8 *p= s;
    out_string->delData();

    while(*s) {
      if(*s== ']') { *s= 0; *out_string= (char *)p; *s= ']'; return (char *)++s; }
      s++;
    }
  }

  return in_string;
}


/// parses a text line command - one command per line
/// <in_line>     - the text line to be parsed [REQUIRED]
/// <out_command> - the command will be placed here [REQUIRED]
/// <out_v1-v4>   - multiple parameters for the command, each parameter is OPTIONAL
/// accepts brakets [], quotes ', double quotes ". BRACKETS FOR MAIN COMMAND ONLY
/// the command can have = or : after it, or simply a space or a comma
void ixUtil::parseGenericTxtCommand(str8 *in_line, str8 *out_command, str8 *out_v1, str8 *out_v2, str8 *out_v3, str8 *out_v4) {
  
  char *p= (char *)in_line->d;
  if(in_line== null || out_command== null) goto ClearText;
  if(!p) goto ClearText;
  
  p= skipWhitespace(p);

  // check to see if it's a comment line
  if(in_line->nrUnicodes> 0) {
    /// # is universal for comment line
    if(in_line->d[0]== '#')
      goto ClearText;

    ///  '//' the C++ line comment
    if(in_line->nrUnicodes> 1) {
      if(in_line->d[0]== '/' && in_line->d[1]== '/')
        goto ClearText;
    }
  }

  // the command
  
  if(*p== '[') p= ixUtil::readWordsInBrackets(p, out_command);
  else         p= ixUtil::readWordOrWordsInQuotes(p, out_command);

  if(out_command->d== null) goto ClearText;
  if(*out_command== "") goto ClearText;

  //out_command->lower();
  if(out_command->len>= 2) {
    if(out_command->d[out_command->len- 2]== '=') *out_command-= 1;
    else if(out_command->d[out_command->len- 2]== ':') *out_command-= 1;
  }

  p= skipWhitespace(p);
  if(*p== '=' || *p== ':' || *p== ',') p++;
  p= skipWhitespace(p);
  
  // the parameters
  if(out_v1) { p= readWordOrWordsInQuotes(p, out_v1); /*out_v1->lower();*/ p= skipWhitespace(p); if(*p== ',') { p= (char *)((int8 *)p+ 1); p= skipWhitespace(p); } }
  if(out_v2) { p= readWordOrWordsInQuotes(p, out_v2); /*out_v2->lower();*/ p= skipWhitespace(p); if(*p== ',') { p= (char *)((int8 *)p+ 1); p= skipWhitespace(p); } }
  if(out_v3) { p= readWordOrWordsInQuotes(p, out_v3); /*out_v3->lower();*/ p= skipWhitespace(p); if(*p== ',') { p= (char *)((int8 *)p+ 1); p= skipWhitespace(p); } }
  if(out_v4) { p= readWordOrWordsInQuotes(p, out_v4); /*out_v4->lower();*/ p= skipWhitespace(p); if(*p== ',') { p= (char *)((int8 *)p+ 1); p= skipWhitespace(p); } }

  return; // the normal return

ClearText:
  /// clear the output text values, in case line is not good
  if(out_command) out_command->delData();
  if(out_v1) out_v1->delData();
  if(out_v2) out_v2->delData();
  if(out_v3) out_v3->delData();
  if(out_v4) out_v4->delData();
}

bool ixUtil::_getBool(str8 *s) {
  if(*s== "true"  || *s== "1") return true;
  if(*s== "false" || *s== "0") return false;
  return false;
}










// RANDOM number generation
///========================

const uint32 ixUtil::randMax= ~0;
uint32 ixUtil::randSeed_thread0;                 // a main thread seed, beware of thread safety if using multiple threads. use a seed for each thread
uint32 ixUtil::randSeed_thread1;                 // secondary thread seed, beware of thread safety if using multiple threads. use a seed for each thread
uint32 ixUtil::randSeed_thread2;                 // 3rd thread seed, beware of thread safety if using multiple threads. use a seed for each thread
uint32 ixUtil::randSeed_thread3;                 // 4th thread seed, beware of thread safety if using multiple threads. use a seed for each thread

/*
void ixUtil::randInterval32(int32 *out_n, int32 in_min, int32 in_max, uint32 *inout_seed) {
  //x - db.nrBlocks;
  //r - RAND_MAX
  //7  x= (r* db.nrBlocks) /RAND_MAX;

  // float r= ((float)rand()/ (float)RAND_MAX)* ((float)db.nrBlocks- 2.0f);


  // rough:
  // *out_n= (rand()/ RAND_MAX)* (maxval);
  //v1 = rand() % 100;         // v1 in the range 0 to 99
  //v2 = rand() % 100 + 1;     // v2 in the range 1 to 100
  //v3 = rand() % 30 + 1985;   // v3 in the range 1985-2014

  //srand((unsigned int)osi.present);
  
  if(in_max< in_min) { int32 tmp= in_min; in_min= in_max; in_max= tmp; }
  int32 delta= in_max- in_min;
  *out_n= (int32)ixUtil::rand(inout_seed)% delta+ in_min;
}

//  with the shitty rand() it's useless.
//void PRJ_NAME::random64(int64 *out_n, int64 in_min, int64in_max) {
//
//}


float ixUtil::randIntervalf(uint32 *inout_seed) {
  //srand((unsigned int)osi.present);
  return (float)((double)ixUtil::rand(inout_seed)/ (double)~ixUtil::randMax)v ;
}




uint32 ixUtil::rand(uint32 *inout_seed) {
  // x = x * 16807 % 2147483647 rand0
  // x = x * 48271 % 2147483647 rand
  if(inout_seed)
    return (*inout_seed= *inout_seed* 48271% 2147483647);
  else
    return (ixUtil::randSeed_thread0= ixUtil::randSeed_thread0* 48271% 2147483647);
}


void ixUtil::randSeed(uint32 *out_seed) {
  if(out_seed)
    *out_seed= (uint32)osi.present;
  else
    ixUtil::randSeed_thread0= (uint32)osi.present;
}
*/


void ixUtil::randInit() {
  uint64 nano;
  osi.getNanosecs(&nano); ixUtil::randSeed_thread0= (uint32)(  (nano& 0xffffffff));
  osi.getNanosecs(&nano); ixUtil::randSeed_thread1= (uint32)( ~(nano& 0xffffffff));
  osi.getNanosecs(&nano); ixUtil::randSeed_thread2= (uint32)( ((nano& 0xffffffff)* 7));
  osi.getNanosecs(&nano); ixUtil::randSeed_thread3= (uint32)(~((nano& 0xffffffff)* 7));
}

int32 ixUtil::rand32(uint32 *inout_seed, int32 in_min, int32 in_max) {
  // x = x * 16807 % 2147483647 rand0
  // x = x * 48271 % 2147483647 rand
  
  if(inout_seed== null) inout_seed= &ixUtil::randSeed_thread0;

  *inout_seed= (*inout_seed)* 48271% 2147483647;
  
  if((in_min== INT32_MIN) && (in_max== INT32_MAX))
    return (int32)(*inout_seed);

  else {
    // 32bit I DON'T THINK IT'S GOOD
    //if(in_max< in_min) { int32 tmp= in_min; in_min= in_max; in_max= tmp; }
    //int32 delta= (in_max- in_min)+ 1;
    //return (*inout_seed)% delta+ in_min;

    // 64bit
    if(in_max< in_min) { int32 tmp= in_min; in_min= in_max; in_max= tmp; }
    int64 delta= (int64)in_max- (int64)in_min+ 1;
    return (int32)(((int64)(*inout_seed))% delta+ (int64)in_min);
  }
}


float ixUtil::randfnorm(uint32 *inout_seed) {
  return (float)ixUtil::rand32(inout_seed, 0, 1000000)/ 1000000.0f;
}






void ixUtil::changePathSeparator(char *out_str) {
  for(char *p= out_str; *p; p++)
    if(*p== '\\')
      *p= '/';
}


void ixUtil::changePathSeparator(str8 *out_str) {
  for(char *p= out_str->d; *p; p++)
    if(*p== '\\')
      *p= '/';
}

