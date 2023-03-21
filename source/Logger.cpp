
#include "gt_base/Logger.h"
#include <iostream>

using namespace gt;
using namespace std;

Logger::Logger(string const & label, string const & filePath)
{
  if(filePath.size())
  {
    if(_logFile.is_open()) { _logFile.close(); }

    _logFile.open(filePath.c_str(), ios::out | ios::trunc);

    cout.rdbuf(_logFile.rdbuf());

    _label = "";
  }
  else
  {
    _label = label;
  }
}

Logger::~Logger()
{
  if(_logFile.is_open())
  {
    // _logFile.flush(); /// this might be crashing maya
    _logFile.close();
  }
}

/// debug
void Logger::debug(u16 i)
{
#ifdef GT_DEBUG
  // _mutex.lock();
  cout << "DEBUG: ";
  if(_label.size()) { cout << _label << ": "; }
  cout << i << endl;
// _mutex.unlock();
#endif
}

void Logger::debug(string const & s)
{
#ifdef GT_DEBUG
  // _mutex.lock();
  cout << "DEBUG: ";
  if(_label.size()) { cout << _label << ": "; }
  cout << s << endl;
// _mutex.unlock();
#endif
}

/// info
void Logger::info(string const & s)
{
  // _mutex.lock();

  if(_label.size()) { cout << _label << ": "; }
  cout << s << endl;

  // _mutex.unlock();
}

/// warning
void Logger::warning(string const & s)
{
  // _mutex.lock();

  cout << "WARNING: ";

  if(_label.size()) { cout << _label << ": "; }

  cout << s << endl;

  // _mutex.unlock();
}

/// error
void Logger::error(string const & s)
{
  // _mutex.lock();
  cout << "ERROR: ";
  if(_label.size()) { cout << _label << ": "; }
  cout << s << endl;
  // _mutex.unlock();
}

void Logger::raw(string const & s) { cout << s; }
