/*
 * hw675_cable_display — HW-675 "ESP32-C3 0.42 OLED Mini"
 *
 * On the built-in screen: is the plugged-in USB-C cable carrying DATA from
 * a real host, or power-only (charge cable / dead port)?
 *
 * Detection: Espressif's official usb_serial_jtag_is_connected() — it counts
 * the host's SOF heartbeat packets (every 1 ms from any real USB host).
 * A power bank or charge-only cable never produces them. We latch "DATA"
 * on the first confirmed heartbeat so a host suspending its port later
 * can't flip the verdict back.
 *
 * Hidden diagnostic: hold the on-board BOOT button (GPIO9) to show the raw
 * USB event register + live verdict, for debugging.
 *
 * Board:  ESP32-C3 Dev Module, USB-CDC-on-boot OFF (driver owns the port).
 * Screen: SSD1306 0.42", 72x40 window at buffer origin (27,24).
 *         I2C: SDA=GPIO5, SCL=GPIO6, addr 0x3C.
 */

#include <Wire.h>
#include <U8g2lib.h>
#include "driver/usb_serial_jtag.h"
#include "hal/usb_serial_jtag_ll.h"

const int OX = 27, OY = 24;      // top-left of the visible 72x40 window
const int VW = 72, VH = 40;

const int BIG_Y    = OY + 19;
const int SUB_Y    = OY + 30;
const int UPTIME_Y = OY + 39;

const int BOOT_BTN = 9;          // on-board BOOT button, active LOW
const uint32_t GRACE_MS = 4000;  // verdict delay after power-up

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

bool     linked = false;
uint32_t bootMs = 0;
char     lastBig[12] = "";
char     lastSub[16] = "";
char     lastUp[12]  = "";

void screen(const char* big, const char* sub, const char* up) {
  if (!strcmp(big, lastBig) && !strcmp(sub, lastSub) && !strcmp(up, lastUp))
    return;
  strcpy(lastBig, big); strcpy(lastSub, sub); strcpy(lastUp, up);

  u8g2.clearBuffer();
  u8g2.setClipWindow(OX, OY, OX + VW, OY + VH);
  u8g2.drawFrame(OX, OY, VW, VH);

  if (big[0]) {
    u8g2.setFont(u8g2_font_9x18_tf);
    u8g2.drawUTF8(OX + (VW - u8g2.getUTF8Width(big)) / 2, BIG_Y, big);
  }
  u8g2.setFont(u8g2_font_6x10_tf);
  if (sub[0])
    u8g2.drawUTF8(OX + (VW - u8g2.getUTF8Width(sub)) / 2, SUB_Y, sub);
  if (up[0])
    u8g2.drawUTF8(OX + VW - 2 - u8g2.getUTF8Width(up), UPTIME_Y, up);

  u8g2.sendBuffer();
}

void diag() {                     // BOOT held: raw USB event register + verdict
  char l1[13], l2[13];
  snprintf(l1, sizeof l1, "raw %08lx",
           (unsigned long)usb_serial_jtag_ll_get_intraw_mask());
  snprintf(l2, sizeof l2, "con %d  lnk %d",
           (int)usb_serial_jtag_is_connected(), (int)linked);
  u8g2.clearBuffer();
  u8g2.setClipWindow(OX, OY, OX + VW, OY + VH);
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawUTF8(OX, OY + 10, l1);
  u8g2.drawUTF8(OX, OY + 22, l2);
  u8g2.sendBuffer();
}

void uptimeStr(char* buf, size_t n) {
  uint32_t s = (millis() - bootMs) / 1000;
  snprintf(buf, n, "up %u:%02u", (unsigned)(s / 60), (unsigned)(s % 60));
}

void setup() {
  bootMs = millis();
  pinMode(BOOT_BTN, INPUT_PULLUP);

  usb_serial_jtag_driver_config_t cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
  usb_serial_jtag_driver_install(&cfg);   // official driver: tracks SOF heartbeat

  Wire.begin(5, 6);
  u8g2.setBusClock(400000);
  u8g2.begin();
}

void loop() {
  if (usb_serial_jtag_is_connected()) linked = true;   // latch the verdict

  uint32_t now = millis();
  char up[12]; uptimeStr(up, sizeof up);

  if (!digitalRead(BOOT_BTN)) {
    diag();
  } else if (linked) {
    screen("DATA", "host linked", up);
  } else if (now - bootMs < GRACE_MS) {
    screen("USB", "listening", up);
  } else {
    screen("CHARGE", "power only", up);
  }
  delay(20);
}
