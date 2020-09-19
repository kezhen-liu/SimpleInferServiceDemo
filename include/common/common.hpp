#ifndef SISD_COMMON_HPP
#define SISD_COMMON_HPP

#include <string>
#include <memory>
#include <stdexcept>


#if defined(__linux__)
#define SISD_DECLSPEC 
#elif defined(WIN32)
#define SISD_DECLSPEC __declspec(dllexport) 
#endif

#define _SISD_PP_CONCAT(A, B) A##B
#define SISD_PP_CONCAT(A, B) _SISD_PP_CONCAT(A, B)

#define SISD_ASSERT(cond) do{if(!(cond)){::SISD::sisdAssertFail(#cond, __LINE__, __FILE__, \
    __func__);}} while(false);

namespace SISD{

SISD_DECLSPEC void sisdAssertFail(const char* cond, int line, const char* file, const char* func);

}

#endif //#ifndef SISD_COMMON_HPP
