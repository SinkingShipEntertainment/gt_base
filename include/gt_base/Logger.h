 
#pragma once

#include "gt_base/types.h"
#include <fstream>
#include <string>

namespace gt
{

class Logger
{
public:

  Logger(std::string const & label = std::string(), std::string const & filePath = std::string());
  ~Logger();

  void setLogPath(std::string const & filePath);

  void debug(u16 i);
  void debug(std::string const & s);

  void info(std::string const & s);

  void warning(std::string const & s);

  void error(std::string const & s);

  void raw(std::string const & s);

  std::string const & label() {return _label;}

  /// alias
  void d(u16 i) { debug(i); }
  void d(std::string const & s) { debug(s); }
  void i(std::string const & s) { info(s); }
  void w(std::string const & s) { warning(s); }
  void e(std::string const & s) { error(s); }
  void r(std::string const & s) { raw(s); }

private:

  std::string _label;
  std::ofstream _logFile;
};

} // namespace gt
