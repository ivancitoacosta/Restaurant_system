#include "data/database.h"
#include <string.h>
#include <stdio.h>
#include "util/hash.h"

static bool database_execute(sqlite3 *db, const char *sql, Logger *logger) {
    char *errmsg = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        if (logger != NULL && errmsg != NULL) {
            logger_log(logger, LOG_LEVEL_ERROR, "database", errmsg);
        }
        sqlite3_free(errmsg);
        return false;
    }
    return true;
}

bool database_open(sqlite3 **db, const char *path, Logger *logger) {
    if (sqlite3_open(path, db) != SQLITE_OK) {
        if (logger != NULL) {
            logger_log(logger, LOG_LEVEL_ERROR, "database", "No se pudo abrir la base de datos");
        }
        return false;
    }
    return true;
}

void database_close(sqlite3 *db) {
    if (db != NULL) {
        sqlite3_close(db);
    }
}

static bool database_get_current_version(sqlite3 *db, int *version) {
    const char *sql = "SELECT version FROM schema_version ORDER BY version DESC LIMIT 1";
    sqlite3_stmt *stmt = NULL;
    int rc;
    *version = 0;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return false;
    }
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        *version = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return true;
}

static bool database_ensure_schema_table(sqlite3 *db) {
    const char *sql = "CREATE TABLE IF NOT EXISTS schema_version (version INTEGER PRIMARY KEY, applied_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP)";
    return database_execute(db, sql, NULL);
}

bool database_apply_migrations(sqlite3 *db, const Migration *migrations, int migration_count, Logger *logger) {
    int current_version = 0;
    int index = 0;
    if (!database_ensure_schema_table(db)) {
        return false;
    }
    if (!database_get_current_version(db, &current_version)) {
        return false;
    }
    while (index < migration_count) {
        if (migrations[index].version > current_version) {
            char insert_sql[128];
            if (!database_execute(db, "BEGIN", logger)) {
                return false;
            }
            if (!database_execute(db, migrations[index].sql, logger)) {
                database_execute(db, "ROLLBACK", logger);
                return false;
            }
            snprintf(insert_sql, sizeof(insert_sql), "INSERT INTO schema_version(version) VALUES(%d)", migrations[index].version);
            if (!database_execute(db, insert_sql, logger)) {
                database_execute(db, "ROLLBACK", logger);
                return false;
            }
            if (!database_execute(db, "COMMIT", logger)) {
                database_execute(db, "ROLLBACK", logger);
                return false;
            }
            current_version = migrations[index].version;
        }
        index = index + 1;
    }
    return true;
}

static bool database_seed_user(sqlite3 *db, const char *username, const char *role, const char *password) {
    const char *sql = "INSERT INTO users(username, role, password_hash) SELECT ?, ?, ? WHERE NOT EXISTS (SELECT 1 FROM users WHERE username = ?)";
    sqlite3_stmt *stmt = NULL;
    int rc;
    char hash_buffer[65];
    char temp[256];
    snprintf(temp, sizeof(temp), "%s:%s", username, password);
    hash_sha256_hex((const unsigned char *)temp, strlen(temp), hash_buffer, sizeof(hash_buffer));
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, role, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, hash_buffer, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, username, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}

static bool database_seed_table(sqlite3 *db, const char *name, int number) {
    const char *sql = "INSERT INTO tables(name, status) SELECT ?, 'libre' WHERE NOT EXISTS (SELECT 1 FROM tables WHERE name = ?)";
    sqlite3_stmt *stmt = NULL;
    char buffer[64];
    int rc;
    snprintf(buffer, sizeof(buffer), "%s %d", name, number);
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, buffer, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, buffer, -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool database_seed(sqlite3 *db, Logger *logger) {
    int table_index = 1;
    if (!database_seed_user(db, "admin", "admin", "admin123")) {
        return false;
    }
    if (!database_seed_user(db, "mozo1", "mozo", "mozo123")) {
        return false;
    }
    if (!database_seed_user(db, "mozo2", "mozo", "mozo123")) {
        return false;
    }
    while (table_index <= 10) {
        if (!database_seed_table(db, "Mesa", table_index)) {
            return false;
        }
        table_index = table_index + 1;
    }
    if (!database_execute(db, "INSERT INTO menu_items(name, category, price, cost, stock) SELECT 'Milanesa', 'Plato Principal', 3500, 1500, 20 WHERE NOT EXISTS (SELECT 1 FROM menu_items WHERE name = 'Milanesa')", logger)) {
        return false;
    }
    if (!database_execute(db, "INSERT INTO menu_items(name, category, price, cost, stock) SELECT 'Ensalada', 'Entrada', 2100, 800, 15 WHERE NOT EXISTS (SELECT 1 FROM menu_items WHERE name = 'Ensalada')", logger)) {
        return false;
    }
    if (!database_execute(db, "INSERT INTO menu_items(name, category, price, cost, stock) SELECT 'Sopa del día', 'Entrada', 1800, 600, 10 WHERE NOT EXISTS (SELECT 1 FROM menu_items WHERE name = 'Sopa del día')", logger)) {
        return false;
    }
    if (!database_execute(db, "INSERT INTO menu_items(name, category, price, cost, stock) SELECT 'Ñoquis', 'Plato Principal', 3200, 1400, 25 WHERE NOT EXISTS (SELECT 1 FROM menu_items WHERE name = 'Ñoquis')", logger)) {
        return false;
    }
    if (!database_execute(db, "INSERT INTO menu_items(name, category, price, cost, stock) SELECT 'Ravioles', 'Plato Principal', 3300, 1500, 18 WHERE NOT EXISTS (SELECT 1 FROM menu_items WHERE name = 'Ravioles')", logger)) {
        return false;
    }
    if (!database_execute(db, "INSERT INTO menu_items(name, category, price, cost, stock) SELECT 'Pizza Margherita', 'Plato Principal', 3600, 1700, 20 WHERE NOT EXISTS (SELECT 1 FROM menu_items WHERE name = 'Pizza Margherita')", logger)) {
        return false;
    }
    if (!database_execute(db, "INSERT INTO menu_items(name, category, price, cost, stock) SELECT 'Hamburguesa Gourmet', 'Plato Principal', 3400, 1600, 30 WHERE NOT EXISTS (SELECT 1 FROM menu_items WHERE name = 'Hamburguesa Gourmet')", logger)) {
        return false;
    }
    if (!database_execute(db, "INSERT INTO menu_items(name, category, price, cost, stock) SELECT 'Flan Casero', 'Postre', 1500, 500, 12 WHERE NOT EXISTS (SELECT 1 FROM menu_items WHERE name = 'Flan Casero')", logger)) {
        return false;
    }
    if (!database_execute(db, "INSERT INTO menu_items(name, category, price, cost, stock) SELECT 'Helado', 'Postre', 1600, 600, 20 WHERE NOT EXISTS (SELECT 1 FROM menu_items WHERE name = 'Helado')", logger)) {
        return false;
    }
    if (!database_execute(db, "INSERT INTO menu_items(name, category, price, cost, stock) SELECT 'Café', 'Bebida', 900, 200, 100 WHERE NOT EXISTS (SELECT 1 FROM menu_items WHERE name = 'Café')", logger)) {
        return false;
    }
    if (!database_execute(db, "INSERT INTO reservations(table_id, customer_name, customer_phone, reserved_at, notes) SELECT 1, 'Cliente Demo', '123456789', datetime('now','+1 day'), 'Mesa junto a ventana' WHERE NOT EXISTS (SELECT 1 FROM reservations WHERE customer_name = 'Cliente Demo')", logger)) {
        return false;
    }
    return true;
}
