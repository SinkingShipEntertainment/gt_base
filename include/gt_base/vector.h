
#pragma once

#include "gt_base/types/base.h"
#include <string>
#include <vector>

namespace gt
{
/// copy
template <typename T>
inline bool find(std::string const & name, std::vector<T> const & v, T & t)
{
  for(auto const & i : v)
  {
    if(name == i.name())
    {
      t = i;
      return true;
    }
  }
  return false;
}

template <typename T>
inline T * find(std::string const & name, std::vector<T *> const & v)
{
  for(auto * t : v)
  {
    if(name == t->name()) return t;
  }
  return NULL;
}

template <typename T>
inline bool contains(std::string const & name, std::vector<T> const & v)
{
  for(auto t : v)
  {
    if(name == t.name()) return true;
  }
  return false;
}

template <typename T>
inline bool contains(std::string const & name, std::vector<T *> const & v)
{
  for(auto * t : v)
  {
    if(name == t->name()) return true;
  }
  return false;
}

template <typename T>
inline bool contains(T const t, std::vector<T> const & v)
{
  for(auto itr : v)
  {
    if(itr == t) return true;
  }
  return false;
}

template <typename T>
inline bool remove(std::string const & name, std::vector<T> & v)
{
  u32 i = 0;
  for(T t : v)
  {
    if(name == t.name())
    {
      v.erase(v.begin() + i);
      return true;
    }
    i++;
  }
  return false;
}

template <typename T>
inline bool remove(std::string const & name, std::vector<T *> & v)
{
  u32 i = 0;
  for(T * t : v)
  {
    if(name == t->name())
    {
      v.erase(v.begin() + i);
      return true;
    }
    i++;
  }
  return false;
}

template <typename T>
inline bool remove(T const * t, std::vector<T *> & v)
{
  u32 i = 0;
  for(T * itr : v)
  {
    if(itr == t)
    {
      v.erase(v.begin() + i);
      return true;
    }
    i++;
  }
  return false;
}

template <typename T>
inline bool remove(T const t, std::vector<T> & v)
{
  u32 i = 0;
  for(T itr : v)
  {
    if(itr == t)
    {
      v.erase(v.begin() + i);
      return true;
    }
    i++;
  }
  return false;
}

template <typename T>
inline bool removeAndDelete(std::string const & name, std::vector<T *> & v)
{
  u32 i = 0;
  for(T * t : v)
  {
    if(name == t->name())
    {
      v.erase(v.begin() + i);
      delete t;
      t = NULL;
      return true;
    }
    i++;
  }
  return false;
}

template <typename T>
inline void clear(std::vector<T *> & v)
{
  for(T * t : v)
  {
    if(t)
    {
      delete t;
      t = NULL;
    }
  }
  v.clear();
}

}  // namespace gt
