#include "mooeditwindowoutput.h"
#include "moocmdview.h"
#include "mooutils/moostock.h"

GtkWidget *
moo_edit_window_get_output (MooEditWindow *window)
{
    MooPaneLabel *label;
    GtkWidget *cmd_view;
    GtkWidget *scrolled_window;

    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), NULL);

    scrolled_window = moo_edit_window_get_pane (window, "moo-edit-window-output");

    if (!scrolled_window)
    {
        scrolled_window = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                        GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                             GTK_SHADOW_ETCHED_IN);

        cmd_view = moo_cmd_view_new ();
        moo_text_view_set_font_from_string (MOO_TEXT_VIEW (cmd_view), "Monospace");
        gtk_container_add (GTK_CONTAINER (scrolled_window), cmd_view);
        gtk_widget_show_all (scrolled_window);
        g_object_set_data (G_OBJECT (scrolled_window), "moo-output", cmd_view);

        label = moo_pane_label_new (MOO_STOCK_TERMINAL, NULL, "Output", "Output");

        if (!moo_edit_window_add_pane (window, "moo-edit-window-output",
                                       scrolled_window, label, MOO_PANE_POS_BOTTOM))
        {
            g_critical ("%s: oops", G_STRLOC);
            moo_pane_label_free (label);
            return NULL;
        }

        moo_edit_window_add_stop_client (window, cmd_view);

        moo_pane_label_free (label);
        return cmd_view;
    }

    return g_object_get_data (G_OBJECT (scrolled_window), "moo-output");
}


GtkWidget *
moo_edit_window_get_output_pane (MooEditWindow *window)
{
    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), NULL);
    return moo_edit_window_get_pane (window, "moo-edit-window-output");
}


void
moo_edit_window_present_output (MooEditWindow *window)
{
    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    moo_edit_window_get_output (window);
    moo_big_paned_present_pane (window->paned,
                                moo_edit_window_get_output_pane (window));
}