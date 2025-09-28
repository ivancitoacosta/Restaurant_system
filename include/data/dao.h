#ifndef DATA_DAO_H
#define DATA_DAO_H

#include <sqlite3.h>
#include <stdbool.h>
#include <time.h>

typedef struct User {
    int id;
    char username[64];
    char role[16];
    char password_hash[256];
} User;

typedef struct MenuItem {
    int id;
    char name[128];
    char category[64];
    double price;
    double cost;
    int stock;
    char photo[256];
} MenuItem;

typedef struct TableStatus {
    int id;
    char name[64];
    char status[16];
    int waiter_id;
} TableStatus;

typedef struct OrderItem {
    int id;
    int order_id;
    int menu_item_id;
    char status[16];
    char notes[256];
} OrderItem;

typedef struct Reservation {
    int id;
    int table_id;
    char customer_name[128];
    char customer_phone[64];
    char reserved_at[32];
    char notes[256];
} Reservation;

bool dao_get_user_by_username(sqlite3 *db, const char *username, User *user);
bool dao_create_order(sqlite3 *db, int table_id, int user_id, int *order_id);
bool dao_add_item_to_order(sqlite3 *db, int order_id, int menu_item_id, const char *notes);
bool dao_update_order_item_status(sqlite3 *db, int order_item_id, const char *status);
bool dao_calculate_order_totals(sqlite3 *db, int order_id, double tax_rate, double tip_rate, double discount, double *subtotal, double *tax, double *total);
bool dao_export_daily_report(sqlite3 *db, const char *date_str, const char *path);
bool dao_list_tables(sqlite3 *db, TableStatus **tables, int *count);
bool dao_list_menu_items(sqlite3 *db, MenuItem **items, int *count);
bool dao_list_open_orders(sqlite3 *db, int **order_ids, int *count);
bool dao_list_order_items(sqlite3 *db, int order_id, OrderItem **items, int *count);
bool dao_create_reservation(sqlite3 *db, int table_id, const char *name, const char *phone, const char *reserved_at, const char *notes);
bool dao_list_reservations(sqlite3 *db, Reservation **reservations, int *count);
void dao_free_tables(TableStatus *tables);
void dao_free_menu_items(MenuItem *items);
void dao_free_order_ids(int *order_ids);
void dao_free_order_items(OrderItem *items);
void dao_free_reservations(Reservation *reservations);

#endif
