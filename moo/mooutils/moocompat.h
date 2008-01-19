/*
 *   moocompat.h
 *
 *   Copyright (C) 2004-2007 by Yevgen Muntyan <muntyan@math.tamu.edu>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License version 2.1 as published by the Free Software Foundation.
 *
 *   See COPYING file that comes with this distribution.
 */

#ifndef MOOUTILS_COMPAT_H
#define MOOUTILS_COMPAT_H

#include <gtk/gtk.h>

#ifndef __WIN32__ /* TODO */
#include <sys/types.h>
#endif /* __WIN32__ */

G_BEGIN_DECLS


#if !GLIB_CHECK_VERSION(2,8,0)
#define g_mkdir_with_parents            _moo_g_mkdir_with_parents
#define g_listenv                       _moo_g_listenv
#define g_utf8_collate_key_for_filename _moo_g_utf8_collate_key_for_filename
#define g_file_set_contents             _moo_g_file_set_contents

int         g_mkdir_with_parents            (const char *pathname,
                                             int         mode);
char      **g_listenv                       (void);
char       *g_utf8_collate_key_for_filename (const char *str,
                                             gssize      len);
gboolean    g_file_set_contents             (const char *filename,
                                             const char *contents,
                                             gssize      length,
                                             GError    **error);
#if     __GNUC__ >= 4
#define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
#else
#define G_GNUC_NULL_TERMINATED
#endif
#endif /* !GLIB_CHECK_VERSION(2,8,0) */


#if !GLIB_CHECK_VERSION(2,10,0)

G_END_DECLS
#include "mooutils/mooutils-misc.h"
G_BEGIN_DECLS

#define g_intern_string _moo_intern_string
#define gdk_atom_intern_static_string _moo_gdk_atom_intern_static_string

GdkAtom     gdk_atom_intern_static_string   (const char *atom_name);

#endif /* !GLIB_CHECK_VERSION(2,10,0) */


#if !GTK_CHECK_VERSION(2,10,0)

#ifndef GTK_STOCK_SELECT_ALL
#define GTK_STOCK_SELECT_ALL  "gtk-select-all"
#endif

typedef enum {
  GTK_UNIT_PIXEL,
  GTK_UNIT_POINTS,
  GTK_UNIT_INCH,
  GTK_UNIT_MM
} GtkUnit;

#define GTK_TYPE_UNIT (gtk_unit_get_type ())
GType gtk_unit_get_type (void) G_GNUC_CONST;

#endif /* !GTK_CHECK_VERSION(2,10,0) */


#if !GTK_CHECK_VERSION(2,12,0)

#ifndef GTK_STOCK_DISCARD
#define GTK_STOCK_DISCARD  "gtk-discard"
#endif

#endif /* !GTK_CHECK_VERSION(2,10,0) */


G_END_DECLS

#endif /* MOOUTILS_COMPAT_H */
