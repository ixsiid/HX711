/**
 *
 * HX711 library for Arduino
 * https://github.com/ixsiid/HX711
 * 
 * Based on https://github.com/bogde/HX711
 *
 * MIT License
 *
**/
#pragma once

#include <sys/types.h>

#include <driver/gpio.h>

enum Gain { Gain_32	 = 2,
		  Gain_64	 = 3,
		  Gain_128 = 1 };

class HX711 {
    private:
	gpio_num_t pd_sck;	// Power Down and Serial Clock Input Pin
	gpio_num_t dout;	// Serial Data Output Pin
	Gain gain;		// amplification factor

	int32_t offset = 0;	 // used for tare weight
	float scale	= 1;	 // used to return weight in grams, kg, ounces, whatever

    public:
	// Initialize library with data output pin, clock input pin and gain factor.
	// Channel selection is made by passing the appropriate gain:
	// - With a gain factor of 64 or 128, channel A is selected
	// - With a gain factor of 32, channel B is selected
	// The library default is "128" (Channel A).
	HX711(gpio_num_t dout, gpio_num_t pd_sck, Gain gain = Gain_128);
	virtual ~HX711();

	// Wait for the HX711 to become ready
	void wait_ready(uint32_t delay_ms = 0);
	bool wait_ready_retry(int retries = 3, uint32_t delay_ms = 0);
	bool wait_ready_timeout(uint32_t timeout = 1000, uint32_t delay_ms = 0);

	// set the gain factor; takes effect only after a call to read()
	// channel A can be set for a 128 or 64 gain; channel B has a fixed 32 gain
	// depending on the parameter, the channel is also set to either A or B
	void set_gain(Gain gain = Gain_128);

	// waits for the chip to be ready and returns a reading
	int32_t read();

	// returns an average reading; times = how many times to read
	int32_t read_average(uint8_t times = 10);

	// returns (read_average() - OFFSET), that is the current value without the tare weight; times = how many readings to do
	double get_value(uint8_t times = 1);

	// returns get_value() divided by SCALE, that is the raw value divided by a value obtained via calibration
	// times = how many readings to do
	float get_units(uint8_t times = 1);

	// set the OFFSET value for tare weight; times = how many times to read the tare value
	void tare(uint8_t times = 10);

	/* below, inline function */

	// Check if HX711 is ready
	// from the datasheet: When output data is not ready for retrieval, digital output pin DOUT is high. Serial clock
	// input PD_SCK should be low. When DOUT goes to low, it indicates data is ready for retrieval.
	bool is_ready();

	// set the SCALE value; this value is used to convert the raw data to "human readable" data (measure units)
	void set_scale(float scale = 1.f);

	// get the current SCALE
	float get_scale();

	// set OFFSET, the value that's subtracted from the actual reading (tare weight)
	void set_offset(int32_t offset = 0);

	// get the current OFFSET
	int32_t get_offset();

	// puts the chip into power down mode
	void power_down();

	// wakes up the chip after power down mode
	void power_up();

    private:
	static uint8_t shiftIn(gpio_num_t dataPin, gpio_num_t clockPin);
};

inline bool HX711::is_ready() { return gpio_get_level(dout) == 0; }
inline void HX711::set_gain(Gain gain) { this->gain = gain; }
inline void HX711::set_scale(float scale) { this->scale = scale; }
inline float HX711::get_scale() { 	return scale; }
inline void HX711::set_offset(int32_t offset) { this->offset = offset;}
inline int32_t HX711::get_offset() { return offset;}

inline void HX711::power_down() {
	gpio_set_level((gpio_num_t)pd_sck, 0);
	gpio_set_level((gpio_num_t)pd_sck, 1);
}

inline void HX711::power_up() {
	gpio_set_level((gpio_num_t)pd_sck, 0);
}
