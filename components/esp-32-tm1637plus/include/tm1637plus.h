#ifndef __TM1637___H
#define __TM1637___H
#include <inttypes.h>
#include <stdbool.h>
#include <driver/gpio.h>
#include <vector>

class TM1637
{
public:
    explicit TM1637(gpio_num_t pin_clk, gpio_num_t pin_dta);
    ~TM1637();

    static uint8_t encode_digit(const int digit);
    static uint8_t encode_char(const char o);
    static const std::vector<uint8_t> &encode_string(const char *s);
    static const std::vector<uint8_t> &hex(uint16_t val);
    static const std::vector<uint8_t> &number(uint16_t val);
    void brightness(const uint8_t val);
    uint8_t brightness();
    void write(std::vector<uint8_t> segments, int pos = 0);
    void show(const char *s, bool colon = false);
    void set_raw(const uint8_t pos, const uint8_t data);
    void set_char(const uint8_t pos, const char ch, const bool dot);
    void scroll(const char *s, int delay = 250);

private:
    gpio_num_t m_pin_dta;
    gpio_num_t m_pin_clk;
    uint8_t m_brightness;

    void _delay();
    void _write_byte(uint8_t b);
    void _start();
    void _stop();
    void _write_data_cmd();
    void _write_dsp_ctrl();
};

#endif