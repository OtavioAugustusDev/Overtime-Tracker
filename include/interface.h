#ifndef INTERFACE_H
#define INTERFACE_H

GtkWidget* create_input_text(char* title, char* placeholder, int secret);
GtkWidget* create_window(GtkApplication* app, char* title, int width, int height);

#endif
