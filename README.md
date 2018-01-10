# UCONN SDP 1611

## Summary

* #### LCD Daughterboard
  Contains EAGLE files for a graphic LCD and directional pad control. While
  ultimately scrapped, the LCD is electrically functional.

* #### Microcontroller
  Contains EAGLE files for an initial test of the SAMD21 microcontroller circuit.
  The board uses as many through-hole components as possible, and is easily
  assembled by hand. It makes an excellent low-cost substitute to other SAMD21
  development boards, despite its many shortcomings (size, USB functionality).
  Power MUST be applied via the headers, as the USB 5V line is disconnected.

* #### SDP_FIRMWARE
  Contains Arduino code for the latest firmware image for the Solid-State timer
  board. Written for ATSAMD21G18A and requires either the Arduino Zero bootloader
  or compilation by Atmel Studio.

* #### Solid State Relay
  Contains EAGLE files for an initial test of the solid-state relay circuit. The
  board uses as many through-hole components as possible and is easily assembled
  by hand. The board should be able to handle current well in excess of the screw
  terminal ratings, and is fully functional.

* #### Solid-State Timer
  Contains EAGLE files for the primary solid-state timer board. Unlike the former
  tests, this makes use of a significant number of surface mount components and
  requires some patience to assemble. A reflow oven (or toaster oven) is recommended.
  The board supports 8 devices at up to 10A each, and can control each device
  according to a preset sequence up to 240 seconds long in 0.25 second intervals.
  Serial communication over USB is supported for configuration and debugging.
