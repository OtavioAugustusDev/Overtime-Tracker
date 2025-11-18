#include <gtk/gtk.h>
#include "interface.h"

GtkWidget* create_input_text(char* title, char* placeholder, int secret)
{
    GtkWidget* input_label = gtk_label_new(title);
    gtk_widget_set_halign(input_label, GTK_ALIGN_START);

    GtkWidget* input_text = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(input_text), placeholder);
    gtk_entry_set_visibility(GTK_ENTRY(input_text), secret);

    return input_text;
}

GtkWidget* create_window(GtkApplication* app, char* title, int width, int height)
{
    GtkWidget* window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), title);
    gtk_window_set_default_size(GTK_WINDOW(window), width, height);
    return window;
}
