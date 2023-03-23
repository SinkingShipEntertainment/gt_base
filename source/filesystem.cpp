
#include "gt_base/filesystem.h"
#include "gt_base/types/base.h"
#include "gt_base/Logger.h"
#include "gt_base/string.h"
// #include <filesystem>
#include <experimental/filesystem>

using namespace gt;
using namespace std;

namespace filesystem = std::experimental::filesystem;

string const getPadded(u32 num, u8 numDigits)
{
  string s = to_string(num);
  s.insert(s.begin(), numDigits - s.size(), '0');
  return s;
}

vector<string> const gt::getSeqPaths(string const & seqPathStr, string const ext)
{
  u8 paddingSize = 4;

  filesystem::path const seqPath(seqPathStr);

  vector<string> seqPaths;

  /// if it's a directory, sequence order isn't guaranteed
  if(filesystem::is_directory(seqPath))
  {
    /// TODO: currently this produces paths like:
    /// C:/Users/tom/work_offline/maya/projects/default/sourceimages/udimTest/mari\test.1013.exr
    /// would be better if it was C:/Users/tom/work_offline/maya/projects/default/sourceimages/udimTest/mari/test.1013.exr
    for(auto const & di : filesystem::directory_iterator(seqPath))
    {
      filesystem::path p = di.path();

      if(filesystem::is_directory(p)) continue;
      if(p.extension().string() != ext) continue;

      seqPaths.emplace_back(p.string());
    }
    return seqPaths;
  }

  /// otherwise try it as a sequence path
  string stem = seqPath.stem().string();

  i8 sepChar = '.';

  vector<string> parts = split(stem, sepChar);
  if(parts.size() < 2)
  {
    /// try _ as well
    sepChar = '_';
    parts   = split(stem, sepChar);
    if(parts.size() < 2) throw runtime_error(f("% couldn't extract frame range; something like: filename.0001.exr is required") % stem);
  }

  string frameRange = parts.back();

  parts.pop_back();
  string basename;
  for(string const & s : parts)
  {
    basename += s + sepChar;
  }

  string dirPath = seqPath.parent_path().string();
  string seqExt  = seqPath.extension().string();

  u32 startFrame = 0;
  u32 endFrame   = 0;

  /// check if it's pound padding ie C:/seqName.####.exr
  bool poundPad = true;
  u8 poundCount = 0;
  for(auto c : frameRange)
  {
    if(c != '#')
    {
      poundPad = false;
      break;
    }
    poundCount++;
  }

  if(poundPad)  /// ie C:/seqName.####.exr
  {
    paddingSize = poundCount;

    startFrame = 4294967295;  /// max u32

    /// get first and last frame
    for(auto const & di : filesystem::directory_iterator(dirPath))
    {
      filesystem::path p = di.path();
      if(p.extension().string() != seqExt) continue;

      string stem_ = p.stem().string();
      if(!stem_.rfind(basename, 0) == 0) { continue; }

      vector<string> parts = split(stem_, sepChar);
      if(!parts.size()) continue;

      string frameNumStr = parts.back();
      if(!isNumber(frameNumStr)) continue;
      u32 frameNum = strToU32(frameNumStr);
      if(startFrame > frameNum) startFrame = frameNum;
      if(endFrame < frameNum) endFrame = frameNum;
    }
  }
  else  /// assume explicit frame range ie C:/seqName.1-20#.exr
  {
    if(frameRange[frameRange.length() - 1] != '#')  /// assume it's not a seq, just a path
    {
      seqPaths.emplace_back(seqPathStr);
      return seqPaths;
      // throw runtime_error(f("% doesn\'t end with #") % frameRange);
    }
    frameRange = frameRange.substr(0, frameRange.length() - 1);  /// remove #

    parts.clear();
    split(frameRange, '-', parts);

    if(parts.size() != 2) { throw runtime_error("couldn't extract first and last frame"); }

    startFrame = strToU32(parts[0]);
    endFrame   = strToU32(parts[1]);
  }

  if(startFrame > endFrame) throw runtime_error(f("bad frame range: % - %") % startFrame % endFrame);

  for(u32 i = startFrame; i <= endFrame; i++)
  {
    string paddedNum = getPadded(i, paddingSize);
    string fullPath  = f("%/%%%") % dirPath % basename % paddedNum % seqExt;

    if(filesystem::exists(fullPath)) seqPaths.emplace_back(fullPath);
    // else l.w(f("% doesn't exist") % fullPath);
  }

  return seqPaths;
}

/// overloaded function that gets the sequence paths from a sequence path (string) and a list of frames (vector<u32>)
vector<string> const gt::getSeqPaths(string const & seqPathStr, vector<u32> const & frames)
{
  vector<string> seqPaths;

  filesystem::path seqPath(seqPathStr);

  /// if it's a directory, iterate through the files and try to find the frames
  if(filesystem::is_directory(seqPath))
  {
    string const & dirPath = seqPathStr;
    for(auto const & di : filesystem::directory_iterator(seqPath))
    {
      filesystem::path p = di.path();

      if(filesystem::is_directory(p)) continue;

      string const & stem = p.stem().string();

      vector<string> parts = split(stem, '.');
      if(parts.size() < 2) continue;
      
      string const & ext = p.extension().string();
      
      for(auto const frame : frames)
      {
        string paddedNum      = getPadded(frame, 4);
        string const filePath = f("%/%.%%") % dirPath % parts[0] % paddedNum % ext;

        if(filesystem::exists(filePath)) seqPaths.emplace_back(filePath);
      }
    }
    return seqPaths;
  }

  string const & dirPath = seqPath.parent_path().string();
  string const & stem    = seqPath.stem().string();
  string const & ext     = seqPath.extension().string();

  vector<string> parts = split(stem, '.');
  if(parts.size() < 2) { throw runtime_error(f("% couldn't extract frame range; something like: filename.0001.exr is required") % stem); }

  for(auto const frame : frames)
  {
    string paddedNum      = getPadded(frame, 4);
    string const filePath = f("%/%.%%") % dirPath % parts[0] % paddedNum % ext;

    if(filesystem::exists(filePath)) seqPaths.emplace_back(filePath);
  }

  return seqPaths;
}
