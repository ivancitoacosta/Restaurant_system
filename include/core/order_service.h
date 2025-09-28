#ifndef CORE_ORDER_SERVICE_H
#define CORE_ORDER_SERVICE_H

#include <stdbool.h>
#include "app_context.h"

bool order_service_create(AppContext *ctx, int table_id, int user_id, int *order_id);
bool order_service_add_item(AppContext *ctx, int order_id, int menu_item_id, const char *notes);
bool order_service_update_status(AppContext *ctx, int order_item_id, const char *status);
bool order_service_calculate_totals(AppContext *ctx, int order_id, double tip_rate, double discount, double *subtotal, double *tax, double *total);

#endif
