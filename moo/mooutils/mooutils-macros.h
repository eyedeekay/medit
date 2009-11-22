/*
 *   mooutils-macros.h
 *
 *   Copyright (C) 2004-2009 by Yevgen Muntyan <muntyan@tamu.edu>
 *
 *   This file is part of medit.  medit is free software; you can
 *   redistribute it and/or modify it under the terms of the
 *   GNU Lesser General Public License as published by the
 *   Free Software Foundation; either version 2.1 of the License,
 *   or (at your option) any later version.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with medit.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MOO_UTILS_MACROS_H
#define MOO_UTILS_MACROS_H

#if defined(__GNUC__)

  #undef MOO_COMPILER_MSVC

  #define MOO_COMPILER_GCC 1
  #define MOO_GCC_VERSION(maj,min) ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))

#elif defined(_MSC_VER)

  #define MOO_COMPILER_MSVC 1

  #undef MOO_COMPILER_GCC
  #define MOO_GCC_VERSION(maj,min) (0)

#else /* not gcc, not visual studio */

  #undef MOO_COMPILER_MSVC

  #undef MOO_COMPILER_GCC
  #define MOO_GCC_VERSION(maj,min) (0)

#endif /* gcc or visual studio */

#if defined(MOO_COMPILER_GCC)
#define MOO_STRFUNC     ((const char*) (__PRETTY_FUNCTION__))
#elif defined(MOO_COMPILER_MSVC)
#define MOO_STRFUNC     ((const char*) (__FUNCTION__))
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define MOO_STRFUNC     ((const char*) (__func__))
#else
#define MOO_STRFUNC     ((const char*) (""))
#endif

#ifdef __COUNTER__
#define _MOO_CODE_LOCATION_ARGS __FILE__, __LINE__, __COUNTER__
#else
#define _MOO_CODE_LOCATION_ARGS __FILE__, __LINE__, 0
#endif

#define _MOO_ASSERT_CHECK(cond, func)                   \
do {                                                    \
    if (cond)                                           \
        ;                                               \
    else                                                \
        func("condition failed: " #cond,                \
             _MOO_CODE_LOCATION_ARGS,                   \
             MOO_STRFUNC);                              \
} while(0)

#ifdef DEBUG
#define _MOO_ASSERT _MOO_ASSERT_CHECK
#else
#define _MOO_ASSERT(cond, func) do {} while(0)
#endif

#define _MOO_CONCAT_(a, b) a##b
#define _MOO_CONCAT(a, b) _MOO_CONCAT_(a, b)
#define __MOO_STATIC_ASSERT_MACRO(cond, counter) enum { _MOO_CONCAT(_MooStaticAssert_##counter##_, __LINE__) = 1 / ((cond) != 0) }
#define _MOO_STATIC_ASSERT_MACRO(cond) __MOO_STATIC_ASSERT_MACRO(cond, 0)
#define MOO_STATIC_ASSERT(cond, message) _MOO_STATIC_ASSERT_MACRO(cond)
MOO_STATIC_ASSERT(sizeof(char) == 1, "test");

#endif /* MOO_UTILS_MACROS_H */
