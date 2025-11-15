#include <stdio.h>
#include <string.h>
#include <mysql.h>
#include <gtk/gtk.h>

// Window properties
#define WINDOW_TITLE "Sistema de Horas Extras - Login"
#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 300

// Database properties
#define DATABASE_ADDRESS "localhost"
#define DATABASE_USER "root"
#define DATABASE_PASSWORD "1234"
#define DATABASE_NAME "pineapple"
#define DATABASE_PORT 3306

// Output
#define SUCCESS 0
#define FAIL 1

typedef struct {
    GtkWidget *entry_username;
    GtkWidget *entry_password;
    GtkWidget *label_message;
    GtkWidget *login_window;
    MYSQL *socket;
} LoginScreen;


MYSQL *connect_to_database()
{
    MYSQL *socket = mysql_init(NULL);
    MYSQL *connection = mysql_real_connect(
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
        return NULL;
    }
    return socket;
}


int send_query_to_database(MYSQL *socket, const char *query)
{
    if (mysql_query(socket, query))
    {
        fprintf(stderr, "Erro na query: %s\n", mysql_error(socket));
        return FAIL;
    }
    return SUCCESS;
}


int validate_login(MYSQL *socket, const char *username, const char *password)
{
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT * FROM users WHERE username='%s' AND password='%s'",
             username, password);

    if (send_query_to_database(socket, query) == FAIL)
        return FAIL;

    MYSQL_RES *result = mysql_store_result(socket);
    if (result == NULL)
        return FAIL;

    int num_rows = mysql_num_rows(result);
    mysql_free_result(result);

    return (num_rows > 0) ? SUCCESS : FAIL;
}


void open_main_window(GtkWidget *login_window)
{
    GtkWidget *main_window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(main_window), "Sistema de Horas Extras");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 800, 600);

    GtkWidget *label = gtk_label_new("Bem-vindo ao Sistema de Horas Extras!");
    gtk_window_set_child(GTK_WINDOW(main_window), label);

    gtk_window_present(GTK_WINDOW(main_window));
    gtk_window_destroy(GTK_WINDOW(login_window));
}


static void on_login_clicked(GtkWidget *widget, gpointer data)
{
    LoginScreen *login_data = (LoginScreen *)data;

    const char *username = gtk_editable_get_text(GTK_EDITABLE(login_data->entry_username));
    const char *password = gtk_editable_get_text(GTK_EDITABLE(login_data->entry_password));

    if (strlen(username) == 0 || strlen(password) == 0)
    {
        gtk_label_set_text(GTK_LABEL(login_data->label_message),
                          "Por favor, preencha todos os campos!");
        return;
    }

    if (validate_login(login_data->socket, username, password) == SUCCESS)
    {
        gtk_label_set_text(GTK_LABEL(login_data->label_message),
                          "Login bem-sucedido!");
        open_main_window(login_data->login_window);
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(login_data->label_message),
                          "Usuario ou senha invalidos!");
    }
}

static void create_login_window(GtkApplication *app, MYSQL *socket)
{
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), WINDOW_TITLE);
    gtk_window_set_default_size(GTK_WINDOW(window), WINDOW_WIDTH, WINDOW_HEIGHT);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(vbox, 20);
    gtk_widget_set_margin_end(vbox, 20);
    gtk_widget_set_margin_top(vbox, 20);
    gtk_widget_set_margin_bottom(vbox, 20);
    gtk_window_set_child(GTK_WINDOW(window), vbox);

    GtkWidget *title_label = gtk_label_new("Autenticacao de usuario");
    gtk_widget_add_css_class(title_label, "title-1");
    gtk_box_append(GTK_BOX(vbox), title_label);

    gtk_box_append(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    GtkWidget *label_username = gtk_label_new("Usuario:");
    gtk_widget_set_halign(label_username, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(vbox), label_username);

    GtkWidget *entry_username = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_username), "Digite seu usuario");
    gtk_box_append(GTK_BOX(vbox), entry_username);

    GtkWidget *label_password = gtk_label_new("Senha:");
    gtk_widget_set_halign(label_password, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(vbox), label_password);

    GtkWidget *entry_password = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_password), "Digite sua senha");
    gtk_entry_set_visibility(GTK_ENTRY(entry_password), FALSE);
    gtk_box_append(GTK_BOX(vbox), entry_password);

    GtkWidget *label_message = gtk_label_new("");
    gtk_box_append(GTK_BOX(vbox), label_message);

    GtkWidget *button_login = gtk_button_new_with_label("Entrar");
    gtk_widget_add_css_class(button_login, "suggested-action");
    gtk_box_append(GTK_BOX(vbox), button_login);

    LoginScreen *login_data = g_malloc(sizeof(LoginScreen));
    login_data->entry_username = entry_username;
    login_data->entry_password = entry_password;
    login_data->label_message = label_message;
    login_data->login_window = window;
    login_data->socket = socket;

    g_signal_connect(button_login, "clicked", G_CALLBACK(on_login_clicked), login_data);
    g_signal_connect_swapped(entry_password, "activate", G_CALLBACK(on_login_clicked), login_data);

    gtk_window_present(GTK_WINDOW(window));
}

static void activate(GtkApplication *app, gpointer user_data)
{
    MYSQL *socket = (MYSQL *)user_data;
    create_login_window(app, socket);
}

int main()
{
    MYSQL *socket = connect_to_database();
    if (socket == NULL)
    {
        fprintf(stderr, "Nao foi possível conectar ao banco de dados\n");
        return ERROR;
    }

    GtkApplication *app = gtk_application_new("dev.otavio.overtime-tracker", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), socket);
    g_application_run(G_APPLICATION(app), 0, 0);
    mysql_close(socket);

    exit(SUCCESS);
}
