/*
 *   moocpp/strutils.h
 *
 *   Copyright (C) 2004-2015 by Yevgen Muntyan <emuntyan@users.sourceforge.net>
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

#pragma once

#include <algorithm>
#include <memory>
#include <vector>
#include <utility>
#include <moocpp/utils.h>
#include <mooglib/moo-glib.h>

void extern_g_free(gpointer);
void extern_g_object_unref(gpointer);
void extern_g_strfreev(char**);

namespace moo {

enum class mem_transfer
{
    take_ownership,
    borrow,
    make_copy
};

template<typename T>
class gbuf
{
public:
    gbuf(T* p = nullptr) : m_p(p) {}
    ~gbuf() { ::g_free(m_p); }

    void set(T* p) { if (m_p != p) { ::g_free(m_p); m_p = p; } }
    void reset(T* p = nullptr) { set(p); }
    operator const T*() const { return m_p; }
    T* get() const { return m_p; }
    T*& _get() { return m_p; }

    operator T*() const = delete;
    T** operator&() = delete;

    T* release() { T* p = m_p; m_p = nullptr; return p; }

    MOO_DISABLE_COPY_OPS(gbuf);

    gbuf(gbuf&& other) : gbuf() { std::swap(m_p, other.m_p); }
    gbuf& operator=(gbuf&& other) { std::swap(m_p, other.m_p); return *this; }

    gbuf& operator=(T* p) { set(p); return *this; }

    operator bool() const { return m_p != nullptr; }
    bool operator !() const { return m_p == nullptr; }

private:
    T* m_p;
};

#define MOO_DEFINE_STANDARD_PTR_METHODS_INLINE(Self, Super)                     \
    Self() : Super() {}                                                         \
    Self(const nullptr_t&) : Super(nullptr) {}                                  \
    Self(const Self& other) = delete;                                           \
    Self(Self&& other) : Super(std::move(other)) {}                             \
                                                                                \
    Self& operator=(const Self& other) = delete;                                \
                                                                                \
    Self& operator=(Self&& other)                                               \
    {                                                                           \
        static_cast<Super&>(*this) = std::move(static_cast<Super&&>(other));    \
        return *this;                                                           \
    }                                                                           \
                                                                                \
    Self& operator=(const nullptr_t&)                                           \
    {                                                                           \
        static_cast<Super&>(*this) = nullptr;                                   \
        return *this;                                                           \
    }

#define MOO_DECLARE_STANDARD_PTR_METHODS(Self, Super)                           \
    Self();                                                                     \
    Self(const nullptr_t&);                                                     \
    Self(const Self& other) = delete;                                           \
    Self(Self&& other);                                                         \
    Self& operator=(const Self& other) = delete;                                \
    Self& operator=(Self&& other);                                              \
    Self& operator=(const nullptr_t&);

#define MOO_DEFINE_STANDARD_PTR_METHODS(Self, Super)                            \
    Self::Self() : Super() {}                                                   \
    Self::Self(const nullptr_t&) : Super(nullptr) {}                            \
    Self::Self(Self&& other) : Super(std::move(other)) {}                       \
                                                                                \
    Self& Self::operator=(Self&& other)                                         \
    {                                                                           \
        static_cast<Super&>(*this) = std::move(static_cast<Super&&>(other));    \
        return *this;                                                           \
    }                                                                           \
                                                                                \
    Self& Self::operator=(const nullptr_t&)                                     \
    {                                                                           \
        static_cast<Super&>(*this) = nullptr;                                   \
        return *this;                                                           \
    }

} // namespace moo

template<typename T>
void g_free(const moo::gbuf<T>&) = delete;
