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
             "SELECT id, username, overtime_hours, work_hours FROM users WHERE id=%d",
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
        gtk_box_append(GTK_BOX(tracking_data->list_container), empty_label);
    }
    else
    {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result)))
        {
            GtkWidget* card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
            gtk_widget_set_margin_start(card, 10);
            gtk_widget_set_margin_end(card, 10);
            gtk_widget_set_margin_top(card, 10);
            gtk_widget_set_margin_bottom(card, 10);

            char info_text[500];
            snprintf(info_text, 500,
                    "Data: %s | Horas: %s | Status: %s\nObservações: %s",
                    row[1], row[2], row[4], row[3]);

            GtkWidget* info_label = gtk_label_new(info_text);
            gtk_widget_set_halign(info_label, GTK_ALIGN_START);
            gtk_label_set_wrap(GTK_LABEL(info_label), TRUE);
            gtk_box_append(GTK_BOX(card), info_label);

            gtk_box_append(GTK_BOX(card), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
            gtk_box_append(GTK_BOX(tracking_data->list_container), card);
        }
    }

    mysql_free_result(result);

    return G_SOURCE_CONTINUE; // Continuar chamando a função
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

    // Card de informações
    GtkWidget* info_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(info_box, 20);
    gtk_widget_set_margin_end(info_box, 20);
    gtk_widget_set_margin_top(info_box, 20);
    gtk_widget_set_margin_bottom(info_box, 20);

    char overtime_text[200];
    snprintf(overtime_text, 200, "⏰ Horas Extras: %.2f horas", app_data->user.overtime_hours);
    GtkWidget* overtime_label = gtk_label_new(overtime_text);
    gtk_widget_set_halign(overtime_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(overtime_label, "body");
    gtk_box_append(GTK_BOX(info_box), overtime_label);

    char workhours_text[200];
    snprintf(workhours_text, 200, "📅 Carga Horária Semanal: %.2f horas", app_data->user.work_hours);
    GtkWidget* workhours_label = gtk_label_new(workhours_text);
    gtk_widget_set_halign(workhours_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(workhours_label, "body");
    gtk_box_append(GTK_BOX(info_box), workhours_label);

    gtk_box_append(GTK_BOX(vbox), info_box);

    // Botões de navegação
    GtkWidget* button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);

    GtkWidget* request_button = gtk_button_new_with_label("📝 Solicitar Folga");
    gtk_widget_add_css_class(request_button, "pill");
    gtk_widget_add_css_class(request_button, "suggested-action");
    gtk_box_append(GTK_BOX(button_box), request_button);

    g_object_set_data(G_OBJECT(request_button), "stack-child-name", "request");
    g_signal_connect(request_button, "clicked", G_CALLBACK(on_navigation_button_clicked), app_data->stack);

    gtk_box_append(GTK_BOX(vbox), button_box);

    // Título da seção de requerimentos
    GtkWidget* section_title = gtk_label_new("📋 Meus Requerimentos");
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
        gtk_label_set_text(GTK_LABEL(req_data->output_label),
                          "Requerimento enviado com sucesso!");

        // Resetar campos
        GDateTime* today = g_date_time_new_now_local();
        gtk_calendar_select_day(GTK_CALENDAR(req_data->calendar), today);
        g_date_time_unref(today);

        gtk_range_set_value(GTK_RANGE(req_data->hours_input), 0);
        gtk_text_buffer_set_text(buffer, "", -1);
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

void create_main_window(GtkApplication* app, MYSQL* socket, int user_id)
{
    // Preparar dados do app
    App_data* app_data = g_malloc(sizeof(App_data));
    app_data->socket = socket;
    load_user_data(socket, user_id, &app_data->user);

    // Janela principal
    GtkWidget* window = create_window(app, WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT);
    app_data->window = window;

    // Box principal
    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), main_box);

    // Stack para trocar entre telas
    GtkWidget* stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(GTK_STACK(stack), 200);
    app_data->stack = stack;

    // Adicionar páginas ao stack
    gtk_stack_add_named(GTK_STACK(stack), create_dashboard_view(app_data), "dashboard");
    gtk_stack_add_named(GTK_STACK(stack), create_request_view(app_data), "request");

    // Adicionar stack diretamente ao main_box
    gtk_box_append(GTK_BOX(main_box), stack);
    gtk_widget_set_vexpand(stack, TRUE);

    // Exibir janela
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
    GtkWidget* username_label = gtk_label_new("👤 Nome de Usuário");
    gtk_widget_set_halign(username_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(username_label, "heading");
    gtk_box_append(GTK_BOX(center_box), username_label);

    GtkWidget* username_input = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(username_input), "Digite seu nome de usuário");
    gtk_entry_set_visibility(GTK_ENTRY(username_input), TRUE);
    gtk_widget_set_size_request(username_input, -1, 40);
    gtk_box_append(GTK_BOX(center_box), username_input);

    GtkWidget* password_label = gtk_label_new("🔒 Senha");
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

    GtkWidget* footer_text = gtk_label_new("Desenvolvido para gestão de horas extras");
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
