#pragma once

#include "configuration.h"

#ifdef HAS_RGB_STATUS_LED

#include "concurrency/OSThread.h"
#include <Adafruit_NeoPixel.h>

/*
 * Status indicator on a single WS2812 (NeoPixel) RGB LED (RGB_STATUS_LED_PIN).
 *
 *   - LoRa packet received     : short burst of fast green flicker
 *   - LoRa transmit in progress: fast red flicker (old HDD activity LED)
 *   - channel message received : white blinking (half brightness) for 3 s
 *   - direct message received  : blue blinking (half brightness) for 3 s
 *
 * Event methods only record state; all LED writes happen in loop() (run by
 * RgbStatusThread every few ms), so they are safe to call from the radio
 * code. A message blink takes priority over the TX flicker, so the ACK sent
 * right after a direct message does not hide the blue blink.
 * Enabled state follows config.device.led_heartbeat_disabled (toggled by the
 * user button, visible in the app as "LED heartbeat disabled").
 */
class RgbStatusLed
{
  public:
    RgbStatusLed(uint8_t pin) : _pixel(1, pin, NEO_GRB + NEO_KHZ800) {}

    void begin();
    void loop();
    void off();
    void setEnabled(bool on); // with a short green/red feedback blink
    bool isEnabled() const { return _enabled; }

    void onTxStart() { _tx = true; }
    void onTxEnd() { _tx = false; }
    void onPacketReceived() { _rx_until = millis() + RX_BURST_MS; }
    void onChannelMessage() { startBlink(128, 128, 128); }
    void onDirectMessage() { startBlink(0, 0, 128); }

  private:
    static const uint32_t MSG_BLINK_MS = 3000; // total duration of a message blink
    static const uint32_t MSG_HALF_MS = 250;   // on/off half period (2 Hz)
    static const uint32_t FLICKER_MS = 30;     // toggle period of the TX/RX flicker
    static const uint32_t RX_BURST_MS = 240;   // length of the flicker burst per received packet
    static const uint8_t ACTIVITY_LEVEL = 40;  // TX/RX flicker brightness (messages use 128)

    void startBlink(uint8_t r, uint8_t g, uint8_t b, uint32_t ms = MSG_BLINK_MS);
    void show(uint32_t color);

    Adafruit_NeoPixel _pixel;
    uint32_t _last_color = 0;
    bool _enabled = true;
    bool _tx = false;
    uint32_t _rx_until = 0;
    uint32_t _blink_start = 0, _blink_until = 0;
    uint32_t _blink_color = 0;
};

class RgbStatusThread : public concurrency::OSThread
{
  public:
    RgbStatusThread();
    int32_t runOnce() override;
};

extern RgbStatusLed rgb_led;
extern RgbStatusThread *rgbStatusThread;

#endif // HAS_RGB_STATUS_LED
