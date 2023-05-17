
#pragma once

#include "gt_base/types.h"
#include <chrono>

namespace gt
{

struct Timer
{
  Timer()
  {
    this->start();
  }
  
  void start()
  {
    startTime = std::chrono::high_resolution_clock::now();
  }

  f32 elapsed() /// in seconds
  {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count() * 0.001;
  }

  void stop()
  {
    totalTime = elapsed();
  }

  f32 totalTime; // in seconds

private:
    std::chrono::high_resolution_clock::time_point startTime;

};

} // namespace gt