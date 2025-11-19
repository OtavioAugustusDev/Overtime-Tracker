#include <gtk/gtk.h>
#include "interface.h"

GtkWidget* create_input_text(const char* title, const char* placeholder, int secret)
{
    GtkWidget* input_text = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(input_text), placeholder);
    gtk_entry_set_visibility(GTK_ENTRY(input_text), !secret);

    return input_text;
}

GtkWidget* create_window(GtkApplication* app, const char* title, int width, int height)
{
    GtkWidget* window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), title);
    gtk_window_set_default_size(GTK_WINDOW(window), width, height);
    return window;
}

GtkWidget* create_button(
    const char* label,
    ButtonType type,
    GCallback callback,
    gpointer user_data,
    const char* data_key,
    gpointer data_value,
    int size[2],
    int margin[4]
)
{
    GtkWidget* button = gtk_button_new_with_label(label);

    switch (type) {
        case PILL:
            gtk_widget_add_css_class(button, "pill");
            break;
        case SUGGESTED:
            gtk_widget_add_css_class(button, "pill");
            gtk_widget_add_css_class(button, "suggested-action");
            break;
        case DESTRUCTIVE:
            gtk_widget_add_css_class(button, "pill");
            gtk_widget_add_css_class(button, "destructive-action");
            break;
        case NORMAL:
        default:
            break;
    }

    if (callback != NULL)
    {
        g_signal_connect(button, "clicked", callback, user_data);
    }

    if (data_key != NULL && data_value != NULL)
    {
        g_object_set_data(G_OBJECT(button), data_key, data_value);
    }

    if (size != NULL)
    {
        gtk_widget_set_size_request(button, size[0], size[1]);
    }

    if (margin != NULL)
    {
        if (margin[0] > 0) gtk_widget_set_margin_top(button, margin[0]);
        if (margin[1] > 0) gtk_widget_set_margin_bottom(button, margin[1]);
        if (margin[2] > 0) gtk_widget_set_margin_start(button, margin[2]);
        if (margin[3] > 0) gtk_widget_set_margin_end(button, margin[3]);
    }

    return button;
}
