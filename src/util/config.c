#include "util/config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void config_trim(char *line) {
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r' || line[len - 1] == ' ' || line[len - 1] == '\t')) {
        line[len - 1] = '\0';
        len = len - 1;
    }
}

void config_default(AppConfig *config) {
    if (config == NULL) {
        return;
    }
    memset(config, 0, sizeof(AppConfig));
    strcpy(config->database_path, "restaurant.db");
    strcpy(config->locale, "es");
    strcpy(config->currency, "ARS");
    config->tax_rate = 0.21;
    strcpy(config->receipt_output_dir, "receipts");
}

bool config_load(AppConfig *config, const char *path) {
    FILE *file = NULL;
    char buffer[512];
    if (config == NULL) {
        return false;
    }
    config_default(config);
    file = fopen(path, "r");
    if (file == NULL) {
        return false;
    }
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        char *equals = NULL;
        config_trim(buffer);
        if (buffer[0] == '\0' || buffer[0] == '#') {
            continue;
        }
        equals = strchr(buffer, '=');
        if (equals == NULL) {
            continue;
        }
        *equals = '\0';
        equals = equals + 1;
        if (strcmp(buffer, "database_path") == 0) {
            strncpy(config->database_path, equals, sizeof(config->database_path) - 1);
            config->database_path[sizeof(config->database_path) - 1] = '\0';
        } else if (strcmp(buffer, "locale") == 0) {
            strncpy(config->locale, equals, sizeof(config->locale) - 1);
            config->locale[sizeof(config->locale) - 1] = '\0';
        } else if (strcmp(buffer, "currency") == 0) {
            strncpy(config->currency, equals, sizeof(config->currency) - 1);
            config->currency[sizeof(config->currency) - 1] = '\0';
        } else if (strcmp(buffer, "tax_rate") == 0) {
            config->tax_rate = atof(equals);
        } else if (strcmp(buffer, "receipt_output_dir") == 0) {
            strncpy(config->receipt_output_dir, equals, sizeof(config->receipt_output_dir) - 1);
            config->receipt_output_dir[sizeof(config->receipt_output_dir) - 1] = '\0';
        }
    }
    fclose(file);
    return true;
}

bool config_save(const AppConfig *config, const char *path) {
    FILE *file = NULL;
    if (config == NULL) {
        return false;
    }
    file = fopen(path, "w");
    if (file == NULL) {
        return false;
    }
    fprintf(file, "database_path=%s\n", config->database_path);
    fprintf(file, "locale=%s\n", config->locale);
    fprintf(file, "currency=%s\n", config->currency);
    fprintf(file, "tax_rate=%.4f\n", config->tax_rate);
    fprintf(file, "receipt_output_dir=%s\n", config->receipt_output_dir);
    fclose(file);
    return true;
}
