#include <time.h>
#include <gtk/gtk.h>

void show_error_dialog(GtkWindow *parent, const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(
        parent,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        "%s", message
    );
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void show_info_dialog(GtkWindow *parent, const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(
        parent,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "%s", message
    );
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

bool show_confirm_dialog(GtkWindow *parent, const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(
        parent,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        "%s", message
    );
    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return (result == GTK_RESPONSE_YES);
}

void clear_container(GtkWidget *container) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(container));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
}
