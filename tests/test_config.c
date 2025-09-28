#include <assert.h>
#include <string.h>
#include "util/config.h"
#include "util/hash.h"

static void test_config_default_values(void) {
    AppConfig config;
    config_default(&config);
    assert(strcmp(config.database_path, "restaurant.db") == 0);
    assert(strcmp(config.locale, "es") == 0);
    assert(strcmp(config.currency, "ARS") == 0);
    assert(config.tax_rate > 0.2);
}

static void test_hash_sha256(void) {
    char output[65];
    hash_sha256_hex((const unsigned char *)"abc", 3, output, sizeof(output));
    assert(strcmp(output, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") == 0);
}

int main(void) {
    test_config_default_values();
    test_hash_sha256();
    return 0;
}
