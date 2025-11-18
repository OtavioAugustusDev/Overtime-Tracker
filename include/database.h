#ifndef DATABASE_H
#define DATABASE_H

#define DATABASE_ADDRESS "localhost"
#define DATABASE_USER "root"
#define DATABASE_PASSWORD "1234"
#define DATABASE_NAME "pineapple"
#define DATABASE_PORT 3306

MYSQL* connect_to_database();
int authenticate_user(MYSQL* socket, char* username, char* password);
MYSQL_ROW load_user_data(MYSQL* socket, int user_id);
MYSQL_RES* get_requests_list(MYSQL* socket);
void answer_request(MYSQL* socket, char* new_status, int request_id);
int refresh_user_balance(MYSQL* socket, int request_id);

#endif
