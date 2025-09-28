#include "ui/main_window.h"
#include "core/auth_service.h"
#include "core/order_service.h"
#include "core/report_service.h"
#include "data/dao.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <stdio.h>
#ifdef _WIN32
#include <direct.h>
#endif

typedef struct {
    AppContext *ctx;
    GtkApplication *app;
    GtkWidget *window;
    GtkWidget *login_panel;
    GtkWidget *content_panel;
    GtkWidget *status_label;
    GtkWidget *tables_list;
    GtkWidget *table_selector;
    GtkWidget *orders_list;
    GtkWidget *order_items_list;
    GtkWidget *menu_selector;
    GtkWidget *notes_entry;
    GtkWidget *order_status_label;
    GtkWidget *totals_label;
    GtkWidget *status_combo;
    GtkWidget *item_id_entry;
    GtkWidget *reservations_list;
    GtkWidget *reservation_table_selector;
    GtkWidget *reservation_name_entry;
    GtkWidget *reservation_phone_entry;
    GtkWidget *reservation_datetime_entry;
    GtkWidget *reservation_notes_entry;
    User current_user;
    int selected_order_id;
} UiState;

static void ui_clear_list_box(GtkListBox *list_box) {
    GtkWidget *child = gtk_widget_get_first_child(GTK_WIDGET(list_box));
    while (child != NULL) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(list_box, child);
        child = next;
    }
}

static void ui_status(UiState *state, const char *message) {
    gtk_label_set_text(GTK_LABEL(state->status_label), message);
}

static void ui_bind_order_buttons(UiState *state);
static void ui_refresh_reservations(UiState *state);

static void ui_refresh_tables(UiState *state) {
    TableStatus *tables = NULL;
    int count = 0;
    int index = 0;
    ui_clear_list_box(GTK_LIST_BOX(state->tables_list));
    gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(state->table_selector));
    if (state->reservation_table_selector != NULL) {
        gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(state->reservation_table_selector));
    }
    if (!dao_list_tables(state->ctx->db, &tables, &count)) {
        ui_status(state, "No se pudieron cargar las mesas");
        return;
    }
    while (index < count) {
        char label_text[128];
        char id_buffer[32];
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        GtkWidget *name_label = gtk_label_new(tables[index].name);
        GtkWidget *status_label = gtk_label_new(tables[index].status);
        snprintf(label_text, sizeof(label_text), "%s (%s)", tables[index].name, tables[index].status);
        snprintf(id_buffer, sizeof(id_buffer), "%d", tables[index].id);
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(state->table_selector), id_buffer, label_text);
        if (state->reservation_table_selector != NULL) {
            gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(state->reservation_table_selector), id_buffer, label_text);
        }
        gtk_box_append(GTK_BOX(row), name_label);
        gtk_box_append(GTK_BOX(row), status_label);
        gtk_list_box_append(GTK_LIST_BOX(state->tables_list), row);
        index = index + 1;
    }
    dao_free_tables(tables);
}

static void ui_refresh_menu(UiState *state) {
    MenuItem *items = NULL;
    int count = 0;
    int index = 0;
    gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(state->menu_selector));
    if (!dao_list_menu_items(state->ctx->db, &items, &count)) {
        ui_status(state, "Error cargando menú");
        return;
    }
    while (index < count) {
        char label_text[256];
        char id_buffer[32];
        snprintf(label_text, sizeof(label_text), "%s - %.2f", items[index].name, items[index].price);
        snprintf(id_buffer, sizeof(id_buffer), "%d", items[index].id);
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(state->menu_selector), id_buffer, label_text);
        index = index + 1;
    }
    dao_free_menu_items(items);
}

static void ui_refresh_orders(UiState *state) {
    int *order_ids = NULL;
    int count = 0;
    int index = 0;
    ui_clear_list_box(GTK_LIST_BOX(state->orders_list));
    if (!dao_list_open_orders(state->ctx->db, &order_ids, &count)) {
        ui_status(state, "No se pudieron cargar las comandas");
        return;
    }
    while (index < count) {
        char label_text[64];
        GtkWidget *button;
        snprintf(label_text, sizeof(label_text), "Comanda #%d", order_ids[index]);
        button = gtk_button_new_with_label(label_text);
        g_object_set_data(G_OBJECT(button), "order-id", GINT_TO_POINTER(order_ids[index]));
        gtk_list_box_append(GTK_LIST_BOX(state->orders_list), button);
        index = index + 1;
    }
    dao_free_order_ids(order_ids);
    ui_bind_order_buttons(state);
}

static void ui_refresh_order_items(UiState *state) {
    OrderItem *items = NULL;
    int count = 0;
    int index = 0;
    char label_text[128];
    ui_clear_list_box(GTK_LIST_BOX(state->order_items_list));
    if (state->selected_order_id <= 0) {
        gtk_label_set_text(GTK_LABEL(state->order_status_label), "Sin comanda seleccionada");
        return;
    }
    snprintf(label_text, sizeof(label_text), "Comanda #%d", state->selected_order_id);
    gtk_label_set_text(GTK_LABEL(state->order_status_label), label_text);
    if (!dao_list_order_items(state->ctx->db, state->selected_order_id, &items, &count)) {
        ui_status(state, "Error al cargar ítems");
        return;
    }
    while (index < count) {
        char row_text[256];
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        snprintf(row_text, sizeof(row_text), "#%d %.160s (%.40s)", items[index].id, items[index].notes, items[index].status);
        gtk_box_append(GTK_BOX(row), gtk_label_new(row_text));
        gtk_list_box_append(GTK_LIST_BOX(state->order_items_list), row);
        index = index + 1;
    }
    dao_free_order_items(items);
}

static void ui_select_order(UiState *state, int order_id) {
    state->selected_order_id = order_id;
    ui_refresh_order_items(state);
}

static void ui_on_order_button(GtkButton *button, UiState *state) {
    gpointer value = g_object_get_data(G_OBJECT(button), "order-id");
    ui_select_order(state, GPOINTER_TO_INT(value));
}

static void ui_bind_order_buttons(UiState *state) {
    GtkWidget *child = gtk_widget_get_first_child(state->orders_list);
    while (child != NULL) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        if (GTK_IS_BUTTON(child)) {
            g_signal_connect(child, "clicked", G_CALLBACK(ui_on_order_button), state);
        }
        child = next;
    }
}

static void ui_on_login(GtkButton *button, UiState *state) {
    GtkWidget *user_entry = g_object_get_data(G_OBJECT(button), "user-entry");
    GtkWidget *pass_entry = g_object_get_data(G_OBJECT(button), "pass-entry");
    const char *username = gtk_editable_get_text(GTK_EDITABLE(user_entry));
    const char *password = gtk_editable_get_text(GTK_EDITABLE(pass_entry));
    if (auth_login(state->ctx, username, password, &state->current_user)) {
        gtk_widget_set_visible(state->login_panel, FALSE);
        gtk_widget_set_visible(state->content_panel, TRUE);
        ui_status(state, "Ingreso correcto");
        ui_refresh_tables(state);
        ui_refresh_menu(state);
        ui_refresh_orders(state);
        ui_bind_order_buttons(state);
        ui_refresh_reservations(state);
    } else {
        ui_status(state, "Credenciales inválidas");
    }
}

static void ui_on_create_order(GtkButton *button, UiState *state) {
    const char *table_id_str = gtk_combo_box_get_active_id(GTK_COMBO_BOX(state->table_selector));
    int table_id;
    (void)button;
    if (table_id_str == NULL) {
        ui_status(state, "Seleccione mesa");
        return;
    }
    table_id = atoi(table_id_str);
    if (order_service_create(state->ctx, table_id, state->current_user.id, NULL)) {
        ui_status(state, "Comanda creada");
        ui_refresh_orders(state);
        ui_bind_order_buttons(state);
    } else {
        ui_status(state, "No se pudo crear la comanda");
    }
}

static void ui_on_add_item(GtkButton *button, UiState *state) {
    const char *menu_id_str = gtk_combo_box_get_active_id(GTK_COMBO_BOX(state->menu_selector));
    const char *notes = gtk_editable_get_text(GTK_EDITABLE(state->notes_entry));
    int menu_id;
    (void)button;
    if (state->selected_order_id <= 0) {
        ui_status(state, "Seleccione comanda");
        return;
    }
    if (menu_id_str == NULL) {
        ui_status(state, "Seleccione plato");
        return;
    }
    menu_id = atoi(menu_id_str);
    if (order_service_add_item(state->ctx, state->selected_order_id, menu_id, notes)) {
        ui_status(state, "Ítem agregado");
        ui_refresh_order_items(state);
    } else {
        ui_status(state, "Error al agregar ítem");
    }
}

static void ui_on_update_status(GtkButton *button, UiState *state) {
    const char *item_id_text = gtk_editable_get_text(GTK_EDITABLE(state->item_id_entry));
    gchar *status_text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(state->status_combo));
    int item_id = atoi(item_id_text);
    (void)button;
    if (item_id <= 0) {
        ui_status(state, "ID inválido");
        g_free(status_text);
        return;
    }
    if (status_text == NULL) {
        ui_status(state, "Estado inválido");
        return;
    }
    if (order_service_update_status(state->ctx, item_id, status_text)) {
        ui_status(state, "Estado actualizado");
        ui_refresh_order_items(state);
    } else {
        ui_status(state, "No se actualizó el estado");
    }
    g_free(status_text);
}

static void ui_generate_ticket(AppContext *ctx, int order_id, double subtotal, double tax, double total) {
    char path[1024];
#ifdef _WIN32
    _mkdir(ctx->config.receipt_output_dir);
#else
    mkdir(ctx->config.receipt_output_dir, 0755);
#endif
    snprintf(path, sizeof(path), "%s/ticket_%d.html", ctx->config.receipt_output_dir, order_id);
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        return;
    }
    fprintf(file, "<html><body><h1>Ticket #%d</h1>", order_id);
    fprintf(file, "<p>Subtotal: %.2f</p>", subtotal);
    fprintf(file, "<p>IVA: %.2f</p>", tax);
    fprintf(file, "<p>Total: %.2f</p>", total);
    fprintf(file, "</body></html>");
    fclose(file);
}

static void ui_on_close_order(GtkButton *button, UiState *state) {
    double subtotal = 0;
    double tax = 0;
    double total = 0;
    char label_text[128];
    (void)button;
    if (state->selected_order_id <= 0) {
        ui_status(state, "Seleccione comanda");
        return;
    }
    if (order_service_calculate_totals(state->ctx, state->selected_order_id, 0.1, 0, &subtotal, &tax, &total)) {
        ui_generate_ticket(state->ctx, state->selected_order_id, subtotal, tax, total);
        snprintf(label_text, sizeof(label_text), "Total: %.2f (IVA %.2f)", total, tax);
        gtk_label_set_text(GTK_LABEL(state->totals_label), label_text);
        ui_status(state, "Ticket generado");
    } else {
        ui_status(state, "No se pudo calcular total");
    }
}

static void ui_on_export_report(GtkButton *button, UiState *state) {
    time_t now = time(NULL);
    struct tm tm_now;
    char date_str[16];
    char path[256];
    (void)button;
#ifdef _WIN32
    localtime_s(&tm_now, &now);
#else
    localtime_r(&now, &tm_now);
#endif
    strftime(date_str, sizeof(date_str), "%Y-%m-%d", &tm_now);
#ifdef _WIN32
    _mkdir("reports");
#else
    mkdir("reports", 0755);
#endif
    snprintf(path, sizeof(path), "reports/ventas_%s.csv", date_str);
    if (report_service_export_daily(state->ctx, date_str, path)) {
        ui_status(state, "Reporte exportado");
    } else {
        ui_status(state, "No se exportó reporte");
    }
}

static void ui_refresh_reservations(UiState *state) {
    Reservation *reservations = NULL;
    int count = 0;
    int index = 0;
    if (state->reservations_list == NULL) {
        return;
    }
    ui_clear_list_box(GTK_LIST_BOX(state->reservations_list));
    if (!dao_list_reservations(state->ctx->db, &reservations, &count)) {
        ui_status(state, "Error cargando reservas");
        return;
    }
    while (index < count) {
        char buffer[256];
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        snprintf(buffer, sizeof(buffer), "Mesa %d - %s", reservations[index].table_id, reservations[index].customer_name);
        gtk_box_append(GTK_BOX(row), gtk_label_new(buffer));
        snprintf(buffer, sizeof(buffer), "%s (%s)", reservations[index].reserved_at, reservations[index].customer_phone);
        gtk_box_append(GTK_BOX(row), gtk_label_new(buffer));
        if (strlen(reservations[index].notes) > 0) {
            gtk_box_append(GTK_BOX(row), gtk_label_new(reservations[index].notes));
        }
        gtk_list_box_append(GTK_LIST_BOX(state->reservations_list), row);
        index = index + 1;
    }
    dao_free_reservations(reservations);
}

static void ui_on_add_reservation(GtkButton *button, UiState *state) {
    const char *table_id_str = gtk_combo_box_get_active_id(GTK_COMBO_BOX(state->reservation_table_selector));
    const char *name = gtk_editable_get_text(GTK_EDITABLE(state->reservation_name_entry));
    const char *phone = gtk_editable_get_text(GTK_EDITABLE(state->reservation_phone_entry));
    const char *datetime = gtk_editable_get_text(GTK_EDITABLE(state->reservation_datetime_entry));
    const char *notes = gtk_editable_get_text(GTK_EDITABLE(state->reservation_notes_entry));
    int table_id;
    (void)button;
    if (table_id_str == NULL || name[0] == '\0' || datetime[0] == '\0') {
        ui_status(state, "Complete la reserva");
        return;
    }
    table_id = atoi(table_id_str);
    if (dao_create_reservation(state->ctx->db, table_id, name, phone, datetime, notes)) {
        ui_status(state, "Reserva creada");
        ui_refresh_reservations(state);
    } else {
        ui_status(state, "No se creó la reserva");
    }
}

static GtkWidget *ui_build_login(UiState *state) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    GtkWidget *user_entry = gtk_entry_new();
    GtkWidget *pass_entry = gtk_password_entry_new();
    GtkWidget *button = gtk_button_new_with_label("Ingresar");
    gtk_entry_set_placeholder_text(GTK_ENTRY(user_entry), "Usuario");
    gtk_entry_set_placeholder_text(GTK_ENTRY(pass_entry), "Contraseña");
    gtk_box_append(GTK_BOX(box), gtk_label_new("Autenticación"));
    gtk_box_append(GTK_BOX(box), user_entry);
    gtk_box_append(GTK_BOX(box), pass_entry);
    gtk_box_append(GTK_BOX(box), button);
    g_object_set_data(G_OBJECT(button), "user-entry", user_entry);
    g_object_set_data(G_OBJECT(button), "pass-entry", pass_entry);
    g_signal_connect(button, "clicked", G_CALLBACK(ui_on_login), state);
    return box;
}

static GtkWidget *ui_build_salon(UiState *state) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *create_button = gtk_button_new_with_label("Crear comanda");
    state->tables_list = gtk_list_box_new();
    state->table_selector = GTK_WIDGET(gtk_combo_box_text_new());
    gtk_box_append(GTK_BOX(box), gtk_label_new("Mesas"));
    gtk_box_append(GTK_BOX(box), state->tables_list);
    gtk_box_append(GTK_BOX(box), state->table_selector);
    gtk_box_append(GTK_BOX(box), create_button);
    g_signal_connect(create_button, "clicked", G_CALLBACK(ui_on_create_order), state);
    return box;
}

static GtkWidget *ui_build_comandas(UiState *state) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *add_button = gtk_button_new_with_label("Agregar ítem");
    GtkWidget *close_button = gtk_button_new_with_label("Cerrar y ticket");
    GtkWidget *update_button = gtk_button_new_with_label("Actualizar estado");
    state->orders_list = gtk_list_box_new();
    state->order_items_list = gtk_list_box_new();
    state->menu_selector = GTK_WIDGET(gtk_combo_box_text_new());
    state->notes_entry = gtk_entry_new();
    state->order_status_label = gtk_label_new("Sin comanda");
    state->totals_label = gtk_label_new("Total: 0");
    state->status_combo = GTK_WIDGET(gtk_combo_box_text_new());
    state->item_id_entry = gtk_entry_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(state->status_combo), NULL, "pedido");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(state->status_combo), NULL, "preparacion");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(state->status_combo), NULL, "listo");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(state->status_combo), NULL, "servido");
    gtk_box_append(GTK_BOX(top), state->orders_list);
    gtk_box_append(GTK_BOX(top), state->order_items_list);
    gtk_box_append(GTK_BOX(actions), state->order_status_label);
    gtk_box_append(GTK_BOX(actions), state->menu_selector);
    gtk_box_append(GTK_BOX(actions), state->notes_entry);
    gtk_box_append(GTK_BOX(actions), add_button);
    gtk_box_append(GTK_BOX(actions), close_button);
    gtk_box_append(GTK_BOX(actions), state->totals_label);
    gtk_box_append(GTK_BOX(box), top);
    gtk_box_append(GTK_BOX(box), actions);
    GtkWidget *status_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(status_bar), gtk_label_new("Ítem"));
    gtk_box_append(GTK_BOX(status_bar), state->item_id_entry);
    gtk_box_append(GTK_BOX(status_bar), state->status_combo);
    gtk_box_append(GTK_BOX(status_bar), update_button);
    gtk_box_append(GTK_BOX(box), status_bar);
    g_signal_connect(add_button, "clicked", G_CALLBACK(ui_on_add_item), state);
    g_signal_connect(close_button, "clicked", G_CALLBACK(ui_on_close_order), state);
    g_signal_connect(update_button, "clicked", G_CALLBACK(ui_on_update_status), state);
    return box;
}

static GtkWidget *ui_build_reportes(UiState *state) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *button = gtk_button_new_with_label("Exportar ventas del día");
    gtk_box_append(GTK_BOX(box), button);
    g_signal_connect(button, "clicked", G_CALLBACK(ui_on_export_report), state);
    return box;
}

static GtkWidget *ui_build_reservas(UiState *state) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *form = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    GtkWidget *add_button = gtk_button_new_with_label("Guardar reserva");
    state->reservations_list = gtk_list_box_new();
    state->reservation_table_selector = GTK_WIDGET(gtk_combo_box_text_new());
    state->reservation_name_entry = gtk_entry_new();
    state->reservation_phone_entry = gtk_entry_new();
    state->reservation_datetime_entry = gtk_entry_new();
    state->reservation_notes_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(state->reservation_name_entry), "Nombre cliente");
    gtk_entry_set_placeholder_text(GTK_ENTRY(state->reservation_phone_entry), "Teléfono");
    gtk_entry_set_placeholder_text(GTK_ENTRY(state->reservation_datetime_entry), "YYYY-MM-DD HH:MM");
    gtk_entry_set_placeholder_text(GTK_ENTRY(state->reservation_notes_entry), "Notas");
    gtk_box_append(GTK_BOX(form), state->reservation_table_selector);
    gtk_box_append(GTK_BOX(form), state->reservation_name_entry);
    gtk_box_append(GTK_BOX(form), state->reservation_phone_entry);
    gtk_box_append(GTK_BOX(form), state->reservation_datetime_entry);
    gtk_box_append(GTK_BOX(form), state->reservation_notes_entry);
    gtk_box_append(GTK_BOX(form), add_button);
    gtk_box_append(GTK_BOX(box), form);
    gtk_box_append(GTK_BOX(box), state->reservations_list);
    g_signal_connect(add_button, "clicked", G_CALLBACK(ui_on_add_reservation), state);
    return box;
}

GtkWidget *ui_main_window_new(AppContext *ctx, GtkApplication *app) {
    UiState *state = g_new0(UiState, 1);
    GtkWidget *window = gtk_application_window_new(app);
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *stack = gtk_stack_new();
    GtkWidget *sidebar = gtk_stack_sidebar_new();
    GtkWidget *status_label = gtk_label_new("Listo");
    GtkWidget *login_box;
    gtk_window_set_default_size(GTK_WINDOW(window), 960, 640);
    gtk_window_set_title(GTK_WINDOW(window), "Sistema de Restaurante");
    state->ctx = ctx;
    state->app = app;
    state->window = window;
    state->login_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    state->content_panel = content_box;
    state->status_label = status_label;
    state->selected_order_id = 0;
    gtk_stack_sidebar_set_stack(GTK_STACK_SIDEBAR(sidebar), GTK_STACK(stack));
    gtk_box_append(GTK_BOX(content_box), sidebar);
    gtk_box_append(GTK_BOX(content_box), stack);
    gtk_stack_add_titled(GTK_STACK(stack), ui_build_salon(state), "salon", "Salón");
    gtk_stack_add_titled(GTK_STACK(stack), ui_build_comandas(state), "comandas", "Comandas");
    gtk_stack_add_titled(GTK_STACK(stack), ui_build_reservas(state), "reservas", "Reservas");
    gtk_stack_add_titled(GTK_STACK(stack), ui_build_reportes(state), "reportes", "Reportes");
    login_box = ui_build_login(state);
    gtk_box_append(GTK_BOX(state->login_panel), login_box);
    gtk_box_append(GTK_BOX(main_box), state->login_panel);
    gtk_box_append(GTK_BOX(main_box), content_box);
    gtk_box_append(GTK_BOX(main_box), status_label);
    gtk_widget_set_visible(content_box, FALSE);
    gtk_window_set_child(GTK_WINDOW(window), main_box);
    g_object_set_data_full(G_OBJECT(window), "ui-state", state, g_free);
    return window;
}

void ui_main_window_show_login(GtkWidget *window) {
    UiState *state = g_object_get_data(G_OBJECT(window), "ui-state");
    if (state == NULL) {
        return;
    }
    gtk_widget_set_visible(state->login_panel, TRUE);
    gtk_widget_set_visible(state->content_panel, FALSE);
}
