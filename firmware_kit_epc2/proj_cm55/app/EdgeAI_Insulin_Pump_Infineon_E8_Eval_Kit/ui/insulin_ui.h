#ifndef EDGEAI_INSULIN_UI_H
#define EDGEAI_INSULIN_UI_H

#include "../core/insulin_runtime.h"

typedef struct
{
    int reserved;
} insulin_ui_t;

void insulin_ui_init(insulin_ui_t *ui);
void insulin_ui_render(insulin_ui_t *ui, const insulin_runtime_t *runtime);

#endif /* EDGEAI_INSULIN_UI_H */
