#ifndef BOARD_TEMP_SENSOR_H
#define BOARD_TEMP_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

void board_temp_sensor_init(void);
bool board_temp_sensor_read_c10(int16_t *temp_c10_out);

#endif /* BOARD_TEMP_SENSOR_H */
