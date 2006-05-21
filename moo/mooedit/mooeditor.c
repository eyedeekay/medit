/*
 *   mooeditor.c
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

#define MOOEDIT_COMPILATION
#include "mooedit/mooeditor.h"
#include "mooedit/mooeditdialogs.h"
#include "mooedit/mooeditfileops.h"
#include "mooedit/mooplugin.h"
#include "mooedit/mooeditprefs.h"
#include "mooedit/mooedit-private.h"
#include "mooedit/moolang-private.h"
#include "mooutils/moomenuaction.h"
#include "mooutils/moocompat.h"
#include "mooutils/moomarshals.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooutils-gobject.h"
#include "mooutils/moofilewatch.h"
#include <string.h>


#define RECENT_ACTION_ID "OpenRecent"

static MooEditor *editor_instance = NULL;


typedef struct {
    MooEditWindow *window;
    GSList        *docs;
} WindowInfo;

static WindowInfo   *window_info_new        (MooEditWindow  *win);
static void          window_info_free       (WindowInfo     *win);
static void          window_info_add        (WindowInfo     *win,
                                             MooEdit        *doc);
static void          window_info_remove     (WindowInfo     *win,
                                             MooEdit        *doc);
static MooEdit      *window_info_find       (WindowInfo     *win,
                                             const char     *filename);

static void          window_list_free       (MooEditor      *editor);
static void          window_list_delete     (MooEditor      *editor,
                                             WindowInfo     *win);
static WindowInfo   *window_list_add        (MooEditor      *editor,
                                             MooEditWindow  *win);
static WindowInfo   *window_list_find       (MooEditor      *editor,
                                             MooEditWindow  *win);
static WindowInfo   *window_list_find_doc   (MooEditor      *editor,
                                             MooEdit        *edit);
static WindowInfo   *window_list_find_filename (MooEditor   *editor,
                                             const char     *filename,
                                             MooEdit       **edit);

static void          set_single_window      (MooEditor      *editor,
                                             gboolean        single);

static GtkAction    *create_recent_action   (MooEditWindow  *window);

static MooEditWindow *create_window         (MooEditor      *editor);
static void          moo_editor_add_doc     (MooEditor      *editor,
                                             MooEditWindow  *window,
                                             MooEdit        *doc,
                                             MooEditLoader  *loader,
                                             MooEditSaver   *saver);
static void          do_close_window        (MooEditor      *editor,
                                             MooEditWindow  *window);
static void          do_close_doc           (MooEditor      *editor,
                                             MooEdit        *doc);
static gboolean      close_docs_real        (MooEditor      *editor,
                                             GSList         *docs,
                                             gboolean        ask_confirm);
static GSList       *find_modified          (GSList         *docs);
static MooEditLoader*get_loader             (MooEditor      *editor,
                                             MooEdit        *doc);
static MooEditSaver *get_saver              (MooEditor      *editor,
                                             MooEdit        *doc);

static void          activate_history_item  (MooEditor      *editor,
                                             MooHistoryListItem *item,
                                             MooEditWindow  *window);

static void          prefs_changed          (const char     *key,
                                             const GValue   *newval,
                                             MooEditor      *editor);
static gboolean      apply_prefs            (MooEditor      *editor);


struct _MooEditorPrivate {
    WindowInfo      *windowless;
    GSList          *windows; /* WindowInfo* */
    GHashTable      *loaders;
    GHashTable      *savers;
    char            *app_name;
    MooUIXML        *ui_xml;
    MooFilterMgr    *filter_mgr;
    MooHistoryList  *history;
    MooLangMgr      *lang_mgr;
    MooFileWatch    *file_watch;
    gboolean         open_single;
    gboolean         allow_empty_window;
    gboolean         single_window;

    gboolean         save_backups;
    gboolean         strip_whitespace;
    gboolean         silent;

    GType            window_type;
    GType            doc_type;

    char            *default_lang;

    guint            prefs_notify;
    guint            prefs_idle;

    gboolean         autosave;
    guint            autosave_interval;
};


static void     moo_editor_finalize     (GObject        *object);
static void     moo_editor_set_property (GObject        *object,
                                         guint           prop_id,
                                         const GValue   *value,
                                         GParamSpec     *pspec);
static void     moo_editor_get_property (GObject        *object,
                                         guint           prop_id,
                                         GValue         *value,
                                         GParamSpec     *pspec);


enum {
    PROP_0,
    PROP_OPEN_SINGLE_FILE_INSTANCE,
    PROP_ALLOW_EMPTY_WINDOW,
    PROP_SINGLE_WINDOW,
    PROP_SAVE_BACKUPS,
    PROP_STRIP_WHITESPACE,
    PROP_SILENT,
    PROP_AUTOSAVE,
    PROP_AUTOSAVE_INTERVAL
};

enum {
    ALL_WINDOWS_CLOSED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];


/* MOO_TYPE_EDITOR */
G_DEFINE_TYPE (MooEditor, moo_editor, G_TYPE_OBJECT)


static void moo_editor_class_init (MooEditorClass *klass)
{
    gpointer ref_class;
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    MooWindowClass *edit_window_class;

    gobject_class->finalize = moo_editor_finalize;
    gobject_class->set_property = moo_editor_set_property;
    gobject_class->get_property = moo_editor_get_property;

    _moo_edit_init_settings ();
    ref_class = g_type_class_ref (MOO_TYPE_EDIT);
    g_type_class_unref (ref_class);
    ref_class = g_type_class_ref (MOO_TYPE_EDIT_WINDOW);
    g_type_class_unref (ref_class);

    g_object_class_install_property (gobject_class,
                                     PROP_OPEN_SINGLE_FILE_INSTANCE,
                                     g_param_spec_boolean ("open-single-file-instance",
                                             "open-single-file-instance",
                                             "open-single-file-instance",
                                             TRUE,
                                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    g_object_class_install_property (gobject_class,
                                     PROP_ALLOW_EMPTY_WINDOW,
                                     g_param_spec_boolean ("allow-empty-window",
                                             "allow-empty-window",
                                             "allow-empty-window",
                                             FALSE,
                                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    g_object_class_install_property (gobject_class,
                                     PROP_SINGLE_WINDOW,
                                     g_param_spec_boolean ("single-window",
                                             "single-window",
                                             "single-window",
                                             FALSE,
                                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    g_object_class_install_property (gobject_class,
                                     PROP_SAVE_BACKUPS,
                                     g_param_spec_boolean ("save-backups",
                                             "save-backups",
                                             "save-backups",
                                             FALSE,
                                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    g_object_class_install_property (gobject_class,
                                     PROP_STRIP_WHITESPACE,
                                     g_param_spec_boolean ("strip-whitespace",
                                             "strip-whitespace",
                                             "strip-whitespace",
                                             FALSE,
                                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    g_object_class_install_property (gobject_class,
                                     PROP_SILENT,
                                     g_param_spec_boolean ("silent",
                                             "silent",
                                             "silent",
                                             FALSE,
                                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    g_object_class_install_property (gobject_class,
                                     PROP_AUTOSAVE,
                                     g_param_spec_boolean ("autosave",
                                             "autosave",
                                             "autosave",
                                             FALSE,
                                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    g_object_class_install_property (gobject_class,
                                     PROP_AUTOSAVE_INTERVAL,
                                     g_param_spec_uint ("autosave-interval",
                                             "autosave-interval",
                                             "autosave-interval",
                                             1, 1000, 5,
                                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    signals[ALL_WINDOWS_CLOSED] =
            moo_signal_new_cb ("all-windows-closed",
                               G_OBJECT_CLASS_TYPE (klass),
                               G_SIGNAL_RUN_LAST,
                               NULL, NULL, NULL,
                               _moo_marshal_VOID__VOID,
                               G_TYPE_NONE, 0);

    edit_window_class = g_type_class_ref (MOO_TYPE_EDIT_WINDOW);
    moo_window_class_new_action_custom (edit_window_class, RECENT_ACTION_ID,
                                        (MooWindowActionFunc) create_recent_action,
                                        NULL, NULL);
    g_type_class_unref (edit_window_class);
}


static void
moo_editor_init (MooEditor *editor)
{
    editor->priv = g_new0 (MooEditorPrivate, 1);

    editor->priv->lang_mgr = moo_lang_mgr_new ();
    editor->priv->filter_mgr = moo_filter_mgr_new ();

    editor->priv->history = moo_history_list_new ("Editor");
    moo_history_list_set_display_func (editor->priv->history,
                                       moo_history_list_display_basename,
                                       NULL);
    g_signal_connect_swapped (editor->priv->history, "activate-item",
                              G_CALLBACK (activate_history_item), editor);

    editor->priv->windows = NULL;
    editor->priv->windowless = window_info_new (NULL);

    editor->priv->loaders =
            g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                   NULL, (GDestroyNotify) _moo_edit_loader_unref);
    editor->priv->savers =
            g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                   NULL, (GDestroyNotify) _moo_edit_saver_unref);

    moo_prefs_new_key_string (moo_edit_setting (MOO_EDIT_PREFS_DEFAULT_LANG),
                              MOO_LANG_NONE);

    editor->priv->prefs_notify =
            moo_prefs_notify_connect (MOO_EDIT_PREFS_PREFIX "/[^/]*",
                                      MOO_PREFS_MATCH_REGEX,
                                      (MooPrefsNotify) prefs_changed,
                                      editor, NULL);

    apply_prefs (editor);
}


static void
moo_editor_set_property (GObject        *object,
                         guint           prop_id,
                         const GValue   *value,
                         GParamSpec     *pspec)
{
    MooEditor *editor = MOO_EDITOR (object);

    switch (prop_id) {
        case PROP_OPEN_SINGLE_FILE_INSTANCE:
            editor->priv->open_single = g_value_get_boolean (value);
            g_object_notify (object, "open-single-file-instance");
            break;

        case PROP_ALLOW_EMPTY_WINDOW:
            editor->priv->allow_empty_window = g_value_get_boolean (value);
            g_object_notify (object, "allow-empty-window");
            break;

        case PROP_SINGLE_WINDOW:
            set_single_window (editor, g_value_get_boolean (value));
            break;

        case PROP_SAVE_BACKUPS:
            editor->priv->save_backups = g_value_get_boolean (value);
            g_object_notify (object, "save-backups");
            break;

        case PROP_STRIP_WHITESPACE:
            editor->priv->strip_whitespace = g_value_get_boolean (value);
            g_object_notify (object, "strip-whitespace");
            break;

        case PROP_SILENT:
            editor->priv->silent = g_value_get_boolean (value);
            g_object_notify (object, "silent");
            break;

        case PROP_AUTOSAVE:
            editor->priv->autosave = g_value_get_boolean (value);
            g_object_notify (object, "autosave");
            do {
                static int c = 0;
                if (!c++)
                    g_message ("implement Editor::autosave");
            } while (0);
            break;

        case PROP_AUTOSAVE_INTERVAL:
            editor->priv->autosave_interval = g_value_get_uint (value);
            g_object_notify (object, "autosave-interval");
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void
moo_editor_get_property (GObject        *object,
                         guint           prop_id,
                         GValue         *value,
                         GParamSpec     *pspec)
{
    MooEditor *editor = MOO_EDITOR (object);

    switch (prop_id) {
        case PROP_OPEN_SINGLE_FILE_INSTANCE:
            g_value_set_boolean (value, editor->priv->open_single);
            break;

        case PROP_ALLOW_EMPTY_WINDOW:
            g_value_set_boolean (value, editor->priv->allow_empty_window);
            break;

        case PROP_SINGLE_WINDOW:
            g_value_set_boolean (value, editor->priv->single_window);
            break;

        case PROP_SAVE_BACKUPS:
            g_value_set_boolean (value, editor->priv->save_backups);
            break;

        case PROP_STRIP_WHITESPACE:
            g_value_set_boolean (value, editor->priv->strip_whitespace);
            break;

        case PROP_SILENT:
            g_value_set_boolean (value, editor->priv->silent);
            break;

        case PROP_AUTOSAVE:
            g_value_set_boolean (value, editor->priv->autosave);
            break;

        case PROP_AUTOSAVE_INTERVAL:
            g_value_set_uint (value, editor->priv->autosave_interval);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void
moo_editor_finalize (GObject *object)
{
    MooEditor *editor = MOO_EDITOR (object);

    g_free (editor->priv->app_name);

    if (editor->priv->ui_xml)
        g_object_unref (editor->priv->ui_xml);
    if (editor->priv->filter_mgr)
        g_object_unref (editor->priv->filter_mgr);
    if (editor->priv->history)
        g_object_unref (editor->priv->history);
    g_object_unref (editor->priv->lang_mgr);

    if (editor->priv->prefs_idle)
        g_source_remove (editor->priv->prefs_idle);
    moo_prefs_notify_disconnect (editor->priv->prefs_notify);

    if (editor->priv->file_watch)
    {
        GError *error = NULL;
        if (!moo_file_watch_close (editor->priv->file_watch, &error))
        {
            g_warning ("%s: error in moo_file_watch_close", G_STRLOC);
            if (error)
            {
                g_warning ("%s: %s", G_STRLOC, error->message);
                g_error_free (error);
            }
        }
        g_object_unref (editor->priv->file_watch);
    }

    if (editor->priv->windows)
    {
        g_critical ("finalizing editor while some windows are open");
        window_list_free (editor);
    }

    window_info_free (editor->priv->windowless);

    g_hash_table_destroy (editor->priv->loaders);
    g_hash_table_destroy (editor->priv->savers);

    g_free (editor->priv->default_lang);

    g_free (editor->priv);

    G_OBJECT_CLASS (moo_editor_parent_class)->finalize (object);
}


gpointer
_moo_editor_get_file_watch (MooEditor      *editor)
{
    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);

    if (!editor->priv->file_watch)
        editor->priv->file_watch = moo_file_watch_new (NULL);

    return editor->priv->file_watch;
}


MooEditor*
moo_editor_instance (void)
{
    if (!editor_instance)
    {
        editor_instance = g_object_new (MOO_TYPE_EDITOR, NULL);
        g_object_add_weak_pointer (G_OBJECT (editor_instance),
                                   (gpointer*) &editor_instance);
    }

    return editor_instance;
}


static GType
get_window_type (MooEditor *editor)
{
    return editor->priv->window_type ?
            editor->priv->window_type : MOO_TYPE_EDIT_WINDOW;
}


static GType
get_doc_type (MooEditor *editor)
{
    return editor->priv->doc_type ?
            editor->priv->doc_type : MOO_TYPE_EDIT;
}


static void
set_single_window (MooEditor      *editor,
                   gboolean        single)
{
    /* XXX */
    editor->priv->single_window = single;
    g_object_notify (G_OBJECT (editor), "single-window");
}


static MooEditWindow*
get_top_window (MooEditor *editor)
{
    GSList *list = NULL, *l;
    GtkWindow *window;

    if (!editor->priv->windows)
        return NULL;

    for (l = editor->priv->windows; l != NULL; l = l->next)
    {
        WindowInfo *info = l->data;
        list = g_slist_prepend (list, info->window);
    }

    list = g_slist_reverse (list);
    window = moo_get_top_window (list);

    g_slist_free (list);
    return MOO_EDIT_WINDOW (window);
}


static void file_info_list_free (GSList *list)
{
    g_slist_foreach (list, (GFunc) moo_edit_file_info_free, NULL);
    g_slist_free (list);
}


void
moo_editor_set_app_name (MooEditor      *editor,
                         const char     *name)
{
    GSList *l;

    g_return_if_fail (MOO_IS_EDITOR (editor));

    g_free (editor->priv->app_name);
    editor->priv->app_name = g_strdup (name);

    for (l = editor->priv->windows; l != NULL; l = l->next)
        moo_edit_window_set_title_prefix (MOO_EDIT_WINDOW (l->data), name);
}


MooFilterMgr*
moo_editor_get_filter_mgr (MooEditor      *editor)
{
    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);
    return editor->priv->filter_mgr;
}


MooHistoryList*
moo_editor_get_history (MooEditor      *editor)
{
    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);
    return editor->priv->history;
}


MooUIXML*
moo_editor_get_ui_xml (MooEditor      *editor)
{
    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);
    return editor->priv->ui_xml;
}


void
moo_editor_set_ui_xml (MooEditor      *editor,
                       MooUIXML       *xml)
{
    GSList *l;

    g_return_if_fail (MOO_IS_EDITOR (editor));

    if (editor->priv->ui_xml == xml)
        return;

    if (editor->priv->ui_xml)
        g_object_unref (editor->priv->ui_xml);

    editor->priv->ui_xml = xml;

    if (editor->priv->ui_xml)
        g_object_ref (editor->priv->ui_xml);

    for (l = editor->priv->windows; l != NULL; l = l->next)
        moo_window_set_ui_xml (l->data, editor->priv->ui_xml);
}


static WindowInfo*
window_info_new (MooEditWindow  *win)
{
    WindowInfo *w = g_new0 (WindowInfo, 1);
    w->window = win;
    return w;
}

static void
window_info_free (WindowInfo     *win)
{
    if (win)
    {
        g_slist_free (win->docs);
        g_free (win);
    }
}

static void
window_info_add (WindowInfo     *win,
                 MooEdit        *edit)
{
    g_return_if_fail (!g_slist_find (win->docs, edit));
    win->docs = g_slist_append (win->docs, edit);
}

static void
window_info_remove (WindowInfo     *win,
                    MooEdit        *edit)
{
    g_return_if_fail (g_slist_find (win->docs, edit));
    win->docs = g_slist_remove (win->docs, edit);
}


static int
edit_and_file_cmp (MooEdit *edit, const char *filename)
{
    const char *edit_filename;
    g_return_val_if_fail (MOO_IS_EDIT (edit) && filename != NULL, TRUE);
    edit_filename = moo_edit_get_filename (edit);
    if (edit_filename)
        return strcmp (edit_filename, filename);
    else
        return TRUE;
}

static MooEdit*
window_info_find (WindowInfo     *win,
                  const char     *filename)
{
    GSList *l;
    g_return_val_if_fail (win != NULL && filename != NULL, NULL);
    l = g_slist_find_custom (win->docs, filename,
                             (GCompareFunc) edit_and_file_cmp);
    return l ? l->data : NULL;
}


static void
window_list_free (MooEditor      *editor)
{
    g_slist_foreach (editor->priv->windows,
                     (GFunc) window_info_free, NULL);
    g_slist_free (editor->priv->windows);
    editor->priv->windows = NULL;
}

static void
window_list_delete (MooEditor      *editor,
                    WindowInfo     *win)
{
    g_return_if_fail (g_slist_find (editor->priv->windows, win));
    window_info_free (win);
    editor->priv->windows =
            g_slist_remove (editor->priv->windows, win);
}

static WindowInfo*
window_list_add (MooEditor      *editor,
                 MooEditWindow  *win)
{
    WindowInfo *w = window_info_new (win);
    editor->priv->windows =
            g_slist_prepend (editor->priv->windows, w);
    return w;
}

static int
window_cmp (WindowInfo *w, MooEditWindow *e)
{
    g_return_val_if_fail (w != NULL, TRUE);
    return !(w->window == e);
}

static WindowInfo*
window_list_find (MooEditor      *editor,
                  MooEditWindow  *win)
{
    GSList *l = g_slist_find_custom (editor->priv->windows, win,
                                     (GCompareFunc) window_cmp);
    if (l)
        return l->data;
    else
        return NULL;
}

static int
doc_cmp (WindowInfo *w, MooEdit *e)
{
    g_return_val_if_fail (w != NULL, 1);
    return !g_slist_find (w->docs, e);
}

static WindowInfo*
window_list_find_doc (MooEditor      *editor,
                      MooEdit        *edit)
{
    GSList *l = g_slist_find_custom (editor->priv->windows, edit,
                                     (GCompareFunc) doc_cmp);

    if (l)
        return l->data;

    if (g_slist_find (editor->priv->windowless->docs, edit))
        return editor->priv->windowless;

    return NULL;
}


static int
filename_and_doc_cmp (WindowInfo *w, gpointer user_data)
{
    struct {
        const char *filename;
        MooEdit    *edit;
    } *data = user_data;

    g_return_val_if_fail (w != NULL, TRUE);

    data->edit = window_info_find (w, data->filename);
    return data->edit == NULL;
}

static WindowInfo*
window_list_find_filename (MooEditor   *editor,
                           const char     *filename,
                           MooEdit       **edit)
{
    struct {
        const char *filename;
        MooEdit    *edit;
    } data = {filename, NULL};

    GSList *l = g_slist_find_custom (editor->priv->windows, &data,
                                     (GCompareFunc) filename_and_doc_cmp);
    if (l)
    {
        g_assert (data.edit != NULL);
        if (edit) *edit = data.edit;
        return l->data;
    }

    if (window_info_find (editor->priv->windowless, filename))
        return editor->priv->windowless;

    return NULL;
}


static GtkAction*
create_recent_action (MooEditWindow  *window)
{
    MooEditor *editor = moo_edit_window_get_editor (window);
    GtkAction *action;
    MooMenuMgr *mgr;

    g_return_val_if_fail (editor != NULL, NULL);

    action = moo_menu_action_new (RECENT_ACTION_ID, "Open Recent");
    mgr = moo_history_list_get_menu_mgr (editor->priv->history);
    moo_menu_action_set_mgr (MOO_MENU_ACTION (action), mgr);
    moo_menu_action_set_menu_data (MOO_MENU_ACTION (action), window, TRUE);

    moo_bind_bool_property (action, "sensitive",
                            editor->priv->history, "empty", TRUE);

    return action;
}


static void
activate_history_item (MooEditor           *editor,
                       MooHistoryListItem  *item,
                       MooEditWindow       *window)
{
    WindowInfo *win_info;

    g_return_if_fail (MOO_IS_EDITOR (editor));
    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (item != NULL && item->data != NULL);

    win_info = window_list_find (editor, window);
    g_return_if_fail (win_info != NULL);

    if (!moo_editor_open_file (editor, window, GTK_WIDGET (window), item->data, NULL))
        moo_history_list_remove (editor->priv->history, item->data);
}


/*****************************************************************************/

static MooEditWindow*
create_window (MooEditor *editor)
{
    MooEditWindow *window = g_object_new (get_window_type (editor),
                                          "editor", editor,
                                          "ui-xml",
                                          moo_editor_get_ui_xml (editor),
                                          NULL);
    moo_edit_window_set_title_prefix (window, editor->priv->app_name);
    window_list_add (editor, window);
    _moo_window_attach_plugins (window);
    gtk_widget_show (GTK_WIDGET (window));
    return window;
}


static void
moo_editor_add_doc (MooEditor      *editor,
                    MooEditWindow  *window,
                    MooEdit        *doc,
                    MooEditLoader  *loader,
                    MooEditSaver   *saver)
{
    WindowInfo *info;

    if (window)
    {
        info = window_list_find (editor, window);
        g_return_if_fail (info != NULL);
    }
    else
    {
        info = editor->priv->windowless;
    }

    g_return_if_fail (g_slist_find (info->docs, doc) == NULL);

    window_info_add (info, doc);

    g_hash_table_insert (editor->priv->loaders, doc, _moo_edit_loader_ref (loader));
    g_hash_table_insert (editor->priv->savers, doc, _moo_edit_saver_ref (saver));

    if (!moo_edit_get_filename (doc) &&
         !moo_edit_config_get_string (doc->config, "lang") &&
         editor->priv->default_lang)
    {
        moo_edit_config_set (doc->config,
                             "lang", MOO_EDIT_CONFIG_SOURCE_FILENAME,
                             editor->priv->default_lang, NULL);
    }

    _moo_edit_apply_settings (doc);
}


MooEditWindow*
moo_editor_new_window (MooEditor *editor)
{
    MooEditWindow *window;
    MooEdit *doc;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);

    window = create_window (editor);

    if (!editor->priv->allow_empty_window)
    {
        doc = g_object_new (get_doc_type (editor), "editor", editor, NULL);
        _moo_edit_window_insert_doc (window, doc, -1);
        moo_editor_add_doc (editor, window, doc,
                            _moo_edit_loader_get_default (),
                            _moo_edit_saver_get_default ());
    }

    return window;
}


/* this creates MooEdit instance which can not be put into a window */
MooEdit*
moo_editor_create_doc (MooEditor      *editor,
                       const char     *filename,
                       const char     *encoding,
                       GError        **error)
{
    MooEdit *doc;
    MooEditLoader *loader;
    MooEditSaver *saver;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);

    doc = g_object_new (get_doc_type (editor), "editor", editor, NULL);
    loader = _moo_edit_loader_get_default ();
    saver = _moo_edit_saver_get_default ();

    if (filename && !_moo_edit_loader_load (loader, doc, filename, encoding, error))
    {
        gtk_object_sink (g_object_ref (doc));
        g_object_unref (doc);
        return NULL;
    }

    moo_editor_add_doc (editor, NULL, doc, loader, saver);
    _moo_doc_attach_plugins (NULL, doc);

    return doc;
}


MooEdit*
moo_editor_new_doc (MooEditor      *editor,
                    MooEditWindow  *window)
{
    MooEdit *doc;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);
    g_return_val_if_fail (!window || MOO_IS_EDIT_WINDOW (window), NULL);
    g_return_val_if_fail (!window || window_list_find (editor, window) != NULL, NULL);

    if (!window)
        window = get_top_window (editor);

    if (!window)
        window = moo_editor_new_window (editor);

    g_return_val_if_fail (window != NULL, NULL);

    doc = g_object_new (get_doc_type (editor), "editor", editor, NULL);
    _moo_edit_window_insert_doc (window, doc, -1);
    moo_editor_add_doc (editor, window, doc,
                        _moo_edit_loader_get_default (),
                        _moo_edit_saver_get_default ());

    return doc;
}


static char *
filename_make_absolute (const char *name)
{
    char *abs_name, *dir;

    if (g_path_is_absolute (name))
        return g_strdup (name);

    /* XXX normalize it */
    dir = g_get_current_dir ();
    abs_name = g_build_filename (dir, name, NULL);

    g_free (dir);
    return abs_name;
}


gboolean
moo_editor_open (MooEditor      *editor,
                 MooEditWindow  *window,
                 GtkWidget      *parent,
                 GSList         *files)
{
    GSList *l;
    MooEditLoader *loader;
    MooEditSaver *saver;
    MooEdit *bring_to_front = NULL;
    gboolean result = TRUE;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), FALSE);
    g_return_val_if_fail (!window || MOO_IS_EDIT_WINDOW (window), FALSE);
    g_return_val_if_fail (!parent || GTK_IS_WIDGET (parent), FALSE);
    g_return_val_if_fail (!window || window_list_find (editor, window) != NULL, FALSE);

    if (window && !parent)
        parent = GTK_WIDGET (window);

    if (!files)
    {
        files = moo_edit_open_dialog (parent, editor->priv->filter_mgr);

        if (!files)
            return FALSE;

        result = moo_editor_open (editor, window, parent, files);
        file_info_list_free (files);

        return result;
    }

    loader = _moo_edit_loader_get_default ();
    saver = _moo_edit_saver_get_default ();

    for (l = files; l != NULL; l = l->next)
    {
        MooEditFileInfo *info = l->data;
        GError *error = NULL;
        gboolean new_doc = FALSE;
        MooEdit *doc = NULL;
        char *filename;

        filename = filename_make_absolute (info->filename);

        if (window_list_find_filename (editor, filename, &bring_to_front))
        {
            moo_history_list_add_filename (editor->priv->history, filename);
            g_free (filename);
            continue;
        }
        else
        {
            bring_to_front = NULL;
        }

        if (window)
        {
            doc = moo_edit_window_get_active_doc (window);
            if (doc && moo_edit_is_empty (doc))
                g_object_ref (doc);
            else
                doc = NULL;
        }

        if (!doc)
        {
            doc = g_object_new (get_doc_type (editor), "editor", editor, NULL);
            gtk_object_sink (g_object_ref (doc));
            new_doc = TRUE;
        }

        /* XXX open_single */
        if (!_moo_edit_loader_load (loader, doc, filename, info->encoding, &error))
        {
            moo_edit_open_error_dialog (parent, filename,
                                        error ? error->message : NULL);
            if (error)
                g_error_free (error);
            result = FALSE;
        }
        else
        {
            if (!window)
                window = moo_editor_get_active_window (editor);
            if (!window)
                window = create_window (editor);

            if (new_doc)
            {
                _moo_edit_window_insert_doc (window, doc, -1);
                moo_editor_add_doc (editor, window, doc, loader, saver);
            }
            else
            {
                bring_to_front = doc;
            }

            moo_history_list_add_filename (editor->priv->history, filename);

            parent = GTK_WIDGET (window);
        }

        g_free (filename);
        g_object_unref (doc);
    }

    if (bring_to_front)
    {
        moo_editor_set_active_doc (editor, bring_to_front);
        gtk_widget_grab_focus (GTK_WIDGET (bring_to_front));
    }

    return result;
}


MooEdit*
moo_editor_get_active_doc (MooEditor *editor)
{
    MooEditWindow *window = moo_editor_get_active_window (editor);
    return window ? moo_edit_window_get_active_doc (window) : NULL;
}


MooEditWindow*
moo_editor_get_active_window (MooEditor *editor)
{
    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);
    return get_top_window (editor);
}


void
moo_editor_present (MooEditor *editor)
{
    MooEditWindow *window;

    g_return_if_fail (MOO_IS_EDITOR (editor));

    window = moo_editor_get_active_window (editor);

    if (!window)
        window = moo_editor_new_window (editor);

    g_return_if_fail (window != NULL);
    moo_window_present (GTK_WINDOW (window));
}


void
moo_editor_set_active_window (MooEditor    *editor,
                              MooEditWindow  *window)
{
    WindowInfo *info;

    g_return_if_fail (MOO_IS_EDITOR (editor));
    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));

    info = window_list_find (editor, window);
    g_return_if_fail (info != NULL);

    gtk_window_present (GTK_WINDOW (info->window));
}


void
moo_editor_set_active_doc (MooEditor      *editor,
                           MooEdit        *doc)
{
    WindowInfo *info;

    g_return_if_fail (MOO_IS_EDITOR (editor));
    g_return_if_fail (MOO_IS_EDIT (doc));

    info = window_list_find_doc (editor, doc);
    g_return_if_fail (info != NULL);
    g_return_if_fail (info->window != NULL);

    moo_window_present (GTK_WINDOW (info->window));
    moo_edit_window_set_active_doc (info->window, doc);
}


gboolean
moo_editor_close_window (MooEditor      *editor,
                         MooEditWindow  *window,
                         gboolean        ask_confirm)
{
    WindowInfo *info;
    MooEditDialogResponse response;
    GSList *modified;
    gboolean do_close = FALSE;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), FALSE);
    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), FALSE);

    info = window_list_find (editor, window);
    g_return_val_if_fail (info != NULL, FALSE);

    modified = find_modified (info->docs);

    if (!modified || !ask_confirm)
    {
        do_close = TRUE;
    }
    else if (!modified->next)
    {
        if (info->window)
            moo_edit_window_set_active_doc (info->window, modified->data);

        response = moo_edit_save_changes_dialog (modified->data);

        switch (response)
        {
            case MOO_EDIT_RESPONSE_SAVE:
                if (_moo_editor_save (editor, modified->data, NULL))
                    do_close = TRUE;
                break;

            case MOO_EDIT_RESPONSE_CANCEL:
                break;

            default:
                do_close = TRUE;
                break;
        }
    }
    else
    {
        GSList *to_save = NULL, *l;
        gboolean saved = TRUE;

        response = moo_edit_save_multiple_changes_dialog (modified, &to_save);

        switch (response)
        {
            case MOO_EDIT_RESPONSE_SAVE:
                for (l = to_save; l != NULL; l = l->next)
                    if (!_moo_editor_save (editor, l->data, NULL))
                    {
                        saved = FALSE;
                        break;
                    }

                if (saved)
                    do_close = TRUE;

                g_slist_free (to_save);
                break;

            case MOO_EDIT_RESPONSE_CANCEL:
                break;

            default:
                do_close = TRUE;
                break;
        }
    }

    if (do_close)
        do_close_window (editor, window);

    g_slist_free (modified);
    return do_close;
}


static void
do_close_window (MooEditor      *editor,
                 MooEditWindow  *window)
{
    WindowInfo *info;
    GSList *l, *list;

    info = window_list_find (editor, window);
    g_return_if_fail (info != NULL);

    list = g_slist_copy (info->docs);

    for (l = list; l != NULL; l = l->next)
        do_close_doc (editor, l->data);

    window_list_delete (editor, info);

    _moo_window_detach_plugins (window);
    gtk_widget_destroy (GTK_WIDGET (window));

    if (!editor->priv->windows)
        g_signal_emit (editor, signals[ALL_WINDOWS_CLOSED], 0);

    g_slist_free (list);
}


static void
do_close_doc (MooEditor      *editor,
              MooEdit        *doc)
{
    WindowInfo *info = window_list_find_doc (editor, doc);
    g_return_if_fail (info != NULL);
    window_info_remove (info, doc);

    if (info->window)
        _moo_edit_window_remove_doc (info->window, doc);
    else
        _moo_doc_detach_plugins (NULL, doc);

    g_hash_table_remove (editor->priv->loaders, doc);
    g_hash_table_remove (editor->priv->savers, doc);
}


gboolean
moo_editor_close_doc (MooEditor      *editor,
                      MooEdit        *doc,
                      gboolean        ask_confirm)
{
    gboolean result;
    GSList *list;

    list = g_slist_prepend (NULL, doc);
    result = moo_editor_close_docs (editor, list, ask_confirm);

    g_slist_free (list);
    return result;
}


gboolean
moo_editor_close_docs (MooEditor      *editor,
                       GSList         *list,
                       gboolean        ask_confirm)
{
    WindowInfo *info;
    GSList *l;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), FALSE);
    g_return_val_if_fail (list != NULL, FALSE);

    for (l = list; l != NULL; l = l->next)
    {
        MooEdit *doc = l->data;
        g_return_val_if_fail (MOO_IS_EDIT (doc), FALSE);
        g_return_val_if_fail (window_list_find_doc (editor, doc) != NULL, FALSE);
    }

    /* do i care? */
    info = window_list_find_doc (editor, list->data);
    g_return_val_if_fail (info != NULL, FALSE);

    if (close_docs_real (editor, list, ask_confirm))
    {
        if (info->window &&
            !moo_edit_window_num_docs (info->window) &&
             !editor->priv->allow_empty_window)
        {
            MooEdit *doc = g_object_new (get_doc_type (editor),
                                         "editor", editor, NULL);
            _moo_edit_window_insert_doc (info->window, doc, -1);
            moo_editor_add_doc (editor, info->window, doc,
                                _moo_edit_loader_get_default (),
                                _moo_edit_saver_get_default ());
        }

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


static gboolean
close_docs_real (MooEditor      *editor,
                 GSList         *docs,
                 gboolean        ask_confirm)
{
    MooEditDialogResponse response;
    GSList *modified, *l;
    gboolean do_close = FALSE;

    modified = find_modified (docs);

    if (!modified || !ask_confirm)
    {
        do_close = TRUE;
    }
    else if (!modified->next)
    {
        WindowInfo *info = window_list_find_doc (editor, modified->data);
        g_return_val_if_fail (info != NULL, FALSE);

        if (info->window)
            moo_edit_window_set_active_doc (info->window, modified->data);

        response = moo_edit_save_changes_dialog (modified->data);

        switch (response)
        {
            case MOO_EDIT_RESPONSE_SAVE:
                if (_moo_editor_save (editor, modified->data, NULL))
                    do_close = TRUE;
                break;

            case MOO_EDIT_RESPONSE_CANCEL:
                break;

            default:
                do_close = TRUE;
                break;
        }
    }
    else
    {
        GSList *to_save = NULL;
        gboolean saved = TRUE;

        response = moo_edit_save_multiple_changes_dialog (modified, &to_save);

        switch (response)
        {
            case MOO_EDIT_RESPONSE_SAVE:
                for (l = to_save; l != NULL; l = l->next)
                    if (!_moo_editor_save (editor, l->data, NULL))
                {
                    saved = FALSE;
                    break;
                }

                if (saved)
                    do_close = TRUE;

                g_slist_free (to_save);
                break;

            case MOO_EDIT_RESPONSE_CANCEL:
                break;

            default:
                do_close = TRUE;
                break;
        }
    }

    if (do_close)
        for (l = docs; l != NULL; l = l->next)
            do_close_doc (editor, l->data);

    g_slist_free (modified);
    return do_close;
}


static GSList*
find_modified (GSList *docs)
{
    GSList *modified = NULL, *l;
    for (l = docs; l != NULL; l = l->next)
        if (MOO_EDIT_IS_MODIFIED (l->data) && !MOO_EDIT_IS_CLEAN (l->data))
            modified = g_slist_prepend (modified, l->data);
    return g_slist_reverse (modified);
}


gboolean
moo_editor_close_all (MooEditor *editor,
                      gboolean   ask_confirm)
{
    GSList *windows, *l;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), FALSE);

    windows = moo_editor_list_windows (editor);

    for (l = windows; l != NULL; l = l->next)
    {
        if (!moo_editor_close_window (editor, l->data, ask_confirm))
        {
            g_slist_free (windows);
            return FALSE;
        }
    }

    g_slist_free (windows);
    return TRUE;
}


GSList*
moo_editor_list_windows (MooEditor *editor)
{
    GSList *windows = NULL, *l;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);

    for (l = editor->priv->windows; l != NULL; l = l->next)
    {
        WindowInfo *info = l->data;
        windows = g_slist_prepend (windows, info->window);
    }

    return g_slist_reverse (windows);
}


GSList*
moo_editor_list_docs (MooEditor *editor)
{
    GSList *docs = NULL, *l, *list;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);

    for (l = editor->priv->windows; l != NULL; l = l->next)
    {
        WindowInfo *info = l->data;
        list = g_slist_copy (info->docs);
        list = g_slist_reverse (list);
        docs = g_slist_concat (list, docs);
    }

    list = g_slist_copy (editor->priv->windowless->docs);
    list = g_slist_reverse (list);
    docs = g_slist_concat (list, docs);

    return g_slist_reverse (docs);
}


gboolean
moo_editor_open_file (MooEditor      *editor,
                      MooEditWindow  *window,
                      GtkWidget      *parent,
                      const char     *filename,
                      const char     *encoding)
{
    MooEditFileInfo *info;
    GSList *list;
    gboolean result;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), FALSE);
    g_return_val_if_fail (!window || MOO_IS_EDIT_WINDOW (window), FALSE);
    g_return_val_if_fail (!parent || GTK_IS_WIDGET (parent), FALSE);

    if (!filename)
        return moo_editor_open (editor, window, parent, NULL);

    info = moo_edit_file_info_new (filename, encoding);
    list = g_slist_prepend (NULL, info);
    result = moo_editor_open (editor, window, parent, list);

    moo_edit_file_info_free (info);
    g_slist_free (list);
    return result;
}


gboolean
moo_editor_new_file (MooEditor      *editor,
                     MooEditWindow  *window,
                     GtkWidget      *parent,
                     const char     *filename,
                     const char     *encoding)
{
    MooEdit *doc = NULL;
    char *freeme = NULL;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), FALSE);
    g_return_val_if_fail (!window || MOO_IS_EDIT_WINDOW (window), FALSE);
    g_return_val_if_fail (!parent || GTK_IS_WIDGET (parent), FALSE);

    if (!filename)
        return moo_editor_open (editor, window, parent, NULL);

    if (g_file_test (filename, G_FILE_TEST_EXISTS))
        return moo_editor_open_file (editor, window, parent,
                                     filename, encoding);

    freeme = filename_make_absolute (filename);
    filename = freeme;

    if (!window)
        window = moo_editor_get_active_window (editor);

    if (window)
    {
        doc = moo_edit_window_get_active_doc (window);

        if (!doc || !moo_edit_is_empty (doc))
            doc = NULL;
    }

    if (!doc)
        doc = moo_editor_new_doc (editor, window);

    doc->priv->status = MOO_EDIT_NEW;
    _moo_edit_set_filename (doc, filename, encoding);
    moo_editor_set_active_doc (editor, doc);
    gtk_widget_grab_focus (GTK_WIDGET (doc));

    g_free (freeme);
    return TRUE;
}


gboolean
moo_editor_open_uri (MooEditor      *editor,
                     MooEditWindow  *window,
                     GtkWidget      *parent,
                     const char     *uri,
                     const char     *encoding)
{
    MooEditFileInfo *info;
    GSList *list;
    gboolean result;
    char *filename;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), FALSE);
    g_return_val_if_fail (!window || MOO_IS_EDIT_WINDOW (window), FALSE);
    g_return_val_if_fail (!parent || GTK_IS_WIDGET (parent), FALSE);
    g_return_val_if_fail (uri != NULL, FALSE);

    filename = g_filename_from_uri (uri, NULL, NULL);
    g_return_val_if_fail (filename != NULL, FALSE);

    info = moo_edit_file_info_new (filename, encoding);
    list = g_slist_prepend (NULL, info);
    result = moo_editor_open (editor, window, parent, list);

    moo_edit_file_info_free (info);
    g_slist_free (list);
    g_free (filename);
    return result;
}


void
_moo_editor_reload (MooEditor      *editor,
                    MooEdit        *doc,
                    GError        **error)
{
    WindowInfo *info;
    MooEditLoader *loader;
    GError *error_here = NULL;
    int cursor_line, cursor_offset;
    GtkTextIter iter;

    g_return_if_fail (MOO_IS_EDITOR (editor));

    info = window_list_find_doc (editor, doc);
    g_return_if_fail (info != NULL);

    loader = get_loader (editor, doc);
    g_return_if_fail (loader != NULL);

    /* XXX */
    g_return_if_fail (moo_edit_get_filename (doc) != NULL);

    if (!editor->priv->silent &&
         !MOO_EDIT_IS_CLEAN (doc) &&
         MOO_EDIT_IS_MODIFIED (doc) &&
         !moo_edit_reload_modified_dialog (doc))
                return;

    moo_text_view_get_cursor (MOO_TEXT_VIEW (doc), &iter);
    cursor_line = gtk_text_iter_get_line (&iter);
    cursor_offset = moo_text_iter_get_visual_line_offset (&iter, 8);

    if (!_moo_edit_loader_reload (loader, doc, &error_here))
    {
        if (!editor->priv->silent)
            moo_edit_reload_error_dialog (GTK_WIDGET (doc), error_here->message);
        else
            g_propagate_error (error, error_here);

        g_object_set_data (G_OBJECT (doc), "moo-scroll-to", NULL);
        return;
    }

    moo_text_view_move_cursor (MOO_TEXT_VIEW (doc), cursor_line,
                               cursor_offset, TRUE, TRUE);
}


static MooEditSaveFlags
moo_editor_get_save_flags (MooEditor *editor)
{
    MooEditSaveFlags flags = 0;

    if (editor->priv->save_backups)
        flags |= MOO_EDIT_SAVE_BACKUP;

    return flags;
}


static gboolean
do_save (MooEditor    *editor,
         MooEditSaver *saver,
         MooEdit      *doc,
         const char   *filename,
         const char   *encoding,
         GError      **error)
{
    gboolean result;
    gboolean strip;

    strip = moo_edit_config_get_bool (doc->config, "strip");

    if (strip)
        moo_text_view_strip_whitespace (MOO_TEXT_VIEW (doc));

    g_signal_emit_by_name (doc, "save-before");
    result = _moo_edit_saver_save (saver, doc, filename, encoding,
                                   moo_editor_get_save_flags (editor), error);
    g_signal_emit_by_name (doc, "save-after");

    return result;
}


gboolean
_moo_editor_save (MooEditor      *editor,
                  MooEdit        *doc,
                  GError        **error)
{
    WindowInfo *info;
    MooEditSaver *saver;
    GError *error_here = NULL;
    char *filename;
    char *encoding;
    gboolean result = FALSE;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), FALSE);

    info = window_list_find_doc (editor, doc);
    g_return_val_if_fail (info != NULL, FALSE);

    saver = get_saver (editor, doc);
    g_return_val_if_fail (saver != NULL, FALSE);

    if (!moo_edit_get_filename (doc))
        return _moo_editor_save_as (editor, doc, NULL, NULL, error);

    filename = g_strdup (moo_edit_get_filename (doc));
    encoding = g_strdup (moo_edit_get_encoding (doc));

    if (!editor->priv->silent &&
         (moo_edit_get_status (doc) & MOO_EDIT_MODIFIED_ON_DISK) &&
         !moo_edit_overwrite_modified_dialog (doc))
            goto out;

    if (!do_save (editor, saver, doc, filename, encoding, &error_here))
    {
        if (!editor->priv->silent)
        {
            moo_edit_save_error_dialog (GTK_WIDGET (doc), filename,
                                        error_here->message);
            g_error_free (error_here);
        }
        else
        {
            g_propagate_error (error, error_here);
        }

        goto out;
    }

    moo_history_list_add_filename (editor->priv->history, filename);
    result = TRUE;

    /* fall through */
out:
    g_free (filename);
    g_free (encoding);
    return result;
}


gboolean
_moo_editor_save_as (MooEditor      *editor,
                     MooEdit        *doc,
                     const char     *filename,
                     const char     *encoding,
                     GError        **error)
{
    WindowInfo *info;
    MooEditSaver *saver;
    GError *error_here = NULL;
    MooEditFileInfo *file_info = NULL;
    gboolean result = FALSE;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), FALSE);

    info = window_list_find_doc (editor, doc);
    g_return_val_if_fail (info != NULL, FALSE);

    saver = get_saver (editor, doc);
    g_return_val_if_fail (saver != NULL, FALSE);

    if (!filename)
    {
        file_info = moo_edit_save_as_dialog (doc, editor->priv->filter_mgr,
                                             moo_edit_get_display_basename (doc));

        if (!file_info)
            goto out;
    }
    else
    {
        file_info = moo_edit_file_info_new (filename, encoding);
    }

    if (!do_save (editor, saver, doc, file_info->filename, file_info->encoding, &error_here))
    {
        if (!editor->priv->silent)
        {
            moo_edit_save_error_dialog (GTK_WIDGET (doc),
                                        file_info->filename,
                                        error_here->message);
            g_error_free (error_here);
        }
        else
        {
            g_propagate_error (error, error_here);
        }

        goto out;
    }

    moo_history_list_add_filename (editor->priv->history, file_info->filename);
    result = TRUE;

    /* fall through */
out:
    moo_edit_file_info_free (file_info);
    return result;
}


gboolean
moo_editor_save_copy (MooEditor      *editor,
                      MooEdit        *doc,
                      const char     *filename,
                      const char     *encoding,
                      GError        **error)
{
    WindowInfo *info;
    MooEditSaver *saver;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), FALSE);
    g_return_val_if_fail (filename != NULL, FALSE);

    info = window_list_find_doc (editor, doc);
    g_return_val_if_fail (info != NULL, FALSE);

    saver = get_saver (editor, doc);
    g_return_val_if_fail (saver != NULL, FALSE);

    return _moo_edit_saver_save_copy (saver, doc, filename, encoding, error);
}


static MooEditLoader*
get_loader (MooEditor      *editor,
            MooEdit        *doc)
{
    return g_hash_table_lookup (editor->priv->loaders, doc);
}


static MooEditSaver*
get_saver (MooEditor      *editor,
           MooEdit        *doc)
{
    return g_hash_table_lookup (editor->priv->savers, doc);
}


MooEdit*
moo_editor_get_doc (MooEditor      *editor,
                    const char     *filename)
{
    MooEdit *doc = NULL;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);
    g_return_val_if_fail (filename != NULL, NULL);

    window_list_find_filename (editor, filename, &doc);
    return doc;
}


MooLangMgr*
moo_editor_get_lang_mgr (MooEditor *editor)
{
    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);
    return editor->priv->lang_mgr;
}


void
moo_editor_set_window_type (MooEditor      *editor,
                            GType           type)
{
    g_return_if_fail (MOO_IS_EDITOR (editor));
    g_return_if_fail (g_type_is_a (type, MOO_TYPE_EDIT_WINDOW));
    editor->priv->window_type = type;
}


void
moo_editor_set_edit_type (MooEditor      *editor,
                          GType           type)
{
    g_return_if_fail (MOO_IS_EDITOR (editor));
    g_return_if_fail (g_type_is_a (type, MOO_TYPE_EDIT));
    editor->priv->doc_type = type;
}


static void
prefs_changed (G_GNUC_UNUSED const char *key,
               G_GNUC_UNUSED const GValue *newval,
               MooEditor *editor)
{
    if (!editor->priv->prefs_idle)
        editor->priv->prefs_idle =
                g_idle_add ((GSourceFunc) apply_prefs, editor);
}


static void
set_default_lang (MooEditor  *editor,
                  const char *name)
{
    g_return_if_fail (MOO_IS_EDITOR (editor));
    g_free (editor->priv->default_lang);
    editor->priv->default_lang = g_strdup (name);
}


static gboolean
apply_prefs (MooEditor *editor)
{
    GSList *docs;
    gboolean use_tabs, autosave, backups, strip;
    int indent_width, autosave_interval;
    const char *color_scheme, *default_lang;

    editor->priv->prefs_idle = 0;

    default_lang = moo_prefs_get_string (moo_edit_setting (MOO_EDIT_PREFS_DEFAULT_LANG));

    if (default_lang && !strcmp (default_lang, MOO_LANG_NONE))
        default_lang = NULL;

    set_default_lang (editor, default_lang);

    use_tabs = !moo_prefs_get_bool (moo_edit_setting (MOO_EDIT_PREFS_SPACES_NO_TABS));
    indent_width = moo_prefs_get_int (moo_edit_setting (MOO_EDIT_PREFS_INDENT_WIDTH));
    strip = moo_prefs_get_bool (moo_edit_setting (MOO_EDIT_PREFS_STRIP));
    autosave = moo_prefs_get_bool (moo_edit_setting (MOO_EDIT_PREFS_AUTO_SAVE));
    autosave_interval = moo_prefs_get_int (moo_edit_setting (MOO_EDIT_PREFS_AUTO_SAVE_INTERVAL));
    backups = moo_prefs_get_bool (moo_edit_setting (MOO_EDIT_PREFS_MAKE_BACKUPS));
    color_scheme = moo_prefs_get_string (moo_edit_setting (MOO_EDIT_PREFS_COLOR_SCHEME));

    moo_edit_config_set_global ("indent-use-tabs", MOO_EDIT_CONFIG_SOURCE_PREFS, use_tabs,
                                "indent-width", MOO_EDIT_CONFIG_SOURCE_PREFS, indent_width,
                                "strip", MOO_EDIT_CONFIG_SOURCE_PREFS, strip,
                                NULL);

    if (color_scheme)
        _moo_lang_mgr_set_active_scheme (editor->priv->lang_mgr, color_scheme);

    docs = moo_editor_list_docs (editor);
    g_slist_foreach (docs, (GFunc) _moo_edit_apply_settings, NULL);
    g_slist_free (docs);

    g_object_set (editor,
                  "autosave", autosave,
                  "autosave-interval", autosave_interval,
                  "save-backups", backups,
                  NULL);

    return FALSE;
}
