/**
 *
 * HX711 library for Arduino
 * https://github.com/bogde/HX711
 *
 * MIT License
 * (c) 2018 Bogdan Necula
 *
**/
#include "hx711.h"

#define HIGH 1
#define LOW 0

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

uint8_t HX711::shiftIn(gpio_num_t dataPin, gpio_num_t clockPin) {
	uint8_t value = 0;
	uint8_t i;

	for (i = 0; i < 8; ++i) {
		gpio_set_level(clockPin, HIGH);
		vTaskDelay(1);
		value |= gpio_get_level(dataPin) << (7 - i);
		gpio_set_level(clockPin, LOW);
		vTaskDelay(1);
	}
	return value;
}

HX711::HX711(gpio_num_t dout, gpio_num_t pd_sck, Gain gain) {
	this->pd_sck = pd_sck;
	this->dout   = dout;

	gpio_pad_select_gpio(pd_sck);
	gpio_set_direction(pd_sck, GPIO_MODE_OUTPUT);
	gpio_pad_select_gpio(dout);
	gpio_set_direction(dout, GPIO_MODE_INPUT);
	gpio_set_pull_mode(dout, GPIO_PULLUP_ONLY);

	set_gain(gain);
}

HX711::~HX711() {}

int32_t HX711::read() {
	// Wait for the chip to become ready.
	wait_ready();

	// Define structures for reading data into.
	uint32_t value	 = 0;
	uint8_t data[3] = {0};
	uint8_t filler	 = 0x00;

	// Protect the read sequence from system interrupts.  If an interrupt occurs during
	// the time the pd_sck signal is high it will stretch the length of the clock pulse.
	// If the total pulse time exceeds 60 uSec this will cause the HX711 to enter
	// power down mode during the middle of the read sequence.  While the device will
	// wake up when pd_sck goes low again, the reset starts a new conversion cycle which
	// forces dout high until that cycle is completed.
	//
	// The result is that all subsequent bits read by shiftIn() will read back as 1,
	// corrupting the value returned by read().  The ATOMIC_BLOCK macro disables
	// interrupts during the sequence and then restores the interrupt mask to its previous
	// state after the sequence completes, insuring that the entire read-and-gain-set
	// sequence is not interrupted.  The macro has a few minor advantages over bracketing
	// the sequence between `noInterrupts()` and `interrupts()` calls.

	// Begin of critical section.
	// Critical sections are used as a valid protection method
	// against simultaneous access in vanilla FreeRTOS.
	// Disable the scheduler and call portDISABLE_INTERRUPTS. This prevents
	// context switches and servicing of ISRs during a critical section.
	portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
	portENTER_CRITICAL(&mux);

	// Pulse the clock pin 24 times to read the data.
	data[2] = shiftIn(dout, pd_sck);
	data[1] = shiftIn(dout, pd_sck);
	data[0] = shiftIn(dout, pd_sck);

	// Set the channel and the gain factor for the next reading using the clock pin.
	for (uint16_t i = 0; i < gain; i++) {
		gpio_set_level((gpio_num_t)pd_sck, HIGH);
		vTaskDelay(1);

		gpio_set_level((gpio_num_t)pd_sck, LOW);
		vTaskDelay(1);
	}

	// End of critical section.
	portEXIT_CRITICAL(&mux);

	// Replicate the most significant bit to pad out a 32-bit signed integer
	if (data[2] & 0x80) {
		filler = 0xFF;
	} else {
		filler = 0x00;
	}

	// Construct a 32-bit signed integer
	value = (static_cast<uint32_t>(filler) << 24 | static_cast<uint32_t>(data[2]) << 16 | static_cast<uint32_t>(data[1]) << 8 | static_cast<uint32_t>(data[0]));

	return static_cast<int32_t>(value);
}

void HX711::wait_ready(uint32_t delay_ms) {
	// Wait for the chip to become ready.
	// This is a blocking implementation and will
	// halt the sketch until a load cell is connected.
	while (!is_ready()) {
		// Probably will do no harm on AVR but will feed the Watchdog Timer (WDT) on ESP.
		// https://github.com/bogde/HX711/issues/73
		ets_delay_us(delay_ms * 1000 + 1);
	}
}

bool HX711::wait_ready_retry(int retries, uint32_t delay_ms) {
	// Wait for the chip to become ready by
	// retrying for a specified amount of attempts.
	// https://github.com/bogde/HX711/issues/76
	int count = 0;
	while (count < retries) {
		if (is_ready()) {
			return true;
		}
		ets_delay_us(delay_ms * 1000 + 1);
		count++;
	}
	return false;
}

bool HX711::wait_ready_timeout(uint32_t timeout, uint32_t delay_ms) {
	// Wait for the chip to become ready until timeout.
	// https://github.com/bogde/HX711/pull/96
	uint64_t t		   = timeout * 1000;
	uint32_t microsStarted = esp_timer_get_time();
	while (esp_timer_get_time() - microsStarted < t) {
		if (is_ready()) {
			return true;
		}
		ets_delay_us(delay_ms * 1000);
	}
	return false;
}

int32_t HX711::read_average(uint8_t times) {
	int32_t sum = 0;
	for (uint8_t i = 0; i < times; i++) {
		sum += read();
		// Probably will do no harm on AVR but will feed the Watchdog Timer (WDT) on ESP.
		// https://github.com/bogde/HX711/issues/73
		ets_delay_us(1);
	}
	return sum / times;
}

double HX711::get_value(uint8_t times) {
	return read_average(times) - offset;
}

float HX711::get_units(uint8_t times) {
	return get_value(times) / scale;
}

void HX711::tare(uint8_t times) {
	double sum = read_average(times);
	set_offset(sum);
}
