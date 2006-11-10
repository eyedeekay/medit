/*
 *   mootextprint.h
 *
 *   Copyright (C) 2004-2006 by Yevgen Muntyan <muntyan@math.tamu.edu>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   See COPYING file that comes with this distribution.
 */

#ifndef MOOEDIT_COMPILATION
#error "This file may not be used directly"
#endif

#ifndef __MOO_TEXT_PRINT_H__
#define __MOO_TEXT_PRINT_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define MOO_TYPE_PRINT_OPERATION              (_moo_print_operation_get_type ())
#define MOO_PRINT_OPERATION(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_PRINT_OPERATION, MooPrintOperation))
#define MOO_PRINT_OPERATION_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_PRINT_OPERATION, MooPrintOperationClass))
#define MOO_IS_PRINT_OPERATION(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_PRINT_OPERATION))
#define MOO_IS_PRINT_OPERATION_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_PRINT_OPERATION))
#define MOO_PRINT_OPERATION_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_PRINT_OPERATION, MooPrintOperationClass))

typedef struct _MooPrintPreview           MooPrintPreview;
typedef struct _MooPrintOperation         MooPrintOperation;
typedef struct _MooPrintOperationClass    MooPrintOperationClass;

typedef enum {
    MOO_PRINT_WRAP       = 1 << 0,
    MOO_PRINT_ELLIPSIZE  = 1 << 1,
    MOO_PRINT_USE_STYLES = 1 << 2,
    MOO_PRINT_HEADER     = 1 << 3,
    MOO_PRINT_FOOTER     = 1 << 4
} MooPrintOptions;

typedef enum {
    MOO_PRINT_POS_LEFT,
    MOO_PRINT_POS_CENTER,
    MOO_PRINT_POS_RIGHT
} MooPrintPos;

typedef struct {
    gboolean do_print;
    PangoFontDescription *font;
    char *format[3];
    gboolean separator;

    PangoLayout *layout;
    double text_height;
    double separator_before;
    double separator_after;
    double separator_height;
    gpointer parsed_format[3];
} MooPrintHeaderFooter;

struct _MooPrintOperation
{
    GtkPrintOperation base;

    GtkWindow *parent;
    GtkTextView *doc;
    GtkTextBuffer *buffer;

    MooPrintPreview *preview;

    /* print settings */
    int first_line;
    int last_line;          /* -1 to print everything after first_line */
    char *font;             /* overrides font set in the doc */
    MooPrintOptions options;
    PangoWrapMode wrap_mode;

    char *filename;
    char *basename;
    MooPrintHeaderFooter header;
    MooPrintHeaderFooter footer;
    gpointer tm; /* struct tm * */

    /* aux stuff */
    GArray *pages;          /* GtkTextIter's pointing to pages start */
    PangoLayout *layout;

    struct {
        double x;
        double y;
        double width;
        double height;
    } page;                 /* text area */
};

struct _MooPrintOperationClass
{
    GtkPrintOperationClass base_class;
};


GType   _moo_print_operation_get_type           (void) G_GNUC_CONST;

void    _moo_print_operation_set_doc            (MooPrintOperation  *print,
                                                 GtkTextView        *doc);
void    _moo_print_operation_set_buffer         (MooPrintOperation  *print,
                                                 GtkTextBuffer      *buffer);

void    _moo_print_operation_set_filename       (MooPrintOperation  *print,
                                                 const char         *filename,
                                                 const char         *basename);
void    _moo_print_operation_set_header_format  (MooPrintOperation  *print,
                                                 const char         *left,
                                                 const char         *center,
                                                 const char         *right);
void    _moo_print_operation_set_footer_format  (MooPrintOperation  *print,
                                                 const char         *left,
                                                 const char         *center,
                                                 const char         *right);

void    _moo_edit_page_setup                    (GtkTextView        *view,
                                                 GtkWidget          *parent);
void    _moo_edit_print                         (GtkTextView        *view,
                                                 GtkWidget          *parent);
void    _moo_edit_print_options_dialog          (GtkWidget          *parent);


G_END_DECLS

#endif /* __MOO_TEXT_PRINT_H__ */
