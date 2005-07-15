/*
 *   mooterm/mooterminput.c
 *
 *   Copyright (C) 2004-2005 by Yevgen Muntyan <muntyan@math.tamu.edu>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   See COPYING file that comes with this distribution.
 *
 *   moo_term_key_press() code is taken from libvte vte.c,
 *   Copyright (C) 2001-2004 Red Hat, Inc.
 */

#define MOOTERM_COMPILATION
#include "mooterm/mooterm-keymap.h"
#include "mooterm/mootermpt.h"

/* must be enough to fit '^' + one unicode character + 0 byte */
#define MANY_CHARS  16
#define META_MASK   GDK_MOD1_MASK


static GtkWidgetClass *widget_class (void)
{
    static GtkWidgetClass *klass = NULL;
    if (!klass)
        klass = GTK_WIDGET_CLASS (g_type_class_peek (GTK_TYPE_WIDGET));
    return klass;
}


void        moo_term_im_commit          (G_GNUC_UNUSED GtkIMContext   *imcontext,
                                         gchar          *arg,
                                         MooTerm        *term)
{
    if (moo_term_pt_child_alive (term->priv->pt))
        moo_term_feed_child (term, arg, -1);
}


/* shamelessly taken from vte.c */
gboolean    moo_term_key_press          (GtkWidget      *widget,
                                         GdkEventKey    *event)
{
    MooTerm *term;
    char *string = NULL;
    gssize string_length = 0;
    int i;
    gboolean scrolled = FALSE;
    gboolean steal = FALSE;
    gboolean is_modifier = FALSE;
    gboolean handled;
    gboolean suppress_meta_esc = FALSE;
    guint keyval = 0;
    gunichar keychar = 0;
    char keybuf[6];  /* 6 bytes for UTF-8 character */
    GdkModifierType modifiers;

    term = MOO_TERM (widget);

    /* First, check if GtkWidget's behavior already does something with
     * this key. */
    if (widget_class()->key_press_event (widget, event))
    {
        return TRUE;
    }

    keyval = event->keyval;

    /* If it's a keypress, record that we got the event, in case the
     * input method takes the event from us. */
    term->priv->modifiers = modifiers = event->state;
    modifiers &= (GDK_SHIFT_MASK | GDK_CONTROL_MASK | META_MASK);

    /* Determine if this is just a modifier key. */
    is_modifier = key_is_modifier (keyval);

    /* Unless it's a modifier key, hide the pointer. */
    if (!is_modifier &&
         term->priv->settings.hide_cursor_on_keypress &&
         moo_term_pt_child_alive (term->priv->pt))
    {
        moo_term_set_pointer_visible (term, FALSE);
    }

    /* We steal many keypad keys here. */
    if (!term->priv->im_preedit_active)
    {
        switch (keyval)
        {
            CASE_GDK_KP_SOMETHING
                steal = TRUE;
        }

        if (modifiers & META_MASK)
        {
            steal = TRUE;
        }
    }

    /* Let the input method at this one first. */
    if (!steal)
    {
        if (gtk_im_context_filter_keypress(term->priv->im, event))
            return TRUE;
    }

    if (is_modifier)
        return FALSE;

    /* Now figure out what to send to the child. */
    handled = FALSE;

    switch (keyval)
    {
        case GDK_BackSpace:
            get_backspace_key (term, &string,
                               &string_length,
                               &suppress_meta_esc);
            handled = TRUE;
            break;

        case GDK_Delete:
            get_delete_key (term, &string,
                            &string_length,
                            &suppress_meta_esc);
            handled = TRUE;
            suppress_meta_esc = TRUE;
            break;

        case GDK_Insert:
            if (modifiers & GDK_SHIFT_MASK)
            {
                moo_term_paste_clipboard (term);
                handled = TRUE;
                suppress_meta_esc = TRUE;
            }
            else if (modifiers & GDK_CONTROL_MASK)
            {
                moo_term_copy_clipboard (term);
                handled = TRUE;
                suppress_meta_esc = TRUE;
            }
            break;

        case GDK_Page_Up:
        case GDK_KP_Page_Up:
            if (modifiers & GDK_SHIFT_MASK)
            {
                moo_term_scroll_pages (term, -1);
                scrolled = TRUE;
                handled = TRUE;
                suppress_meta_esc = TRUE;
            }
            break;

        case GDK_Page_Down:
        case GDK_KP_Page_Down:
            if (modifiers & GDK_SHIFT_MASK)
            {
                moo_term_scroll_pages (term, 1);
                scrolled = TRUE;
                handled = TRUE;
                suppress_meta_esc = TRUE;
            }
            break;

        case GDK_Home:
        case GDK_KP_Home:
            if (modifiers & GDK_SHIFT_MASK)
            {
                moo_term_scroll_to_top (term);
                scrolled = TRUE;
                handled = TRUE;
            }
            break;
        case GDK_End:
        case GDK_KP_End:
            if (modifiers & GDK_SHIFT_MASK)
            {
                moo_term_scroll_to_bottom (term);
                scrolled = TRUE;
                handled = TRUE;
            }
            break;
        case GDK_Up:
        case GDK_KP_Up:
            if (modifiers & GDK_SHIFT_MASK)
            {
                moo_term_scroll_lines (term, -1);
                scrolled = TRUE;
                handled = TRUE;
            }
            break;
        case GDK_Down:
        case GDK_KP_Down:
            if (modifiers & GDK_SHIFT_MASK)
            {
                moo_term_scroll_lines (term, 1);
                scrolled = TRUE;
                handled = TRUE;
            }
            break;

        case GDK_Break:
            moo_term_ctrl_c (term);
            handled = TRUE;
            break;
    }

    /* If the above switch statement didn't do the job, try mapping
     * it to a literal or capability name. */
    if (!handled)
    {
        if (!(modifiers & GDK_CONTROL_MASK))
            get_vt_key (term, keyval, &string, &string_length);
        else
            get_vt_ctl_key (term, keyval, &string, &string_length);

        /* If we found something this way, suppress
            * escape-on-meta. */
        if (string != NULL && string_length > 0)
            suppress_meta_esc = TRUE;
    }

    /* If we didn't manage to do anything, try to salvage a
        * printable string. */
    if (!handled && !string)
    {
        /* Convert the keyval to a gunichar. */
        keychar = gdk_keyval_to_unicode(keyval);
        string_length = 0;

        if (keychar != 0)
        {
            /* Convert the gunichar to a string. */
            string_length = g_unichar_to_utf8(keychar, keybuf);

            if (string_length)
            {
                string = g_malloc0 (string_length + 1);
                memcpy (string, keybuf, string_length);
            }
            else
            {
                string = NULL;
            }
        }

        if (string && (modifiers & GDK_CONTROL_MASK))
        {
            /* Replace characters which have "control"
                * counterparts with those counterparts. */
            for (i = 0; i < string_length; i++)
            {
                if ((((guint8)string[i]) >= 0x40) &&
                       (((guint8)string[i]) <  0x80))
                {
                    string[i] &= (~(0x60));
                }
            }
        }
    }

    /* If we got normal characters, send them to the child. */
    if (string)
    {
        if (moo_term_pt_child_alive (term->priv->pt))
        {
            if (term->priv->settings.meta_sends_escape &&
                !suppress_meta_esc &&
                string_length > 0 &&
                (modifiers & META_MASK))
            {
                moo_term_feed_child (term, "\033", 1);
            }

            if (string_length > 0)
            {
                moo_term_feed_child (term, string, string_length);
            }
        }

        /* Keep the cursor on-screen. */
        if (!scrolled && term->priv->settings.scroll_on_keystroke)
            moo_term_scroll_to_bottom (term);

        g_free(string);
    }

    return TRUE;
}


gboolean    moo_term_key_release        (GtkWidget      *widget,
                                         GdkEventKey    *event)
{
    MooTerm *term = MOO_TERM (widget);
    if (!gtk_im_context_filter_keypress (term->priv->im, event))
        return widget_class()->key_release_event (widget, event);
    else
        return TRUE;
}


void        moo_term_set_mouse_tracking     (MooTerm    *term,
                                             int         tracking_type)
{
    term_implement_me ();
}
