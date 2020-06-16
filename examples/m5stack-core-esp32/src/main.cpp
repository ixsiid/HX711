#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <hx711.h>
#include <ili9341.hpp>

extern "C" {
void app_main();
}

void app_main() {
	LCD::ILI9341 *lcd = new LCD::ILI9341();

	xTaskCreatePinnedToCore([](void *lcd_p) {
		LCD::ILI9341 *lcd = (LCD::ILI9341 *)lcd_p;

		HX711 *scale1 = new HX711((gpio_num_t)21, (gpio_num_t)22);
		HX711 *scale2 = new HX711((gpio_num_t)5, (gpio_num_t)2);
		char buf[64];

		int32_t s1 = 0, s2 = 0;

		while (true) {
			vTaskDelay(10);

			lcd->clear(LCD::BLACK);
			if (scale1->is_ready()) {
				s1 = scale1->read();
				sprintf(buf, "SCALE1: %d      ", s1);
				lcd->drawString(20, 25, LCD::WHITE, buf);
			}
			if (scale2->is_ready()) {
				s2 = scale2->read();
				sprintf(buf, "SCALE2: %d      ", s2);
				lcd->drawString(20, 50, LCD::WHITE, buf);
			}
			sprintf(buf, "SUM   : %d      ", s1 + s2);
			lcd->drawString(20, 80, LCD::WHITE, buf);
			lcd->update();
		}
	}, "HX711", 4096, lcd, 1, NULL, 1);
}
