#ifndef DATABASE_H
#define DATABASE_H

#include <mysql.h>

// Definições de configuração do banco
#define DATABASE_ADDRESS "localhost"
#define DATABASE_USER "root"
#define DATABASE_PASSWORD "1234"
#define DATABASE_NAME "pineapple"
#define DATABASE_PORT 3306

// Funções de conexão
MYSQL* connect_to_database();

// Funções de autenticação
int authenticate_user(MYSQL* socket, const char* username, const char* password);

// Funções de usuário
MYSQL_ROW load_user_data(MYSQL* socket, int user_id);

// Funções de requisições
MYSQL_RES* get_requests_list(MYSQL* socket);
MYSQL_RES* get_user_requests_list(MYSQL* socket, int user_id);
int get_pending_requests_count(MYSQL* socket);

// Funções de atualização
void answer_request(MYSQL* socket, const char* new_status, int request_id);
void refresh_user_balance(MYSQL* socket, int request_id);

// Funções de gerenciamento de usuários
void create_user(MYSQL* socket, const char* username, const char* password,
                const char* role, int work_hours);
void update_user(MYSQL* socket, int user_id, const char* username,
                const char* password, const char* role, int work_hours);
void delete_user(MYSQL* socket, int user_id);
MYSQL_RES* get_users_list(MYSQL* socket);

// Funções de requisições
int create_time_off_request(MYSQL* socket, int user_id, const char* date,
                            double hours, const char* notes);

#endif
