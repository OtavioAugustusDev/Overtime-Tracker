#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Bibliotecas externas
#include <mysql.h>
#include <gtk/gtk.h>

// Propriedades da janela
#define WINDOW_TITLE "Sistema de Horas Extras"
#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 300

// Propriedades do banco de dados
#define DATABASE_ADDRESS "localhost"
#define DATABASE_USER "root"
#define DATABASE_PASSWORD "1234"
#define DATABASE_NAME "pineapple"
#define DATABASE_PORT 3306

// Consultas
#define BUFFER_SIZE 512

// Aplicativo
#define APP_ID "dev.otavio.overtime-tracker"

// Saída
#define CONNECTION_ERROR -3
#define USER_NOT_FOUND -2
#define FATAL_ERROR -1
#define SUCCESS 0

// Variáveis da tela de autenticação
typedef struct {
    GtkWidget* username_input;
    GtkWidget* password_input;
    GtkWidget* output_label;
    GtkWidget* login_window;
    MYSQL* socket;
} Login_data;

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






/*
    Escuta pressionamentos do botão de login e envia as informações inseridas
    para o servidor caso os campos de texto não estejam vazios
*/
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
    snprintf(query, BUFFER_SIZE, "SELECT * FROM users WHERE username='%s' AND password='%s'", username, password);

    mysql_query(data->socket, query);

    MYSQL_RES* matches = mysql_store_result(data->socket);
    int results = mysql_num_rows(matches);

    return results ? exit(SUCCESS) : gtk_label_set_text(GTK_LABEL(data->output_label), "Credenciais falsas");
}

/*
    Constrói e configura uma janela de autenticação
*/
static void login_panel(GtkApplication* app, MYSQL* socket)
{
    GtkWidget* window = create_window(app, WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT);

    GtkWidget* username_input = create_input_text("Nome:", "Insira seu nome", TRUE);
    GtkWidget* password_input = create_input_text("Senha:", "Insira sua senha", FALSE);
    GtkWidget* output_label = gtk_label_new("");
    GtkWidget* button_login = gtk_button_new_with_label("Enviar");

    GtkWidget* widgets[] = {username_input, password_input, output_label, button_login};

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), vbox);

    for(int widget = 0; widget < (sizeof(widgets) / sizeof(*widgets)); widget++)
    {
        gtk_box_append(GTK_BOX(vbox), widgets[widget]);
    }

    Login_data* login_data = malloc(sizeof(Login_data));
    login_data->username_input = username_input;
    login_data->password_input = password_input;
    login_data->output_label = output_label;
    login_data->login_window = window;
    login_data->socket = socket;

    g_signal_connect(button_login, "clicked", G_CALLBACK(on_login_button_clicked), login_data);

    gtk_window_present(GTK_WINDOW(window));
}

/*
    Função executada quando a aplicação é ativada
*/
static void awake(GtkApplication *app, gpointer socket)
{
    login_panel(app, socket);
}

/*
    Se conecta ao banco de dados e cria um aplicativo com
    o socket de conexão imbutido
*/
int main()
{
    MYSQL* socket = connect_to_database();

    GtkApplication* app = gtk_application_new(APP_ID, 0);
    g_signal_connect(app, "activate", G_CALLBACK(awake), socket);
    g_application_run(G_APPLICATION(app), 0, 0);
    mysql_close(socket);

    exit(SUCCESS);
}
