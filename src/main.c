#include <locale.h>
#include <stdbool.h>
#include <time.h>
#include "database.h"
#include "interface.h"

#define SYSTEM_CLOSE_HOUR 19
#define SYSTEM_CLOSE_MINUTE 0

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

void on_manage_users_clicked(GtkButton *button, gpointer user_data);
void on_manage_requests_clicked(GtkButton *button, gpointer user_data);
void update_dashboard();

bool is_system_closed() {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    return (t->tm_hour > SYSTEM_CLOSE_HOUR ||
            (t->tm_hour == SYSTEM_CLOSE_HOUR && t->tm_min >= SYSTEM_CLOSE_MINUTE));
}

GtkWidget* get_widget(const char *name) {
    return GTK_WIDGET(gtk_builder_get_object(app_data.builder, name));
}

void load_user_requests() {
    GtkBox *box = GTK_BOX(get_widget("requests_list_box"));
    clear_container(GTK_WIDGET(box));

    MYSQL_RES *result = get_user_requests_list(app_data.db, app_data.current_user_id);
    if (!result) return;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        GtkWidget *frame = gtk_frame_new(NULL);
        gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);

        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

        char info[512];
        snprintf(info, sizeof(info),
                "<b>Data:</b> %s\n<b>Horas:</b> %s\n<b>Status:</b> %s\n<b>Observações:</b> %s",
                row[1], row[2], row[4], row[3]);

        GtkWidget *label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(label), info);
        gtk_label_set_xalign(GTK_LABEL(label), 0);
        gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);

        gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);
        gtk_container_add(GTK_CONTAINER(frame), vbox);
        gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);
    }

    mysql_free_result(result);
    gtk_widget_show_all(GTK_WIDGET(box));
}

void update_request_button_state() {
    GtkButton *btn = GTK_BUTTON(get_widget("request_button"));
    gtk_widget_set_sensitive(GTK_WIDGET(btn), FALSE);

    if (is_system_closed())
        gtk_button_set_label(btn, "🔒 Sistema fechado");
    else if (!app_data.current_overtime)
        gtk_button_set_label(btn, "⚠️ Sem saldo disponível");
    else {
        gtk_button_set_label(btn, "📝 Novo requerimento");
        gtk_widget_set_sensitive(GTK_WIDGET(btn), TRUE);
    }
}

void update_dashboard() {
    MYSQL_ROW user_data = load_user_data(app_data.db, app_data.current_user_id);
    if (user_data) {
        app_data.current_overtime = atof(user_data[2]);
        app_data.current_workhours = atof(user_data[3]);

        char text[512];
        snprintf(text, sizeof(text), "Olá, %s! 👋", app_data.current_username);
        gtk_label_set_text(GTK_LABEL(get_widget("greeting_label")), text);

        snprintf(text, sizeof(text), "%.2f horas", app_data.current_overtime);
        gtk_label_set_text(GTK_LABEL(get_widget("overtime_label")), text);

        snprintf(text, sizeof(text), "Carga Horária Semanal: %.2f horas", app_data.current_workhours);
        gtk_label_set_text(GTK_LABEL(get_widget("workhours_label")), text);
    }

    update_request_button_state();
    load_user_requests();
}

void update_manager_dashboard() {
    int count = get_pending_requests_count(app_data.db);
    char text[32];
    snprintf(text, sizeof(text), "%d", count);
    gtk_label_set_text(GTK_LABEL(get_widget("pending_label")), text);
}

void on_approve_request_clicked(GtkButton *button, gpointer user_data) {
    int request_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "request_id"));
    GtkWindow *win = GTK_WINDOW(get_widget("main_window"));

    if (show_confirm_dialog(win, "Deseja aprovar este requerimento?")) {
        answer_request(app_data.db, "APROVADO", request_id);
        show_info_dialog(win, "Requerimento aprovado.");
        on_manage_requests_clicked(NULL, NULL);
    }
}

void on_reject_request_clicked(GtkButton *button, gpointer user_data) {
    int request_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "request_id"));
    GtkWindow *win = GTK_WINDOW(get_widget("main_window"));

    if (show_confirm_dialog(win, "Deseja negar este requerimento?")) {
        answer_request(app_data.db, "NEGADO", request_id);
        show_info_dialog(win, "Requerimento negado.");
        on_manage_requests_clicked(NULL, NULL);
    }
}

void on_edit_user_clicked(GtkButton *button, gpointer user_data) {
    int user_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "user_id"));
    MYSQL_ROW data = load_user_data(app_data.db, user_id);
    if (!data) return;

    GtkDialog *dialog = GTK_DIALOG(get_widget("user_edit_dialog"));
    gtk_window_set_title(GTK_WINDOW(dialog), "Editar Usuário");

    GtkEntry *username = GTK_ENTRY(get_widget("entry_username"));
    GtkEntry *password = GTK_ENTRY(get_widget("entry_password"));
    GtkComboBoxText *role = GTK_COMBO_BOX_TEXT(get_widget("entry_role"));
    GtkSpinButton *hours = GTK_SPIN_BUTTON(get_widget("entry_hours"));

    gtk_entry_set_text(username, data[1]);
    gtk_entry_set_text(password, data[5]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(role), strcmp(data[4], "GESTOR") == 0 ? 1 : 0);
    gtk_spin_button_set_value(hours, atof(data[3]));

    if (gtk_dialog_run(dialog) == GTK_RESPONSE_OK) {
        const char *new_username = gtk_entry_get_text(username);
        const char *new_password = gtk_entry_get_text(password);
        const char *new_role = gtk_combo_box_text_get_active_text(role);
        int new_hours = gtk_spin_button_get_value_as_int(hours);

        if (strlen(new_username) > 0 && strlen(new_password) > 0 && new_role) {
            update_user(app_data.db, user_id, new_username, new_password, new_role, new_hours);
            show_info_dialog(GTK_WINDOW(get_widget("main_window")), "Sucesso!");
            on_manage_users_clicked(NULL, NULL);
        }
        g_free((char*)new_role);
    }

    gtk_widget_hide(GTK_WIDGET(dialog));
}

void on_delete_user_clicked(GtkButton *button, gpointer user_data) {
    int user_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "user_id"));
    GtkWindow *win = GTK_WINDOW(get_widget("main_window"));

    if (show_confirm_dialog(win, "Tem certeza que deseja deletar este usuário?")) {
        delete_user(app_data.db, user_id);
        show_info_dialog(win, "Sucesso!");
        on_manage_users_clicked(NULL, NULL);
    }
}

void on_login_button_clicked(GtkButton *button, gpointer user_data) {
    GtkEntry *username_entry = GTK_ENTRY(get_widget("username_input"));
    GtkEntry *password_entry = GTK_ENTRY(get_widget("password_input"));
    GtkLabel *output = GTK_LABEL(get_widget("output_label"));

    const char *username = gtk_entry_get_text(username_entry);
    const char *password = gtk_entry_get_text(password_entry);

    if (strlen(username) == 0 || strlen(password) == 0) {
        gtk_label_set_text(output, "Preencha todos os campos!");
        return;
    }

    int user_id = authenticate_user(app_data.db, username, password);

    if (user_id > 0) {
        app_data.current_user_id = user_id;
        strncpy(app_data.current_username, username, sizeof(app_data.current_username) - 1);

        MYSQL_ROW data = load_user_data(app_data.db, user_id);
        if (data) {
            strncpy(app_data.current_role, data[4], sizeof(app_data.current_role) - 1);

            gtk_widget_hide(get_widget("login_window"));

            GtkStack *stack = GTK_STACK(get_widget("main_stack"));
            if (strcmp(app_data.current_role, "GESTOR") == 0) {
                gtk_stack_set_visible_child_name(stack, "manager_dashboard");
                update_manager_dashboard();
            } else {
                gtk_stack_set_visible_child_name(stack, "dashboard");
                update_dashboard();
            }

            gtk_widget_show(get_widget("main_window"));
        }
    } else {
        gtk_label_set_text(output, "Credenciais inválidas.");
    }
}

void on_request_button_clicked(GtkButton *button, gpointer user_data) {
    GtkStack *stack = GTK_STACK(get_widget("main_stack"));
    gtk_stack_set_visible_child_name(stack, "request");

    GtkScale *slider = GTK_SCALE(get_widget("hours_slider"));
    GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(slider));
    gtk_adjustment_set_upper(adj, app_data.current_overtime);
    gtk_adjustment_set_value(adj, 0);

    time_t now = time(NULL);
    struct tm *today = localtime(&now);
    GtkCalendar *cal = GTK_CALENDAR(get_widget("calendar"));
    gtk_calendar_select_day(cal, today->tm_mday + 1);
    gtk_calendar_select_month(cal, today->tm_mon, today->tm_year + 1900);

    gtk_label_set_text(GTK_LABEL(get_widget("request_output_label")), "");
    gtk_label_set_text(GTK_LABEL(get_widget("hours_value_label")), "0");

    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(get_widget("notes_input")));
    gtk_text_buffer_set_text(buf, "", -1);
}

void on_back_button_clicked(GtkButton *button, gpointer user_data) {
    gtk_stack_set_visible_child_name(GTK_STACK(get_widget("main_stack")), "dashboard");
    update_dashboard();
}

void on_back_to_dashboard_clicked(GtkButton *button, gpointer user_data) {
    gtk_stack_set_visible_child_name(GTK_STACK(get_widget("main_stack")), "manager_dashboard");
    update_manager_dashboard();
}

void on_manage_users_clicked(GtkButton *button, gpointer user_data) {
    gtk_stack_set_visible_child_name(GTK_STACK(get_widget("main_stack")), "manage_users");

    GtkBox *box = GTK_BOX(get_widget("users_list_box"));
    clear_container(GTK_WIDGET(box));

    MYSQL_RES *result = get_users_list(app_data.db);
    if (!result) return;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        int user_id = atoi(row[0]);

        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);

        char text[256];
        snprintf(text, sizeof(text), "ID: %d - %s", user_id, row[1]);
        GtkWidget *label = gtk_label_new(text);
        gtk_label_set_xalign(GTK_LABEL(label), 0);
        gtk_widget_set_hexpand(label, TRUE);

        GtkWidget *edit_btn = gtk_button_new_with_label("Editar");
        GtkWidget *delete_btn = gtk_button_new_with_label("Excluir");
        gtk_style_context_add_class(gtk_widget_get_style_context(delete_btn), "destructive-action");

        g_object_set_data(G_OBJECT(edit_btn), "user_id", GINT_TO_POINTER(user_id));
        g_object_set_data(G_OBJECT(delete_btn), "user_id", GINT_TO_POINTER(user_id));

        g_signal_connect(edit_btn, "clicked", G_CALLBACK(on_edit_user_clicked), NULL);
        g_signal_connect(delete_btn, "clicked", G_CALLBACK(on_delete_user_clicked), NULL);

        gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), edit_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), delete_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 0);
    }

    mysql_free_result(result);
    gtk_widget_show_all(GTK_WIDGET(box));
}

void on_manage_requests_clicked(GtkButton *button, gpointer user_data) {
    gtk_stack_set_visible_child_name(GTK_STACK(get_widget("main_stack")), "manage_requests");

    GtkBox *box = GTK_BOX(get_widget("manage_requests_list_box"));
    clear_container(GTK_WIDGET(box));

    MYSQL_RES *result = get_requests_list(app_data.db);
    if (!result) return;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        int request_id = atoi(row[0]);

        GtkWidget *frame = gtk_frame_new(NULL);
        gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);

        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

        char info[512];
        snprintf(info, sizeof(info),
                "<b>Usuário:</b> %s\n<b>Data:</b> %s\n<b>Horas:</b> %s\n<b>Status:</b> %s\n<b>Observações:</b> %s",
                row[1], row[2], row[3], row[5], row[4]);

        GtkWidget *label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(label), info);
        gtk_label_set_xalign(GTK_LABEL(label), 0);
        gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);

        gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

        if (strcmp(row[5], "PENDENTE") == 0) {
            GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

            GtkWidget *approve_btn = gtk_button_new_with_label("Deferir");
            GtkWidget *reject_btn = gtk_button_new_with_label("Indeferir");

            gtk_style_context_add_class(gtk_widget_get_style_context(approve_btn), "suggested-action");
            gtk_style_context_add_class(gtk_widget_get_style_context(reject_btn), "destructive-action");

            g_object_set_data(G_OBJECT(approve_btn), "request_id", GINT_TO_POINTER(request_id));
            g_object_set_data(G_OBJECT(reject_btn), "request_id", GINT_TO_POINTER(request_id));

            g_signal_connect(approve_btn, "clicked", G_CALLBACK(on_approve_request_clicked), NULL);
            g_signal_connect(reject_btn, "clicked", G_CALLBACK(on_reject_request_clicked), NULL);

            gtk_box_pack_start(GTK_BOX(hbox), approve_btn, TRUE, TRUE, 0);
            gtk_box_pack_start(GTK_BOX(hbox), reject_btn, TRUE, TRUE, 0);
            gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        }

        gtk_container_add(GTK_CONTAINER(frame), vbox);
        gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);
    }

    mysql_free_result(result);
    gtk_widget_show_all(GTK_WIDGET(box));
}

void on_hours_value_changed(GtkRange *range, gpointer user_data) {
    char text[32];
    snprintf(text, sizeof(text), "%.0f", gtk_range_get_value(range));
    gtk_label_set_text(GTK_LABEL(get_widget("hours_value_label")), text);
}

gboolean return_to_dashboard(gpointer user_data) {
    gtk_stack_set_visible_child_name(GTK_STACK(get_widget("main_stack")), "dashboard");
    update_dashboard();
    return FALSE;
}

void on_submit_request_clicked(GtkButton *button, gpointer user_data) {
    GtkCalendar *cal = GTK_CALENDAR(get_widget("calendar"));
    GtkScale *slider = GTK_SCALE(get_widget("hours_slider"));
    GtkTextView *notes_view = GTK_TEXT_VIEW(get_widget("notes_input"));
    GtkLabel *output = GTK_LABEL(get_widget("request_output_label"));

    guint year, month, day;
    gtk_calendar_get_date(cal, &year, &month, &day);
    month++;

    time_t now = time(NULL);
    struct tm *current = localtime(&now);
    struct tm selected = {.tm_year = year - 1900, .tm_mon = month - 1, .tm_mday = day};

    current->tm_hour = current->tm_min = current->tm_sec = 0;

    if (mktime(&selected) <= mktime(current)) {
        gtk_label_set_markup(output, "<span foreground='red'>Data inválida!</span>");
        return;
    }

    double hours = gtk_range_get_value(GTK_RANGE(slider));
    if (!hours) {
        gtk_label_set_markup(output, "<span foreground='red'>Preencha todos os campos!</span>");
        return;
    }

    GtkTextBuffer *buf = gtk_text_view_get_buffer(notes_view);
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buf, &start, &end);
    char *notes = gtk_text_buffer_get_text(buf, &start, &end, FALSE);

    char date[32];
    snprintf(date, sizeof(date), "%04d-%02d-%02d", year, month, day);

    create_time_off_request(app_data.db, app_data.current_user_id, date, hours, notes);
    g_free(notes);

    gtk_label_set_markup(output, "<span foreground='green'>Sucesso!</span>");
    g_timeout_add(500, return_to_dashboard, NULL);
}

void on_create_user_clicked(GtkButton *button, gpointer user_data) {
    GtkDialog *dialog = GTK_DIALOG(get_widget("user_edit_dialog"));
    gtk_window_set_title(GTK_WINDOW(dialog), "Novo Usuário");

    GtkEntry *username = GTK_ENTRY(get_widget("entry_username"));
    GtkEntry *password = GTK_ENTRY(get_widget("entry_password"));
    GtkComboBoxText *role = GTK_COMBO_BOX_TEXT(get_widget("entry_role"));
    GtkSpinButton *hours = GTK_SPIN_BUTTON(get_widget("entry_hours"));

    gtk_entry_set_text(username, "");
    gtk_entry_set_text(password, "");
    gtk_combo_box_set_active(GTK_COMBO_BOX(role), 0);
    gtk_spin_button_set_value(hours, 40);

    if (gtk_dialog_run(dialog) == GTK_RESPONSE_OK) {
        const char *user = gtk_entry_get_text(username);
        const char *pass = gtk_entry_get_text(password);
        const char *role_text = gtk_combo_box_text_get_active_text(role);
        int work_hours = gtk_spin_button_get_value_as_int(hours);

        if (strlen(user) > 0 && strlen(pass) > 0 && role_text) {
            create_user(app_data.db, user, pass, role_text, work_hours);
            show_info_dialog(GTK_WINDOW(get_widget("main_window")), "Sucesso!");
            on_manage_users_clicked(NULL, NULL);
        }
        g_free((char*)role_text);
    }

    gtk_widget_hide(GTK_WIDGET(dialog));
}

void on_user_form_changed(GtkWidget *widget, gpointer user_data) {
    const char *user = gtk_entry_get_text(GTK_ENTRY(get_widget("entry_username")));
    const char *pass = gtk_entry_get_text(GTK_ENTRY(get_widget("entry_password")));
    const char *role = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(get_widget("entry_role")));

    gboolean valid = (strlen(user) > 0 && strlen(pass) > 0 && role != NULL);
    gtk_widget_set_sensitive(get_widget("save_button"), valid);

    if (role) g_free((char*)role);
}

void connect_signal(const char *widget, const char *signal, GCallback callback) {
    g_signal_connect(gtk_builder_get_object(app_data.builder, widget), signal, callback, NULL);
}

int main() {
    setlocale(LC_ALL, "");

    app_data.db = connect_to_database();

    gtk_init(NULL, NULL);
    app_data.builder = gtk_builder_new();
    gtk_builder_add_from_file(app_data.builder, "glade/app.glade", NULL);

    connect_signal("login_button", "clicked", G_CALLBACK(on_login_button_clicked));
    connect_signal("request_button", "clicked", G_CALLBACK(on_request_button_clicked));
    connect_signal("back_button", "clicked", G_CALLBACK(on_back_button_clicked));
    connect_signal("manage_users_button", "clicked", G_CALLBACK(on_manage_users_clicked));
    connect_signal("manage_requests_button", "clicked", G_CALLBACK(on_manage_requests_clicked));
    connect_signal("manage_users_back_button", "clicked", G_CALLBACK(on_back_to_dashboard_clicked));
    connect_signal("manage_requests_back_button", "clicked", G_CALLBACK(on_back_to_dashboard_clicked));
    connect_signal("submit_button", "clicked", G_CALLBACK(on_submit_request_clicked));
    connect_signal("hours_slider", "value-changed", G_CALLBACK(on_hours_value_changed));
    connect_signal("create_user_button", "clicked", G_CALLBACK(on_create_user_clicked));
    connect_signal("entry_username", "changed", G_CALLBACK(on_user_form_changed));
    connect_signal("entry_password", "changed", G_CALLBACK(on_user_form_changed));
    connect_signal("entry_role", "changed", G_CALLBACK(on_user_form_changed));

    GtkWindow *login_window = GTK_WINDOW(get_widget("login_window"));
    GtkWindow *main_window = GTK_WINDOW(get_widget("main_window"));

    g_signal_connect(login_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_window_set_default_icon_from_file("icon.png", NULL);

    gtk_widget_show(GTK_WIDGET(login_window));
    gtk_main();

    mysql_close(app_data.db);

    return 0;
}

