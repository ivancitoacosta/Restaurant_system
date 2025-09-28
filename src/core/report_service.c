#include "core/report_service.h"
#include "data/dao.h"

bool report_service_export_daily(AppContext *ctx, const char *date_str, const char *output_csv_path) {
    if (!dao_export_daily_report(ctx->db, date_str, output_csv_path)) {
        logger_log(&ctx->logger, LOG_LEVEL_ERROR, "report", "No se pudo exportar el reporte");
        return false;
    }
    logger_log(&ctx->logger, LOG_LEVEL_INFO, "report", "Reporte diario exportado");
    return true;
}
