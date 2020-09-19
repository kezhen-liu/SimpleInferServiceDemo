#include <common/common.hpp>

namespace inference{

void sisdAssertFail(const char* cond, int line, const char* file, const char* func){
    fprintf(stderr,
            "%s:%d: Assertion \"%s\" in function \"%s\" failed\n",
            file, line, cond, func);
    fflush(stderr);
    abort();
}

}
