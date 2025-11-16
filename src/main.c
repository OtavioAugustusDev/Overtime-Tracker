#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

// Bibliotecas externas
#include <mysql.h>
#include <gtk/gtk.h>

// Propriedades da janela
#define WINDOW_TITLE "Sistema de Horas Extras"
#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 500

// Propriedades do banco de dados
#define DATABASE_ADDRESS "localhost"
#define DATABASE_USER "root"
#define DATABASE_PASSWORD "1234"
#define DATABASE_NAME "pineapple"
#define DATABASE_PORT 3306

// Consultas
#define BUFFER_SIZE 1024
#define SYSTEM_LOCK_TIME 18

// Aplicativo
#define APP_ID "dev.otavio.overtime-tracker"

// Saída
#define CONNECTION_ERROR -3
#define USER_NOT_FOUND -2
#define FATAL_ERROR -1
#define SUCCESS 0

// Estrutura para dados do usuário
typedef struct {
    int user_id;
    char username[100];
    double overtime_hours;
    double work_hours;
    char role[10]; // "USER" ou "GESTOR"
} User;

// Variáveis da tela de autenticação
typedef struct {
    GtkWidget* username_input;
    GtkWidget* password_input;
    GtkWidget* output_label;
    GtkWidget* login_window;
    MYSQL* socket;
} Login_data;

// Variáveis da janela principal
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

// Adicionar após a estrutura Request_data:
typedef struct {
    GtkWidget* list_container;
    App_data* app_data;
    guint timeout_id;
} Tracking_data;

/*
    MID-LEVEL ABSTRACTION: Tenta conexão com o banco de dados,
    retorna um socket caso tenha êxito, senão, encerra o programa.
*/
MYSQL* connect_to_database()
{
    MYSQL* socket = mysql_init(NULL);
    MYSQL* connection = mysql_real_connect(
        socket,
        DATABASE_ADDRESS,
        DATABASE_USER,
        DATABASE_PASSWORD,
        DATABASE_NAME,
        DATABASE_PORT,
        NULL,
        0);

    if (connection == NULL)
    {
        fprintf(stderr, "Erro ao conectar: %s\n", mysql_error(socket));
        mysql_close(socket);
        exit(CONNECTION_ERROR);
    }

    return socket;
}

/*
    MID-LEVEL ABSTRACTION: Cria um campo de inserção de texto com título e placeholder
*/
GtkWidget* create_input_text(char* title, char* placeholder, int secret)
{
    GtkWidget* input_label = gtk_label_new(title);
    gtk_widget_set_halign(input_label, GTK_ALIGN_START);

    GtkWidget* input_text = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(input_text), placeholder);
    gtk_entry_set_visibility(GTK_ENTRY(input_text), secret);

    return input_text;
}

/*
    MID-LEVEL ABSTRACTION: Cria e retorna uma janela pré-configurada
*/
GtkWidget* create_window(GtkApplication* app, char* title, int width, int height)
{
    GtkWidget* window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), title);
    gtk_window_set_default_size(GTK_WINDOW(window), width, height);
    return window;
}




void load_user_data(MYSQL* socket, int user_id, User* user)
{
    char query[BUFFER_SIZE];
    snprintf(query, BUFFER_SIZE,
             "SELECT id, username, overtime_hours, work_hours, role FROM users WHERE id=%d",
             user_id);

    mysql_query(socket, query);
    MYSQL_RES* result = mysql_store_result(socket);
    MYSQL_ROW row = mysql_fetch_row(result);

    if (row)
    {
        user->user_id = atoi(row[0]);
        strncpy(user->username, row[1], 99);
        user->overtime_hours = atof(row[2]);
        user->work_hours = atof(row[3]);
        strncpy(user->role, row[4], 9);
    }

    mysql_free_result(result);
}

static void on_navigation_button_clicked(GtkWidget* button, gpointer stack)
{
    const char* page_name = g_object_get_data(G_OBJECT(button), "stack-child-name");
    gtk_stack_set_visible_child_name(GTK_STACK(stack), page_name);
}
static gboolean refresh_tracking_list(gpointer data)
{
    Tracking_data* tracking_data = (Tracking_data*)data;

    // Limpar lista atual
    GtkWidget* child = gtk_widget_get_first_child(tracking_data->list_container);
    while (child != NULL)
    {
        GtkWidget* next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(tracking_data->list_container), child);
        child = next;
    }

    // Buscar requerimentos atualizados
    char query[BUFFER_SIZE];
    snprintf(query, BUFFER_SIZE,
             "SELECT id, date, hours, notes, status, created_at "
             "FROM time_off_requests WHERE user_id=%d ORDER BY created_at DESC",
             tracking_data->app_data->user.user_id);

    mysql_query(tracking_data->app_data->socket, query);
    MYSQL_RES* result = mysql_store_result(tracking_data->app_data->socket);

    if (mysql_num_rows(result) == 0)
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
            // Card container
            GtkWidget* card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
            gtk_widget_set_margin_start(card, 12);
            gtk_widget_set_margin_end(card, 12);
            gtk_widget_set_margin_top(card, 8);
            gtk_widget_set_margin_bottom(card, 8);
            gtk_widget_add_css_class(card, "card");

            // Cabeçalho com data e status
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
            gtk_widget_add_css_class(status_label, "dim-label");
            gtk_box_append(GTK_BOX(header_box), status_label);

            gtk_box_append(GTK_BOX(card), header_box);

            // Horas
            char hours_text[64];
            snprintf(hours_text, sizeof(hours_text), "Horas solicitadas: %s", row[2]);
            GtkWidget* hours_label = gtk_label_new(hours_text);
            gtk_widget_set_halign(hours_label, GTK_ALIGN_START);
            gtk_box_append(GTK_BOX(card), hours_label);

            // Observações
            if (strlen(row[3]) > 0)
            {
                char notes_text[256];
                snprintf(notes_text, sizeof(notes_text), "Observações: %s", row[3]);
                GtkWidget* notes_label = gtk_label_new(notes_text);
                gtk_widget_set_halign(notes_label, GTK_ALIGN_START);
                gtk_label_set_wrap(GTK_LABEL(notes_label), TRUE);
                gtk_box_append(GTK_BOX(card), notes_label);
            }

            // Linha separadora
            gtk_box_append(GTK_BOX(card), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

            gtk_box_append(GTK_BOX(tracking_data->list_container), card);
        }
    }

    mysql_free_result(result);

    return G_SOURCE_CONTINUE; // Continuar chamando a função
}

// Função auxiliar para verificar se o sistema está fechado
static gboolean is_system_closed()
{
    // Exemplo: sistema fecha às 18h
    GDateTime* now = g_date_time_new_now_local();
    int hour = g_date_time_get_hour(now);
    g_date_time_unref(now);

    return (hour >= SYSTEM_LOCK_TIME); // fechado a partir das 18h
}


GtkWidget* create_dashboard_view(App_data* app_data)
{
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(vbox, 40);
    gtk_widget_set_margin_end(vbox, 40);
    gtk_widget_set_margin_top(vbox, 40);
    gtk_widget_set_margin_bottom(vbox, 40);

    // Saudação personalizada
    char greeting_text[300];
    snprintf(greeting_text, 300, "Olá, %s! 👋", app_data->user.username);
    GtkWidget* greeting = gtk_label_new(greeting_text);
    gtk_widget_add_css_class(greeting, "title-1");
    gtk_box_append(GTK_BOX(vbox), greeting);

    // Card com fundo cinza usando GtkFrame
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

    // Rótulo discreto
    GtkWidget* label_title = gtk_label_new("Saldo no banco de horas:");
    gtk_widget_set_halign(label_title, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(label_title, "dim-label");
    gtk_box_append(GTK_BOX(info_card), label_title);

    // Valor principal
    char overtime_text[100];
    snprintf(overtime_text, sizeof(overtime_text), "%.1f horas", app_data->user.overtime_hours);
    GtkWidget* overtime_label = gtk_label_new(overtime_text);
    gtk_widget_set_halign(overtime_label, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(overtime_label, "title-1");
    gtk_box_append(GTK_BOX(info_card), overtime_label);

    // Subtítulo
    char workhours_text[100];
    snprintf(workhours_text, sizeof(workhours_text), "Carga Horária Semanal: %.1f horas", app_data->user.work_hours);
    GtkWidget* workhours_label = gtk_label_new(workhours_text);
    gtk_widget_set_halign(workhours_label, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(workhours_label, "dim-label");
    gtk_box_append(GTK_BOX(info_card), workhours_label);

    gtk_box_append(GTK_BOX(vbox), info_frame);

    // Botões de navegação
    GtkWidget* button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);

    GtkWidget* request_button;

    // Verificar condições
    if (app_data->user.overtime_hours <= 0) {
        request_button = gtk_button_new_with_label("Sem saldo de horas extras");
        gtk_widget_set_sensitive(request_button, FALSE);
    }
    else if (is_system_closed()) {
        request_button = gtk_button_new_with_label("Sistema fechado para requerimentos");
        gtk_widget_set_sensitive(request_button, FALSE);
    }
    else {
        request_button = gtk_button_new_with_label("📝 Novo requerimento");
        gtk_widget_add_css_class(request_button, "pill");
        gtk_widget_add_css_class(request_button, "suggested-action");
        g_object_set_data(G_OBJECT(request_button), "stack-child-name", "request");
        g_signal_connect(request_button, "clicked", G_CALLBACK(on_navigation_button_clicked), app_data->stack);
    }

    gtk_box_append(GTK_BOX(button_box), request_button);
    gtk_box_append(GTK_BOX(vbox), button_box);


    // Título da seção de requerimentos
    GtkWidget* section_title = gtk_label_new("Seus requerimentos");
    gtk_widget_add_css_class(section_title, "title-2");
    gtk_box_append(GTK_BOX(vbox), section_title);

    // ScrolledWindow para lista de requerimentos
    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);

    GtkWidget* list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), list_box);
    gtk_box_append(GTK_BOX(vbox), scrolled);

    // Preparar dados para auto-refresh
    Tracking_data* tracking_data = g_malloc(sizeof(Tracking_data));
    tracking_data->list_container = list_box;
    tracking_data->app_data = app_data;

    refresh_tracking_list(tracking_data);
    tracking_data->timeout_id = g_timeout_add_seconds(2, refresh_tracking_list, tracking_data);

    g_object_set_data_full(G_OBJECT(vbox), "tracking_data", tracking_data, (GDestroyNotify)g_free);

    return vbox;
}

// Função auxiliar para trocar de tela após o delay
static gboolean go_to_dashboard(gpointer stack)
{
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "dashboard");
    return G_SOURCE_REMOVE; // Executa uma vez só
}


static void on_submit_request(GtkWidget* widget, gpointer data)
{
    Request_data* req_data = (Request_data*)data;

    // Obter data do GtkCalendar
    GDateTime* gdate = gtk_calendar_get_date(GTK_CALENDAR(req_data->calendar));
    char* date = g_date_time_format(gdate, "%Y-%m-%d"); // formato YYYY-MM-DD

    // Obter data atual
    GDateTime* now = g_date_time_new_now_local();

    // Validação: só permite datas futuras
    if (g_date_time_compare(gdate, now) <= 0) {
        gtk_label_set_text(GTK_LABEL(req_data->output_label),
                          "Por favor, escolha uma data futura!");
        g_date_time_unref(gdate);
        g_date_time_unref(now);
        g_free(date);
        return;
    }

    g_date_time_unref(now);

    // Horas (slider)
    double hours_val = gtk_range_get_value(GTK_RANGE(req_data->hours_input));
    char hours[8];
    snprintf(hours, sizeof(hours), "%.0f", hours_val);

    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(req_data->notes_input));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char* notes = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    // Validações adicionais
    if (hours_val <= 0)
    {
        gtk_label_set_text(GTK_LABEL(req_data->output_label),
                          "Por favor, selecione uma quantidade de horas!");
        g_free(notes);
        g_free(date);
        g_date_time_unref(gdate);
        return;
    }

    // Inserir no banco de dados
    char query[BUFFER_SIZE];
    snprintf(query, BUFFER_SIZE,
             "INSERT INTO time_off_requests (user_id, date, hours, notes, status) "
             "VALUES (%d, '%s', %s, '%s', 'PENDENTE')",
             req_data->app_data->user.user_id, date, hours, notes);

    if (mysql_query(req_data->app_data->socket, query) == 0)
    {
        // Após enviar com sucesso
        gtk_label_set_text(GTK_LABEL(req_data->output_label),
                          "Requerimento enviado com sucesso!");

        // Resetar campos
        GDateTime* today = g_date_time_new_now_local();
        gtk_calendar_select_day(GTK_CALENDAR(req_data->calendar), today);
        g_date_time_unref(today);

        gtk_range_set_value(GTK_RANGE(req_data->hours_input), 0);
        gtk_text_buffer_set_text(buffer, "", -1);

        // Voltar automaticamente para a tela principal após 500ms
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

static void on_hours_changed(GtkRange* range, gpointer user_data)
{
    GtkLabel* value_label = GTK_LABEL(user_data);
    double value = gtk_range_get_value(range);

    char text[16];
    snprintf(text, sizeof(text), "%.0f", value);
    gtk_label_set_text(value_label, text);
}

GtkWidget* create_request_view(App_data* app_data)
{
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(vbox, 20);
    gtk_widget_set_margin_end(vbox, 20);
    gtk_widget_set_margin_top(vbox, 20);
    gtk_widget_set_margin_bottom(vbox, 20);

    // Botão Voltar
    GtkWidget* back_button = gtk_button_new_with_label("← Voltar");
    gtk_widget_add_css_class(back_button, "pill");
    gtk_widget_set_halign(back_button, GTK_ALIGN_START);
    g_object_set_data(G_OBJECT(back_button), "stack-child-name", "dashboard");
    g_signal_connect(back_button, "clicked", G_CALLBACK(on_navigation_button_clicked), app_data->stack);
    gtk_box_append(GTK_BOX(vbox), back_button);

    // Título
    GtkWidget* title = gtk_label_new("Solicitar Folga");
    gtk_widget_add_css_class(title, "title-1");
    gtk_box_append(GTK_BOX(vbox), title);

    // Data
    GtkWidget* date_label = gtk_label_new("Data da Folga:");
    gtk_widget_set_halign(date_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(vbox), date_label);

    GtkWidget* calendar = gtk_calendar_new();
    gtk_box_append(GTK_BOX(vbox), calendar);

    // Horas
    GtkWidget* hours_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget* hours_label = gtk_label_new("Quantidade de horas:");
    gtk_widget_set_halign(hours_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(hours_box), hours_label);

    GtkWidget* hours_value_label = gtk_label_new("0");
    gtk_box_append(GTK_BOX(hours_box), hours_value_label);
    gtk_box_append(GTK_BOX(vbox), hours_box);

    GtkWidget* hours_slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, app_data->user.overtime_hours, 1);
    gtk_scale_set_digits(GTK_SCALE(hours_slider), 0);
    gtk_box_append(GTK_BOX(vbox), hours_slider);
    g_signal_connect(hours_slider, "value-changed", G_CALLBACK(on_hours_changed), hours_value_label);

    // Observações
    GtkWidget* notes_label = gtk_label_new("Observações para o gestor:");
    gtk_widget_set_halign(notes_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(vbox), notes_label);

    GtkWidget* notes_input = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(notes_input), GTK_WRAP_WORD);
    gtk_widget_set_size_request(notes_input, -1, 100);

    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), notes_input);
    gtk_box_append(GTK_BOX(vbox), scrolled);

    // Output
    GtkWidget* output_label = gtk_label_new("");
    gtk_box_append(GTK_BOX(vbox), output_label);

    // Botão Enviar
    GtkWidget* submit_button = gtk_button_new_with_label("Enviar Requerimento");
    gtk_widget_add_css_class(submit_button, "suggested-action");
    gtk_box_append(GTK_BOX(vbox), submit_button);

    Request_data* req_data = g_malloc(sizeof(Request_data));
    req_data->calendar = calendar;
    req_data->hours_input = hours_slider;
    req_data->notes_input = notes_input;
    req_data->output_label = output_label;
    req_data->app_data = app_data;

    g_signal_connect(submit_button, "clicked", G_CALLBACK(on_submit_request), req_data);

    return vbox;
}

int get_pending_requests_count(MYSQL* socket)
{
    const char* query = "SELECT COUNT(*) FROM time_off_requests WHERE status='PENDENTE'";
    if (mysql_query(socket, query) != 0) return 0;

    MYSQL_RES* result = mysql_store_result(socket);
    MYSQL_ROW row = mysql_fetch_row(result);

    int count = 0;
    if (row) count = atoi(row[0]);

    mysql_free_result(result);
    return count;
}







// Protótipos
static void refresh_users_list(GtkWidget* list_box, MYSQL* socket);
static void on_delete_user_clicked(GtkWidget* widget, gpointer data);
static void on_edit_user_clicked(GtkWidget* widget, gpointer data);
static void on_create_user_clicked(GtkWidget* widget, gpointer data);
GtkWidget* create_manage_users_view(App_data* app_data);












static void refresh_users_list(GtkWidget* list_box, MYSQL* socket)
{
    // Limpar lista atual
    GtkWidget* child = gtk_widget_get_first_child(list_box);
    while (child != NULL) {
        GtkWidget* next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(list_box), child);
        child = next;
    }

    // Buscar usuários
    const char* query = "SELECT id, username FROM users ORDER BY id ASC";
    mysql_query(socket, query);
    MYSQL_RES* result = mysql_store_result(socket);

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        GtkWidget* user_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

        char info[128];
        snprintf(info, sizeof(info), "ID: %s | Nome: %s", row[0], row[1]);

        GtkWidget* label = gtk_label_new(info);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(user_row), label);

        // Dentro de refresh_users_list, ao criar o botão Editar:
        GtkWidget* edit_button = gtk_button_new_with_label("✏️ Editar");
        gtk_widget_add_css_class(edit_button, "pill");
        g_object_set_data(G_OBJECT(edit_button), "user_id", GINT_TO_POINTER(atoi(row[0])));
        g_signal_connect(edit_button, "clicked", G_CALLBACK(on_edit_user_clicked), list_box);
        gtk_box_append(GTK_BOX(user_row), edit_button);


        // Botão excluir
        GtkWidget* delete_button = gtk_button_new_with_label("🗑️ Excluir");
        gtk_widget_add_css_class(delete_button, "pill");
        gtk_widget_add_css_class(delete_button, "destructive-action");
        g_object_set_data(G_OBJECT(delete_button), "user_id", GINT_TO_POINTER(atoi(row[0])));
        g_signal_connect(delete_button, "clicked", G_CALLBACK(on_delete_user_clicked), list_box);
        gtk_box_append(GTK_BOX(user_row), delete_button);

        gtk_box_append(GTK_BOX(list_box), user_row);
    }

    mysql_free_result(result);
}



static void on_create_user_response(GtkDialog* dialog, int response_id, gpointer data)
{
    GtkWidget* list_box = GTK_WIDGET(data);  // <- agora existe
    App_data* app_data = g_object_get_data(G_OBJECT(list_box), "app_data");

    if (response_id == GTK_RESPONSE_OK) {
        // recuperar widgets do diálogo
        GtkWidget* content = gtk_dialog_get_content_area(dialog);
        GtkWidget* grid = gtk_widget_get_first_child(content);

        GtkWidget* entry_name  = gtk_grid_get_child_at(GTK_GRID(grid), 0, 0);
        GtkWidget* entry_pass  = gtk_grid_get_child_at(GTK_GRID(grid), 0, 1);
        GtkWidget* entry_role  = gtk_grid_get_child_at(GTK_GRID(grid), 0, 2);
        GtkWidget* entry_hours = gtk_grid_get_child_at(GTK_GRID(grid), 0, 3);

        const char* name = gtk_editable_get_text(GTK_EDITABLE(entry_name));
        const char* pass = gtk_editable_get_text(GTK_EDITABLE(entry_pass));
        const char* role = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(entry_role));
        int hours = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(entry_hours));

        char query[BUFFER_SIZE];
        snprintf(query, BUFFER_SIZE,
                 "INSERT INTO users (username, password, role, work_hours) "
                 "VALUES ('%s', '%s', '%s', %d)",
                 name, pass, role, hours);

        mysql_query(app_data->socket, query);
        refresh_users_list(list_box, app_data->socket);
    }

    gtk_window_destroy(GTK_WINDOW(dialog));
}


static void on_edit_user_response(GtkDialog* dialog, int response_id, gpointer data)
{
    GtkWidget* list_box = GTK_WIDGET(data);
    App_data* app_data = g_object_get_data(G_OBJECT(list_box), "app_data");

    if (response_id == GTK_RESPONSE_OK) {
        GtkWidget* content = gtk_dialog_get_content_area(dialog);
        GtkWidget* grid = gtk_widget_get_first_child(content);

        GtkWidget* entry_name  = gtk_grid_get_child_at(GTK_GRID(grid), 0, 0);
        GtkWidget* entry_pass  = gtk_grid_get_child_at(GTK_GRID(grid), 0, 1);
        GtkWidget* entry_role  = gtk_grid_get_child_at(GTK_GRID(grid), 0, 2);
        GtkWidget* entry_hours = gtk_grid_get_child_at(GTK_GRID(grid), 0, 3);

        const char* name = gtk_editable_get_text(GTK_EDITABLE(entry_name));
        const char* pass = gtk_editable_get_text(GTK_EDITABLE(entry_pass));
        const char* role = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(entry_role));
        int hours = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(entry_hours));

        int user_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dialog), "user_id"));

        char query[BUFFER_SIZE];
        snprintf(query, BUFFER_SIZE,
                 "UPDATE users SET username='%s', password='%s', role='%s', work_hours=%d WHERE id=%d",
                 name, pass, role, hours, user_id);

        mysql_query(app_data->socket, query);
        refresh_users_list(list_box, app_data->socket);
    }

    gtk_window_destroy(GTK_WINDOW(dialog));
}


// Função para abrir o diálogo
static void on_create_user_clicked(GtkWidget* widget, gpointer data)
{
    GtkWidget* list_box = GTK_WIDGET(data);
    App_data* app_data = g_object_get_data(G_OBJECT(list_box), "app_data");

    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "Criar novo usuário",
        GTK_WINDOW(app_data->window),
        GTK_DIALOG_MODAL,
        "Cancelar", GTK_RESPONSE_CANCEL,
        "Criar", GTK_RESPONSE_OK,
        NULL);

    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_box_append(GTK_BOX(content), grid);

    GtkWidget* entry_name = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_name), "Nome de usuário");
    gtk_grid_attach(GTK_GRID(grid), entry_name, 0, 0, 1, 1);

    GtkWidget* entry_pass = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_pass), "Senha");
    gtk_entry_set_visibility(GTK_ENTRY(entry_pass), FALSE);
    gtk_grid_attach(GTK_GRID(grid), entry_pass, 0, 1, 1, 1);

    GtkWidget* entry_role = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry_role), "USER");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry_role), "GESTOR");
    gtk_combo_box_set_active(GTK_COMBO_BOX(entry_role), 0);
    gtk_grid_attach(GTK_GRID(grid), entry_role, 0, 2, 1, 1);

    GtkWidget* entry_hours = gtk_spin_button_new_with_range(1, 80, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry_hours), 40);
    gtk_grid_attach(GTK_GRID(grid), entry_hours, 0, 3, 1, 1);

    // Conectar callback e mostrar
    g_signal_connect(dialog, "response", G_CALLBACK(on_create_user_response), list_box);
    gtk_window_present(GTK_WINDOW(dialog));
}

// Excluir usuário
static void on_delete_user_clicked(GtkWidget* widget, gpointer data)
{
    GtkWidget* list_box = GTK_WIDGET(data);
    int user_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "user_id"));

    char query[BUFFER_SIZE];
    snprintf(query, BUFFER_SIZE, "DELETE FROM users WHERE id=%d", user_id);

    MYSQL* socket = ((App_data*)g_object_get_data(G_OBJECT(list_box), "app_data"))->socket;
    mysql_query(socket, query);

    refresh_users_list(list_box, socket);
}

// Abre o diálogo de edição e pré-carrega dados do usuário
static void on_edit_user_clicked(GtkWidget* widget, gpointer data)
{
    // Recupera o list_box passado no g_signal_connect
    GtkWidget* list_box = GTK_WIDGET(data);
    App_data* app_data = g_object_get_data(G_OBJECT(list_box), "app_data");
    MYSQL* socket = app_data->socket;

    // ID do usuário associado ao botão Editar
    int user_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "user_id"));

    // Buscar dados atuais do usuário
    char query[BUFFER_SIZE];
    snprintf(query, BUFFER_SIZE,
             "SELECT username, password, role, work_hours FROM users WHERE id=%d",
             user_id);

    if (mysql_query(socket, query) != 0) {
        // opcional: log de erro
        return;
    }

    MYSQL_RES* result = mysql_store_result(socket);
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        return;
    }

    const char* current_name  = row[0];
    const char* current_pass  = row[1];
    const char* current_role  = row[2];
    int current_hours         = atoi(row[3]);
    mysql_free_result(result);

    // Diálogo de edição
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "Editar usuário",
        GTK_WINDOW(app_data->window),  // janela pai para modal correto
        GTK_DIALOG_MODAL,
        "Cancelar", GTK_RESPONSE_CANCEL,
        "Salvar",   GTK_RESPONSE_OK,
        NULL
    );

    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_box_append(GTK_BOX(content), grid);

    // Nome
    GtkWidget* entry_name = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(entry_name), current_name);
    gtk_grid_attach(GTK_GRID(grid), entry_name, 0, 0, 1, 1);

    // Senha
    GtkWidget* entry_pass = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(entry_pass), current_pass);
    gtk_entry_set_visibility(GTK_ENTRY(entry_pass), FALSE);
    gtk_grid_attach(GTK_GRID(grid), entry_pass, 0, 1, 1, 1);

    // Tipo (role)
    GtkWidget* entry_role = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry_role), "USER");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry_role), "GESTOR");
    gtk_combo_box_set_active(GTK_COMBO_BOX(entry_role),
                             strcmp(current_role, "GESTOR") == 0 ? 1 : 0);
    gtk_grid_attach(GTK_GRID(grid), entry_role, 0, 2, 1, 1);

    // Carga horária
    GtkWidget* entry_hours = gtk_spin_button_new_with_range(1, 80, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry_hours), current_hours);
    gtk_grid_attach(GTK_GRID(grid), entry_hours, 0, 3, 1, 1);

    // Guardar o user_id no próprio diálogo para uso no callback de resposta
    g_object_set_data(G_OBJECT(dialog), "user_id", GINT_TO_POINTER(user_id));

    // Conectar callback de resposta e apresentar
    g_signal_connect(dialog, "response", G_CALLBACK(on_edit_user_response), list_box);
    gtk_window_present(GTK_WINDOW(dialog));
}


GtkWidget* create_manage_users_view(App_data* app_data)
{
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(vbox, 20);
    gtk_widget_set_margin_end(vbox, 20);
    gtk_widget_set_margin_top(vbox, 20);
    gtk_widget_set_margin_bottom(vbox, 20);

    // Botão Voltar
    GtkWidget* back_button = gtk_button_new_with_label("← Voltar");
    gtk_widget_add_css_class(back_button, "pill");
    gtk_widget_set_halign(back_button, GTK_ALIGN_START);
    g_object_set_data(G_OBJECT(back_button), "stack-child-name", "dashboard");
    g_signal_connect(back_button, "clicked", G_CALLBACK(on_navigation_button_clicked), app_data->stack);
    gtk_box_append(GTK_BOX(vbox), back_button);

    // Título
    GtkWidget* title = gtk_label_new("Gerenciar Usuários");
    gtk_widget_add_css_class(title, "title-1");
    gtk_box_append(GTK_BOX(vbox), title);

    // Scrolled list
    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);
    GtkWidget* list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), list_box);
    gtk_box_append(GTK_BOX(vbox), scrolled);

    GtkWidget* create_button = gtk_button_new_with_label("➕ Criar novo usuário");
    gtk_widget_add_css_class(create_button, "pill");
    gtk_widget_add_css_class(create_button, "suggested-action");
    g_signal_connect(create_button, "clicked", G_CALLBACK(on_create_user_clicked), list_box);
    gtk_box_append(GTK_BOX(vbox), create_button);

    g_object_set_data(G_OBJECT(list_box), "app_data", app_data);
    refresh_users_list(list_box, app_data->socket);


    return vbox;
}


GtkWidget* create_manager_dashboard_view(App_data* app_data)
{
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(vbox, 40);
    gtk_widget_set_margin_end(vbox, 40);
    gtk_widget_set_margin_top(vbox, 40);
    gtk_widget_set_margin_bottom(vbox, 40);

    // Saudação personalizada
    char greeting_text[300];
    snprintf(greeting_text, 300, "Olá, %s! 👋", app_data->user.username);
    GtkWidget* greeting = gtk_label_new(greeting_text);
    gtk_widget_add_css_class(greeting, "title-1");
    gtk_box_append(GTK_BOX(vbox), greeting);

    // Card com número de requerimentos pendentes
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

    // Botões de navegação (um embaixo do outro)
    GtkWidget* button_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);

    GtkWidget* manage_users_button = gtk_button_new_with_label("👥 Gerir usuários");
    gtk_widget_add_css_class(manage_users_button, "pill");
    gtk_widget_add_css_class(manage_users_button, "suggested-action");
    g_object_set_data(G_OBJECT(manage_users_button), "stack-child-name", "manage_users");
    g_signal_connect(manage_users_button, "clicked", G_CALLBACK(on_navigation_button_clicked), app_data->stack);
    gtk_box_append(GTK_BOX(button_box), manage_users_button);

    GtkWidget* manage_requests_button = gtk_button_new_with_label("📋 Gerir requerimentos");
    gtk_widget_add_css_class(manage_requests_button, "pill");
    gtk_widget_add_css_class(manage_requests_button, "destructive-action"); // vermelho
    gtk_box_append(GTK_BOX(button_box), manage_requests_button);

    gtk_box_append(GTK_BOX(vbox), button_box);

    return vbox;
}


void create_main_window(GtkApplication* app, MYSQL* socket, int user_id)
{
    App_data* app_data = g_malloc(sizeof(App_data));
    app_data->socket = socket;
    load_user_data(socket, user_id, &app_data->user);

    GtkWidget* window = create_window(app, WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT);
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
    } else {
        gtk_stack_add_named(GTK_STACK(stack), create_dashboard_view(app_data), "dashboard");
        gtk_stack_add_named(GTK_STACK(stack), create_request_view(app_data), "request");
    }

    gtk_box_append(GTK_BOX(main_box), stack);
    gtk_widget_set_vexpand(stack, TRUE);

    gtk_window_present(GTK_WINDOW(window));
}






static void on_login_button_clicked(GtkWidget* widget, gpointer login_data)
{
    Login_data* data = login_data;

    const char* username = gtk_editable_get_text(GTK_EDITABLE(data->username_input));
    const char* password = gtk_editable_get_text(GTK_EDITABLE(data->password_input));

    bool empty_fields = strlen(username) == 0 || strlen(password) == 0;

    if (empty_fields)
    {
        gtk_label_set_text(GTK_LABEL(data->output_label), "Existem campos vazios");
        return;
    }

    char query[BUFFER_SIZE];
    snprintf(query, BUFFER_SIZE,
             "SELECT id FROM users WHERE username='%s' AND password='%s'",
             username, password);

    mysql_query(data->socket, query);

    MYSQL_RES* matches = mysql_store_result(data->socket);
    int results = mysql_num_rows(matches);

    if (results > 0)
    {
        MYSQL_ROW row = mysql_fetch_row(matches);
        int user_id = atoi(row[0]);
        mysql_free_result(matches);

        // Fechar janela de login e abrir janela principal
        GtkApplication* app = gtk_window_get_application(GTK_WINDOW(data->login_window));
        gtk_window_destroy(GTK_WINDOW(data->login_window));
        create_main_window(app, data->socket, user_id);
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(data->output_label), "Credenciais falsas");
        mysql_free_result(matches);
    }
}


static void login_panel(GtkApplication* app, MYSQL* socket)
{
    GtkWidget* window = create_window(app, "Login - Sistema de Horas Extras",
                                     WINDOW_WIDTH, 350);

    // Container principal com espaçamento generoso
    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), main_box);

    // Box centralizado para o conteúdo
    GtkWidget* center_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(center_box, 50);
    gtk_widget_set_margin_end(center_box, 50);
    gtk_widget_set_margin_top(center_box, 40);
    gtk_widget_set_margin_bottom(center_box, 40);
    gtk_widget_set_valign(center_box, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(center_box, TRUE);
    gtk_box_append(GTK_BOX(main_box), center_box);

    // Cabeçalho com ícone e título
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

    // Separador visual
    gtk_box_append(GTK_BOX(center_box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    // Campos de entrada com labels melhoradas
    GtkWidget* username_label = gtk_label_new("ID de Colaborador");
    gtk_widget_set_halign(username_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(username_label, "heading");
    gtk_box_append(GTK_BOX(center_box), username_label);

    GtkWidget* username_input = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(username_input), "Digite seu ID");
    gtk_entry_set_visibility(GTK_ENTRY(username_input), TRUE);
    gtk_widget_set_size_request(username_input, -1, 40);
    gtk_box_append(GTK_BOX(center_box), username_input);

    GtkWidget* password_label = gtk_label_new("Senha");
    gtk_widget_set_halign(password_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(password_label, "heading");
    gtk_widget_set_margin_top(password_label, 10);
    gtk_box_append(GTK_BOX(center_box), password_label);

    GtkWidget* password_input = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(password_input), "Digite sua senha");
    gtk_entry_set_visibility(GTK_ENTRY(password_input), FALSE);
    gtk_widget_set_size_request(password_input, -1, 40);
    gtk_box_append(GTK_BOX(center_box), password_input);

    // Label de output (mensagens de erro/sucesso)
    GtkWidget* output_label = gtk_label_new("");
    gtk_widget_set_margin_top(output_label, 10);
    gtk_widget_add_css_class(output_label, "error");
    gtk_label_set_wrap(GTK_LABEL(output_label), TRUE);
    gtk_box_append(GTK_BOX(center_box), output_label);

    // Botão de login estilizado
    GtkWidget* button_login = gtk_button_new_with_label("Entrar");
    gtk_widget_add_css_class(button_login, "suggested-action");
    gtk_widget_add_css_class(button_login, "pill");
    gtk_widget_set_size_request(button_login, -1, 45);
    gtk_widget_set_margin_top(button_login, 15);
    gtk_box_append(GTK_BOX(center_box), button_login);

    // Rodapé informativo
    GtkWidget* footer_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_margin_top(footer_box, 20);
    gtk_widget_set_halign(footer_box, GTK_ALIGN_CENTER);

    GtkWidget* footer_text = gtk_label_new("Projeto Integrador @ Universidade Vila Velha");
    gtk_widget_add_css_class(footer_text, "dim-label");
    gtk_widget_add_css_class(footer_text, "caption");
    gtk_box_append(GTK_BOX(footer_box), footer_text);

    gtk_box_append(GTK_BOX(center_box), footer_box);

    // Preparar dados para callback
    Login_data* login_data = malloc(sizeof(Login_data));
    login_data->username_input = username_input;
    login_data->password_input = password_input;
    login_data->output_label = output_label;
    login_data->login_window = window;
    login_data->socket = socket;

    g_signal_connect(button_login, "clicked", G_CALLBACK(on_login_button_clicked), login_data);

    // Permitir login com Enter na senha
    g_signal_connect(password_input, "activate", G_CALLBACK(on_login_button_clicked), login_data);

    gtk_window_present(GTK_WINDOW(window));
}











static void awake(GtkApplication *app, gpointer socket)
{
    login_panel(app, socket);
}


int main()
{
    MYSQL* socket = connect_to_database();

    GtkApplication* app = gtk_application_new(APP_ID, 0);
    g_signal_connect(app, "activate", G_CALLBACK(awake), socket);
    g_application_run(G_APPLICATION(app), 0, 0);
    mysql_close(socket);

    exit(SUCCESS);
}
