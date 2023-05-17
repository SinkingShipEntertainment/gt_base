#pragma once

#include "gt_base/types.h"
#include <string>
#include <sstream>
#include <vector>

namespace gt
{

class f32x2;
class f32x3;
class f32x4;

template<typename T> inline std::string const operator % (std::string const s, T const & x)
{
  std::string retStr;

  bool found = false;
  bool escapeChar = false;
  for(auto const i: s)
  {
    if(i == '%' && !found && !escapeChar)
    {
      std::stringstream ss(std::stringstream::out);
      ss << x;
      retStr += ss.str();
      found = true;
    }
    else
    {
      retStr += i;
    }
    
    if(i == 60) /// backslash
    {
      escapeChar = true;
    }
    else
    {
      escapeChar = false;
    }
  }
  
  return retStr;
}

std::vector<std::string> split(std::string const & str, i8 const & delim); /// shorthand version
void split(std::string const & str, i8 const & delim, std::vector<std::string> & tokens);

bool isNumber(std::string const & s);

inline u32 charToU32(i8 const c) {return c - '0';}
inline u32 strToU32(std::string const & s) {return ::atoi(s.c_str());}
inline i32 strToI32(std::string const & s) {return ::atoi(s.c_str());}
inline i64 strToI64(std::string const & s) {return ::atol(s.c_str());}
inline f32 strToF32(std::string const & s) {return ::atof(s.c_str());}
inline f32 strToF64(std::string const & s) {return ::atof(s.c_str());}
bool  strToBool(std::string const & s);
f32x2  strToF32x2(std::string const & s);
f32x3 strToF32x3(std::string const & s);
f32x4 strToF32x4(std::string const & s);

typedef std::string f; /// f for format

} // namespace gt
