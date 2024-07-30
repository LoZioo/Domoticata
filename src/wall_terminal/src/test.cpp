#include <Arduino.h>

extern "C" {
	#include <ul_errors.h>
	#include <ul_utils.h>

	#include <button_states.h>
	#include <dup.h>
}

void setup(){
}

void loop(){
	static int c = 0;
	Serial.println(c++);
	delay(1000);
}
