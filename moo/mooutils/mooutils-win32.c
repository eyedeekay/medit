/*
 *   mooutils-win32.c
 *
 *   Copyright (C) 2004-2007 by Yevgen Muntyan <muntyan@math.tamu.edu>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License version 2.1 as published by the Free Software Foundation.
 *
 *   See COPYING file that comes with this distribution.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mooutils/mooutils-misc.h"
#include "mooutils/mooutils-fs.h"
#include "mooutils/moowin32/mingw/fnmatch.h"
#include "mooutils/moowin32/mingw/sys/mman.h"
#include <windows.h>
#include <shellapi.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <io.h>


static char *libmoo_dll_name;


BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);

BOOL WINAPI
DllMain (HINSTANCE            hinstDLL,
	 DWORD                fdwReason,
	 G_GNUC_UNUSED LPVOID lpvReserved)
{
    char *name = NULL;
    wchar_t buf[MAX_PATH+1];

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            if (GetModuleFileNameW ((HMODULE) hinstDLL, buf, G_N_ELEMENTS (buf)))
                name = g_utf16_to_utf8 (buf, -1, NULL, NULL, NULL);
            if (name)
                libmoo_dll_name = g_path_get_basename (name);
            if (!libmoo_dll_name)
                libmoo_dll_name = g_strdup ("libmoo.dll");
            g_free (name);
            break;
    }

    return TRUE;
}


const char *
_moo_win32_get_locale_dir (void)
{
    G_LOCK_DEFINE_STATIC (moo_locale_dir);
    static char *moo_locale_dir = NULL;

    G_LOCK (moo_locale_dir);

    if (!moo_locale_dir)
    {
        char *tmp;
        tmp = g_win32_get_package_installation_subdirectory (NULL, libmoo_dll_name,
                                                             "lib\\locale");
        moo_locale_dir = g_win32_locale_filename_from_utf8 (tmp);
        g_free (tmp);
    }

    G_UNLOCK (moo_locale_dir);

    return moo_locale_dir;
}


static void
add_win32_data_dirs_for_dll (GPtrArray  *list,
                             const char *subdir_name,
                             const char *dllname)
{
    char *dlldir, *datadir;

    dlldir = moo_win32_get_dll_dir (dllname);

    if (g_str_has_suffix (dlldir, "\\"))
    {
        char *tmp = g_strndup (dlldir, strlen(dlldir) - 1);
        g_free (dlldir);
        dlldir = tmp;
    }

    if (g_str_has_suffix (dlldir, "bin") ||
        g_str_has_suffix (dlldir, "lib"))
    {
        char *tmp = g_path_get_dirname (dlldir);
        datadir = g_build_filename (tmp, subdir_name, NULL);
        g_free (tmp);
    }
    else
    {
        datadir = g_strdup (dlldir);
    }

    g_free (dlldir);
    g_ptr_array_add (list, datadir);
}

void
_moo_win32_add_data_dirs (GPtrArray  *list,
                          const char *prefix)
{
    char *subdir;

    subdir = g_strdup_printf ("%s\\" MOO_PACKAGE_NAME, prefix);
    add_win32_data_dirs_for_dll (list, subdir, NULL);
    add_win32_data_dirs_for_dll (list, subdir, libmoo_dll_name);

    g_free (subdir);
}


char *
moo_win32_get_app_dir (void)
{
    static char *moo_app_dir;
    G_LOCK_DEFINE_STATIC(moo_app_dir);

    G_LOCK (moo_app_dir);

    if (!moo_app_dir)
        moo_app_dir = moo_win32_get_dll_dir (NULL);

    G_UNLOCK (moo_app_dir);

    return g_strdup (moo_app_dir);
}

char *
moo_win32_get_dll_dir (const char *dll)
{
	wchar_t *dll_utf16 = NULL;
    char *dir;
    char *dllname = NULL;
    HMODULE handle;
    wchar_t buf[MAX_PATH+1];

    if (dll)
	{
		GError *error = NULL;

		dll_utf16 = g_utf8_to_utf16 (dll, -1, NULL, NULL, &error);

		if (!dll_utf16)
		{
			g_critical ("could not convert name '%s' to UTF16: %s",
						dll, error ? error->message : "");
			g_error_free (error);
			return g_strdup (".");
		}
	}

    handle = GetModuleHandleW (dll_utf16);
    g_return_val_if_fail (handle != NULL, g_strdup ("."));

    if (GetModuleFileNameW (handle, buf, G_N_ELEMENTS (buf)) > 0)
        dllname = g_utf16_to_utf8 (buf, -1, NULL, NULL, NULL);

    if (dllname)
        dir = g_path_get_dirname (dllname);
    else
        dir = g_strdup (".");

    g_free (dllname);
	g_free (dll_utf16);
    return dir;
}


gboolean
_moo_win32_open_uri (const char *uri)
{
    HINSTANCE h;

    g_return_val_if_fail (uri != NULL, FALSE);

    h = ShellExecuteA (NULL, "open", uri, NULL, NULL, SW_SHOWNORMAL);

    if ((int)h <= 32)
    {
        char *msg = g_win32_error_message (GetLastError());
        g_warning ("%s: %s", G_STRLOC, msg);
        g_free (msg);
        return FALSE;
    }

    return TRUE;
}


void
_moo_win32_show_fatal_error (const char *domain,
                             const char *logmsg)
{
    char *msg = NULL;

#define PLEASE_REPORT \
    "Please report it to " PACKAGE_BUGREPORT " and provide "\
    "steps needed to reproduce this error."
    if (domain)
        msg = g_strdup_printf ("Fatal " PACKAGE " error:\n---\n%s: %s\n---\n"
                PLEASE_REPORT, domain, logmsg);
    else
        msg = g_strdup_printf ("Fatal " PACKAGE " error:\n---\n%s\n---\n"
                PLEASE_REPORT, logmsg);
#undef PLEASE_REPORT

    MessageBoxA (NULL, msg, "Error",
                 MB_ICONERROR | MB_APPLMODAL | MB_SETFOREGROUND);

    g_free (msg);
}


#ifndef __MINGW32__
int
_moo_win32_gettimeofday (struct timeval *tp,
                         G_GNUC_UNUSED gpointer tzp)
{
    time_t sec;

    if (tp == NULL || tzp != NULL)
    {
        errno = EINVAL;
        return -1;
    }

    sec = time (NULL);

    if (sec == (time_t) -1)
        return -1;

    tp->tv_sec = sec;
    tp->tv_usec = 0;

     return 0;
}
#endif /* __MINGW32__ */


int
_moo_win32_fnmatch (const char *pattern,
                    const char *string,
                    int         flags)
{
    if (flags != 0)
    {
        errno = EINVAL;
        return -1;
    }

    return _moo_glob_match_simple (pattern, string) ? 0 : 1;
}


/***************************************************************************
 * mmap for poor
 */
static GHashTable *mapped_files;

#define MAPPING_ALLOCATED ((HANDLE) -2)

static void
add_mapped_file (gpointer buffer,
                 HANDLE   mapping)
{
    if (!mapped_files)
        mapped_files = g_hash_table_new (g_direct_hash, g_direct_equal);

    g_hash_table_insert (mapped_files, buffer, mapping);
}

static gboolean
remove_mapped_file (gpointer buffer)
{
    HANDLE mapping;

    if (!mapped_files)
        return FALSE;

    mapping = g_hash_table_lookup (mapped_files, buffer);
    g_return_val_if_fail (mapping != NULL, FALSE);

    g_hash_table_remove (mapped_files, buffer);

    if (mapping == MAPPING_ALLOCATED)
    {
        g_free (buffer);
    }
    else
    {
        UnmapViewOfFile (buffer);
        CloseHandle (mapping);
    }

    if (g_hash_table_size (mapped_files) == 0)
    {
        g_hash_table_destroy (mapped_files);
        mapped_files = NULL;
    }

    return TRUE;
}

void *
_moo_win32_mmap (gpointer start,
                 guint64  length,
                 int      prot,
                 int      flags,
                 int      fd,
                 guint64  offset)
{
    struct stat st;
    HANDLE mapping;
    char *buffer;

    g_return_val_if_fail (start == NULL, NULL);
    g_return_val_if_fail (prot == PROT_READ, NULL);
    g_return_val_if_fail (flags == MAP_SHARED, NULL);
    g_return_val_if_fail (offset == 0, NULL);

    errno = 0;
    if (fstat (fd, &st) != 0)
        return MAP_FAILED;

    if ((guint64) st.st_size != length)
    {
        errno = EINVAL;
        return MAP_FAILED;
    }

    if (length == 0)
    {
        buffer = g_new (char, 1);
        buffer[0] = 0;
        add_mapped_file (buffer, MAPPING_ALLOCATED);
        return buffer;
    }

    mapping = CreateFileMapping ((HANDLE) _get_osfhandle (fd), NULL,
                                 PAGE_READONLY, 0, 0, NULL);

    if (!mapping)
    {
        errno = EINVAL;
        return MAP_FAILED;
    }

    buffer = MapViewOfFile (mapping, FILE_MAP_READ, 0, 0, 0);

    if (!buffer)
    {
        CloseHandle (mapping);
        errno = EINVAL;
        return MAP_FAILED;
    }

    add_mapped_file (buffer, mapping);

    return buffer;
}

int
_moo_win32_munmap (gpointer start,
                   G_GNUC_UNUSED gsize length)
{
    if (!remove_mapped_file (start))
    {
        errno = EINVAL;
        return -1;
    }
    else
    {
        return 0;
    }
}
