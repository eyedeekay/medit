/*
 *   moomenuaction.h
 *
 *   Copyright (C) 2004-2007 by Yevgen Muntyan <muntyan@math.tamu.edu>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   See COPYING file that comes with this distribution.
 */

#ifndef __MOO_MENU_ACTION_H__
#define __MOO_MENU_ACTION_H__

#include <mooutils/mooaction.h>
#include <mooutils/moomenumgr.h>

G_BEGIN_DECLS


#define MOO_TYPE_MENU_ACTION              (moo_menu_action_get_type ())
#define MOO_MENU_ACTION(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_MENU_ACTION, MooMenuAction))
#define MOO_MENU_ACTION_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_MENU_ACTION, MooMenuActionClass))
#define MOO_IS_MENU_ACTION(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_MENU_ACTION))
#define MOO_IS_MENU_ACTION_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_MENU_ACTION))
#define MOO_MENU_ACTION_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_MENU_ACTION, MooMenuActionClass))


typedef struct _MooMenuAction        MooMenuAction;
typedef struct _MooMenuActionClass   MooMenuActionClass;

typedef GtkWidget *(*MooMenuFunc) (GtkAction *action);

struct _MooMenuAction
{
    MooAction       base;
    MooMenuFunc     func;
    MooMenuMgr     *mgr;
    gpointer        data;
    guint           is_object : 1;
};

struct _MooMenuActionClass
{
    MooActionClass base_class;
};


GType       moo_menu_action_get_type        (void) G_GNUC_CONST;

GtkAction  *moo_menu_action_new             (const char     *id,
                                             const char     *label);

MooMenuMgr *moo_menu_action_get_mgr         (MooMenuAction  *action);
void        moo_menu_action_set_mgr         (MooMenuAction  *action,
                                             MooMenuMgr     *mgr);
void        moo_menu_action_set_func        (MooMenuAction  *action,
                                             MooMenuFunc     func);

void        moo_menu_action_set_menu_data   (MooMenuAction  *action,
                                             gpointer        data,
                                             gboolean        is_object);


G_END_DECLS

#endif /* __MOO_MENU_ACTION_H__ */
