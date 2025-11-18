#include <stdio.h>
#include <mysql.h>

#include "database.h"

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
        exit(0);
    }

    return socket;
}

int authenticate_user(MYSQL* socket, char* username, char* password)
{
    char query[1024];
    snprintf(query, 1024,
             "SELECT id FROM users WHERE username='%s' AND password='%s'",
             username, password);

    mysql_query(socket, query);

    MYSQL_RES* matches = mysql_store_result(socket);

    int result = mysql_num_rows(matches);

    if (result)
    {
        MYSQL_ROW data = mysql_fetch_row(matches);
        int user_id = atoi(data[0]);

        mysql_free_result(matches);

        return user_id;
    }

    mysql_free_result(matches);

    return result;
}

MYSQL_ROW load_user_data(MYSQL* socket, int user_id)
{
    char query[1024];
    snprintf(query, 1024,
             "SELECT id, username, overtime_hours, work_hours, role FROM users WHERE id=%d",
             user_id);

    mysql_query(socket, query);
    MYSQL_RES* result = mysql_store_result(socket);
    MYSQL_ROW user_data = mysql_fetch_row(result);

    mysql_free_result(result);

    /*
        [0] id
        [1] username
        [2] overtime_hours
        [3] work_hours
        [4] role
    */
    return user_data;
}

MYSQL_RES* get_requests_list(MYSQL* socket)
{
    const char* query =
        "SELECT r.id, u.username, r.date, r.hours, r.notes, r.status, r.created_at "
        "FROM time_off_requests r "
        "JOIN users u ON u.id = r.user_id "
        "ORDER BY r.created_at DESC";

    mysql_query(socket, query);
    MYSQL_RES* list = mysql_store_result(socket);

    return list;
}

void answer_request(MYSQL* socket, char* new_status, int request_id)
{
    char query[1024];
    snprintf(query, 1024, "UPDATE time_off_requests SET status='%s' WHERE id=%d", new_status, request_id);
    mysql_query(socket, query);

    if (strcmp(new_status, "APROVADO") == 0) refresh_user_balance(socket, request_id);
}

int refresh_user_balance(MYSQL* socket, int request_id)
{
    char query[1024];
    snprintf(query, 1024,
            "UPDATE users u "
            "JOIN time_off_requests r ON r.user_id = u.id "
            "SET u.overtime_hours = GREATEST(0, u.overtime_hours - r.hours) "
            "WHERE r.id=%d", request_id);
    mysql_query(socket, query);
}

