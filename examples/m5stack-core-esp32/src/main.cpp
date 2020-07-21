#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <hx711.h>
#include <ili9341.hpp>

extern "C" {
void app_main();
}


void sprint_bin(char * buffer, int32_t value) {
	char t[16];
	sprintf(t, "%o", value);

	size_t j = 0;
	char oct[8][4] = {
		"000", "001", "010", "011", "100", "101", "110", "111"
	};
	for(size_t i =0; t[i] != '\0'; i++) {
		sprintf(buffer + j, "%s", oct[t[i] - '0']);
		j += 3;
	}
}

void app_main() {
	LCD::ILI9341 *lcd = new LCD::ILI9341();

	xTaskCreatePinnedToCore([](void *lcd_p) {
		LCD::ILI9341 *lcd = (LCD::ILI9341 *)lcd_p;

		HX711 *scale1 = new HX711((gpio_num_t)21, (gpio_num_t)22);
		HX711 *scale2 = new HX711((gpio_num_t)5, (gpio_num_t)2);
		char buf[64];

		int32_t s1 = 0, s2 = 0;

		int32_t history[11];
		size_t h = 0;

		while(scale1->is_ready()) {
			s1 = scale1->read();
			for(int i=0; i<11; i++) {
				history[i] = s1;
			}
		}

		char lines[11][64];
		char l[64];
		for(int i=0; i<11; i++) lines[i][0] = '\0';


		while (true) {
			vTaskDelay(10);

			lcd->clear(LCD::BLACK);
			if (scale1->is_ready()) {
				s1 = scale1->read();
				sprintf(buf, "SCALE1: %d      ", s1);
				lcd->drawString(20, 25, LCD::WHITE, buf);

				history[h++] = s1;
				if (h >= 11) h = 0;

				int32_t sum = 0;
				for(int i=0; i<11; i++) sum += history[i];
				int32_t ave = sum / 11;
				size_t t = h + 6;
				while (t >= 11) t-=11;
				sprintf(buf, "%d, %d (%d)", sum, ave, history[t]);
				lcd->drawString(50, 3, LCD::WHITE, buf);
				float diff = history[t] - ave;
				float thr = ave * 0.2f;
				diff *= diff;
				thr *= thr;
				if (diff > thr) {
					for(int i=0; i<11; i++) {
						size_t n = t + 6 + i;
						while(n >= 11) n -= 11;
						sprint_bin(l, history[n]);
						sprintf(lines[n], "%33s %d", l, history[n]);
					}
				}
			}
			for(int i=0; i<11; i++) lcd->drawString(5, 70 + i * 15, LCD::WHITE, lines[i]);

			if (scale2->is_ready()) {
				s2 = scale2->read();
				sprintf(buf, "SCALE2: %d      ", s2);
				lcd->drawString(20, 40, LCD::WHITE, buf);
			}
			
			lcd->update();
		}
	}, "HX711", 4096, lcd, 1, NULL, 1);
}
