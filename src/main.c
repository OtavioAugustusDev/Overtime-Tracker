#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <locale.h>
#include <mysql.h>
#include <gtk/gtk.h>

#include "database.h"
#include "interface.h"

#define SYSTEM_LOCK_TIME 999

// ==================== ESTRUTURAS ====================

typedef struct {
    int user_id;
    char username[100];
    double overtime_hours;
    double work_hours;
    char role[10];
} User;

typedef struct {
    GtkWidget* username_input;
    GtkWidget* password_input;
    GtkWidget* output_label;
    GtkWidget* login_window;
    MYSQL* socket;
} Login_data;

typedef struct {
    GtkWidget* window;
    GtkWidget* stack;
    MYSQL* socket;
    User user;
} App_data;

typedef struct {
    GtkWidget* calendar;
    GtkWidget* hours_input;
    GtkWidget* notes_input;
    GtkWidget* output_label;
    App_data* app_data;
} Request_data;

typedef struct {
    GtkWidget* list_container;
    App_data* app_data;
    guint timeout_id;
} Tracking_data;

// ==================== FUNÇÕES AUXILIARES ====================

static gboolean is_system_closed()
{
    GDateTime* now = g_date_time_new_now_local();
    int hour = g_date_time_get_hour(now);
    g_date_time_unref(now);

    return (hour >= SYSTEM_LOCK_TIME);
}

static void on_navigation_button_clicked(GtkWidget* button, gpointer stack)
{
    const char* page_name = g_object_get_data(G_OBJECT(button), "stack-child-name");
    gtk_stack_set_visible_child_name(GTK_STACK(stack), page_name);
}

static gboolean go_to_dashboard(gpointer stack)
{
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "dashboard");
    return G_SOURCE_REMOVE;
}

// ==================== TRACKING LIST ====================

static gboolean refresh_tracking_list(gpointer data)
{
    Tracking_data* tracking_data = (Tracking_data*)data;

    // Limpar lista
    GtkWidget* child = gtk_widget_get_first_child(tracking_data->list_container);
    while (child != NULL)
    {
        GtkWidget* next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(tracking_data->list_container), child);
        child = next;
    }

    MYSQL_RES* result = get_user_requests_list(tracking_data->app_data->socket,
                                                tracking_data->app_data->user.user_id);

    if (!result || mysql_num_rows(result) == 0)
    {
        GtkWidget* empty_label = gtk_label_new("Você ainda não fez nenhum requerimento.");
        gtk_widget_add_css_class(empty_label, "dim-label");
        gtk_box_append(GTK_BOX(tracking_data->list_container), empty_label);
    }
    else
    {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result)))
        {
            GtkWidget* card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
            gtk_widget_set_margin_start(card, 12);
            gtk_widget_set_margin_end(card, 12);
            gtk_widget_set_margin_top(card, 8);
            gtk_widget_set_margin_bottom(card, 8);
            gtk_widget_add_css_class(card, "card");

            GtkWidget* header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

            char date_text[64];
            snprintf(date_text, sizeof(date_text), "%s", row[1]);
            GtkWidget* date_label = gtk_label_new(date_text);
            gtk_widget_set_halign(date_label, GTK_ALIGN_START);
            gtk_widget_add_css_class(date_label, "heading");
            gtk_box_append(GTK_BOX(header_box), date_label);

            char status_text[64];
            snprintf(status_text, sizeof(status_text), "Status: %s", row[4]);
            GtkWidget* status_label = gtk_label_new(status_text);
            gtk_widget_set_halign(status_label, GTK_ALIGN_END);
            gtk_widget_set_hexpand(status_label, TRUE);
            gtk_widget_add_css_class(status_label, "dim-label");
            gtk_box_append(GTK_BOX(header_box), status_label);

            gtk_box_append(GTK_BOX(card), header_box);

            char hours_text[64];
            snprintf(hours_text, sizeof(hours_text), "Horas solicitadas: %s", row[2]);
            GtkWidget* hours_label = gtk_label_new(hours_text);
            gtk_widget_set_halign(hours_label, GTK_ALIGN_START);
            gtk_box_append(GTK_BOX(card), hours_label);

            if (strlen(row[3]) > 0)
            {
                char notes_text[256];
                snprintf(notes_text, sizeof(notes_text), "Observações: %s", row[3]);
                GtkWidget* notes_label = gtk_label_new(notes_text);
                gtk_widget_set_halign(notes_label, GTK_ALIGN_START);
                gtk_label_set_wrap(GTK_LABEL(notes_label), TRUE);
                gtk_box_append(GTK_BOX(card), notes_label);
            }

            gtk_box_append(GTK_BOX(card), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
            gtk_box_append(GTK_BOX(tracking_data->list_container), card);
        }
    }

    if (result) mysql_free_result(result);
    return G_SOURCE_CONTINUE;
}

// ==================== DASHBOARD VIEW ====================

typedef struct {
    GtkWidget* overtime_label;
    App_data* app_data;
    guint timeout_id;
} DashboardData;

static void cleanup_dashboard_data(gpointer data)
{
    DashboardData* dashboard_data = (DashboardData*)data;

    if (dashboard_data->timeout_id > 0)
    {
        g_source_remove(dashboard_data->timeout_id);
        dashboard_data->timeout_id = 0;
    }

    g_free(dashboard_data);
}

static gboolean refresh_dashboard_balance(gpointer data)
{
    DashboardData* dashboard_data = (DashboardData*)data;

    // Recarrega dados do usuário
    MYSQL_ROW user_data = load_user_data(dashboard_data->app_data->socket,
                                         dashboard_data->app_data->user.user_id);

    if (user_data)
    {
        dashboard_data->app_data->user.overtime_hours = atof(user_data[2]);

        // Atualiza o label
        char overtime_text[100];
        snprintf(overtime_text, sizeof(overtime_text), "%.1f horas",
                dashboard_data->app_data->user.overtime_hours);
        gtk_label_set_text(GTK_LABEL(dashboard_data->overtime_label), overtime_text);
    }

    return G_SOURCE_CONTINUE;
}

GtkWidget* create_dashboard_view(App_data* app_data)
{
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(vbox, 40);
    gtk_widget_set_margin_end(vbox, 40);
    gtk_widget_set_margin_top(vbox, 40);
    gtk_widget_set_margin_bottom(vbox, 40);

    char greeting_text[300];
    snprintf(greeting_text, sizeof(greeting_text), "Olá, %s! 👋", app_data->user.username);
    GtkWidget* greeting = gtk_label_new(greeting_text);
    gtk_widget_add_css_class(greeting, "title-1");
    gtk_box_append(GTK_BOX(vbox), greeting);

    GtkWidget* info_frame = gtk_frame_new(NULL);
    gtk_widget_set_margin_top(info_frame, 20);
    gtk_widget_set_margin_bottom(info_frame, 20);
    gtk_widget_set_margin_start(info_frame, 10);
    gtk_widget_set_margin_end(info_frame, 10);
    gtk_widget_set_hexpand(info_frame, TRUE);
    gtk_widget_set_vexpand(info_frame, FALSE);

    GtkWidget* info_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(info_card, 20);
    gtk_widget_set_margin_bottom(info_card, 20);
    gtk_widget_set_margin_start(info_card, 20);
    gtk_widget_set_margin_end(info_card, 20);
    gtk_frame_set_child(GTK_FRAME(info_frame), info_card);

    GtkWidget* label_title = gtk_label_new("Saldo no banco de horas:");
    gtk_widget_set_halign(label_title, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(label_title, "dim-label");
    gtk_box_append(GTK_BOX(info_card), label_title);

    char overtime_text[100];
    snprintf(overtime_text, sizeof(overtime_text), "%.1f horas", app_data->user.overtime_hours);
    GtkWidget* overtime_label = gtk_label_new(overtime_text);
    gtk_widget_set_halign(overtime_label, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(overtime_label, "title-1");
    gtk_box_append(GTK_BOX(info_card), overtime_label);

    char workhours_text[100];
    snprintf(workhours_text, sizeof(workhours_text), "Carga Horária Semanal: %.1f horas",
             app_data->user.work_hours);
    GtkWidget* workhours_label = gtk_label_new(workhours_text);
    gtk_widget_set_halign(workhours_label, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(workhours_label, "dim-label");
    gtk_box_append(GTK_BOX(info_card), workhours_label);

    gtk_box_append(GTK_BOX(vbox), info_frame);

    GtkWidget* button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);

    GtkWidget* request_button;

    if (app_data->user.overtime_hours <= 0)
    {
        int size[] = {-1, -1}, margin[] = {0, 0, 0, 0};
        request_button = create_button("Sem saldo de horas extras", NORMAL, NULL, NULL,
                                      NULL, NULL, size, margin);
        gtk_widget_set_sensitive(request_button, FALSE);
    }
    else if (is_system_closed())
    {
        int size[] = {-1, -1}, margin[] = {0, 0, 0, 0};
        request_button = create_button("Sistema fechado para requerimentos", NORMAL, NULL, NULL,
                                      NULL, NULL, size, margin);
        gtk_widget_set_sensitive(request_button, FALSE);
    }
    else
    {
        int size[] = {-1, -1}, margin[] = {0, 0, 0, 0};
        request_button = create_button("📝 Novo requerimento", SUGGESTED,
                                      G_CALLBACK(on_navigation_button_clicked), app_data->stack,
                                      "stack-child-name", "request", size, margin);
    }

    gtk_box_append(GTK_BOX(button_box), request_button);
    gtk_box_append(GTK_BOX(vbox), button_box);

    GtkWidget* section_title = gtk_label_new("Seus requerimentos");
    gtk_widget_add_css_class(section_title, "title-2");
    gtk_box_append(GTK_BOX(vbox), section_title);

    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);

    GtkWidget* list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), list_box);
    gtk_box_append(GTK_BOX(vbox), scrolled);

    Tracking_data* tracking_data = g_malloc(sizeof(Tracking_data));
    tracking_data->list_container = list_box;
    tracking_data->app_data = app_data;

    refresh_tracking_list(tracking_data);
    tracking_data->timeout_id = g_timeout_add_seconds(2, refresh_tracking_list, tracking_data);

    g_object_set_data_full(G_OBJECT(vbox), "tracking_data", tracking_data,
                          (GDestroyNotify)g_free);

    // Cria estrutura para atualizar saldo
    DashboardData* dashboard_data = g_malloc(sizeof(DashboardData));
    dashboard_data->overtime_label = overtime_label;
    dashboard_data->app_data = app_data;
    dashboard_data->timeout_id = g_timeout_add_seconds(2, refresh_dashboard_balance, dashboard_data);

    g_object_set_data_full(G_OBJECT(vbox), "dashboard_data", dashboard_data,
                          cleanup_dashboard_data);

    return vbox;
}

// ==================== REQUEST VIEW ====================

static void on_hours_changed(GtkRange* range, gpointer user_data)
{
    GtkLabel* value_label = GTK_LABEL(user_data);
    double value = gtk_range_get_value(range);

    char text[16];
    snprintf(text, sizeof(text), "%.0f", value);
    gtk_label_set_text(value_label, text);
}

static void on_submit_request(GtkWidget* widget, gpointer data)
{
    Request_data* req_data = (Request_data*)data;

    GDateTime* gdate = gtk_calendar_get_date(GTK_CALENDAR(req_data->calendar));
    char* date = g_date_time_format(gdate, "%Y-%m-%d");

    GDateTime* now = g_date_time_new_now_local();

    if (g_date_time_compare(gdate, now) <= 0)
    {
        gtk_label_set_text(GTK_LABEL(req_data->output_label),
                          "Por favor, escolha uma data futura!");
        g_date_time_unref(gdate);
        g_date_time_unref(now);
        g_free(date);
        return;
    }

    g_date_time_unref(now);

    double hours_val = gtk_range_get_value(GTK_RANGE(req_data->hours_input));

    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(req_data->notes_input));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char* notes = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    if (hours_val <= 0)
    {
        gtk_label_set_text(GTK_LABEL(req_data->output_label),
                          "Por favor, selecione uma quantidade de horas!");
        g_free(notes);
        g_free(date);
        g_date_time_unref(gdate);
        return;
    }

    if (create_time_off_request(req_data->app_data->socket,
                                 req_data->app_data->user.user_id,
                                 date, hours_val, notes))
    {
        gtk_label_set_text(GTK_LABEL(req_data->output_label),
                          "Requerimento enviado com sucesso!");

        GDateTime* today = g_date_time_new_now_local();
        gtk_calendar_select_day(GTK_CALENDAR(req_data->calendar), today);
        g_date_time_unref(today);

        gtk_range_set_value(GTK_RANGE(req_data->hours_input), 0);
        gtk_text_buffer_set_text(buffer, "", -1);

        g_timeout_add(500, go_to_dashboard, req_data->app_data->stack);
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(req_data->output_label),
                          "Erro ao enviar requerimento!");
    }

    g_free(notes);
    g_free(date);
    g_date_time_unref(gdate);
}

GtkWidget* create_request_view(App_data* app_data)
{
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(vbox, 20);
    gtk_widget_set_margin_end(vbox, 20);
    gtk_widget_set_margin_top(vbox, 20);
    gtk_widget_set_margin_bottom(vbox, 20);

    int back_size[] = {-1, -1}, back_margin[] = {0, 0, 0, 0};
    GtkWidget* back_button = create_button("← Voltar", PILL,
                                          G_CALLBACK(on_navigation_button_clicked), app_data->stack,
                                          "stack-child-name", "dashboard", back_size, back_margin);
    gtk_widget_set_halign(back_button, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(vbox), back_button);

    GtkWidget* title = gtk_label_new("Solicitar Folga");
    gtk_widget_add_css_class(title, "title-1");
    gtk_box_append(GTK_BOX(vbox), title);

    GtkWidget* date_label = gtk_label_new("Data da Folga:");
    gtk_widget_set_halign(date_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(vbox), date_label);

    GtkWidget* calendar = gtk_calendar_new();
    gtk_box_append(GTK_BOX(vbox), calendar);

    GtkWidget* hours_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget* hours_label = gtk_label_new("Quantidade de horas:");
    gtk_widget_set_halign(hours_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(hours_box), hours_label);

    GtkWidget* hours_value_label = gtk_label_new("0");
    gtk_box_append(GTK_BOX(hours_box), hours_value_label);
    gtk_box_append(GTK_BOX(vbox), hours_box);

    GtkWidget* hours_slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0,
                                                        app_data->user.overtime_hours, 1);
    gtk_scale_set_digits(GTK_SCALE(hours_slider), 0);
    gtk_box_append(GTK_BOX(vbox), hours_slider);
    g_signal_connect(hours_slider, "value-changed", G_CALLBACK(on_hours_changed),
                    hours_value_label);

    GtkWidget* notes_label = gtk_label_new("Observações para o gestor:");
    gtk_widget_set_halign(notes_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(vbox), notes_label);

    GtkWidget* notes_input = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(notes_input), GTK_WRAP_WORD);
    gtk_widget_set_size_request(notes_input, -1, 100);

    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), notes_input);
    gtk_box_append(GTK_BOX(vbox), scrolled);

    GtkWidget* output_label = gtk_label_new("");
    gtk_box_append(GTK_BOX(vbox), output_label);

    int submit_size[] = {-1, -1}, submit_margin[] = {0, 0, 0, 0};
    GtkWidget* submit_button = create_button("Enviar Requerimento", SUGGESTED, NULL, NULL,
                                            NULL, NULL, submit_size, submit_margin);
    gtk_box_append(GTK_BOX(vbox), submit_button);

    Request_data* req_data = g_malloc(sizeof(Request_data));
    req_data->calendar = calendar;
    req_data->hours_input = hours_slider;
    req_data->notes_input = notes_input;
    req_data->output_label = output_label;
    req_data->app_data = app_data;

    g_signal_connect(submit_button, "clicked", G_CALLBACK(on_submit_request), req_data);
    g_object_set_data_full(G_OBJECT(vbox), "request_data", req_data, (GDestroyNotify)g_free);

    return vbox;
}

// ==================== MANAGE USERS VIEW ====================

static void refresh_users_list(GtkWidget* list_box, MYSQL* socket);

static void validate_user_form(GtkWidget* widget, gpointer data)
{
    GtkDialog* dialog = GTK_DIALOG(data);
    GtkWidget* content = gtk_dialog_get_content_area(dialog);
    GtkWidget* grid = gtk_widget_get_first_child(content);

    GtkWidget* entry_name = gtk_grid_get_child_at(GTK_GRID(grid), 1, 0);
    GtkWidget* entry_pass = gtk_grid_get_child_at(GTK_GRID(grid), 1, 1);
    GtkWidget* entry_role = gtk_grid_get_child_at(GTK_GRID(grid), 1, 2);

    const char* name = gtk_editable_get_text(GTK_EDITABLE(entry_name));
    const char* pass = gtk_editable_get_text(GTK_EDITABLE(entry_pass));
    int role_index = gtk_combo_box_get_active(GTK_COMBO_BOX(entry_role));

    gboolean is_valid = (name && strlen(name) > 0) &&
                        (pass && strlen(pass) > 0) &&
                        (role_index >= 0);

    gtk_dialog_set_response_sensitive(dialog, GTK_RESPONSE_OK, is_valid);
}

static void on_edit_user_response(GtkDialog* dialog, int response_id, gpointer data)
{
    GtkWidget* list_box = GTK_WIDGET(data);
    App_data* app_data = g_object_get_data(G_OBJECT(list_box), "app_data");

    if (response_id == GTK_RESPONSE_OK)
    {
        GtkWidget* content = gtk_dialog_get_content_area(dialog);
        GtkWidget* grid = gtk_widget_get_first_child(content);

        GtkWidget* entry_name = gtk_grid_get_child_at(GTK_GRID(grid), 1, 0);
        GtkWidget* entry_pass = gtk_grid_get_child_at(GTK_GRID(grid), 1, 1);
        GtkWidget* entry_role = gtk_grid_get_child_at(GTK_GRID(grid), 1, 2);
        GtkWidget* entry_hours = gtk_grid_get_child_at(GTK_GRID(grid), 1, 3);

        const char* name = gtk_editable_get_text(GTK_EDITABLE(entry_name));
        const char* pass = gtk_editable_get_text(GTK_EDITABLE(entry_pass));
        const char* role = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(entry_role));
        int hours = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(entry_hours));

        int user_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dialog), "user_id"));

        if (!user_id)
        {
            create_user(app_data->socket, name, pass, role, hours);
        }
        else
        {
            update_user(app_data->socket, user_id, name, pass, role, hours);
        }

        refresh_users_list(list_box, app_data->socket);
    }

    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_edit_user_clicked(GtkWidget* widget, gpointer data)
{
    GtkWidget* list_box = GTK_WIDGET(data);
    App_data* app_data = g_object_get_data(G_OBJECT(list_box), "app_data");

    int user_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "user_id"));

    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        user_id ? "Editar Usuário" : "Novo Usuário",
        GTK_WINDOW(app_data->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "Cancelar", GTK_RESPONSE_CANCEL,
        "Salvar", GTK_RESPONSE_OK,
        NULL
    );

    gtk_window_set_default_size(GTK_WINDOW(dialog), 450, -1);

    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_widget_set_margin_top(content, 20);
    gtk_widget_set_margin_bottom(content, 20);
    gtk_widget_set_margin_start(content, 30);
    gtk_widget_set_margin_end(content, 30);

    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 15);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 15);
    gtk_widget_set_hexpand(grid, TRUE);
    gtk_box_append(GTK_BOX(content), grid);

    GtkWidget* label_name = gtk_label_new("Nome de usuário:");
    gtk_widget_set_halign(label_name, GTK_ALIGN_END);
    gtk_widget_add_css_class(label_name, "dim-label");
    gtk_grid_attach(GTK_GRID(grid), label_name, 0, 0, 1, 1);

    GtkWidget* entry_name = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_name), "Digite o nome de usuário");
    gtk_widget_set_hexpand(entry_name, TRUE);
    gtk_grid_attach(GTK_GRID(grid), entry_name, 1, 0, 1, 1);

    GtkWidget* label_pass = gtk_label_new("Senha:");
    gtk_widget_set_halign(label_pass, GTK_ALIGN_END);
    gtk_widget_add_css_class(label_pass, "dim-label");
    gtk_grid_attach(GTK_GRID(grid), label_pass, 0, 1, 1, 1);

    GtkWidget* entry_pass = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_pass), "Digite a senha");
    gtk_entry_set_visibility(GTK_ENTRY(entry_pass), FALSE);
    gtk_widget_set_hexpand(entry_pass, TRUE);
    gtk_grid_attach(GTK_GRID(grid), entry_pass, 1, 1, 1, 1);

    GtkWidget* label_role = gtk_label_new("Cargo:");
    gtk_widget_set_halign(label_role, GTK_ALIGN_END);
    gtk_widget_add_css_class(label_role, "dim-label");
    gtk_grid_attach(GTK_GRID(grid), label_role, 0, 2, 1, 1);

    GtkWidget* entry_role = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry_role), "USER");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry_role), "GESTOR");
    gtk_combo_box_set_active(GTK_COMBO_BOX(entry_role), -1);
    gtk_widget_set_hexpand(entry_role, TRUE);
    gtk_grid_attach(GTK_GRID(grid), entry_role, 1, 2, 1, 1);

    GtkWidget* label_hours = gtk_label_new("Horas semanais:");
    gtk_widget_set_halign(label_hours, GTK_ALIGN_END);
    gtk_widget_add_css_class(label_hours, "dim-label");
    gtk_grid_attach(GTK_GRID(grid), label_hours, 0, 3, 1, 1);

    GtkWidget* entry_hours = gtk_spin_button_new_with_range(1, 80, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry_hours), 40);
    gtk_widget_set_hexpand(entry_hours, TRUE);
    gtk_grid_attach(GTK_GRID(grid), entry_hours, 1, 3, 1, 1);

    GtkWidget* separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_margin_top(separator, 10);
    gtk_widget_set_margin_bottom(separator, 10);
    gtk_grid_attach(GTK_GRID(grid), separator, 0, 4, 2, 1);

    g_object_set_data(G_OBJECT(dialog), "user_id", GINT_TO_POINTER(user_id));

    g_signal_connect(entry_name, "changed", G_CALLBACK(validate_user_form), dialog);
    g_signal_connect(entry_pass, "changed", G_CALLBACK(validate_user_form), dialog);
    g_signal_connect(entry_role, "changed", G_CALLBACK(validate_user_form), dialog);

    validate_user_form(NULL, dialog);

    g_signal_connect(dialog, "response", G_CALLBACK(on_edit_user_response), list_box);

    gtk_window_present(GTK_WINDOW(dialog));
}

static void on_delete_user_clicked(GtkWidget* widget, gpointer data)
{
    GtkWidget* list_box = GTK_WIDGET(data);
    int user_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "user_id"));

    MYSQL* socket = ((App_data*)g_object_get_data(G_OBJECT(list_box), "app_data"))->socket;
    delete_user(socket, user_id);

    refresh_users_list(list_box, socket);
}

static void refresh_users_list(GtkWidget* list_box, MYSQL* socket)
{
    GtkWidget* child = gtk_widget_get_first_child(list_box);
    while (child != NULL)
    {
        GtkWidget* next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(list_box), child);
        child = next;
    }

    MYSQL_RES* result = get_users_list(socket);
    if (!result) return;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)))
    {
        GtkWidget* user_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

        GtkWidget* info_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        char id_text[32];
        snprintf(id_text, sizeof(id_text), "ID: %s", row[0]);
        GtkWidget* label_id = gtk_label_new(id_text);
        gtk_widget_add_css_class(label_id, "dim-label");
        gtk_widget_set_halign(label_id, GTK_ALIGN_START);

        char name_text[64];
        snprintf(name_text, sizeof(name_text), "%s", row[1]);
        GtkWidget* label_name = gtk_label_new(name_text);
        gtk_widget_add_css_class(label_name, "heading");
        gtk_widget_set_halign(label_name, GTK_ALIGN_START);

        gtk_box_append(GTK_BOX(info_box), label_name);
        gtk_box_append(GTK_BOX(info_box), label_id);

        gtk_box_append(GTK_BOX(user_row), info_box);

        GtkWidget* button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        gtk_widget_set_halign(button_box, GTK_ALIGN_END);
        gtk_widget_set_hexpand(button_box, TRUE);

        int edit_size[] = {-1, -1}, edit_margin[] = {0, 0, 0, 0};
        GtkWidget* edit_button = create_button("Editar", PILL,
                                              G_CALLBACK(on_edit_user_clicked), list_box,
                                              "user_id", GINT_TO_POINTER(atoi(row[0])),
                                              edit_size, edit_margin);
        gtk_box_append(GTK_BOX(button_box), edit_button);

        int delete_size[] = {-1, -1}, delete_margin[] = {0, 0, 0, 0};
        GtkWidget* delete_button = create_button("Excluir", DESTRUCTIVE,
                                                G_CALLBACK(on_delete_user_clicked), list_box,
                                                "user_id", GINT_TO_POINTER(atoi(row[0])),
                                                delete_size, delete_margin);
        gtk_box_append(GTK_BOX(button_box), delete_button);

        gtk_box_append(GTK_BOX(user_row), button_box);
        gtk_box_append(GTK_BOX(list_box), user_row);
    }

    mysql_free_result(result);
}

GtkWidget* create_manage_users_view(App_data* app_data)
{
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(vbox, 20);
    gtk_widget_set_margin_end(vbox, 20);
    gtk_widget_set_margin_top(vbox, 20);
    gtk_widget_set_margin_bottom(vbox, 20);

    int back_size[] = {-1, -1}, back_margin[] = {0, 0, 0, 0};
    GtkWidget* back_button = create_button("← Voltar", PILL,
                                          G_CALLBACK(on_navigation_button_clicked), app_data->stack,
                                          "stack-child-name", "dashboard", back_size, back_margin);
    gtk_widget_set_halign(back_button, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(vbox), back_button);

    GtkWidget* title = gtk_label_new("Gerenciar Usuários");
    gtk_widget_add_css_class(title, "title-1");
    gtk_box_append(GTK_BOX(vbox), title);

    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);
    GtkWidget* list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), list_box);
    gtk_box_append(GTK_BOX(vbox), scrolled);

    int create_size[] = {-1, -1}, create_margin[] = {0, 0, 0, 0};
    GtkWidget* create__user_button = create_button("➕ Criar novo usuário", SUGGESTED,
                                            G_CALLBACK(on_edit_user_clicked), list_box,
                                            NULL, NULL, create_size, create_margin);
    gtk_box_append(GTK_BOX(vbox), create__user_button);

    g_object_set_data(G_OBJECT(list_box), "app_data", app_data);
    refresh_users_list(list_box, app_data->socket);

    return vbox;
}

// ==================== MANAGE REQUESTS VIEW ====================

typedef struct {
    GtkWidget* list_box;
    App_data* app_data;
    guint timeout_id;
} RequestsAdminData;

static void cleanup_requests_admin_data(gpointer data)
{
    RequestsAdminData* admin_data = (RequestsAdminData*)data;

    // Cancela o timeout antes de destruir
    if (admin_data->timeout_id > 0)
    {
        g_source_remove(admin_data->timeout_id);
        admin_data->timeout_id = 0;
    }

    g_free(admin_data);
}

static void refresh_requests_admin_list(GtkWidget* list_box, App_data* app_data);

static void on_update_request_status_clicked(GtkWidget* widget, gpointer data)
{
    RequestsAdminData* admin_data = (RequestsAdminData*)data;

    int request_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "request_id"));
    const char* new_status = g_object_get_data(G_OBJECT(widget), "new_status");

    answer_request(admin_data->app_data->socket, new_status, request_id);

    // Atualiza lista imediatamente
    refresh_requests_admin_list(admin_data->list_box, admin_data->app_data);
}

static void refresh_requests_admin_list(GtkWidget* list_box, App_data* app_data)
{
    GtkWidget* child = gtk_widget_get_first_child(list_box);
    while (child != NULL)
    {
        GtkWidget* next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(list_box), child);
        child = next;
    }

    MYSQL_RES* result = get_requests_list(app_data->socket);
    if (!result) return;

    if (mysql_num_rows(result) > 0)
    {
        MYSQL_ROW request_data;
        while ((request_data = mysql_fetch_row(result)))
        {
            int id = atoi(request_data[0]);
            char* username = request_data[1];
            char* date = request_data[2];
            char* hours = request_data[3];
            char* notes = request_data[4];
            char* status = request_data[5];

            GtkWidget* card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
            gtk_widget_set_margin_start(card, 12);
            gtk_widget_set_margin_end(card, 12);
            gtk_widget_set_margin_top(card, 8);
            gtk_widget_set_margin_bottom(card, 8);
            gtk_widget_add_css_class(card, "card");

            GtkWidget* header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

            char title_text[256];
            snprintf(title_text, sizeof(title_text), "Funcionário: %s", username);
            GtkWidget* title_label = gtk_label_new(title_text);
            gtk_widget_set_halign(title_label, GTK_ALIGN_START);
            gtk_widget_add_css_class(title_label, "heading");
            gtk_box_append(GTK_BOX(header), title_label);

            char status_text[128];
            snprintf(status_text, sizeof(status_text), "Status: %s", status);
            GtkWidget* status_label = gtk_label_new(status_text);
            gtk_widget_set_halign(status_label, GTK_ALIGN_END);
            gtk_widget_set_hexpand(status_label, TRUE);
            gtk_widget_add_css_class(status_label, "dim-label");
            gtk_box_append(GTK_BOX(header), status_label);

            gtk_box_append(GTK_BOX(card), header);

            char date_text[128];
            snprintf(date_text, sizeof(date_text), "Data: %s", date);
            GtkWidget* date_label = gtk_label_new(date_text);
            gtk_widget_set_halign(date_label, GTK_ALIGN_START);
            gtk_box_append(GTK_BOX(card), date_label);

            char hours_text[128];
            snprintf(hours_text, sizeof(hours_text), "Horas solicitadas: %s", hours);
            GtkWidget* hours_label = gtk_label_new(hours_text);
            gtk_widget_set_halign(hours_label, GTK_ALIGN_START);
            gtk_box_append(GTK_BOX(card), hours_label);

            if (notes && strlen(notes) > 0)
            {
                char notes_text[512];
                snprintf(notes_text, sizeof(notes_text), "Observações: %s", notes);
                GtkWidget* notes_label = gtk_label_new(notes_text);
                gtk_widget_set_halign(notes_label, GTK_ALIGN_START);
                gtk_label_set_wrap(GTK_LABEL(notes_label), TRUE);
                gtk_box_append(GTK_BOX(card), notes_label);
            }

            GtkWidget* actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
            gtk_widget_set_halign(actions, GTK_ALIGN_CENTER);

            int approve_size[] = {-1, -1}, approve_margin[] = {0, 0, 0, 0};
            GtkWidget* approve_btn = create_button("Deferir", PILL, NULL, NULL,
                                                  NULL, NULL, approve_size, approve_margin);
            g_object_set_data(G_OBJECT(approve_btn), "request_id", GINT_TO_POINTER(id));
            g_object_set_data(G_OBJECT(approve_btn), "new_status", "APROVADO");

            int reject_size[] = {-1, -1}, reject_margin[] = {0, 0, 0, 0};
            GtkWidget* reject_btn = create_button("Indeferir", DESTRUCTIVE, NULL, NULL,
                                                 NULL, NULL, reject_size, reject_margin);
            g_object_set_data(G_OBJECT(reject_btn), "request_id", GINT_TO_POINTER(id));
            g_object_set_data(G_OBJECT(reject_btn), "new_status", "NEGADO");

            // Pega os dados do container pai
            RequestsAdminData* admin_data = g_object_get_data(G_OBJECT(list_box), "admin_data");

            g_signal_connect(approve_btn, "clicked",
                           G_CALLBACK(on_update_request_status_clicked), admin_data);
            gtk_box_append(GTK_BOX(actions), approve_btn);

            g_signal_connect(reject_btn, "clicked",
                           G_CALLBACK(on_update_request_status_clicked), admin_data);
            gtk_box_append(GTK_BOX(actions), reject_btn);

            if (strcmp(status, "APROVADO") == 0 || strcmp(status, "NEGADO") == 0)
            {
                gtk_widget_set_sensitive(approve_btn, FALSE);
                gtk_widget_set_sensitive(reject_btn, FALSE);
            }

            gtk_box_append(GTK_BOX(card), actions);
            gtk_box_append(GTK_BOX(card), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
            gtk_box_append(GTK_BOX(list_box), card);
        }
    }
    else
    {
        GtkWidget* empty_label = gtk_label_new("Nenhum requerimento encontrado.");
        gtk_widget_add_css_class(empty_label, "dim-label");
        gtk_box_append(GTK_BOX(list_box), empty_label);
    }

    mysql_free_result(result);
}

static gboolean refresh_requests_admin_periodic(gpointer data)
{
    RequestsAdminData* admin_data = (RequestsAdminData*)data;
    refresh_requests_admin_list(admin_data->list_box, admin_data->app_data);
    return G_SOURCE_CONTINUE;
}

GtkWidget* create_manage_requests_view(App_data* app_data)
{
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(vbox, 20);
    gtk_widget_set_margin_end(vbox, 20);
    gtk_widget_set_margin_top(vbox, 20);
    gtk_widget_set_margin_bottom(vbox, 20);

    int back_size[] = {-1, -1}, back_margin[] = {0, 0, 0, 0};
    GtkWidget* back_button = create_button("← Voltar", PILL,
                                          G_CALLBACK(on_navigation_button_clicked), app_data->stack,
                                          "stack-child-name", "dashboard", back_size, back_margin);
    gtk_widget_set_halign(back_button, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(vbox), back_button);

    GtkWidget* title = gtk_label_new("Gerir requerimentos");
    gtk_widget_add_css_class(title, "title-1");
    gtk_box_append(GTK_BOX(vbox), title);

    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);
    GtkWidget* list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), list_box);
    gtk_box_append(GTK_BOX(vbox), scrolled);

    // Cria estrutura de dados para gerenciar o timeout
    RequestsAdminData* admin_data = g_malloc(sizeof(RequestsAdminData));
    admin_data->list_box = list_box;
    admin_data->app_data = app_data;
    admin_data->timeout_id = 0;

    // Associa os dados ao list_box
    g_object_set_data(G_OBJECT(list_box), "admin_data", admin_data);

    // Carrega lista inicial
    refresh_requests_admin_list(list_box, app_data);

    // Agenda refresh periódico
    admin_data->timeout_id = g_timeout_add_seconds(2, refresh_requests_admin_periodic, admin_data);

    // Garante limpeza ao destruir o vbox
    g_object_set_data_full(G_OBJECT(vbox), "admin_data", admin_data,
                          cleanup_requests_admin_data);

    return vbox;
}

// ==================== MANAGER DASHBOARD VIEW ====================

typedef struct {
    GtkWidget* pending_label;
    MYSQL* socket;
    guint timeout_id;
} ManagerDashboardData;

static void cleanup_manager_dashboard_data(gpointer data)
{
    ManagerDashboardData* manager_data = (ManagerDashboardData*)data;

    if (manager_data->timeout_id > 0)
    {
        g_source_remove(manager_data->timeout_id);
        manager_data->timeout_id = 0;
    }

    g_free(manager_data);
}

static gboolean refresh_pending_count(gpointer data)
{
    ManagerDashboardData* manager_data = (ManagerDashboardData*)data;

    int pending_count = get_pending_requests_count(manager_data->socket);

    char pending_text[100];
    snprintf(pending_text, sizeof(pending_text), "%d", pending_count);
    gtk_label_set_text(GTK_LABEL(manager_data->pending_label), pending_text);

    return G_SOURCE_CONTINUE;
}

GtkWidget* create_manager_dashboard_view(App_data* app_data)
{
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(vbox, 40);
    gtk_widget_set_margin_end(vbox, 40);
    gtk_widget_set_margin_top(vbox, 40);
    gtk_widget_set_margin_bottom(vbox, 40);

    char greeting_text[300];
    snprintf(greeting_text, sizeof(greeting_text), "Olá, %s! 👋", app_data->user.username);
    GtkWidget* greeting = gtk_label_new(greeting_text);
    gtk_widget_add_css_class(greeting, "title-1");
    gtk_box_append(GTK_BOX(vbox), greeting);

    int pending_count = get_pending_requests_count(app_data->socket);

    GtkWidget* info_frame = gtk_frame_new(NULL);
    gtk_widget_set_margin_top(info_frame, 20);
    gtk_widget_set_margin_bottom(info_frame, 20);
    gtk_widget_set_margin_start(info_frame, 10);
    gtk_widget_set_margin_end(info_frame, 10);
    gtk_widget_set_hexpand(info_frame, TRUE);

    GtkWidget* info_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(info_card, 20);
    gtk_widget_set_margin_bottom(info_card, 20);
    gtk_widget_set_margin_start(info_card, 20);
    gtk_widget_set_margin_end(info_card, 20);
    gtk_frame_set_child(GTK_FRAME(info_frame), info_card);

    GtkWidget* label_title = gtk_label_new("Requerimentos pendentes:");
    gtk_widget_set_halign(label_title, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(label_title, "dim-label");
    gtk_box_append(GTK_BOX(info_card), label_title);

    char pending_text[100];
    snprintf(pending_text, sizeof(pending_text), "%d", pending_count);
    GtkWidget* pending_label = gtk_label_new(pending_text);
    gtk_widget_set_halign(pending_label, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(pending_label, "title-1");
    gtk_box_append(GTK_BOX(info_card), pending_label);

    gtk_box_append(GTK_BOX(vbox), info_frame);

    GtkWidget* button_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);

    int size1[] = {-1, -1}, margin1[] = {0, 0, 0, 0};
    GtkWidget* manage_users_button = create_button(
        "👥 Gerir Funcionários", SUGGESTED, G_CALLBACK(on_navigation_button_clicked),
        app_data->stack, "stack-child-name", "manage_users", size1, margin1
    );
    gtk_box_append(GTK_BOX(button_box), manage_users_button);

    int size2[] = {-1, -1}, margin2[] = {0, 0, 0, 0};
    GtkWidget* manage_requests_button = create_button(
        "📋 Gerir requerimentos", DESTRUCTIVE, G_CALLBACK(on_navigation_button_clicked),
        app_data->stack, "stack-child-name", "manage_requests", size2, margin2
    );
    gtk_box_append(GTK_BOX(button_box), manage_requests_button);

    gtk_box_append(GTK_BOX(vbox), button_box);

    // Cria estrutura para atualizar contador de pendentes
    ManagerDashboardData* manager_data = g_malloc(sizeof(ManagerDashboardData));
    manager_data->pending_label = pending_label;
    manager_data->socket = app_data->socket;
    manager_data->timeout_id = g_timeout_add_seconds(2, refresh_pending_count, manager_data);

    g_object_set_data_full(G_OBJECT(vbox), "manager_data", manager_data,
                          cleanup_manager_dashboard_data);

    return vbox;
}

// ==================== MAIN WINDOW ====================

void create_main_window(GtkApplication* app, MYSQL* socket, int user_id)
{
    App_data* app_data = g_malloc(sizeof(App_data));
    app_data->socket = socket;

    MYSQL_ROW user_data = load_user_data(socket, user_id);

    if (!user_data)
    {
        fprintf(stderr, "Erro ao carregar dados do usuário\n");
        exit(1);
    }

    app_data->user.user_id = atoi(user_data[0]);
    strncpy(app_data->user.username, user_data[1], sizeof(app_data->user.username) - 1);
    app_data->user.username[sizeof(app_data->user.username) - 1] = '\0';
    app_data->user.overtime_hours = atof(user_data[2]);
    app_data->user.work_hours = atof(user_data[3]);
    strncpy(app_data->user.role, user_data[4], sizeof(app_data->user.role) - 1);
    app_data->user.role[sizeof(app_data->user.role) - 1] = '\0';

    GtkWidget* window = create_window(app, "Sistema de banco de horas", 400, 500);
    app_data->window = window;

    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), main_box);

    GtkWidget* stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(GTK_STACK(stack), 200);
    app_data->stack = stack;

    if (strcmp(app_data->user.role, "GESTOR") == 0)
    {
        gtk_stack_add_named(GTK_STACK(stack), create_manager_dashboard_view(app_data), "dashboard");
        gtk_stack_add_named(GTK_STACK(stack), create_manage_users_view(app_data), "manage_users");
        gtk_stack_add_named(GTK_STACK(stack), create_manage_requests_view(app_data), "manage_requests");
    }
    else
    {
        gtk_stack_add_named(GTK_STACK(stack), create_dashboard_view(app_data), "dashboard");
        gtk_stack_add_named(GTK_STACK(stack), create_request_view(app_data), "request");
    }

    gtk_box_append(GTK_BOX(main_box), stack);
    gtk_widget_set_vexpand(stack, TRUE);

    gtk_window_present(GTK_WINDOW(window));
}

// ==================== LOGIN ====================

static void on_login_button_clicked(GtkWidget* widget, gpointer login_data)
{
    Login_data* data = login_data;

    const char* username = gtk_editable_get_text(GTK_EDITABLE(data->username_input));
    const char* password = gtk_editable_get_text(GTK_EDITABLE(data->password_input));

    if (strlen(username) == 0 || strlen(password) == 0)
    {
        gtk_label_set_text(GTK_LABEL(data->output_label), "Preencha todos os campos!");
        return;
    }

    int user_id = authenticate_user(data->socket, username, password);

    if (user_id)
    {
        GtkApplication* app = gtk_window_get_application(GTK_WINDOW(data->login_window));
        gtk_window_destroy(GTK_WINDOW(data->login_window));
        create_main_window(app, data->socket, user_id);
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(data->output_label), "Credenciais inválidas!");
    }
}

static void login_panel(GtkApplication* app, MYSQL* socket)
{
    GtkWidget* window = create_window(app, "Login - Sistema de Horas Extras", 400, 350);

    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), main_box);

    GtkWidget* center_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(center_box, 50);
    gtk_widget_set_margin_end(center_box, 50);
    gtk_widget_set_margin_top(center_box, 40);
    gtk_widget_set_margin_bottom(center_box, 40);
    gtk_widget_set_valign(center_box, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(center_box, TRUE);
    gtk_box_append(GTK_BOX(main_box), center_box);

    GtkWidget* header_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(header_box, GTK_ALIGN_CENTER);

    GtkWidget* icon_label = gtk_label_new("⏰");
    gtk_widget_add_css_class(icon_label, "title-1");
    gtk_box_append(GTK_BOX(header_box), icon_label);

    GtkWidget* title = gtk_label_new("Sistema de Horas Extras");
    gtk_widget_add_css_class(title, "title-2");
    gtk_box_append(GTK_BOX(header_box), title);

    GtkWidget* subtitle = gtk_label_new("Faça login para continuar");
    gtk_widget_add_css_class(subtitle, "dim-label");
    gtk_box_append(GTK_BOX(header_box), subtitle);

    gtk_box_append(GTK_BOX(center_box), header_box);

    gtk_box_append(GTK_BOX(center_box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    GtkWidget* username_label = gtk_label_new("ID de Colaborador");
    gtk_widget_set_halign(username_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(username_label, "heading");
    gtk_box_append(GTK_BOX(center_box), username_label);

    GtkWidget* username_input = create_input_text("ID de Colaborador", "Digite seu ID", 0);
    gtk_widget_set_size_request(username_input, -1, 40);
    gtk_box_append(GTK_BOX(center_box), username_input);

    GtkWidget* password_label = gtk_label_new("Senha");
    gtk_widget_set_halign(password_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(password_label, "heading");
    gtk_widget_set_margin_top(password_label, 10);
    gtk_box_append(GTK_BOX(center_box), password_label);

    GtkWidget* password_input = create_input_text("Senha", "Digite sua senha", 1);
    gtk_widget_set_size_request(password_input, -1, 40);
    gtk_box_append(GTK_BOX(center_box), password_input);

    GtkWidget* output_label = gtk_label_new("");
    gtk_widget_set_margin_top(output_label, 10);
    gtk_widget_add_css_class(output_label, "error");
    gtk_label_set_wrap(GTK_LABEL(output_label), TRUE);
    gtk_box_append(GTK_BOX(center_box), output_label);

    Login_data* login_data = g_malloc(sizeof(Login_data));
    login_data->username_input = username_input;
    login_data->password_input = password_input;
    login_data->output_label = output_label;
    login_data->login_window = window;
    login_data->socket = socket;

    int size[] = {-1, 45}, margin[] = {15, 0, 0, 0};
    GtkWidget* button_login = create_button("Entrar", SUGGESTED,
                                           G_CALLBACK(on_login_button_clicked), login_data,
                                           NULL, NULL, size, margin);

    gtk_box_append(GTK_BOX(center_box), button_login);

    GtkWidget* footer_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_margin_top(footer_box, 20);
    gtk_widget_set_halign(footer_box, GTK_ALIGN_CENTER);

    GtkWidget* footer_text = gtk_label_new("Projeto Integrador @ Universidade Vila Velha");
    gtk_widget_add_css_class(footer_text, "dim-label");
    gtk_widget_add_css_class(footer_text, "caption");
    gtk_box_append(GTK_BOX(footer_box), footer_text);

    gtk_box_append(GTK_BOX(center_box), footer_box);

    g_object_set_data_full(G_OBJECT(window), "login_data", login_data, (GDestroyNotify)g_free);

    gtk_window_present(GTK_WINDOW(window));
}

static void awake(GtkApplication* app, gpointer socket)
{
    login_panel(app, socket);
}

// ==================== MAIN ====================

int main()
{
    // Define locale para C (ponto decimal) para queries SQL
    setlocale(LC_NUMERIC, "C");

    MYSQL* socket = connect_to_database();

    GtkApplication* app = gtk_application_new("dev.otavio.overtime-tracker", 0);
    g_signal_connect(app, "activate", G_CALLBACK(awake), socket);
    int status = g_application_run(G_APPLICATION(app), 0, NULL);

    g_object_unref(app);
    mysql_close(socket);

    return status;
}
