#ifndef INTERFACE_H
#define INTERFACE_H

#include <gtk/gtk.h>

// Janela de alerta
void show_error_dialog(GtkWindow *parent, const char *message);

// Janela informativa
void show_info_dialog(GtkWindow *parent, const char *message);

// Janela de confirmação - Sim / Não
bool show_confirm_dialog(GtkWindow *parent, const char *message);

// Limpa o container
void clear_container(GtkWidget *container);

#endif
