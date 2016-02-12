#ifndef NEOPIXEL_H
#define NEOPIXEL_H

void ledsetup();
void sendByte( unsigned char byte );
void sendPixel( unsigned char r, unsigned char g , unsigned char b );
void show();

#endif
