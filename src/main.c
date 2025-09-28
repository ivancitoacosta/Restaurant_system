#include <gtk/gtk.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include "app_context.h"
#include "data/database.h"
#include "ui/main_window.h"
#include "core/report_service.h"

static const char MIGRATION_1[] =
    "CREATE TABLE IF NOT EXISTS users("\
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"\
    "username TEXT UNIQUE NOT NULL,"\
    "role TEXT NOT NULL,"\
    "password_hash TEXT NOT NULL"\
    ");"\
    "CREATE TABLE IF NOT EXISTS menu_items("\
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"\
    "name TEXT NOT NULL,"\
    "category TEXT NOT NULL,"\
    "price REAL NOT NULL,"\
    "cost REAL NOT NULL,"\
    "stock INTEGER DEFAULT 0,"\
    "photo TEXT"\
    ");"\
    "CREATE TABLE IF NOT EXISTS tables("\
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"\
    "name TEXT NOT NULL,"\
    "status TEXT NOT NULL DEFAULT 'libre',"\
    "waiter_id INTEGER,"\
    "FOREIGN KEY(waiter_id) REFERENCES users(id)"\
    ");"\
    "CREATE TABLE IF NOT EXISTS orders("\
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"\
    "table_id INTEGER NOT NULL,"\
    "waiter_id INTEGER NOT NULL,"\
    "status TEXT NOT NULL DEFAULT 'abierta',"\
    "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"\
    "FOREIGN KEY(table_id) REFERENCES tables(id),"\
    "FOREIGN KEY(waiter_id) REFERENCES users(id)"\
    ");"\
    "CREATE TABLE IF NOT EXISTS order_items("\
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"\
    "order_id INTEGER NOT NULL,"\
    "menu_item_id INTEGER NOT NULL,"\
    "status TEXT NOT NULL DEFAULT 'pedido',"\
    "notes TEXT,"\
    "FOREIGN KEY(order_id) REFERENCES orders(id),"\
    "FOREIGN KEY(menu_item_id) REFERENCES menu_items(id)"\
    ");"\
    "CREATE TABLE IF NOT EXISTS reservations("\
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"\
    "table_id INTEGER NOT NULL,"\
    "customer_name TEXT NOT NULL,"\
    "customer_phone TEXT,"\
    "reserved_at TEXT NOT NULL,"\
    "notes TEXT,"\
    "FOREIGN KEY(table_id) REFERENCES tables(id)"\
    ");"\
    "CREATE TABLE IF NOT EXISTS payments("\
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"\
    "order_id INTEGER NOT NULL,"\
    "amount REAL NOT NULL,"\
    "method TEXT NOT NULL,"\
    "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"\
    "FOREIGN KEY(order_id) REFERENCES orders(id)"\
    ");"\
    "CREATE INDEX IF NOT EXISTS idx_orders_table ON orders(table_id);"\
    "CREATE INDEX IF NOT EXISTS idx_order_items_order ON order_items(order_id);";

static const Migration MIGRATIONS[] = {
    {1, MIGRATION_1}
};

static void ensure_directory(const char *path) {
#ifdef _WIN32
    _mkdir(path);
#else
    mkdir(path, 0755);
#endif
}

static void on_activate(GtkApplication *app, gpointer user_data) {
    AppContext *ctx = (AppContext *)user_data;
    GtkWidget *window = ui_main_window_new(ctx, app);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    AppContext ctx;
    GtkApplication *app = NULL;
    int status = 0;
    bool bootstrap_only = false;
    bool export_report = false;
    char report_date[16];
    int arg_index = 1;
    report_date[0] = '\0';
    memset(&ctx, 0, sizeof(AppContext));
    config_default(&ctx.config);
    if (!config_load(&ctx.config, "config.ini")) {
        config_default(&ctx.config);
        config_save(&ctx.config, "config.ini");
    }
    ensure_directory("logs");
    ensure_directory(ctx.config.receipt_output_dir);
    if (!logger_init(&ctx.logger, "logs", LOG_LEVEL_INFO)) {
        return 1;
    }
    if (database_open(&ctx.db, ctx.config.database_path, &ctx.logger) == false) {
        logger_log(&ctx.logger, LOG_LEVEL_ERROR, "main", "No se pudo abrir la base de datos");
        logger_close(&ctx.logger);
        return 1;
    }
    while (arg_index < argc) {
        if (strcmp(argv[arg_index], "--bootstrap") == 0) {
            bootstrap_only = true;
        } else if (strncmp(argv[arg_index], "--export-report=", 15) == 0) {
            strncpy(report_date, argv[arg_index] + 15, sizeof(report_date) - 1);
            report_date[sizeof(report_date) - 1] = '\0';
            export_report = true;
        } else if (strncmp(argv[arg_index], "--locale=", 9) == 0) {
            strncpy(ctx.config.locale, argv[arg_index] + 9, sizeof(ctx.config.locale) - 1);
            ctx.config.locale[sizeof(ctx.config.locale) - 1] = '\0';
        }
        arg_index = arg_index + 1;
    }
    if (!database_apply_migrations(ctx.db, MIGRATIONS, sizeof(MIGRATIONS) / sizeof(Migration), &ctx.logger)) {
        logger_log(&ctx.logger, LOG_LEVEL_ERROR, "main", "Migraciones fallidas");
        database_close(ctx.db);
        logger_close(&ctx.logger);
        return 1;
    }
    if (!database_seed(ctx.db, &ctx.logger)) {
        logger_log(&ctx.logger, LOG_LEVEL_ERROR, "main", "Seed fallido");
        database_close(ctx.db);
        logger_close(&ctx.logger);
        return 1;
    }
    char locale_path[64];
    snprintf(locale_path, sizeof(locale_path), "po/%s.txt", ctx.config.locale);
    if (!i18n_load(&ctx.catalog, locale_path)) {
        i18n_load(&ctx.catalog, "po/es.txt");
    }
    if (bootstrap_only) {
        i18n_free(&ctx.catalog);
        database_close(ctx.db);
        logger_close(&ctx.logger);
        return 0;
    }
    if (export_report) {
        char csv_path[256];
        snprintf(csv_path, sizeof(csv_path), "reports/ventas_%s.csv", report_date);
        ensure_directory("reports");
        if (!report_service_export_daily(&ctx, report_date, csv_path)) {
            logger_log(&ctx.logger, LOG_LEVEL_ERROR, "main", "No se exportó reporte");
            i18n_free(&ctx.catalog);
            database_close(ctx.db);
            logger_close(&ctx.logger);
            return 1;
        }
        i18n_free(&ctx.catalog);
        database_close(ctx.db);
        logger_close(&ctx.logger);
        return 0;
    }
    app = gtk_application_new("com.prompt.maestro", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), &ctx);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    i18n_free(&ctx.catalog);
    database_close(ctx.db);
    logger_close(&ctx.logger);
    return status;
}
