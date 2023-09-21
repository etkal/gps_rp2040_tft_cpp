/*
 * Copyright (c) 2023 Erik Tkal
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "gps_tft.h"

#if defined(RASPBERRYPI_PICO_W)
#include "pico/cyw43_arch.h"
#endif

#define UART_DEVICE    uart_default             // Default is uart0
#define PIN_UART_TX    PICO_DEFAULT_UART_TX_PIN // Default is 0
#define PIN_UART_RX    PICO_DEFAULT_UART_RX_PIN // Default is 1
#define UART_BAUD_RATE 9600
#define DATA_BITS      8
#define STOP_BITS      1
#define PARITY         UART_PARITY_NONE

#if defined(RASPBERRYPI_PICO) || defined(RASPBERRYPI_PICO_W)
#define SPI_DEVICE spi_default              // Default is SPI0 for Pico
#define PIN_MISO   PICO_DEFAULT_SPI_RX_PIN  // White  16
#define PIN_CS     PICO_DEFAULT_SPI_CSN_PIN // Org    17
#define PIN_SCK    PICO_DEFAULT_SPI_SCK_PIN // Purple 18
#define PIN_MOSI   PICO_DEFAULT_SPI_TX_PIN  // Blue   19
#define PIN_RST    20                       // Yellow
#define PIN_DC     21                       // Green
#elif defined(SEEED_XIAO_RP2040)
// XIAO has spi0 CSn overlap with uart0, so override
#define SPI_DEVICE spi_default              // Default is SPI0 for XIAO
#define PIN_MISO   PICO_DEFAULT_SPI_RX_PIN  // White  4
#define PIN_CS     26                       // Orange
#define PIN_SCK    PICO_DEFAULT_SPI_SCK_PIN // Purple 2
#define PIN_MOSI   PICO_DEFAULT_SPI_TX_PIN  // Blue   3
#define PIN_RST    27                       // Yellow
#define PIN_DC     28                       // Green
#elif defined(WAVESHARE_RP2040_ZERO)
// RP2040-Zero has default spi1, which is on the bottom, so use spi0
#define SPI_DEVICE spi0 // override
#define PIN_MISO   4    // White
#define PIN_CS     5    // Org
#define PIN_SCK    6    // Purple
#define PIN_MOSI   7    // Blue
#define PIN_RST    14   // Yellow
#define PIN_DC     15   // Green
#else
#error unknown board
#endif

// #define USE_WS2812_PIN 12 // Override
// #define USE_LED_PIN 16    // Override

int main()
{
    stdio_init_all();

    // Set up UART for GPS device
    uart_init(UART_DEVICE, UART_BAUD_RATE);
    gpio_set_function(PIN_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_UART_RX, GPIO_FUNC_UART);
    uart_set_hw_flow(UART_DEVICE, false, false);
    uart_set_format(UART_DEVICE, DATA_BITS, STOP_BITS, PARITY);

    // Set up the TFT display
    spi_init(SPI_DEVICE, 40000000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);

#if defined(SEEED_XIAO_RP2040)
    // Clear LED(s) on XIAO (default on)
    LED_pico ledBlue(25);  // blue
    LED_pico ledGreen(16); // green
    LED_pico ledRed(17);   // red
#endif

#if defined(RASPBERRYPI_PICO_W)
    cyw43_arch_init();
#endif

#if defined(USE_WS2812_PIN)
    LED* pLED = new LED_neo(1, USE_WS2812_PIN);
    pLED->init();
    pLED->setPixel(0, led_green);
#elif defined(PICO_DEFAULT_WS2812_PIN) && !defined(USE_LED_PIN)
    LED* pLED = new LED_neo(1, PICO_DEFAULT_WS2812_PIN);
    pLED->init();
    pLED->setPixel(0, led_green);
#elif defined(USE_LED_PIN)
    LED* pLED = new LED_pico(USE_LED_PIN);
    pLED->setIgnore({led_red});
#elif defined(PICO_DEFAULT_LED_PIN)
    LED* pLED = new LED_pico(PICO_DEFAULT_LED_PIN);
    pLED->setIgnore({led_red});
#elif defined(RASPBERRYPI_PICO_W)
    LED* pLED = new LED_pico_w(CYW43_WL_GPIO_LED_PIN);
    pLED->setIgnore({led_red});
#else
    LED* pLED = nullptr;
#endif
    GPS* pGPS = new GPS(UART_DEVICE);

    // Create the display.  ILI9341, w=320 h=240 rotate 270
    ILI934X* pDisplay = new ILI934X(SPI_DEVICE, PIN_CS, PIN_DC, PIN_RST, 320, 240, R270DEG, COLOUR_ORDER_BGR);
    sleep_ms(500);

    GPS_TFT* pDevice = new GPS_TFT(pDisplay, pGPS, pLED);

    pDevice->init();
    pDevice->run();

    return 0;
}
