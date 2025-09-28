#ifndef CORE_AUTH_SERVICE_H
#define CORE_AUTH_SERVICE_H

#include <stdbool.h>
#include "app_context.h"
#include "data/dao.h"

bool auth_login(AppContext *ctx, const char *username, const char *password, User *user_out);

#endif
