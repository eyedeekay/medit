/*
 *   mooeditinput.c
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

#define MOOEDIT_COMPILATION
#include "mooedit/mootextview-private.h"
#include "mooedit/mooedit-private.h"
#include "mooedit/mootextiter.h"
#include "mooedit/mootextbuffer.h"
#include "mooutils/moocompat.h"
#include "mooutils/mooutils-misc.h"
#include <gdk/gdkkeysyms.h>


inline static GtkWidgetClass*
parent_class (void)
{
    static gpointer textview_class = NULL;

    if (!textview_class)
        textview_class = GTK_WIDGET_CLASS (gtk_type_class (GTK_TYPE_TEXT_VIEW));

    return GTK_WIDGET_CLASS (textview_class);
}


inline static gboolean
is_word_char (const GtkTextIter *iter)
{
    gunichar c = gtk_text_iter_get_char (iter);
    g_return_val_if_fail (c != 0, FALSE);
    return g_unichar_isalnum (c) || c == '_';
}

/* Glib docs say: "(Note: don't use this to do word breaking; you have
 * to use Pango or equivalent to get word breaking right, the algorithm
 * is fairly complex.)"
 */
inline static gboolean
is_space (const GtkTextIter *iter)
{
    gunichar c = gtk_text_iter_get_char (iter);
    g_return_val_if_fail (c != 0, FALSE);
    return g_unichar_isspace (c);
}

inline static gboolean
is_word_start (const GtkTextIter *iter)
{
    GtkTextIter i;
    if (!is_word_char (iter)) return FALSE;
    i = *iter;
    if (!gtk_text_iter_backward_char (&i)) return TRUE;
    return !is_word_char (&i);
}


static gboolean
text_iter_forward_word_start (GtkTextIter *iter)
{
    gboolean moved = FALSE;
    if (gtk_text_iter_is_end (iter)) return FALSE;

    /* if iter points to word char, then go to the first non-space char after the word
     * otherwise, go to the next word char
     * stop at end of line
     */

    if (is_word_char (iter)) {
        while (!gtk_text_iter_is_end (iter) && is_word_char (iter))
        {
            gtk_text_iter_forward_cursor_position (iter);
            moved = TRUE;
        }
        if (gtk_text_iter_is_end (iter)) return FALSE;
        while (!gtk_text_iter_is_end (iter) &&
               is_space (iter) &&
               !gtk_text_iter_ends_line (iter))
        {
            gtk_text_iter_forward_cursor_position (iter);
            moved = TRUE;
        }
    }
    else {
        if (gtk_text_iter_ends_line (iter)) {
            gtk_text_iter_forward_cursor_position (iter);
            moved = TRUE;
        }
        else {
            while (!gtk_text_iter_is_end (iter) &&
                   !is_word_char (iter) &&
                   !gtk_text_iter_ends_line (iter))
            {
                gtk_text_iter_forward_cursor_position (iter);
                moved = TRUE;
            }
        }
    }
    return moved && !gtk_text_iter_is_end (iter);
}

inline static gboolean
text_iter_forward_word_start_n (GtkTextIter *iter, guint count)
{
    if (!count) return FALSE;
    while (count) {
        if (!text_iter_forward_word_start (iter)) {
            gtk_text_iter_forward_to_end (iter);
            return FALSE;
        }
        else
            --count;
    }
    return TRUE;
}


inline static gboolean
text_iter_backward_word_start (GtkTextIter *iter)
{
    gboolean moved = FALSE;
    if (gtk_text_iter_starts_line (iter)) {
        moved = gtk_text_iter_backward_cursor_position (iter);
        /* it may point now to \n in \r\n combination, fixed in gtk-2.10 */
        if (moved && !gtk_text_iter_ends_line (iter))
            gtk_text_iter_backward_char (iter);
    }
    else {
        while (gtk_text_iter_backward_cursor_position (iter) &&
               !is_word_start (iter) &&
               !gtk_text_iter_starts_line (iter))
            moved = TRUE;
    }
    return moved;
}

inline static gboolean
text_iter_backward_word_start_n (GtkTextIter *iter, guint count)
{
    gboolean moved = FALSE;
    while (count && text_iter_backward_word_start (iter)) {
        moved = TRUE;
        --count;
    }
    return moved;
}


/* TODO: may I do this? */
inline static void
text_view_reset_im_context (GtkTextView *text_view)
{
  if (text_view->need_im_reset)
    {
      text_view->need_im_reset = FALSE;
      gtk_im_context_reset (text_view->im_context);
    }
}


static void
move_cursor_to (GtkTextView *text_view,
                GtkTextIter *where,
                gboolean     extend_selection)
{
    GtkTextBuffer *buffer;
    GtkTextMark *insert;

    buffer = gtk_text_view_get_buffer (text_view);
    insert = gtk_text_buffer_get_insert (buffer);
    text_view_reset_im_context (text_view);

    if (extend_selection)
        gtk_text_buffer_move_mark (buffer, insert, where);
    else
        gtk_text_buffer_place_cursor (buffer, where);

    gtk_text_view_scroll_mark_onscreen (text_view, insert);
}


static void
moo_text_view_move_cursor_words (G_GNUC_UNUSED MooTextView *view,
                                 GtkTextIter *iter,
                                 gint         count)
{
    if (count < 0)
        text_iter_backward_word_start_n (iter, -count);
    else if (count > 0)
        text_iter_forward_word_start_n (iter, count);
}


static void
moo_text_view_home_end (MooTextView *view,
                        GtkTextIter *iter,
                        gint         count)
{
    if (gtk_text_iter_is_end (iter) && gtk_text_iter_is_start (iter))
        return;

    if (view->priv->smart_home_end && count == -1)
    {
        GtkTextIter first = *iter;

        gtk_text_iter_set_line_offset (&first, 0);

        while (!gtk_text_iter_ends_line (&first))
        {
            if (is_space (&first))
                gtk_text_iter_forward_cursor_position (&first);
            else
                break;
        }

        if (gtk_text_iter_starts_line (iter) || !gtk_text_iter_equal (&first, iter))
            *iter = first;
        else
            gtk_text_iter_set_line_offset (iter, 0);
    }
    else if (view->priv->smart_home_end && count == 1)
    {
        GtkTextIter last = *iter;

        if (gtk_text_iter_ends_line (&last))
        {
            while (!gtk_text_iter_starts_line (&last))
            {
                gtk_text_iter_backward_cursor_position (&last);

                if (!is_space (&last))
                {
                    gtk_text_iter_forward_cursor_position (&last);
                    break;
                }
            }

            if (gtk_text_iter_ends_line (iter) || !gtk_text_iter_equal (&last, iter))
                *iter = last;
            else
                gtk_text_iter_forward_to_line_end (iter);
        }
        else
        {
            gtk_text_iter_forward_to_line_end (iter);
        }
    }
    else if (count == -1)
    {
        gtk_text_iter_set_line_offset (iter, 0);
    }
    else
    {
        if (!gtk_text_iter_ends_line (iter))
            gtk_text_iter_forward_to_line_end (iter);
    }
}


void
_moo_text_view_move_cursor (GtkTextView        *text_view,
                            GtkMovementStep     step,
                            gint                count,
                            gboolean            extend_selection)
{
    GtkTextBuffer *buffer;
    GtkTextMark *insert;
    GtkTextIter iter;
    MooTextView *view = MOO_TEXT_VIEW (text_view);

    if (!text_view->cursor_visible && !view->priv->overwrite_mode)
    {
        GTK_TEXT_VIEW_CLASS (parent_class())->move_cursor (text_view, step, count, extend_selection);
        return;
    }

    buffer = gtk_text_view_get_buffer (text_view);
    insert = gtk_text_buffer_get_insert (buffer);
    gtk_text_buffer_get_iter_at_mark (buffer, &iter, insert);

    switch (step)
    {
        case GTK_MOVEMENT_WORDS:
            moo_text_view_move_cursor_words (MOO_TEXT_VIEW (text_view), &iter, count);
            break;

        case GTK_MOVEMENT_DISPLAY_LINE_ENDS:
            moo_text_view_home_end (MOO_TEXT_VIEW (text_view), &iter, count);
            break;

        default:
            if (view->priv->overwrite_mode)
                gtk_text_view_set_cursor_visible (text_view, TRUE);
            GTK_TEXT_VIEW_CLASS (parent_class())->move_cursor (text_view, step, count, extend_selection);
            if (view->priv->overwrite_mode)
                gtk_text_view_set_cursor_visible (text_view, FALSE);
            return;
    }

    move_cursor_to (text_view, &iter, extend_selection);
    _moo_text_view_pend_cursor_blink (view);
}


void
_moo_text_view_page_horizontally (GtkTextView *text_view,
                                  int          count,
                                  gboolean     extend_selection)
{
    MooTextView *view = MOO_TEXT_VIEW (text_view);
    if (view->priv->overwrite_mode)
        gtk_text_view_set_cursor_visible (text_view, TRUE);
    GTK_TEXT_VIEW_CLASS (parent_class())->page_horizontally (text_view, count, extend_selection);
    if (view->priv->overwrite_mode)
        gtk_text_view_set_cursor_visible (text_view, FALSE);
    _moo_text_view_pend_cursor_blink (view);
}


void
_moo_text_view_delete_from_cursor (GtkTextView        *text_view,
                                   GtkDeleteType       type,
                                   gint                count)
{
    GtkTextBuffer *buf;
    GtkTextMark *insert_mark;
    GtkTextIter insert, start, end;

    if (type != GTK_DELETE_WORD_ENDS)
    {
        GTK_TEXT_VIEW_CLASS (parent_class())->delete_from_cursor (text_view, type, count);
        return;
    }

    text_view_reset_im_context (text_view);

    buf = gtk_text_view_get_buffer (text_view);
    insert_mark = gtk_text_buffer_get_insert (buf);

    /* XXX */
    if (gtk_text_buffer_get_selection_bounds (buf, &start, &end))
    {
        gtk_text_buffer_delete_interactive (buf, &start, &end, text_view->editable);
        gtk_text_view_scroll_mark_onscreen (text_view, insert_mark);
        return;
    }

    gtk_text_buffer_get_iter_at_mark (buf, &insert, insert_mark);
    start = insert;
    end = insert;

    if (count > 0)
        text_iter_forward_word_start_n (&end, count);
    else if (count < 0)
        text_iter_backward_word_start_n (&start, -count);

    if (!gtk_text_iter_equal (&start, &end))
    {
        gtk_text_buffer_delete_interactive (buf, &start, &end, text_view->editable);
        gtk_text_view_scroll_mark_onscreen (text_view, insert_mark);
    }
}


/****************************************************************************/
/* Mouse
 */

inline static gboolean
extend_selection (MooTextView          *view,
                  MooTextSelectionType  type,
                  GtkTextIter          *insert,
                  GtkTextIter          *selection_bound)
{
    return MOO_TEXT_VIEW_GET_CLASS(view)->extend_selection (view, type, insert, selection_bound);
}

inline static void
clear_drag_stuff (MooTextView *view)
{
    view->priv->dnd.moved = FALSE;
    view->priv->dnd.type = MOO_TEXT_VIEW_DRAG_NONE;
    view->priv->dnd.start_x = -1;
    view->priv->dnd.start_y = -1;
    view->priv->dnd.button = GDK_BUTTON_RELEASE;
}

#define get_buf(view) \
    GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)))

/* from gtktextview.c */
inline static void
text_view_unobscure_mouse_cursor (GtkTextView *text_view)
{
    if (text_view->mouse_cursor_obscured)
    {
        GdkCursor *cursor =
            gdk_cursor_new_for_display (gtk_widget_get_display (GTK_WIDGET (text_view)),
                                        GDK_XTERM);
        gdk_window_set_cursor (gtk_text_view_get_window (text_view, GTK_TEXT_WINDOW_TEXT), cursor);
        gdk_cursor_unref (cursor);
        text_view->mouse_cursor_obscured = FALSE;
    }
}


static int
left_window_to_line (GtkTextView *text_view,
                     int          window_y)
{
    int y;
    GtkTextIter iter;
    gtk_text_view_window_to_buffer_coords (text_view, GTK_TEXT_WINDOW_LEFT,
                                           0, window_y, NULL, &y);
    gtk_text_view_get_line_at_y (text_view, &iter, y, NULL);
    return gtk_text_iter_get_line (&iter);
}


static gboolean
left_window_click (GtkTextView    *text_view,
                   GdkEventButton *event,
                   gboolean       *line_numbers)
{
    int window_width;
    MooTextView *view = MOO_TEXT_VIEW (text_view);

    *line_numbers = FALSE;
    gdk_drawable_get_size (event->window, &window_width, NULL);

    if (view->priv->lm.show_icons && event->x >= 0 && event->x < view->priv->lm.icon_width)
    {
        gboolean ret;
        int line = left_window_to_line (text_view, event->y);
        g_signal_emit_by_name (text_view, "line-mark-clicked", line, &ret);
        return ret;
    }
    else if (view->priv->enable_folding &&
             event->x >= window_width - view->priv->lm.fold_width &&
             event->x < window_width)
    {
        MooTextBuffer *buffer = MOO_TEXT_BUFFER (gtk_text_view_get_buffer (text_view));
        MooFold *fold;
        int line;

        line = left_window_to_line (text_view, event->y);
        fold = moo_text_buffer_get_fold_at_line (buffer, line);

        if (fold)
        {
            moo_text_buffer_toggle_fold (buffer, fold);
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else if (view->priv->lm.show_numbers &&
             event->x >= view->priv->lm.icon_width &&
             event->x < view->priv->lm.icon_width + view->priv->lm.numbers_width)
    {
        *line_numbers = TRUE;
        return TRUE;
    }

    return FALSE;
}

static void
select_range (GtkTextBuffer     *buffer,
              const GtkTextIter *sel_bound,
              const GtkTextIter *cursor)
{
    GtkTextIter sel_start, sel_end;

    gtk_text_buffer_get_iter_at_mark (buffer, &sel_start, gtk_text_buffer_get_selection_bound (buffer));
    gtk_text_buffer_get_iter_at_mark (buffer, &sel_end, gtk_text_buffer_get_insert (buffer));

    if (gtk_text_iter_compare (sel_bound, &sel_start))
    {
        if (gtk_text_iter_compare (cursor, &sel_end))
            gtk_text_buffer_select_range (buffer, cursor, sel_bound);
        else
            gtk_text_buffer_move_mark_by_name (buffer, "selection_bound", sel_bound);
    }
    else if (gtk_text_iter_compare (cursor, &sel_end))
    {
        gtk_text_buffer_move_mark_by_name (buffer, "insert", cursor);
    }
}

static void
select_lines (MooTextView *view,
              int          y)
{
    GtkTextView *text_view;
    GtkTextBuffer *buffer;
    GtkTextIter start, end;
    int cmp;

    text_view = GTK_TEXT_VIEW (view);
    buffer = gtk_text_view_get_buffer (text_view);

    gtk_text_buffer_get_iter_at_mark (buffer, &start, view->priv->dnd.start_mark);
    gtk_text_view_get_line_at_y (text_view, &end, y, NULL);
    cmp = gtk_text_iter_compare (&start, &end);

    if (cmp <= 0)
        gtk_text_iter_forward_line (&end);
    else
        gtk_text_iter_forward_line (&start);

    select_range (buffer, &start, &end);
}


static void     start_drag_scroll               (MooTextView        *view);
static void     stop_drag_scroll                (MooTextView        *view);
static gboolean drag_scroll_timeout_func        (MooTextView        *view);
/* from gtktextview.c */
static void     text_view_start_selection_dnd   (GtkTextView        *text_view,
                                                 const GtkTextIter  *iter,
                                                 GdkEventMotion     *event);
static const int SCROLL_TIMEOUT = 100;


static void
place_start_mark (MooTextView       *view,
                  const GtkTextIter *where)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

    if (!view->priv->dnd.start_mark)
        view->priv->dnd.start_mark =
            gtk_text_buffer_create_mark (buffer, NULL, where, FALSE);
    else
        gtk_text_buffer_move_mark (buffer, view->priv->dnd.start_mark, where);
}

static void
get_start_mark (MooTextView *view,
                GtkTextIter *iter)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
    g_assert (view->priv->dnd.start_mark != NULL);
    gtk_text_buffer_get_iter_at_mark (buffer, iter, view->priv->dnd.start_mark);
}

int
_moo_text_view_button_press_event (GtkWidget          *widget,
                                   GdkEventButton     *event)
{
    GtkTextView *text_view;
    MooTextView *view;
    GtkTextBuffer *buffer;
    int x, y;
    GtkTextIter iter;
    gboolean line_numbers = FALSE;
    gboolean ret;

    text_view = GTK_TEXT_VIEW (widget);
    text_view_unobscure_mouse_cursor (text_view);

    switch (gtk_text_view_get_window_type (text_view, event->window))
    {
        case GTK_TEXT_WINDOW_TEXT:
            break;

        case GTK_TEXT_WINDOW_LEFT:
            ret = left_window_click (text_view, event, &line_numbers);
            if (!line_numbers)
                return ret;
            break;

        default:
            return FALSE;
    }

    view = MOO_TEXT_VIEW (widget);
    buffer = gtk_text_view_get_buffer (text_view);
    gtk_widget_grab_focus (widget);

    text_view_reset_im_context (text_view);

    if (!line_numbers)
    {
        gtk_text_view_window_to_buffer_coords (text_view, GTK_TEXT_WINDOW_TEXT,
                                               event->x, event->y, &x, &y);
        gtk_text_view_get_iter_at_location (text_view, &iter, x, y);
    }
    else
    {
        int line;
        gtk_text_view_window_to_buffer_coords (text_view, GTK_TEXT_WINDOW_LEFT,
                                               event->x, event->y, &x, &y);
        line = left_window_to_line (text_view, event->y);
        gtk_text_buffer_get_iter_at_line (buffer, &iter, line);
    }

    if (event->type == GDK_BUTTON_PRESS)
    {
        if (event->button == 1)
        {
            GtkTextIter sel_start, sel_end;

            view->priv->dnd.button = GDK_BUTTON_PRESS;
            view->priv->dnd.start_x = x;
            view->priv->dnd.start_y = y;
            place_start_mark (view, &iter);

            /* if clicked in selected, start drag */
            if (!line_numbers && gtk_text_buffer_get_selection_bounds (buffer, &sel_start, &sel_end))
            {
                gtk_text_iter_order (&sel_start, &sel_end);

                if (gtk_text_iter_in_range (&iter, &sel_start, &sel_end))
                {
                    /* clicked inside of selection,
                     * set up drag and return */
                    view->priv->dnd.type = MOO_TEXT_VIEW_DRAG_DRAG;
                    return TRUE;
                }
            }

            if (!line_numbers)
            {
                /* otherwise, clear selection, and position cursor at clicked point */
                if (event->state & GDK_SHIFT_MASK)
                {
                    GtkTextIter start_mark;
                    gtk_text_buffer_get_iter_at_mark (buffer, &start_mark,
                                                      gtk_text_buffer_get_selection_bound (buffer));
                    place_start_mark (view, &start_mark);
                    gtk_text_buffer_move_mark (buffer,
                                               gtk_text_buffer_get_insert (buffer),
                                               &iter);
                }
                else
                {
                    gtk_text_buffer_place_cursor (buffer, &iter);
                }
            }
            else
            {
                GtkTextIter start_mark;

                if (event->state & GDK_SHIFT_MASK)
                {
                    gtk_text_buffer_get_iter_at_mark (buffer, &start_mark,
                                                      gtk_text_buffer_get_selection_bound (buffer));

                    if (gtk_text_iter_starts_line (&start_mark))
                    {
                        GtkTextIter tmp;
                        gtk_text_buffer_get_iter_at_mark (buffer, &tmp,
                                                          gtk_text_buffer_get_insert (buffer));
                        if (gtk_text_iter_compare (&start_mark, &tmp) > 0)
                            gtk_text_iter_backward_line (&start_mark);
                    }

                    place_start_mark (view, &start_mark);
                }

                get_start_mark (view, &start_mark);
                gtk_text_iter_set_line_offset (&start_mark, 0);
                place_start_mark (view, &start_mark);
                select_lines (view, y);
            }

            if (!line_numbers)
                view->priv->dnd.type = MOO_TEXT_VIEW_DRAG_SELECT;
            else
                view->priv->dnd.type = MOO_TEXT_VIEW_DRAG_SELECT_LINES;
        }
        else if (event->button == 3 && MOO_IS_EDIT (widget))
        {
            _moo_edit_do_popup (MOO_EDIT (widget), event);
            return TRUE;
        }
        else if (event->button == 2 || event->button == 3)
        {
            return parent_class()->button_press_event (widget, event);
        }
        else
        {
            g_warning ("got button %u in button_press callback", event->button);
        }
    }
    else if (event->type == GDK_2BUTTON_PRESS && event->button == 1)
    {
        GtkTextIter bound;

        if (gtk_text_buffer_get_selection_bounds (buffer, NULL, NULL))
        {
            /* it may happen sometimes, if you click fast enough */
            gtk_text_buffer_place_cursor (buffer, &iter);
        }

        view->priv->dnd.button = GDK_2BUTTON_PRESS;
        view->priv->dnd.start_x = x;
        view->priv->dnd.start_y = y;
        view->priv->dnd.type = MOO_TEXT_VIEW_DRAG_SELECT;

        bound = iter;
        if (extend_selection (view, MOO_TEXT_SELECT_WORDS, &iter, &bound))
            select_range (buffer, &bound, &iter);
    }
    else if (event->type == GDK_3BUTTON_PRESS && event->button == 1)
    {
        GtkTextIter bound = iter;
        view->priv->dnd.button = GDK_3BUTTON_PRESS;
        view->priv->dnd.start_x = x;
        view->priv->dnd.start_y = y;
        view->priv->dnd.type = MOO_TEXT_VIEW_DRAG_SELECT;
        if (extend_selection (view, MOO_TEXT_SELECT_LINES, &iter, &bound))
            select_range (buffer, &bound, &iter);
    }

    return TRUE;
}


int
_moo_text_view_button_release_event (GtkWidget      *widget,
                                     G_GNUC_UNUSED GdkEventButton *event)
{
    GtkTextView *text_view = GTK_TEXT_VIEW (widget);
    MooTextView *view = MOO_TEXT_VIEW (widget);
    GtkTextIter iter;

    switch (view->priv->dnd.type)
    {
        case MOO_TEXT_VIEW_DRAG_NONE:
            /* it may happen after right-click, or clicking outside
             * of widget or something like that
             * everything has been taken care of, so do nothing */
            break;

        case MOO_TEXT_VIEW_DRAG_SELECT:
        case MOO_TEXT_VIEW_DRAG_SELECT_LINES:
            /* everything should be done already in button_press and
             * motion_notify handlers */
            stop_drag_scroll (view);
            break;

        case MOO_TEXT_VIEW_DRAG_DRAG:
            /* if we were really dragging, drop it
             * otherwise, it was just a single click in selected text */
            if (view->priv->dnd.moved)
            {
                /* parent should handle drag */
                clear_drag_stuff (view);
                g_return_val_if_reached (FALSE);
            }
            get_start_mark (view, &iter);
            gtk_text_buffer_place_cursor (gtk_text_view_get_buffer (text_view), &iter);
            break;
    }

    clear_drag_stuff (view);
    return TRUE;
}


#define OUTSIDE(x,y,rect)               \
    ((x) < (rect).x ||                  \
     (y) < (rect).y ||                  \
     (x) >= (rect).x + (rect).width ||  \
     (y) >= (rect).y + (rect).height)

int
_moo_text_view_motion_event (GtkWidget          *widget,
                             GdkEventMotion     *event)
{
    GtkTextView *text_view = GTK_TEXT_VIEW (widget);
    MooTextView *view = MOO_TEXT_VIEW (widget);
    int x, y, event_x, event_y;
    GtkTextIter iter;

    text_view_unobscure_mouse_cursor (text_view);

    if (!view->priv->dnd.type)
        return FALSE;

    if (event->is_hint)
    {
        gdk_window_get_pointer (event->window, &event_x, &event_y, NULL);
    }
    else
    {
        event_x = event->x;
        event_y = event->y;
    }

    gtk_text_view_window_to_buffer_coords (text_view,
                                           gtk_text_view_get_window_type (text_view, event->window),
                                           event_x, event_y, &x, &y);

    if (view->priv->dnd.type == MOO_TEXT_VIEW_DRAG_SELECT_LINES)
    {
        GdkRectangle rect;
        GdkWindow *window;

        window = gtk_text_view_get_window (text_view, GTK_TEXT_WINDOW_LEFT);
        gtk_text_view_get_visible_rect (text_view, &rect);

        select_lines (view, y);

        if (y < rect.y || y >= rect.y + rect.height)
            start_drag_scroll (view);
        else
            stop_drag_scroll (view);

        return TRUE;
    }

    gtk_text_view_get_iter_at_location (text_view, &iter, x, y);

    if (view->priv->dnd.type == MOO_TEXT_VIEW_DRAG_SELECT)
    {
        GdkRectangle rect;
        GtkTextIter start;
        MooTextSelectionType t;
        GtkTextBuffer *buffer;

        view->priv->dnd.moved = TRUE;
        gtk_text_view_get_visible_rect (text_view, &rect);

        if (OUTSIDE (x, y, rect))
        {
            start_drag_scroll (view);
            return TRUE;
        }

        stop_drag_scroll (view);

        buffer = gtk_text_view_get_buffer (text_view);
        get_start_mark (view, &start);

        if (view->priv->dnd.button == GDK_BUTTON_PRESS)
            t = MOO_TEXT_SELECT_CHARS;
        else if (view->priv->dnd.button == GDK_2BUTTON_PRESS)
            t = MOO_TEXT_SELECT_WORDS;
        else
            t = MOO_TEXT_SELECT_LINES;

        if (extend_selection (view, t, &start, &iter))
            select_range (buffer, &start, &iter);
        else
            gtk_text_buffer_place_cursor (buffer, &iter);
    }
    else
    {
        /* this piece is from gtktextview.c */
        int x, y;
        GtkTextIter start;
        gdk_window_get_pointer (gtk_text_view_get_window (text_view, GTK_TEXT_WINDOW_TEXT),
                                &x, &y, NULL);

        if (gtk_drag_check_threshold (widget,
                                      view->priv->dnd.start_x,
                                      view->priv->dnd.start_y,
                                      x, y))
        {
            view->priv->dnd.type = MOO_TEXT_VIEW_DRAG_NONE;
            get_start_mark (view, &start);
            text_view_start_selection_dnd (text_view, &start, event);
        }
    }

    return TRUE;
}


static void
text_view_start_selection_dnd (GtkTextView       *text_view,
                               G_GNUC_UNUSED const GtkTextIter *iter,
                               GdkEventMotion    *event)
{
    GdkDragContext *context;
    GtkTargetList *target_list;

#if !GTK_CHECK_VERSION(2,10,0)
    static const GtkTargetEntry target_table[] = {
        { (char*) "GTK_TEXT_BUFFER_CONTENTS", GTK_TARGET_SAME_APP, 0 }
    };

    target_list = gtk_target_list_new (target_table, G_N_ELEMENTS (target_table));
#else
    target_list = gtk_target_list_new (NULL, 0);
#endif

    gtk_target_list_add_text_targets (target_list, 0);

    text_view->drag_start_x = -1;
    text_view->drag_start_y = -1;
    text_view->pending_place_cursor_button = 0;

    context = gtk_drag_begin (GTK_WIDGET (text_view), target_list,
                              (GdkDragAction) (GDK_ACTION_COPY | GDK_ACTION_MOVE),
                              1, (GdkEvent*)event);

    gtk_target_list_unref (target_list);
}


inline static int
char_class (const GtkTextIter *iter)
{
    if (gtk_text_iter_ends_line (iter)) return -1;
    else if (is_space (iter)) return 0;
    else if (is_word_char (iter)) return 1;
    else return 2;
}

#define FIND_BRACKET_LIMIT 2000

int
_moo_text_view_extend_selection (MooTextView        *view,
                                 MooTextSelectionType type,
                                 GtkTextIter        *start,
                                 GtkTextIter        *end)
{
    int order = gtk_text_iter_compare (start, end);

    if (type == MOO_TEXT_SELECT_CHARS)
    {
        return order;
    }
    else if (type == MOO_TEXT_SELECT_WORDS)
    {
        int ch_class;

        if (!order && view->priv->dnd.double_click_selects_brackets)
        {
            GtkTextIter rstart = *start;
            if (moo_text_iter_at_bracket (&rstart) &&
                !(order == 1 && gtk_text_iter_compare (&rstart, start) == -1))  /* this means (...)| */
            {
                GtkTextIter rend = rstart;
                if (moo_text_iter_find_matching_bracket (&rend,FIND_BRACKET_LIMIT) ==
                    MOO_BRACKET_MATCH_CORRECT)
                {
                    if (gtk_text_iter_compare (&rstart, &rend) > 0)
                    {   /*  <rend>(     <rstart>) */
                        if (view->priv->dnd.double_click_selects_inside)
                            gtk_text_iter_forward_cursor_position (&rend);
                        else
                            gtk_text_iter_forward_cursor_position (&rstart);
                    }
                    else
                    {   /*  <rstart>(     <rend>) */
                        if (view->priv->dnd.double_click_selects_inside)
                            gtk_text_iter_forward_cursor_position (&rstart);
                        else
                            gtk_text_iter_forward_cursor_position (&rend);
                    }
                    *start = rstart;
                    *end = rend;
                    return TRUE;
                }
            }
        }

        if (order > 0)
        {
            GtkTextIter *tmp = start;
            start = end; end = tmp;
        }

        ch_class = char_class (end);

        while (!gtk_text_iter_ends_line (end) &&
                char_class (end) == ch_class)
        {
            gtk_text_iter_forward_cursor_position (end);
        }

        ch_class = char_class (start);

        while (!gtk_text_iter_starts_line (start) &&
                char_class (start) == ch_class)
        {
            gtk_text_iter_backward_cursor_position (start);
        }

        if (char_class (start) != ch_class)
            gtk_text_iter_forward_cursor_position (start);

        return gtk_text_iter_compare (start, end);
    }
    else if (type == MOO_TEXT_SELECT_LINES)
    {
        if (order > 0)
        {
            GtkTextIter *tmp = start;
            start = end; end = tmp;
        }
        gtk_text_iter_set_line_offset (start, 0);
        gtk_text_iter_forward_line (end);
        return gtk_text_iter_compare (start, end);
    }

    g_return_val_if_reached (FALSE);
}


static void
start_drag_scroll (MooTextView *view)
{
    if (!view->priv->dnd.scroll_timeout)
        view->priv->dnd.scroll_timeout =
            _moo_timeout_add (SCROLL_TIMEOUT,
                              (GSourceFunc)drag_scroll_timeout_func,
                              view);
    drag_scroll_timeout_func (view);
}


static void
stop_drag_scroll (MooTextView *view)
{
    if (view->priv->dnd.scroll_timeout)
        g_source_remove (view->priv->dnd.scroll_timeout);
    view->priv->dnd.scroll_timeout = 0;
}


static gboolean
drag_scroll_timeout_func (MooTextView *view)
{
    GtkTextView *text_view;
    int x, y, px, py;
    GtkTextIter iter;
    GtkTextIter start;
    MooTextSelectionType t;
    GtkTextBuffer *buffer;
    GtkTextWindowType win_type;

    g_assert (view->priv->dnd.type == MOO_TEXT_VIEW_DRAG_SELECT ||
              view->priv->dnd.type == MOO_TEXT_VIEW_DRAG_SELECT_LINES);

    text_view = GTK_TEXT_VIEW (view);
    buffer = gtk_text_view_get_buffer (text_view);
    win_type = view->priv->dnd.type == MOO_TEXT_VIEW_DRAG_SELECT ?
        GTK_TEXT_WINDOW_TEXT : GTK_TEXT_WINDOW_LEFT;

    gdk_window_get_pointer (gtk_text_view_get_window (text_view, win_type), &px, &py, NULL);
    gtk_text_view_window_to_buffer_coords (text_view, win_type, px, py, &x, &y);
    gtk_text_view_get_iter_at_location (text_view, &iter, x, y);

    get_start_mark (view, &start);

    if (view->priv->dnd.type == MOO_TEXT_VIEW_DRAG_SELECT)
    {
        if (view->priv->dnd.button == GDK_BUTTON_PRESS)
            t = MOO_TEXT_SELECT_CHARS;
        else if (view->priv->dnd.button == GDK_2BUTTON_PRESS)
            t = MOO_TEXT_SELECT_WORDS;
        else
            t = MOO_TEXT_SELECT_LINES;

        if (extend_selection (view, t, &start, &iter))
            select_range (buffer, &start, &iter);
        else
            gtk_text_buffer_place_cursor (buffer, &iter);
    }
    else
    {
        select_lines (view, y);
    }

    gtk_text_view_scroll_mark_onscreen (text_view,
                                        gtk_text_buffer_get_insert (buffer));
    return TRUE;
}


/****************************************************************************/
/* Keyboard
 */

/* these two functions are taken from gtk/gtktextview.c */
static void text_view_obscure_mouse_cursor (GtkTextView *text_view);
static void set_invisible_cursor (GdkWindow *window);

static gboolean handle_tab          (MooTextView    *view,
                                     GdkEventKey    *event);
static gboolean handle_backspace    (MooTextView    *view,
                                     GdkEventKey    *event);
static gboolean handle_enter        (MooTextView    *view,
                                     GdkEventKey    *event);
static gboolean handle_ctrl_up      (MooTextView    *view,
                                     GdkEventKey    *event,
                                     gboolean        up);
static gboolean handle_ctrl_pgup    (MooTextView    *view,
                                     GdkEventKey    *event,
                                     gboolean        up);


int
_moo_text_view_key_press_event (GtkWidget          *widget,
                                GdkEventKey        *event)
{
    MooTextView *view;
    GtkTextView *text_view;
    GtkTextBuffer *buffer;
    gboolean obscure = TRUE;
    gboolean handled = FALSE;
    int keyval = event->keyval;
    GdkModifierType mods =
            event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK);

    view = MOO_TEXT_VIEW (widget);
    text_view = GTK_TEXT_VIEW (widget);
    buffer = gtk_text_view_get_buffer (text_view);

    /* ignore key events from the search entry */
    if (view->priv->qs.in_search)
        return FALSE;

    if (!mods)
    {
        switch (keyval)
        {
            case GDK_Tab:
            case GDK_KP_Tab:
                handled = handle_tab (view, event);
                break;
            case GDK_BackSpace:
                gtk_text_buffer_begin_user_action (buffer);
                handled = handle_backspace (view, event);
                gtk_text_buffer_end_user_action (buffer);
                break;
            case GDK_KP_Enter:
            case GDK_Return:
                gtk_text_buffer_begin_user_action (buffer);
                handled = handle_enter (view, event);
                gtk_text_buffer_end_user_action (buffer);
                break;
        }
    }
    else if (mods == GDK_SHIFT_MASK)
    {
        switch (keyval)
        {
            case GDK_ISO_Left_Tab:
            case GDK_KP_Tab:
                handled = handle_tab (view, event);
                break;
        }
    }
    else if (mods == GDK_CONTROL_MASK)
    {
        switch (keyval)
        {
            case GDK_Up:
            case GDK_KP_Up:
                handled = handle_ctrl_up (view, event, TRUE);
                /* if we scroll, let mouse cursor stay */
                obscure = FALSE;
                break;
            case GDK_Down:
            case GDK_KP_Down:
                handled = handle_ctrl_up (view, event, FALSE);
                obscure = FALSE;
                break;
            case GDK_Page_Up:
            case GDK_KP_Page_Up:
                handled = handle_ctrl_pgup (view, event, TRUE);
                obscure = FALSE;
                break;
            case GDK_Page_Down:
            case GDK_KP_Page_Down:
                handled = handle_ctrl_pgup (view, event, FALSE);
                obscure = FALSE;
                break;
        }
    }
    else
    {
        obscure = FALSE;
    }

    if (obscure && handled)
        text_view_obscure_mouse_cursor (text_view);

    if (handled)
        return TRUE;

    view->priv->in_key_press = TRUE;
    gtk_text_buffer_begin_user_action (buffer);
    handled = parent_class()->key_press_event (widget, event);
    gtk_text_buffer_end_user_action (buffer);
    view->priv->in_key_press = FALSE;

    _moo_text_view_check_char_inserted (view);
    _moo_text_view_pend_cursor_blink (view);

    return handled;
}


static void
text_view_obscure_mouse_cursor (GtkTextView *text_view)
{
    if (!text_view->mouse_cursor_obscured)
    {
        GdkWindow *window =
                gtk_text_view_get_window (text_view,
                                          GTK_TEXT_WINDOW_TEXT);
        set_invisible_cursor (window);
        text_view->mouse_cursor_obscured = TRUE;
    }
}


static void
set_invisible_cursor (GdkWindow *window)
{
    GdkBitmap *empty_bitmap;
    GdkCursor *cursor;
    GdkColor useless;
    char invisible_cursor_bits[] = { 0x0 };

    useless.red = useless.green = useless.blue = 0;
    useless.pixel = 0;

    empty_bitmap =
            gdk_bitmap_create_from_data (window,
                                         invisible_cursor_bits,
                                         1, 1);

    cursor = gdk_cursor_new_from_pixmap (empty_bitmap,
                                         empty_bitmap,
                                         &useless,
                                         &useless, 0, 0);

    gdk_window_set_cursor (window, cursor);

    gdk_cursor_unref (cursor);
    g_object_unref (empty_bitmap);
}


static gboolean
tab_unindent (MooTextView *view)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
    GtkTextIter start, end;
    int first_line, last_line;

    if (!view->priv->indenter)
        return FALSE;

    gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

    first_line = gtk_text_iter_get_line (&start);
    last_line = gtk_text_iter_get_line (&end);

    if (gtk_text_iter_starts_line (&end) && first_line != last_line)
        last_line -= 1;

    gtk_text_buffer_begin_user_action (buffer);
    moo_indenter_shift_lines (view->priv->indenter, buffer, first_line, last_line, -1);
    gtk_text_buffer_end_user_action (buffer);

    gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (view),
                                        gtk_text_buffer_get_insert (buffer));
    return TRUE;
}


static gboolean
tab_indent (MooTextView *view)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
    GtkTextIter insert, bound, start, end;
    gboolean starts_line, insert_last, has_selection;
    int first_line, last_line;

    if (!view->priv->indenter)
        return FALSE;

    has_selection = gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

    gtk_text_buffer_begin_user_action (buffer);

    if (has_selection)
    {
        gtk_text_buffer_get_iter_at_mark (buffer, &insert,
                                          gtk_text_buffer_get_insert (buffer));
        gtk_text_buffer_get_iter_at_mark (buffer, &bound,
                                          gtk_text_buffer_get_selection_bound (buffer));

        insert_last = (gtk_text_iter_compare (&insert, &bound) > 0);
        starts_line = gtk_text_iter_starts_line (&start);

        first_line = gtk_text_iter_get_line (&start);
        last_line = gtk_text_iter_get_line (&end);

        if (gtk_text_iter_starts_line (&end))
            last_line -= 1;

        moo_indenter_shift_lines (view->priv->indenter, buffer, first_line, last_line, 1);

        if (starts_line)
        {
            if (insert_last)
            {
                gtk_text_buffer_get_iter_at_mark (buffer, &insert, gtk_text_buffer_get_selection_bound (buffer));
                gtk_text_iter_set_line_offset (&insert, 0);
                gtk_text_buffer_move_mark_by_name (buffer, "selection_bound", &insert);
            }
            else
            {
                gtk_text_buffer_get_iter_at_mark (buffer, &insert, gtk_text_buffer_get_insert (buffer));
                gtk_text_iter_set_line_offset (&insert, 0);
                gtk_text_buffer_move_mark_by_name (buffer, "insert", &insert);
            }
        }
    }
    else
    {
        moo_indenter_tab (view->priv->indenter, buffer);
    }

    gtk_text_buffer_end_user_action (buffer);
    gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (view),
                                        gtk_text_buffer_get_insert (buffer));
    return TRUE;
}


void
moo_text_view_indent (MooTextView *view)
{
    g_return_if_fail (MOO_IS_TEXT_VIEW (view));
    tab_indent (view);
}


void
moo_text_view_unindent (MooTextView *view)
{
    g_return_if_fail (MOO_IS_TEXT_VIEW (view));
    tab_unindent (view);
}


static void
move_focus (GtkWidget       *widget,
            GtkDirectionType direction)
{
    GtkWidget *toplevel = gtk_widget_get_toplevel (widget);
    g_signal_emit_by_name (toplevel, "move-focus", direction);
}

static gboolean
handle_tab (MooTextView *view,
            GdkEventKey *event)
{
    switch (view->priv->tab_key_action)
    {
        case MOO_TEXT_TAB_KEY_DO_NOTHING:
            move_focus (GTK_WIDGET (view),
                        event->state & GDK_SHIFT_MASK ?
                                GTK_DIR_TAB_BACKWARD : GTK_DIR_TAB_FORWARD);
            return TRUE;

        case MOO_TEXT_TAB_KEY_FIND_PLACEHOLDER:
            if (event->state & GDK_SHIFT_MASK)
                moo_text_view_prev_placeholder (view);
            else
                moo_text_view_next_placeholder (view);
            return TRUE;

        case MOO_TEXT_TAB_KEY_INDENT:
            if (event->state & GDK_SHIFT_MASK)
                return tab_unindent (view);
            else
                return tab_indent (view);
    }

    g_return_val_if_reached (FALSE);
}


static gboolean
handle_backspace (MooTextView        *view,
                  G_GNUC_UNUSED GdkEventKey    *event)
{
    GtkTextBuffer *buffer;
    GtkTextIter start, end;
    int offset;
    guint new_offset;
    guint tab_width;
    char *insert = NULL;

    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

    if (!view->priv->backspace_indents || !view->priv->indenter)
        return FALSE;

    if (gtk_text_buffer_get_selection_bounds (buffer, NULL, NULL))
        return FALSE;

    gtk_text_buffer_get_iter_at_mark (buffer, &end,
                                      gtk_text_buffer_get_insert (buffer));

    if (gtk_text_iter_starts_line (&end))
        return FALSE;

    tab_width = view->priv->indenter->tab_width;
    offset = moo_iter_get_blank_offset (&end, tab_width);

    if (offset < 0)
        return FALSE;

    gtk_text_buffer_begin_user_action (buffer);

    if (offset <= 1)
        new_offset = 0;
    else
        new_offset = moo_text_iter_get_prev_stop (&end, tab_width, offset - 1, FALSE);

    start = end;
    gtk_text_iter_set_line_offset (&start, 0);
    gtk_text_buffer_delete (buffer, &start, &end);
    insert = moo_indenter_make_space (view->priv->indenter, new_offset, 0);

    if (insert)
    {
        gtk_text_buffer_insert (buffer, &start, insert, -1);
        g_free (insert);
    }

    gtk_text_buffer_end_user_action (buffer);

    gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (view),
                                        gtk_text_buffer_get_insert (buffer));
    return TRUE;
}


static gboolean
handle_enter (MooTextView        *view,
              G_GNUC_UNUSED GdkEventKey    *event)
{
    GtkTextBuffer *buffer;
    GtkTextIter start, end;
    gboolean has_selection;

    if (!view->priv->indenter || !view->priv->enter_indents)
        return FALSE;

    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
    has_selection = gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

    gtk_text_buffer_begin_user_action (buffer);

    if (has_selection)
        gtk_text_buffer_delete (buffer, &start, &end);
    /* XXX insert "\r\n" on windows? */
    gtk_text_buffer_insert (buffer, &start, "\n", 1);
    moo_indenter_character (view->priv->indenter, '\n', &start);

    gtk_text_buffer_end_user_action (buffer);

    gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (view),
                                        gtk_text_buffer_get_insert (buffer));
    return TRUE;
}


static gboolean
handle_ctrl_up (MooTextView        *view,
                G_GNUC_UNUSED GdkEventKey    *event,
                gboolean        up)
{
    GtkTextView *text_view;
    GtkAdjustment *adjustment;
    double value;
    int line_height;

    if (!view->priv->ctrl_up_down_scrolls)
        return FALSE;

    text_view = GTK_TEXT_VIEW (view);
    adjustment = text_view->vadjustment;

    if (!adjustment)
        return FALSE;

    line_height = _moo_text_view_get_line_height (view);

    if (up)
    {
        value = adjustment->value - line_height;

        if (value < adjustment->lower)
            value = adjustment->lower;
    }
    else
    {
        value = adjustment->value + line_height;

        if (value > adjustment->upper - adjustment->page_size)
            value = adjustment->upper - adjustment->page_size;
    }

    gtk_adjustment_set_value (adjustment, value);

    return TRUE;
}


static gboolean
handle_ctrl_pgup (MooTextView        *view,
                  G_GNUC_UNUSED GdkEventKey    *event,
                  gboolean        up)
{
    GtkTextView *text_view;
    GtkAdjustment *adjustment;
    double value;

    if (!view->priv->ctrl_page_up_down_scrolls)
        return FALSE;

    text_view = GTK_TEXT_VIEW (view);
    adjustment = text_view->vadjustment;

    if (!adjustment)
        return FALSE;

    if (up)
    {
        value = adjustment->value - adjustment->page_increment;
        if (value < adjustment->lower)
            value = adjustment->lower;
        gtk_adjustment_set_value (adjustment, value);
    }
    else
    {
        value = adjustment->value + adjustment->page_increment;
        if (value > adjustment->upper - adjustment->page_size)
            value = adjustment->upper - adjustment->page_size;
        gtk_adjustment_set_value (adjustment, value);
    }

    return TRUE;
}
