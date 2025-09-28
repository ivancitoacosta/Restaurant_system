#ifndef DATA_DATABASE_H
#define DATA_DATABASE_H

#include <sqlite3.h>
#include <stdbool.h>
#include "util/logger.h"

typedef struct Migration {
    int version;
    const char *sql;
} Migration;

bool database_open(sqlite3 **db, const char *path, Logger *logger);
void database_close(sqlite3 *db);
bool database_apply_migrations(sqlite3 *db, const Migration *migrations, int migration_count, Logger *logger);
bool database_seed(sqlite3 *db, Logger *logger);

#endif
