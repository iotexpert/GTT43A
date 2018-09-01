#ifndef __BASICTEST_H
#define __BASICTEST_H

#include <gtt_protocol.h>

/* Module definition for : GTT43A */
#define gtt_module 6144
/* Objects for screen : Screen1 */
#define id_screen1_circle_button_1 1

/* Objects for screen : Screen2 */


eStatusCode gtt_get_screen1_circle_button_1_text(gtt_device* device, gtt_text  *value);
eStatusCode gtt_set_screen1_circle_button_1_text(gtt_device* device, gtt_text  value);
eStatusCode gtt_get_screen1_circle_button_1_foreground_r(gtt_device* device, uint8_t *value);
eStatusCode gtt_set_screen1_circle_button_1_foreground_r(gtt_device* device, uint8_t value);
eStatusCode gtt_get_screen1_circle_button_1_foreground_g(gtt_device* device, uint8_t *value);
eStatusCode gtt_set_screen1_circle_button_1_foreground_g(gtt_device* device, uint8_t value);
eStatusCode gtt_get_screen1_circle_button_1_foreground_b(gtt_device* device, uint8_t *value);
eStatusCode gtt_set_screen1_circle_button_1_foreground_b(gtt_device* device, uint8_t value);


#endif
