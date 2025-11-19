#ifndef INTERFACE_H
#define INTERFACE_H

#include <gtk/gtk.h>

// Enumeração para tipos de botão
typedef enum {
    NORMAL,
    PILL,
    SUGGESTED,
    DESTRUCTIVE
} ButtonType;

// Funções de criação de widgets
GtkWidget* create_window(GtkApplication* app, const char* title, int width, int height);
GtkWidget* create_input_text(const char* title, const char* placeholder, int secret);
GtkWidget* create_button(
    const char* label,
    ButtonType type,
    GCallback callback,
    gpointer user_data,
    const char* data_key,
    gpointer data_value,
    int size[2],
    int margin[4]
);

#endif
