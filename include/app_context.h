#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include <sqlite3.h>
#include <gtk/gtk.h>
#include "util/config.h"
#include "util/logger.h"
#include "util/i18n.h"

typedef struct AppContext {
    sqlite3 *db;
    AppConfig config;
    Logger logger;
    I18nCatalog catalog;
} AppContext;

#endif
