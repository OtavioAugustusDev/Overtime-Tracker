#ifndef DATABASE_H
#define DATABASE_H

#define DATABASE_ADDRESS "localhost"
#define DATABASE_USER "root"
#define DATABASE_PASSWORD "1234"
#define DATABASE_NAME "pineapple"
#define DATABASE_PORT 3306

MYSQL* connect_to_database();
MYSQL_ROW load_user_data(MYSQL* socket, int user_id);

#endif
