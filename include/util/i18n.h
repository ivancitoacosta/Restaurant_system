#ifndef UTIL_I18N_H
#define UTIL_I18N_H

#include <stdbool.h>

#define I18N_MAX_KEY 128
#define I18N_MAX_VALUE 256

typedef struct I18nEntry {
    char key[I18N_MAX_KEY];
    char value[I18N_MAX_VALUE];
} I18nEntry;

typedef struct I18nCatalog {
    I18nEntry *entries;
    int count;
} I18nCatalog;

bool i18n_load(I18nCatalog *catalog, const char *path);
const char *i18n_translate(const I18nCatalog *catalog, const char *key, const char *fallback);
void i18n_free(I18nCatalog *catalog);

#endif
