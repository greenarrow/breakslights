/*
 * C wrapper around Adafruit_NeoPixel.
 */

#include <Arduino.h>
#include "Adafruit_NeoPixel.h"
#ifdef __AVR__
#include <avr/power.h>
#endif

extern "C"
{

#include "config.h"
#include "pixel.h"

void pixel_init(struct pixel *p, byte pin)
{
	Adafruit_NeoPixel *np = new Adafruit_NeoPixel(RING_PIXELS, pin,
						NEO_GRB + NEO_KHZ800);

	np->begin();
	p->np = np;
}

void pixel_destroy(struct pixel *p)
{
	delete p->np;
}

void pixel_set(struct pixel *p, byte n, struct colour c)
{
	((Adafruit_NeoPixel *)p->np)->setPixelColor(n,
				((Adafruit_NeoPixel *)p->np)->Color(c.r, c.g, c.b));
}

void pixel_flush(struct pixel *p)
{
	((Adafruit_NeoPixel *)p->np)->show();
}

}
