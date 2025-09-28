#include "core/auth_service.h"
#include "util/hash.h"
#include <string.h>
#include <stdio.h>

static void auth_compute_hash(const char *username, const char *password, char *output, size_t output_len) {
    char buffer[256];
    size_t needed = strlen(username) + strlen(password) + 2;
    if (needed >= sizeof(buffer)) {
        needed = sizeof(buffer) - 1;
    }
    snprintf(buffer, sizeof(buffer), "%s:%s", username, password);
    hash_sha256_hex((const unsigned char *)buffer, strlen(buffer), output, output_len);
}

bool auth_login(AppContext *ctx, const char *username, const char *password, User *user_out) {
    User user;
    char expected_hash[65];
    if (!dao_get_user_by_username(ctx->db, username, &user)) {
        logger_log(&ctx->logger, LOG_LEVEL_WARN, "auth", "Usuario inexistente");
        return false;
    }
    auth_compute_hash(username, password, expected_hash, sizeof(expected_hash));
    if (strcmp(expected_hash, user.password_hash) != 0) {
        logger_log(&ctx->logger, LOG_LEVEL_WARN, "auth", "Password inválido");
        return false;
    }
    if (user_out != NULL) {
        *user_out = user;
    }
    logger_log(&ctx->logger, LOG_LEVEL_INFO, "auth", "Login exitoso");
    return true;
}
