/*
 *   moohistorycombo.h
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

#ifndef __MOO_HISTORY_COMBO_H__
#define __MOO_HISTORY_COMBO_H__

#include <mooutils/moocombo.h>
#include <mooutils/moohistorylist.h>

G_BEGIN_DECLS


#define MOO_TYPE_HISTORY_COMBO              (moo_history_combo_get_type ())
#define MOO_HISTORY_COMBO(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_HISTORY_COMBO, MooHistoryCombo))
#define MOO_HISTORY_COMBO_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_HISTORY_COMBO, MooHistoryComboClass))
#define MOO_IS_HISTORY_COMBO(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_HISTORY_COMBO))
#define MOO_IS_HISTORY_COMBO_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_HISTORY_COMBO))
#define MOO_HISTORY_COMBO_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_HISTORY_COMBO, MooHistoryComboClass))

typedef struct _MooHistoryCombo         MooHistoryCombo;
typedef struct _MooHistoryComboPrivate  MooHistoryComboPrivate;
typedef struct _MooHistoryComboClass    MooHistoryComboClass;

struct _MooHistoryCombo
{
    MooCombo parent;
    MooHistoryComboPrivate *priv;
};

struct _MooHistoryComboClass
{
    MooComboClass parent_class;
};

typedef gboolean (*MooHistoryComboFilterFunc)   (const char    *text,
                                                 GtkTreeModel  *model,
                                                 GtkTreeIter   *iter,
                                                 gpointer       data);


GType           moo_history_combo_get_type  (void) G_GNUC_CONST;

GtkWidget      *moo_history_combo_new       (const char         *user_id);

void            moo_history_combo_set_list  (MooHistoryCombo    *combo,
                                             MooHistoryList     *list);
MooHistoryList *moo_history_combo_get_list  (MooHistoryCombo    *combo);

void            moo_history_combo_add_text  (MooHistoryCombo    *combo,
                                             const char         *text);
void            moo_history_combo_commit    (MooHistoryCombo    *combo);


G_END_DECLS

#endif /* __MOO_HISTORY_COMBO_H__ */
