#ifndef DOWNLINK_H
#define DOWNLINK_H

#include <stdint.h>

/* Arm the LoRa radio for continuous RX. Call once at boot after LoRa_Init(). */
void Downlink_ArmRx(void);

/* Poll for one downlink command. Call once per comms loop iteration.
   Returns 1 if a command was received, 0 if nothing arrived. Non-blocking. */
uint8_t Downlink_Poll(void);

#endif
