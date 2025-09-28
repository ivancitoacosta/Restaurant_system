#include "util/i18n.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

static void i18n_trim(char *text) {
    size_t len = strlen(text);
    while (len > 0 && (text[len - 1] == '\n' || text[len - 1] == '\r' || text[len - 1] == ' ' || text[len - 1] == '\t')) {
        text[len - 1] = '\0';
        len = len - 1;
    }
}

bool i18n_load(I18nCatalog *catalog, const char *path) {
    FILE *file = NULL;
    char buffer[512];
    int capacity = 0;
    if (catalog == NULL) {
        return false;
    }
    catalog->entries = NULL;
    catalog->count = 0;
    file = fopen(path, "r");
    if (file == NULL) {
        return false;
    }
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        char *equals = NULL;
        i18n_trim(buffer);
        if (buffer[0] == '\0' || buffer[0] == '#') {
            continue;
        }
        equals = strchr(buffer, '=');
        if (equals == NULL) {
            continue;
        }
        *equals = '\0';
        equals = equals + 1;
        if (catalog->count == capacity) {
            int new_capacity = capacity == 0 ? 16 : capacity * 2;
            I18nEntry *new_entries = realloc(catalog->entries, sizeof(I18nEntry) * new_capacity);
            if (new_entries == NULL) {
                fclose(file);
                return false;
            }
            catalog->entries = new_entries;
            capacity = new_capacity;
        }
        g_strlcpy(catalog->entries[catalog->count].key, buffer, I18N_MAX_KEY);
        g_strlcpy(catalog->entries[catalog->count].value, equals, I18N_MAX_VALUE);
        catalog->count = catalog->count + 1;
    }
    fclose(file);
    return true;
}

const char *i18n_translate(const I18nCatalog *catalog, const char *key, const char *fallback) {
    int index = 0;
    if (catalog == NULL || key == NULL) {
        return fallback;
    }
    while (index < catalog->count) {
        if (strcmp(catalog->entries[index].key, key) == 0) {
            return catalog->entries[index].value;
        }
        index = index + 1;
    }
    return fallback;
}

void i18n_free(I18nCatalog *catalog) {
    if (catalog == NULL) {
        return;
    }
    free(catalog->entries);
    catalog->entries = NULL;
    catalog->count = 0;
}
