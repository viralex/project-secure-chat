#ifndef  _COMMON_H_
#define  _COMMON_H_

#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <exception>

#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>

#define NEW_RETURN(POINTER,CONSTRUCTOR,RET_VAL) \
   do{ try{ POINTER = new CONSTRUCTOR; } catch (std::bad_alloc)\
     { errno = ENOMEM; POINTER = NULL; return RET_VAL; } \
   }while(0)
#define NEW(POINTER,CONSTRUCTOR) \
   do{ try{ POINTER = new CONSTRUCTOR; } catch (std::bad_alloc)\
     { errno = ENOMEM; POINTER = NULL; } \
   }while(0)

#if COMPILER == COMPILER_GNU && (__GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 3)
# include <tr1/unordered_map>
#elif COMPILER == COMPILER_GNU && __GNUC__ >= 3
# include <ext/hash_map>
#elif COMPILER == COMPILER_INTEL
# include <ext/hash_map>
#elif COMPILER == COMPILER_MICROSOFT && (_MSC_VER > 1500 || _MSC_VER == 1500 && _HAS_TR1) // VC9.0 SP1 and later
# include <unordered_map>
#else
# include <hash_map>
#endif

#if COMPILER == COMPILER_GNU && (__GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#define UNORDERED_MAP std::tr1::unordered_map
#elif (COMPILER == COMPILER_GNU && __GNUC__ >= 3) || COMPILER == COMPILER_INTEL
#define UNORDERED_MAP __gnu_cxx::hash_map

namespace __gnu_cxx
{
    template<> struct hash<unsigned long long>
    {
        size_t operator()(const unsigned long long &__x) const { return (size_t)__x; }
    };
    template<typename T> struct hash<T *>
    {
        size_t operator()(T * const &__x) const { return (size_t)__x; }
    };
    template<> struct hash<std::string>
    {
        size_t operator()(const std::string &__x) const
        {
            return hash<const char *>()(__x.c_str());
        }
    };
};

#else
#define UNORDERED_MAP std::hash_map
using std::hash_map;
#endif

#define uint64 uint64_t
#define uint32 uint32_t
#define uint16 uint16_t
#define uint8  uint8_t
#define int64 int64_t
#define int32 int32_t
#define int16 int16_t
#define int8  int8_t

#include "utility/logger.h"
#include "utility/file.h"
#include "utility/timer.h"
#include "utility/config.h"
#include "revision.h"

void   welcome();

#endif

