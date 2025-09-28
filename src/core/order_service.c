#include "core/order_service.h"
#include "data/dao.h"
#include <stdio.h>

bool order_service_create(AppContext *ctx, int table_id, int user_id, int *order_id) {
    if (!dao_create_order(ctx->db, table_id, user_id, order_id)) {
        logger_log(&ctx->logger, LOG_LEVEL_ERROR, "order", "No se pudo crear la comanda");
        return false;
    }
    return true;
}

bool order_service_add_item(AppContext *ctx, int order_id, int menu_item_id, const char *notes) {
    if (!dao_add_item_to_order(ctx->db, order_id, menu_item_id, notes)) {
        logger_log(&ctx->logger, LOG_LEVEL_ERROR, "order", "No se pudo agregar el ítem");
        return false;
    }
    return true;
}

bool order_service_update_status(AppContext *ctx, int order_item_id, const char *status) {
    if (!dao_update_order_item_status(ctx->db, order_item_id, status)) {
        logger_log(&ctx->logger, LOG_LEVEL_ERROR, "order", "No se pudo actualizar el estado");
        return false;
    }
    return true;
}

bool order_service_calculate_totals(AppContext *ctx, int order_id, double tip_rate, double discount, double *subtotal, double *tax, double *total) {
    if (!dao_calculate_order_totals(ctx->db, order_id, ctx->config.tax_rate, tip_rate, discount, subtotal, tax, total)) {
        logger_log(&ctx->logger, LOG_LEVEL_ERROR, "order", "No se pudo calcular el total");
        return false;
    }
    return true;
}
