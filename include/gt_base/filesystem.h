
#pragma once

#include "gt_base/types/base.h"
#include <string>
#include <vector>

namespace gt
{

std::vector<std::string> const getSeqPaths(std::string const & seqPath, std::string const ext = "");
std::vector<std::string> const getSeqPaths(std::string const & seqPathStr, std::vector<u32> const & frames);

} // namespace gt

