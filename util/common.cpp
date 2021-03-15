#include "ix/ix.h"
//#include "osi/include/util/typeShortcuts.h"
//#include "osi/include/util/str8.h"
//#include "common.hpp"


bool isWhitespace(uint32 c) {
  if     (c== ' ')  return true; // (0x20) (SPC) space 
  else if(c== '\t') return true; // (0x09) (TAB) horizontal tab 
  else if(c== '\n') return true; // (0x0a) (LF)  newline 
  else if(c== '\v') return true; // (0x0b) (VT)  vertical tab 
  else if(c== '\f') return true; // (0x0c) (FF)  feed 
  else if(c== '\r') return true; // (0x0d) (CR)  carriage return 
  else return false;
}


char *skipWhitespace(char *in_string) {
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


char *readWordOrWordsInQuotes(char *in_string, str8 *out_string) {
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

char *readWordsInBrackets(char *in_string, str8 *out_string) {
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
void parseGenericTxtCommand(str8 *in_line, str8 *out_command, str8 *out_v1, str8 *out_v2, str8 *out_v3, str8 *out_v4) {
  
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
  
  if(*p== '[') p= readWordsInBrackets(p, out_command);
  else         p= readWordOrWordsInQuotes(p, out_command);

  if(out_command->d== null) goto ClearText;
  if(*out_command== "") goto ClearText;

  out_command->lower();
  if(out_command->len>= 2) {
    if(out_command->d[out_command->len- 2]== '=') *out_command-= 1;
    else if(out_command->d[out_command->len- 2]== ':') *out_command-= 1;
  }

  p= skipWhitespace(p);
  if(*p== '=' || *p== ':' || *p== ',') p++;
  p= skipWhitespace(p);
  
  // the parameters
  if(out_v1) { p= readWordOrWordsInQuotes(p, out_v1); out_v1->lower(); p= skipWhitespace(p); if(*p== ',') { p= (char *)((int8 *)p+ 1); p= skipWhitespace(p); } }
  if(out_v2) { p= readWordOrWordsInQuotes(p, out_v2); out_v2->lower(); p= skipWhitespace(p); if(*p== ',') { p= (char *)((int8 *)p+ 1); p= skipWhitespace(p); } }
  if(out_v3) { p= readWordOrWordsInQuotes(p, out_v3); out_v3->lower(); p= skipWhitespace(p); if(*p== ',') { p= (char *)((int8 *)p+ 1); p= skipWhitespace(p); } }
  if(out_v4) { p= readWordOrWordsInQuotes(p, out_v4); out_v4->lower(); p= skipWhitespace(p); if(*p== ',') { p= (char *)((int8 *)p+ 1); p= skipWhitespace(p); } }

  return; // the normal return

ClearText:
  /// clear the output text values, in case line is not good
  if(out_command) out_command->delData();
  if(out_v1) out_v1->delData();
  if(out_v2) out_v2->delData();
  if(out_v3) out_v3->delData();
  if(out_v4) out_v4->delData();
}

bool _getBool(str8 *s) {
  if(*s== "true"  || *s== "1") return true;
  if(*s== "false" || *s== "0") return false;
  return false;
}









