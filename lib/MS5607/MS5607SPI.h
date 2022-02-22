/*
Copyright (c) 2012, Senio Networks, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef MS5607_SPI_H
#define MS5607_SPI_H

#include "MS5607Base.h"

class MS5607SPI : public MS5607Base {
public:
    MS5607SPI(PinName mosi, PinName miso, PinName sclk, PinName csb) : spi(mosi, miso, sclk), csb(csb) {
        init();
    }

private:
    SPI spi;
    DigitalOut csb;

    virtual void writeCommand(int command, int ms = 0) {
        csb = 0;
        spi.write(command);
        if (ms) wait_ms(ms);
        csb = 1;
    }

    virtual int readPROM(int address) {
        csb = 0;
        spi.write(PROM_READ | address << 1);
        int hi = spi.write(0);
        int low = spi.write(0);
        csb = 1;
        return hi << 8 | low;
    }

    virtual int readADC(int command) {
        csb = 0;
        spi.write(ADC_CONV | command);
        static int duration[] = {500, 1100, 2100, 4100, 8220};
        wait_us(duration[(command & 0x0F) >> 1]);
        csb = 1;
        csb = 0;
        spi.write(ADC_READ);
        int hi = spi.write(0);
        int mid = spi.write(0);
        int low = spi.write(0);
        csb = 1;
        return hi << 16 | mid << 8 | low;
    }
};

#endif