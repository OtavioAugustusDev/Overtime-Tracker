#include <gtk/gtk.h>
#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <stdbool.h>

#include "database.h"
#include "interface.h"

// Dados globais da aplicação
typedef struct {
    GtkBuilder *builder;
    MYSQL *db;
    int current_user_id;
    char current_username[256];
    char current_role[20];
    double current_overtime;
    double current_workhours;
} AppData;

AppData app_data;

#define SYSTEM_CLOSE_HOUR 99
#define SYSTEM_CLOSE_MINUTE 0

bool is_system_closed() {
    time_t now = time(NULL);
    struct tm *current_time = localtime(&now);

    int current_hour = current_time->tm_hour;
    int current_minute = current_time->tm_min;

    if (current_hour > SYSTEM_CLOSE_HOUR) {
        return true;
    }
    if (current_hour == SYSTEM_CLOSE_HOUR && current_minute >= SYSTEM_CLOSE_MINUTE) {
        return true;
    }

    return false;
}

// PROTOTYPES: https://www.geeksforgeeks.org/c/function-prototype-in-c/
void on_manage_users_clicked(GtkButton *button, gpointer user_data);
void on_manage_requests_clicked(GtkButton *button, gpointer user_data);
void update_dashboard();

/*
    UI
*/
void load_user_requests() {
    GtkBox *requests_box = GTK_BOX(gtk_builder_get_object(app_data.builder, "requests_list_box"));
    clear_container(GTK_WIDGET(requests_box));

    MYSQL_RES *result = get_user_requests_list(app_data.db, app_data.current_user_id);
    if (!result) return;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        GtkWidget *frame = gtk_frame_new(NULL);
        gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);

        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_set_border_width(GTK_CONTAINER(box), 10);

        char info[512];
        snprintf(info, sizeof(info),
                "<b>Data:</b> %s\n<b>Horas:</b> %s\n<b>Status:</b> %s\n<b>Observações:</b> %s",
                row[1], row[2], row[4], row[3]);

        GtkWidget *label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(label), info);
        gtk_label_set_xalign(GTK_LABEL(label), 0);
        gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);

        gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
        gtk_container_add(GTK_CONTAINER(frame), box);
        gtk_box_pack_start(GTK_BOX(requests_box), frame, FALSE, FALSE, 0);
    }

    mysql_free_result(result);
    gtk_widget_show_all(GTK_WIDGET(requests_box));
}

void update_request_button_state() {
    GtkButton *request_button = GTK_BUTTON(gtk_builder_get_object(app_data.builder, "request_button"));

    if (is_system_closed()) {
        gtk_button_set_label(request_button, "🔒 Sistema fechado");
        gtk_widget_set_sensitive(GTK_WIDGET(request_button), FALSE);
    } else if (app_data.current_overtime <= 0) {
        gtk_button_set_label(request_button, "⚠️ Sem saldo disponível");
        gtk_widget_set_sensitive(GTK_WIDGET(request_button), FALSE);
    } else {
        gtk_button_set_label(request_button, "📝 Novo requerimento");
        gtk_widget_set_sensitive(GTK_WIDGET(request_button), TRUE);
    }
}

void update_dashboard() {
    MYSQL_ROW user_data = load_user_data(app_data.db, app_data.current_user_id);
    if (user_data) {
        app_data.current_overtime = atof(user_data[2]);
        app_data.current_workhours = atof(user_data[3]);

        char greeting[256];
        snprintf(greeting, sizeof(greeting), "Olá, %s! 👋", app_data.current_username);

        GtkLabel *greeting_label = GTK_LABEL(gtk_builder_get_object(app_data.builder, "greeting_label"));
        gtk_label_set_text(greeting_label, greeting);

        char overtime_text[64];
        snprintf(overtime_text, sizeof(overtime_text), "%.2f horas", app_data.current_overtime);
        GtkLabel *overtime_label = GTK_LABEL(gtk_builder_get_object(app_data.builder, "overtime_label"));
        gtk_label_set_text(overtime_label, overtime_text);

        char workhours_text[128];
        snprintf(workhours_text, sizeof(workhours_text), "Carga Horária Semanal: %.2f horas", app_data.current_workhours);
        GtkLabel *workhours_label = GTK_LABEL(gtk_builder_get_object(app_data.builder, "workhours_label"));
        gtk_label_set_text(workhours_label, workhours_text);
    }

    update_request_button_state();
    load_user_requests();
}

void update_manager_dashboard() {
    int pending_count = get_pending_requests_count(app_data.db);

    char count_text[32];
    snprintf(count_text, sizeof(count_text), "%d", pending_count);

    GtkLabel *pending_label = GTK_LABEL(gtk_builder_get_object(app_data.builder, "pending_label"));
    gtk_label_set_text(pending_label, count_text);
}

/*
    CALLBACKS
*/
void on_approve_request_clicked(GtkButton *button, gpointer user_data) {
    int request_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "request_id"));

    GtkWindow *main_window = GTK_WINDOW(gtk_builder_get_object(app_data.builder, "main_window"));
    if (show_confirm_dialog(main_window, "Deseja aprovar este requerimento?")) {
        answer_request(app_data.db, "APROVADO", request_id);
        show_info_dialog(main_window, "Requerimento aprovado com sucesso!");

        // Recarrega lista
        on_manage_requests_clicked(NULL, NULL);
    }
}

void on_reject_request_clicked(GtkButton *button, gpointer user_data) {
    int request_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "request_id"));

    GtkWindow *main_window = GTK_WINDOW(gtk_builder_get_object(app_data.builder, "main_window"));
    if (show_confirm_dialog(main_window, "Deseja negar este requerimento?")) {
        answer_request(app_data.db, "NEGADO", request_id);
        show_info_dialog(main_window, "Requerimento negado.");

        // Recarrega lista
        on_manage_requests_clicked(NULL, NULL);
    }
}

void on_edit_user_clicked(GtkButton *button, gpointer user_data) {
    int user_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "user_id"));

    MYSQL_ROW user_data_row = load_user_data(app_data.db, user_id);
    if (!user_data_row) return;

    GtkDialog *dialog = GTK_DIALOG(gtk_builder_get_object(app_data.builder, "user_edit_dialog"));
    gtk_window_set_title(GTK_WINDOW(dialog), "Editar Usuário");

    GtkEntry *username = GTK_ENTRY(gtk_builder_get_object(app_data.builder, "entry_username"));
    GtkEntry *password = GTK_ENTRY(gtk_builder_get_object(app_data.builder, "entry_password"));
    GtkComboBoxText *role = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(app_data.builder, "entry_role"));
    GtkSpinButton *hours = GTK_SPIN_BUTTON(gtk_builder_get_object(app_data.builder, "entry_hours"));

    gtk_entry_set_text(username, user_data_row[1]);
    gtk_entry_set_text(password, user_data_row[5]);

    if (strcmp(user_data_row[4], "GESTOR") == 0) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(role), 1);
    } else {
        gtk_combo_box_set_active(GTK_COMBO_BOX(role), 0);
    }

    gtk_spin_button_set_value(hours, atof(user_data_row[3]));

    gint response = gtk_dialog_run(dialog);

    if (response == GTK_RESPONSE_OK) {
        const char *new_username = gtk_entry_get_text(username);
        const char *new_password = gtk_entry_get_text(password);
        const char *new_role = gtk_combo_box_text_get_active_text(role);
        int new_hours = gtk_spin_button_get_value_as_int(hours);

        if (strlen(new_username) > 0 && strlen(new_password) > 0 && new_role) {
            update_user(app_data.db, user_id, new_username, new_password, new_role, new_hours);

            GtkWindow *main_window = GTK_WINDOW(gtk_builder_get_object(app_data.builder, "main_window"));
            show_info_dialog(main_window, "Usuário atualizado com sucesso!");

            // Recarregar lista
            on_manage_users_clicked(NULL, NULL);
        }

        g_free((char*)new_role);
    }

    gtk_widget_hide(GTK_WIDGET(dialog));
}

void on_delete_user_clicked(GtkButton *button, gpointer user_data) {
    int user_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "user_id"));

    GtkWindow *main_window = GTK_WINDOW(gtk_builder_get_object(app_data.builder, "main_window"));
    if (show_confirm_dialog(main_window, "Tem certeza que deseja deletar este usuário?")) {
        delete_user(app_data.db, user_id);
        show_info_dialog(main_window, "Usuário deletado com sucesso!");

        // Recarrega lista
        on_manage_users_clicked(NULL, NULL);
    }
}

void on_login_button_clicked(GtkButton *button, gpointer user_data) {
    GtkEntry *username_entry = GTK_ENTRY(gtk_builder_get_object(app_data.builder, "username_input"));
    GtkEntry *password_entry = GTK_ENTRY(gtk_builder_get_object(app_data.builder, "password_input"));
    GtkLabel *output_label = GTK_LABEL(gtk_builder_get_object(app_data.builder, "output_label"));

    const char *username = gtk_entry_get_text(username_entry);
    const char *password = gtk_entry_get_text(password_entry);

    if (strlen(username) == 0 || strlen(password) == 0) {
        gtk_label_set_text(output_label, "Preencha todos os campos!");
        return;
    }

    int user_id = authenticate_user(app_data.db, username, password);

    if (user_id > 0) {
        app_data.current_user_id = user_id;
        strncpy(app_data.current_username, username, sizeof(app_data.current_username) - 1);

        MYSQL_ROW user_data_row = load_user_data(app_data.db, user_id);
        if (user_data_row) {
            strncpy(app_data.current_role, user_data_row[4], sizeof(app_data.current_role) - 1);

            GtkWindow *login_window = GTK_WINDOW(gtk_builder_get_object(app_data.builder, "login_window"));
            gtk_widget_hide(GTK_WIDGET(login_window));

            GtkWindow *main_window = GTK_WINDOW(gtk_builder_get_object(app_data.builder, "main_window"));
            GtkStack *stack = GTK_STACK(gtk_builder_get_object(app_data.builder, "main_stack"));

            if (strcmp(app_data.current_role, "GESTOR") == 0) {
                gtk_stack_set_visible_child_name(stack, "manager_dashboard");
                update_manager_dashboard();
            } else {
                gtk_stack_set_visible_child_name(stack, "dashboard");
                update_dashboard();
            }

            gtk_widget_show(GTK_WIDGET(main_window));
        }
    } else {
        gtk_label_set_text(output_label, "Credenciais inválidas.");
    }
}

void on_request_button_clicked(GtkButton *button, gpointer user_data) {
    GtkStack *stack = GTK_STACK(gtk_builder_get_object(app_data.builder, "main_stack"));
    gtk_stack_set_visible_child_name(stack, "request");

    GtkScale *slider = GTK_SCALE(gtk_builder_get_object(app_data.builder, "hours_slider"));
    GtkAdjustment *adjustment = gtk_range_get_adjustment(GTK_RANGE(slider));
    gtk_adjustment_set_upper(adjustment, app_data.current_overtime);
    gtk_adjustment_set_value(adjustment, 0);

    GtkCalendar *calendar = GTK_CALENDAR(gtk_builder_get_object(app_data.builder, "calendar"));

    time_t now = time(NULL);
    struct tm *today = localtime(&now);
    gtk_calendar_select_day(calendar, today->tm_mday + 1);
    gtk_calendar_select_month(calendar, today->tm_mon, today->tm_year + 1900);

    // Limpa formulário
    GtkLabel *output = GTK_LABEL(gtk_builder_get_object(app_data.builder, "request_output_label"));
    gtk_label_set_text(output, "");

    GtkLabel *hours_value = GTK_LABEL(gtk_builder_get_object(app_data.builder, "hours_value_label"));
    gtk_label_set_text(hours_value, "0");

    GtkTextView *notes = GTK_TEXT_VIEW(gtk_builder_get_object(app_data.builder, "notes_input"));
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(notes);
    gtk_text_buffer_set_text(buffer, "", -1);
}

void on_back_button_clicked(GtkButton *button, gpointer user_data) {
    GtkStack *stack = GTK_STACK(gtk_builder_get_object(app_data.builder, "main_stack"));
    gtk_stack_set_visible_child_name(stack, "dashboard");
    update_dashboard();
}

void on_back_to_dashboard_clicked(GtkButton *button, gpointer user_data) {
    GtkStack *stack = GTK_STACK(gtk_builder_get_object(app_data.builder, "main_stack"));
    gtk_stack_set_visible_child_name(stack, "manager_dashboard");
    update_manager_dashboard();
}

void on_manage_users_clicked(GtkButton *button, gpointer user_data) {
    GtkStack *stack = GTK_STACK(gtk_builder_get_object(app_data.builder, "main_stack"));
    gtk_stack_set_visible_child_name(stack, "manage_users");

    // Carrega lista de usuários
    GtkBox *users_box = GTK_BOX(gtk_builder_get_object(app_data.builder, "users_list_box"));
    clear_container(GTK_WIDGET(users_box));

    MYSQL_RES *result = get_users_list(app_data.db);
    if (!result) return;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        int user_id = atoi(row[0]);
        const char *username = row[1];

        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_container_set_border_width(GTK_CONTAINER(box), 5);

        char label_text[256];
        snprintf(label_text, sizeof(label_text), "ID: %d - %s", user_id, username);
        GtkWidget *label = gtk_label_new(label_text);
        gtk_label_set_xalign(GTK_LABEL(label), 0);
        gtk_widget_set_hexpand(label, TRUE);

        GtkWidget *edit_btn = gtk_button_new_with_label("Editar");
        GtkWidget *delete_btn = gtk_button_new_with_label("Deletar");

        gtk_style_context_add_class(gtk_widget_get_style_context(GTK_WIDGET(delete_btn)), "destructive-action");

        g_object_set_data(G_OBJECT(edit_btn), "user_id", GINT_TO_POINTER(user_id));
        g_object_set_data(G_OBJECT(delete_btn), "user_id", GINT_TO_POINTER(user_id));

        // Conectar sinais
        g_signal_connect(edit_btn, "clicked", G_CALLBACK(on_edit_user_clicked), NULL);
        g_signal_connect(delete_btn, "clicked", G_CALLBACK(on_delete_user_clicked), NULL);

        gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(box), edit_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), delete_btn, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(users_box), box, FALSE, FALSE, 0);
    }

    mysql_free_result(result);
    gtk_widget_show_all(GTK_WIDGET(users_box));
}

void on_manage_requests_clicked(GtkButton *button, gpointer user_data) {
    GtkStack *stack = GTK_STACK(gtk_builder_get_object(app_data.builder, "main_stack"));
    gtk_stack_set_visible_child_name(stack, "manage_requests");

    // Carrega lista de requerimentos
    GtkBox *requests_box = GTK_BOX(gtk_builder_get_object(app_data.builder, "manage_requests_list_box"));
    clear_container(GTK_WIDGET(requests_box));

    MYSQL_RES *result = get_requests_list(app_data.db);
    if (!result) return;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        int request_id = atoi(row[0]);
        const char *username = row[1];
        const char *date = row[2];
        const char *hours = row[3];
        const char *notes = row[4];
        const char *status = row[5];

        GtkWidget *frame = gtk_frame_new(NULL);
        gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);

        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

        char info[512];
        snprintf(info, sizeof(info),
                "<b>Usuário:</b> %s\n<b>Data:</b> %s\n<b>Horas:</b> %s\n<b>Status:</b> %s\n<b>Observações:</b> %s",
                username, date, hours, status, notes);

        GtkWidget *label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(label), info);
        gtk_label_set_xalign(GTK_LABEL(label), 0);
        gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);

        gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

        if (strcmp(status, "PENDENTE") == 0) {
            GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

            GtkWidget *approve_btn = gtk_button_new_with_label("Deferir");
            GtkWidget *reject_btn = gtk_button_new_with_label("Indeferir");

            gtk_style_context_add_class(gtk_widget_get_style_context(approve_btn), "suggested-action");
            gtk_style_context_add_class(gtk_widget_get_style_context(reject_btn), "destructive-action");

            g_object_set_data(G_OBJECT(approve_btn), "request_id", GINT_TO_POINTER(request_id));
            g_object_set_data(G_OBJECT(reject_btn), "request_id", GINT_TO_POINTER(request_id));

            // Conectar sinais
            g_signal_connect(approve_btn, "clicked", G_CALLBACK(on_approve_request_clicked), NULL);
            g_signal_connect(reject_btn, "clicked", G_CALLBACK(on_reject_request_clicked), NULL);

            gtk_box_pack_start(GTK_BOX(hbox), approve_btn, TRUE, TRUE, 0);
            gtk_box_pack_start(GTK_BOX(hbox), reject_btn, TRUE, TRUE, 0);

            gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        }

        gtk_container_add(GTK_CONTAINER(frame), vbox);
        gtk_box_pack_start(GTK_BOX(requests_box), frame, FALSE, FALSE, 0);
    }

    mysql_free_result(result);
    gtk_widget_show_all(GTK_WIDGET(requests_box));
}

void on_hours_value_changed(GtkRange *range, gpointer user_data) {
    double value = gtk_range_get_value(range);
    char text[32];
    snprintf(text, sizeof(text), "%.0f", value);

    GtkLabel *label = GTK_LABEL(gtk_builder_get_object(app_data.builder, "hours_value_label"));
    gtk_label_set_text(label, text);
}

gboolean return_to_dashboard(gpointer user_data) {
    GtkStack *stack = GTK_STACK(gtk_builder_get_object(app_data.builder, "main_stack"));
    gtk_stack_set_visible_child_name(stack, "dashboard");
    update_dashboard();
    return false;
}

void on_submit_request_clicked(GtkButton *button, gpointer user_data) {
    GtkCalendar *calendar = GTK_CALENDAR(gtk_builder_get_object(app_data.builder, "calendar"));
    GtkScale *slider = GTK_SCALE(gtk_builder_get_object(app_data.builder, "hours_slider"));
    GtkTextView *notes_view = GTK_TEXT_VIEW(gtk_builder_get_object(app_data.builder, "notes_input"));
    GtkLabel *output = GTK_LABEL(gtk_builder_get_object(app_data.builder, "request_output_label"));

    guint year, month, day;
    gtk_calendar_get_date(calendar, &year, &month, &day);
    month++; // Meses no GTK começam em zero

    time_t now = time(NULL);
    struct tm *current_time = localtime(&now);
    struct tm selected_date = {0};
    selected_date.tm_year = year - 1900;
    selected_date.tm_mon = month - 1;
    selected_date.tm_mday = day;

    time_t selected_timestamp = mktime(&selected_date);
    time_t today_timestamp = mktime(current_time);

    current_time->tm_hour = 0;
    current_time->tm_min = 0;
    current_time->tm_sec = 0;
    today_timestamp = mktime(current_time);

    if (selected_timestamp <= today_timestamp) {
        gtk_label_set_markup(output, "<span foreground='red'>Selecione uma data futura (a partir de amanhã).</span>");
        return;
    }

    double hours = gtk_range_get_value(GTK_RANGE(slider));

    if (hours <= 0) {
        gtk_label_set_markup(output, "<span foreground='red'>Selecione uma quantidade de horas válida.</span>");
        return;
    }

    if (hours > app_data.current_overtime) {
        gtk_label_set_markup(output, "<span foreground='red'>Você não possui horas suficientes no banco.</span>");
        return;
    }

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(notes_view);
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char *notes = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    char date[32];
    snprintf(date, sizeof(date), "%04d-%02d-%02d", year, month, day);

    int success = create_time_off_request(app_data.db, app_data.current_user_id, date, hours, notes);

    g_free(notes);

    if (success) {
        gtk_label_set_markup(output, "<span foreground='green'>Requerimento enviado com sucesso!</span>");

        g_timeout_add(500, return_to_dashboard, NULL);
    } else {
        gtk_label_set_markup(output, "<span foreground='red'>Erro ao enviar requerimento.</span>");
    }
}

void on_create_user_clicked(GtkButton *button, gpointer user_data) {
    GtkDialog *dialog = GTK_DIALOG(gtk_builder_get_object(app_data.builder, "user_edit_dialog"));
    gtk_window_set_title(GTK_WINDOW(dialog), "Criar Novo Usuário");

    // Limpa campos
    GtkEntry *username = GTK_ENTRY(gtk_builder_get_object(app_data.builder, "entry_username"));
    GtkEntry *password = GTK_ENTRY(gtk_builder_get_object(app_data.builder, "entry_password"));
    GtkComboBoxText *role = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(app_data.builder, "entry_role"));
    GtkSpinButton *hours = GTK_SPIN_BUTTON(gtk_builder_get_object(app_data.builder, "entry_hours"));

    gtk_entry_set_text(username, "");
    gtk_entry_set_text(password, "");
    gtk_combo_box_set_active(GTK_COMBO_BOX(role), 0);
    gtk_spin_button_set_value(hours, 40);

    gint response = gtk_dialog_run(dialog);

    if (response == GTK_RESPONSE_OK) {
        const char *user = gtk_entry_get_text(username);
        const char *pass = gtk_entry_get_text(password);
        const char *role_text = gtk_combo_box_text_get_active_text(role);
        int work_hours = gtk_spin_button_get_value_as_int(hours);

        if (strlen(user) > 0 && strlen(pass) > 0 && role_text) {
            create_user(app_data.db, user, pass, role_text, work_hours);

            GtkWindow *main_window = GTK_WINDOW(gtk_builder_get_object(app_data.builder, "main_window"));
            show_info_dialog(main_window, "Usuário criado com sucesso!");

            // Recarrega lista
            on_manage_users_clicked(NULL, NULL);
        }

        g_free((char*)role_text);
    }

    gtk_widget_hide(GTK_WIDGET(dialog));
}

void on_user_form_changed(GtkWidget *widget, gpointer user_data) {
    GtkEntry *username = GTK_ENTRY(gtk_builder_get_object(app_data.builder, "entry_username"));
    GtkEntry *password = GTK_ENTRY(gtk_builder_get_object(app_data.builder, "entry_password"));
    GtkComboBoxText *role = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(app_data.builder, "entry_role"));
    GtkButton *save_button = GTK_BUTTON(gtk_builder_get_object(app_data.builder, "save_button"));

    const char *user = gtk_entry_get_text(username);
    const char *pass = gtk_entry_get_text(password);
    const char *role_text = gtk_combo_box_text_get_active_text(role);

    gboolean is_valid = (strlen(user) > 0 && strlen(pass) > 0 && role_text != NULL);
    gtk_widget_set_sensitive(GTK_WIDGET(save_button), is_valid);

    if (role_text) g_free((char*)role_text);
}



int main() {
    setlocale(LC_ALL, "");

    app_data.db = connect_to_database();

    gtk_init(NULL, NULL);
    app_data.builder = gtk_builder_new();
    gtk_builder_add_from_file(app_data.builder, "glade/app.glade", NULL);

    // Callbacks dos botões
    g_signal_connect(gtk_builder_get_object(app_data.builder, "login_button"),
                     "clicked", G_CALLBACK(on_login_button_clicked), NULL);

    g_signal_connect(gtk_builder_get_object(app_data.builder, "request_button"),
                     "clicked", G_CALLBACK(on_request_button_clicked), NULL);

    g_signal_connect(gtk_builder_get_object(app_data.builder, "back_button"),
                     "clicked", G_CALLBACK(on_back_button_clicked), NULL);

    g_signal_connect(gtk_builder_get_object(app_data.builder, "manage_users_button"),
                     "clicked", G_CALLBACK(on_manage_users_clicked), NULL);

    g_signal_connect(gtk_builder_get_object(app_data.builder, "manage_requests_button"),
                     "clicked", G_CALLBACK(on_manage_requests_clicked), NULL);

    g_signal_connect(gtk_builder_get_object(app_data.builder, "manage_users_back_button"),
                     "clicked", G_CALLBACK(on_back_to_dashboard_clicked), NULL);

    g_signal_connect(gtk_builder_get_object(app_data.builder, "manage_requests_back_button"),
                     "clicked", G_CALLBACK(on_back_to_dashboard_clicked), NULL);

    g_signal_connect(gtk_builder_get_object(app_data.builder, "submit_button"),
                     "clicked", G_CALLBACK(on_submit_request_clicked), NULL);

    g_signal_connect(gtk_builder_get_object(app_data.builder, "hours_slider"),
                     "value-changed", G_CALLBACK(on_hours_value_changed), NULL);

    g_signal_connect(gtk_builder_get_object(app_data.builder, "create_user_button"),
                     "clicked", G_CALLBACK(on_create_user_clicked), NULL);

    g_signal_connect(gtk_builder_get_object(app_data.builder, "entry_username"),
                     "changed", G_CALLBACK(on_user_form_changed), NULL);

    g_signal_connect(gtk_builder_get_object(app_data.builder, "entry_password"),
                     "changed", G_CALLBACK(on_user_form_changed), NULL);

    g_signal_connect(gtk_builder_get_object(app_data.builder, "entry_role"),
                     "changed", G_CALLBACK(on_user_form_changed), NULL);

    GtkWindow *login_window = GTK_WINDOW(gtk_builder_get_object(app_data.builder, "login_window"));
    GtkWindow *main_window = GTK_WINDOW(gtk_builder_get_object(app_data.builder, "main_window"));

    g_signal_connect(login_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_window_set_default_icon_from_file("icon.png", NULL);

    gtk_widget_show(GTK_WIDGET(login_window));
    gtk_main();

    mysql_close(app_data.db);

    return 0;
}

