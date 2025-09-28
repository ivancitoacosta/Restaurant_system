#ifndef UI_MAIN_WINDOW_H
#define UI_MAIN_WINDOW_H

#include <gtk/gtk.h>
#include "app_context.h"

GtkWidget *ui_main_window_new(AppContext *ctx, GtkApplication *app);
void ui_main_window_show_login(GtkWidget *window);

#endif
