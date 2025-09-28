#ifndef UTIL_CONFIG_H
#define UTIL_CONFIG_H

#include <stdbool.h>

typedef struct AppConfig {
    char database_path[512];
    char locale[16];
    char currency[8];
    double tax_rate;
    char receipt_output_dir[512];
} AppConfig;

bool config_load(AppConfig *config, const char *path);
bool config_save(const AppConfig *config, const char *path);
void config_default(AppConfig *config);

#endif
