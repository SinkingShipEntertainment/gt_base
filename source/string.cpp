
#include "gt_base/string.h"
#include "gt_base/types/f32x2.h"
#include "gt_base/types/f32x3.h"
#include "gt_base/types/f32x4.h"

using namespace gt;
using namespace std;

void gt::split(string const & str, i8 const & delim, vector<string> & tokens)
{
  string s;
  for(auto const c: str)
  {
    if(c == delim)
    {
      if(s.size())
      {
        tokens.emplace_back(s);
        s.clear();
      }
      continue;
    }
    s += c;
  }
  if(s.size()) tokens.emplace_back(s);
}

vector<string> gt::split(string const & str, i8 const & delim) /// shorthand version
{
  vector<string> tokens;
  split(str, delim, tokens);
  return tokens;
}

bool gt::isNumber(string const & s)
{
  if(s.empty()) return false;
  for(auto const c: s)
  {
    if(!isdigit(c)) return false;
  }
  return true;
}

bool gt::strToBool(string const & s)
{
  if(s == "true" || s == "True" || s == "1") { return true; }
  return false;
}

f32x2 gt::strToF32x2(string const & s)
{
  f32x2 v(0);

  vector<string> tokens;
  split(s, ',', tokens);
  if(tokens.size() != 2) return v; // throw runtime_error(""); ?

  v.x = atof(tokens[0].c_str());
  v.y = atof(tokens[1].c_str());

  return v;
}

f32x3 gt::strToF32x3(string const & s)
{
  f32x3 v(0);

  vector<string> tokens;
  split(s, ',', tokens);
  if(tokens.size() != 3) return v;

  v.x = atof(tokens[0].c_str());
  v.y = atof(tokens[1].c_str());
  v.z = atof(tokens[2].c_str());

  return v;
}


f32x4 gt::strToF32x4(string const & s)
{
  f32x4 v(0);

  vector<string> tokens;
  split(s, ',', tokens);
  if(tokens.size() != 4) return v;

  v.x = atof(tokens[0].c_str());
  v.y = atof(tokens[1].c_str());
  v.z = atof(tokens[2].c_str());
  v.w = atof(tokens[3].c_str());

  return v;
}
