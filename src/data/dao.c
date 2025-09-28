#include "data/dao.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool dao_get_user_by_username(sqlite3 *db, const char *username, User *user) {
    const char *sql = "SELECT id, username, role, password_hash FROM users WHERE username = ?";
    sqlite3_stmt *stmt = NULL;
    int rc;
    if (user == NULL) {
        return false;
    }
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        user->id = sqlite3_column_int(stmt, 0);
        strncpy(user->username, (const char *)sqlite3_column_text(stmt, 1), sizeof(user->username) - 1);
        user->username[sizeof(user->username) - 1] = '\0';
        strncpy(user->role, (const char *)sqlite3_column_text(stmt, 2), sizeof(user->role) - 1);
        user->role[sizeof(user->role) - 1] = '\0';
        strncpy(user->password_hash, (const char *)sqlite3_column_text(stmt, 3), sizeof(user->password_hash) - 1);
        user->password_hash[sizeof(user->password_hash) - 1] = '\0';
        sqlite3_finalize(stmt);
        return true;
    }
    sqlite3_finalize(stmt);
    return false;
}

bool dao_create_order(sqlite3 *db, int table_id, int user_id, int *order_id) {
    const char *sql_insert = "INSERT INTO orders(table_id, waiter_id, status, created_at) VALUES(?, ?, 'abierta', CURRENT_TIMESTAMP)";
    const char *sql_update = "UPDATE tables SET status='ocupada', waiter_id = ? WHERE id = ?";
    sqlite3_stmt *stmt = NULL;
    int rc;
    if (sqlite3_exec(db, "BEGIN", NULL, NULL, NULL) != SQLITE_OK) {
        return false;
    }
    rc = sqlite3_prepare_v2(db, sql_insert, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
        return false;
    }
    sqlite3_bind_int(stmt, 1, table_id);
    sqlite3_bind_int(stmt, 2, user_id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
        return false;
    }
    if (order_id != NULL) {
        *order_id = (int)sqlite3_last_insert_rowid(db);
    }
    rc = sqlite3_prepare_v2(db, sql_update, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
        return false;
    }
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, table_id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
        return false;
    }
    if (sqlite3_exec(db, "COMMIT", NULL, NULL, NULL) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
        return false;
    }
    return true;
}

bool dao_add_item_to_order(sqlite3 *db, int order_id, int menu_item_id, const char *notes) {
    const char *sql = "INSERT INTO order_items(order_id, menu_item_id, status, notes) VALUES(?, ?, 'pedido', ?)";
    sqlite3_stmt *stmt = NULL;
    int rc;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(stmt, 1, order_id);
    sqlite3_bind_int(stmt, 2, menu_item_id);
    sqlite3_bind_text(stmt, 3, notes == NULL ? "" : notes, -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool dao_update_order_item_status(sqlite3 *db, int order_item_id, const char *status) {
    const char *sql = "UPDATE order_items SET status = ? WHERE id = ?";
    sqlite3_stmt *stmt = NULL;
    int rc;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, status, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, order_item_id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool dao_calculate_order_totals(sqlite3 *db, int order_id, double tax_rate, double tip_rate, double discount, double *subtotal, double *tax, double *total) {
    const char *sql = "SELECT SUM(m.price) FROM order_items oi JOIN menu_items m ON m.id = oi.menu_item_id WHERE oi.order_id = ?";
    sqlite3_stmt *stmt = NULL;
    int rc;
    double sum = 0.0;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(stmt, 1, order_id);
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        sum = sqlite3_column_double(stmt, 0);
    }
    sqlite3_finalize(stmt);
    if (subtotal != NULL) {
        *subtotal = sum;
    }
    if (tax != NULL) {
        *tax = sum * tax_rate;
    }
    if (total != NULL) {
        double tip_value = sum * tip_rate;
        double tax_value = sum * tax_rate;
        *total = sum - discount + tip_value + tax_value;
    }
    return true;
}

bool dao_export_daily_report(sqlite3 *db, const char *date_str, const char *path) {
    const char *sql = "SELECT o.id, t.name, u.username, SUM(m.price) FROM orders o JOIN tables t ON o.table_id = t.id JOIN users u ON o.waiter_id = u.id JOIN order_items oi ON oi.order_id = o.id JOIN menu_items m ON m.id = oi.menu_item_id WHERE DATE(o.created_at) = ? GROUP BY o.id, t.name, u.username";
    sqlite3_stmt *stmt = NULL;
    FILE *file = NULL;
    int rc;
    file = fopen(path, "w");
    if (file == NULL) {
        return false;
    }
    fprintf(file, "order_id,table,waiter,total\n");
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fclose(file);
        return false;
    }
    sqlite3_bind_text(stmt, 1, date_str, -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    while (rc == SQLITE_ROW) {
        int order_id = sqlite3_column_int(stmt, 0);
        const char *table_name = (const char *)sqlite3_column_text(stmt, 1);
        const char *waiter_name = (const char *)sqlite3_column_text(stmt, 2);
        double total = sqlite3_column_double(stmt, 3);
        fprintf(file, "%d,%s,%s,%.2f\n", order_id, table_name, waiter_name, total);
        rc = sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    fclose(file);
    return true;
}

bool dao_list_tables(sqlite3 *db, TableStatus **tables, int *count) {
    const char *sql = "SELECT id, name, status, IFNULL(waiter_id, 0) FROM tables ORDER BY id";
    sqlite3_stmt *stmt = NULL;
    int rc;
    int capacity = 0;
    int local_count = 0;
    TableStatus *local_tables = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return false;
    }
    rc = sqlite3_step(stmt);
    while (rc == SQLITE_ROW) {
        if (local_count == capacity) {
            int new_capacity = capacity == 0 ? 8 : capacity * 2;
            TableStatus *new_tables = realloc(local_tables, sizeof(TableStatus) * new_capacity);
            if (new_tables == NULL) {
                sqlite3_finalize(stmt);
                free(local_tables);
                return false;
            }
            local_tables = new_tables;
            capacity = new_capacity;
        }
        local_tables[local_count].id = sqlite3_column_int(stmt, 0);
        strncpy(local_tables[local_count].name, (const char *)sqlite3_column_text(stmt, 1), sizeof(local_tables[local_count].name) - 1);
        local_tables[local_count].name[sizeof(local_tables[local_count].name) - 1] = '\0';
        strncpy(local_tables[local_count].status, (const char *)sqlite3_column_text(stmt, 2), sizeof(local_tables[local_count].status) - 1);
        local_tables[local_count].status[sizeof(local_tables[local_count].status) - 1] = '\0';
        local_tables[local_count].waiter_id = sqlite3_column_int(stmt, 3);
        local_count = local_count + 1;
        rc = sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    *tables = local_tables;
    if (count != NULL) {
        *count = local_count;
    }
    return true;
}

bool dao_list_menu_items(sqlite3 *db, MenuItem **items, int *count) {
    const char *sql = "SELECT id, name, category, price, cost, stock, IFNULL(photo, '') FROM menu_items ORDER BY category, name";
    sqlite3_stmt *stmt = NULL;
    int rc;
    int capacity = 0;
    int local_count = 0;
    MenuItem *local_items = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return false;
    }
    rc = sqlite3_step(stmt);
    while (rc == SQLITE_ROW) {
        if (local_count == capacity) {
            int new_capacity = capacity == 0 ? 8 : capacity * 2;
            MenuItem *new_items = realloc(local_items, sizeof(MenuItem) * new_capacity);
            if (new_items == NULL) {
                sqlite3_finalize(stmt);
                free(local_items);
                return false;
            }
            local_items = new_items;
            capacity = new_capacity;
        }
        local_items[local_count].id = sqlite3_column_int(stmt, 0);
        strncpy(local_items[local_count].name, (const char *)sqlite3_column_text(stmt, 1), sizeof(local_items[local_count].name) - 1);
        local_items[local_count].name[sizeof(local_items[local_count].name) - 1] = '\0';
        strncpy(local_items[local_count].category, (const char *)sqlite3_column_text(stmt, 2), sizeof(local_items[local_count].category) - 1);
        local_items[local_count].category[sizeof(local_items[local_count].category) - 1] = '\0';
        local_items[local_count].price = sqlite3_column_double(stmt, 3);
        local_items[local_count].cost = sqlite3_column_double(stmt, 4);
        local_items[local_count].stock = sqlite3_column_int(stmt, 5);
        strncpy(local_items[local_count].photo, (const char *)sqlite3_column_text(stmt, 6), sizeof(local_items[local_count].photo) - 1);
        local_items[local_count].photo[sizeof(local_items[local_count].photo) - 1] = '\0';
        local_count = local_count + 1;
        rc = sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    *items = local_items;
    if (count != NULL) {
        *count = local_count;
    }
    return true;
}

bool dao_list_open_orders(sqlite3 *db, int **order_ids, int *count) {
    const char *sql = "SELECT id FROM orders WHERE status='abierta' ORDER BY created_at DESC";
    sqlite3_stmt *stmt = NULL;
    int rc;
    int capacity = 0;
    int local_count = 0;
    int *local_ids = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return false;
    }
    rc = sqlite3_step(stmt);
    while (rc == SQLITE_ROW) {
        if (local_count == capacity) {
            int new_capacity = capacity == 0 ? 8 : capacity * 2;
            int *new_ids = realloc(local_ids, sizeof(int) * new_capacity);
            if (new_ids == NULL) {
                sqlite3_finalize(stmt);
                free(local_ids);
                return false;
            }
            local_ids = new_ids;
            capacity = new_capacity;
        }
        local_ids[local_count] = sqlite3_column_int(stmt, 0);
        local_count = local_count + 1;
        rc = sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    *order_ids = local_ids;
    if (count != NULL) {
        *count = local_count;
    }
    return true;
}

bool dao_list_order_items(sqlite3 *db, int order_id, OrderItem **items, int *count) {
    const char *sql = "SELECT id, order_id, menu_item_id, status, IFNULL(notes, '') FROM order_items WHERE order_id = ?";
    sqlite3_stmt *stmt = NULL;
    int rc;
    int capacity = 0;
    int local_count = 0;
    OrderItem *local_items = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(stmt, 1, order_id);
    rc = sqlite3_step(stmt);
    while (rc == SQLITE_ROW) {
        if (local_count == capacity) {
            int new_capacity = capacity == 0 ? 8 : capacity * 2;
            OrderItem *new_items = realloc(local_items, sizeof(OrderItem) * new_capacity);
            if (new_items == NULL) {
                sqlite3_finalize(stmt);
                free(local_items);
                return false;
            }
            local_items = new_items;
            capacity = new_capacity;
        }
        local_items[local_count].id = sqlite3_column_int(stmt, 0);
        local_items[local_count].order_id = sqlite3_column_int(stmt, 1);
        local_items[local_count].menu_item_id = sqlite3_column_int(stmt, 2);
        strncpy(local_items[local_count].status, (const char *)sqlite3_column_text(stmt, 3), sizeof(local_items[local_count].status) - 1);
        local_items[local_count].status[sizeof(local_items[local_count].status) - 1] = '\0';
        strncpy(local_items[local_count].notes, (const char *)sqlite3_column_text(stmt, 4), sizeof(local_items[local_count].notes) - 1);
        local_items[local_count].notes[sizeof(local_items[local_count].notes) - 1] = '\0';
        local_count = local_count + 1;
        rc = sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    *items = local_items;
    if (count != NULL) {
        *count = local_count;
    }
    return true;
}

void dao_free_tables(TableStatus *tables) {
    free(tables);
}

void dao_free_menu_items(MenuItem *items) {
    free(items);
}

void dao_free_order_ids(int *order_ids) {
    free(order_ids);
}

void dao_free_order_items(OrderItem *items) {
    free(items);
}

bool dao_create_reservation(sqlite3 *db, int table_id, const char *name, const char *phone, const char *reserved_at, const char *notes) {
    const char *sql = "INSERT INTO reservations(table_id, customer_name, customer_phone, reserved_at, notes) VALUES(?, ?, ?, ?, ?)";
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(stmt, 1, table_id);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, phone == NULL ? "" : phone, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, reserved_at, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, notes == NULL ? "" : notes, -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool dao_list_reservations(sqlite3 *db, Reservation **reservations, int *count) {
    const char *sql = "SELECT id, table_id, customer_name, IFNULL(customer_phone,''), reserved_at, IFNULL(notes,'') FROM reservations ORDER BY reserved_at";
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    int capacity = 0;
    int local_count = 0;
    Reservation *local_reservations = NULL;
    if (rc != SQLITE_OK) {
        return false;
    }
    rc = sqlite3_step(stmt);
    while (rc == SQLITE_ROW) {
        if (local_count == capacity) {
            int new_capacity = capacity == 0 ? 8 : capacity * 2;
            Reservation *new_reservations = realloc(local_reservations, sizeof(Reservation) * new_capacity);
            if (new_reservations == NULL) {
                sqlite3_finalize(stmt);
                free(local_reservations);
                return false;
            }
            local_reservations = new_reservations;
            capacity = new_capacity;
        }
        local_reservations[local_count].id = sqlite3_column_int(stmt, 0);
        local_reservations[local_count].table_id = sqlite3_column_int(stmt, 1);
        strncpy(local_reservations[local_count].customer_name, (const char *)sqlite3_column_text(stmt, 2), sizeof(local_reservations[local_count].customer_name) - 1);
        local_reservations[local_count].customer_name[sizeof(local_reservations[local_count].customer_name) - 1] = '\0';
        strncpy(local_reservations[local_count].customer_phone, (const char *)sqlite3_column_text(stmt, 3), sizeof(local_reservations[local_count].customer_phone) - 1);
        local_reservations[local_count].customer_phone[sizeof(local_reservations[local_count].customer_phone) - 1] = '\0';
        strncpy(local_reservations[local_count].reserved_at, (const char *)sqlite3_column_text(stmt, 4), sizeof(local_reservations[local_count].reserved_at) - 1);
        local_reservations[local_count].reserved_at[sizeof(local_reservations[local_count].reserved_at) - 1] = '\0';
        strncpy(local_reservations[local_count].notes, (const char *)sqlite3_column_text(stmt, 5), sizeof(local_reservations[local_count].notes) - 1);
        local_reservations[local_count].notes[sizeof(local_reservations[local_count].notes) - 1] = '\0';
        local_count = local_count + 1;
        rc = sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    *reservations = local_reservations;
    if (count != NULL) {
        *count = local_count;
    }
    return true;
}

void dao_free_reservations(Reservation *reservations) {
    free(reservations);
}
