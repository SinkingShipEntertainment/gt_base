
#include <stdlib.h>  

namespace gt
{

#if defined(WINDOWS) || defined(WIN32)
  inline int setenv(char const * name, char const * value, int overwrite)
  {
      int errcode = 0;
      if(!overwrite) {
          size_t envsize = 0;
          errcode = getenv_s(&envsize, NULL, 0, name);
          if(errcode || envsize) return errcode;
      }
      return _putenv_s(name, value);
  }
#endif

}
