#ifndef CORE_REPORT_SERVICE_H
#define CORE_REPORT_SERVICE_H

#include <stdbool.h>
#include "app_context.h"

bool report_service_export_daily(AppContext *ctx, const char *date_str, const char *output_csv_path);

#endif
