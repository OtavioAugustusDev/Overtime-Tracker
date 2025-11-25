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
        printf("Impossível se conectar ao banco de dados.");
        mysql_close(socket);
        exit(1);
    }

    return socket;
}

int authenticate_user(MYSQL* socket, const char* username, const char* password)
{
    char query[1024];
    char escaped_username[256];
    char escaped_password[256];

    mysql_real_escape_string(socket, escaped_username, username, strlen(username));
    mysql_real_escape_string(socket, escaped_password, password, strlen(password));

    snprintf(query, sizeof(query),
             "SELECT id FROM users WHERE username='%s' AND password='%s'",
             escaped_username, escaped_password);

    mysql_query(socket, query);

    MYSQL_RES* matches = mysql_store_result(socket);
    if (!matches) return 0;

    int result = 0;
    if (mysql_num_rows(matches) > 0)
    {
        MYSQL_ROW data = mysql_fetch_row(matches);
        result = atoi(data[0]);
    }

    mysql_free_result(matches);
    return result;
}

MYSQL_ROW load_user_data(MYSQL* socket, int user_id)
{
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT id, username, overtime_hours, work_hours, role, password "
             "FROM users WHERE id=%d",
             user_id);

    mysql_query(socket, query);

    MYSQL_RES* result = mysql_store_result(socket);
    if (!result) return NULL;

    MYSQL_ROW user_data = mysql_fetch_row(result);

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

    return mysql_store_result(socket);
}

MYSQL_RES* get_user_requests_list(MYSQL* socket, int user_id)
{
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT id, date, hours, notes, status, created_at "
             "FROM time_off_requests WHERE user_id=%d ORDER BY created_at DESC",
             user_id);

    mysql_query(socket, query);

    return mysql_store_result(socket);
}

int get_pending_requests_count(MYSQL* socket)
{
    const char* query = "SELECT COUNT(*) FROM time_off_requests WHERE status='PENDENTE'";

    mysql_query(socket, query);

    MYSQL_RES* result = mysql_store_result(socket);
    if (!result) return 0;

    MYSQL_ROW row = mysql_fetch_row(result);
    int count = row ? atoi(row[0]) : 0;

    mysql_free_result(result);
    return count;
}

void answer_request(MYSQL* socket, const char* new_status, int request_id)
{
    char query[512];
    char escaped_status[64];

    mysql_real_escape_string(socket, escaped_status, new_status, strlen(new_status));

    snprintf(query, sizeof(query),
             "UPDATE time_off_requests SET status='%s' WHERE id=%d",
             escaped_status, request_id);

    mysql_query(socket, query);

    if (strcmp(new_status, "APROVADO") == 0)
    {
        refresh_user_balance(socket, request_id);
    }
}

void refresh_user_balance(MYSQL* socket, int request_id)
{
    char query[512];
    snprintf(query, sizeof(query),
            "UPDATE users u "
            "JOIN time_off_requests r ON r.user_id = u.id "
            "SET u.overtime_hours = GREATEST(0, u.overtime_hours - r.hours) "
            "WHERE r.id=%d", request_id);

    mysql_query(socket, query);
}

void create_user(MYSQL* socket, const char* username, const char* password,
                const char* role, int work_hours)
{
    char query[1024];
    char escaped_username[256];
    char escaped_password[256];
    char escaped_role[64];

    mysql_real_escape_string(socket, escaped_username, username, strlen(username));
    mysql_real_escape_string(socket, escaped_password, password, strlen(password));
    mysql_real_escape_string(socket, escaped_role, role, strlen(role));

    snprintf(query, sizeof(query),
             "INSERT INTO users (username, password, role, work_hours, overtime_hours) "
             "VALUES ('%s', '%s', '%s', %d, 0)",
             escaped_username, escaped_password, escaped_role, work_hours);

    mysql_query(socket, query);
}

void update_user(MYSQL* socket, int user_id, const char* username,
                const char* password, const char* role, int work_hours)
{
    char query[1024];
    char escaped_username[256];
    char escaped_password[256];
    char escaped_role[64];

    mysql_real_escape_string(socket, escaped_username, username, strlen(username));
    mysql_real_escape_string(socket, escaped_password, password, strlen(password));
    mysql_real_escape_string(socket, escaped_role, role, strlen(role));

    snprintf(query, sizeof(query),
             "UPDATE users SET username='%s', password='%s', role='%s', work_hours=%d "
             "WHERE id=%d",
             escaped_username, escaped_password, escaped_role, work_hours, user_id);

    mysql_query(socket, query);
}

void delete_user(MYSQL* socket, int user_id)
{
    char query[256];
    snprintf(query, sizeof(query), "DELETE FROM users WHERE id=%d", user_id);

    mysql_query(socket, query);
}

MYSQL_RES* get_users_list(MYSQL* socket)
{
    const char* query = "SELECT id, username FROM users ORDER BY id ASC";

    mysql_query(socket, query);

    return mysql_store_result(socket);
}

int create_time_off_request(MYSQL* socket, int user_id, const char* date,
                            double hours, const char* notes)
{
    char query[1024];
    char escaped_date[64];
    char escaped_notes[512];
    char hours_str[32];

    mysql_real_escape_string(socket, escaped_date, date, strlen(date));
    mysql_real_escape_string(socket, escaped_notes, notes, strlen(notes));

    snprintf(hours_str, sizeof(hours_str), "%.2f", hours);
    for (int i = 0; hours_str[i] != '\0'; i++) {
        if (hours_str[i] == ',') hours_str[i] = '.';
    }

    snprintf(query, sizeof(query),
             "INSERT INTO time_off_requests (user_id, date, hours, notes, status) "
             "VALUES (%d, '%s', %s, '%s', 'PENDENTE')",
             user_id, escaped_date, hours_str, escaped_notes);

    mysql_query(socket, query);

    return 1;
}
