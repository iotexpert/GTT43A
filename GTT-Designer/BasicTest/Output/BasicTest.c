#include "BasicTest.h"

eStatusCode gtt_get_screen1_circle_button_1_text(gtt_device* device, gtt_text *value)
{
	return gtt25_baseobject_get_property_text(device, id_screen1_circle_button_1, ePropertyID_Button_Text, value);
}

eStatusCode gtt_set_screen1_circle_button_1_text(gtt_device* device, gtt_text value)
{
	return gtt25_baseobject_set_property_text(device, id_screen1_circle_button_1, ePropertyID_Button_Text, value);
}
eStatusCode gtt_get_screen1_circle_button_1_foreground_r(gtt_device* device, uint8_t *value)
{
	return gtt25_baseobject_get_property_u8(device, id_screen1_circle_button_1, ePropertyID_Button_ForegroundR, value);
}

eStatusCode gtt_set_screen1_circle_button_1_foreground_r(gtt_device* device, uint8_t value)
{
	return gtt25_baseobject_set_property_u8(device, id_screen1_circle_button_1, ePropertyID_Button_ForegroundR, value);
}
eStatusCode gtt_get_screen1_circle_button_1_foreground_g(gtt_device* device, uint8_t *value)
{
	return gtt25_baseobject_get_property_u8(device, id_screen1_circle_button_1, ePropertyID_Button_ForegroundG, value);
}

eStatusCode gtt_set_screen1_circle_button_1_foreground_g(gtt_device* device, uint8_t value)
{
	return gtt25_baseobject_set_property_u8(device, id_screen1_circle_button_1, ePropertyID_Button_ForegroundG, value);
}
eStatusCode gtt_get_screen1_circle_button_1_foreground_b(gtt_device* device, uint8_t *value)
{
	return gtt25_baseobject_get_property_u8(device, id_screen1_circle_button_1, ePropertyID_Button_ForegroundB, value);
}

eStatusCode gtt_set_screen1_circle_button_1_foreground_b(gtt_device* device, uint8_t value)
{
	return gtt25_baseobject_set_property_u8(device, id_screen1_circle_button_1, ePropertyID_Button_ForegroundB, value);
}


