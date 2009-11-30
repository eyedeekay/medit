/*
 *   moocpp-exc.h
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

#ifndef MOO_CPP_EXC_H
#define MOO_CPP_EXC_H

#include <mooutils/moocpp-macros.h>

namespace moo {

#if defined(MOO_COMPILER_MSVC)
#define MOO_NOTHROW __declspec(nothrow)
#else
#define MOO_NOTHROW throw()
#endif

class Exception
{
protected:
    Exception(const char *what, const MooCodeLoc *loc) MOO_NOTHROW
        : m_what(what ? what : "")
        , m_loc(loc ? *loc : moo_default_code_loc ())
    {
    }

    virtual ~Exception() MOO_NOTHROW
    {
    }

public:
    const char *what() const MOO_NOTHROW { return m_what; }

private:
    const char *m_what;
    MooCodeLoc m_loc;

    MOO_DISABLE_COPY_AND_ASSIGN(Exception)
};

class ExcUnexpected : public Exception
{
protected:
    ExcUnexpected(const char *msg, const MooCodeLoc &loc) MOO_NOTHROW
        : Exception(msg, &loc)
    {
    }

    virtual ~ExcUnexpected() MOO_NOTHROW
    {
    }

    MOO_DISABLE_COPY_AND_ASSIGN(ExcUnexpected)

public:
    MOO_NORETURN static void raise(const char *msg, const MooCodeLoc &loc)
    {
        moo_assert_message(msg, loc);
        throw ExcUnexpected(msg, loc);
    }
};

} // namespace moo

#define mooThrowIfFalse(cond)                                   \
do {                                                            \
    if (cond)                                                   \
        ;                                                       \
    else                                                        \
        moo::ExcUnexpected::raise("condition failed: " #cond,   \
                                  MOO_CODE_LOC);                \
} while(0)

#define mooThrowIfReached()                                     \
do {                                                            \
    moo::ExcUnexpected::raise("should not be reached",          \
                              MOO_CODE_LOC);                    \
} while(0)

#define MOO_BEGIN_NO_EXCEPTIONS                                 \
try {

#define MOO_END_NO_EXCEPTIONS                                   \
} catch (...) {                                                 \
    mooCheckNotReached();                                       \
}

#define MOO_BEGIN_CATCH_ALL                                     \
try {

#define MOO_END_CATCH_ALL                                       \
} catch (...) {                                                 \
    mooAssertNotReached();                                      \
}

#endif /* MOO_CPP_EXC_H */
/* -%- strip:true -%- */