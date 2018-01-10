/* ME Team 9 Senior Design Project - Microcontroller Firmware v0.4
 * Created 2016-10-26
 * Updated 2016-10-26 (v0.1)
 * Updated 2016-11-07 (v0.2)
 * Updated 2017-01-13 (v0.3)
 * Updated 2017-04-24 (v0.4)
 * 
 * Designer: Patrick Dunham
 * 
 * CHANGELOG
 * -v0.4
 *     -Start Condition check removed - timer starts on boot
 *     -RELAY_PINS definition changed to correspond to new pin mapping - again
 *     -Added preliminary support for USB programming state
 * -v0.3 
 *     -RELAY_PINS definition changed to correspond to new pin mapping
 *     -MAX_CYCLE_TIME increased to proper values
 *     -Demo now counts in binary
 *     -New pin mapping
 *     -Rewrote initRelayOutputs(), initPins(), setRelayOutputs(), to use register update method
 * -v0.2 [TESTED-WORKING]
 *     -RELAY_PINS definition corrected
 *     -MAX_CYCLE_TIME and associated variables reduced for testing purposes
 * -v0.1
 *     -Code created
 * Notes (v0.4)
 * -Relay Pin initiation (configured for ACTIVE HIGH)
 * -Relay Pin set        (configured for ACTIVE HIGH)
 * -Relay Pin clear      (configured for ACTIVE HIGH)
 * -Relay State Machine Logic
 *    -IDLE_STATE: Waits for timer to start
 *    -CYCLE_STATE: Runs through timer sequence
 *    -RESET_STATE: Timer & state variables are reset
 *    -USB_PROGRAM_STATE: Timer can be configured
 * 
 * NEEDS:
 * -Method of changing relay outputs via UI
 * -Method of changing cycle time
 *
 * -UI state machine
 */

/***** LIBRARY INCLUDES *****/
#include <Wire.h>
#include <I2C_eeprom.h>



/***** CONSTANT DEFINITIONS - THESE CAN BE CHANGED *****/

// EEPROM Data
#define EEPROM_SIZE 1024
#define EEPROM_ADDRESS 0x58
#define PAUSE_RESUME_START 0
#define RELAY_TABLE_START 480
#define CYCLE_LENGTH_START 960
#define STATE_FINISHED_CHAR 'U'

// Relay Pin ID     31..............................0
#define RELAY_PINS 0b00000000000000000000001111111100UL
#define NUM_OUTPUTS 8

// Button Input Pin Definition - Used to check start condition
#define START_INPUT 3

// Time in ms between relay pin updates 
#define UPDATE_INTERVAL 250

// Upper limit on cycle time in seconds - need for array preallocation
#define MAX_CYCLE_TIME 120

// System state definitions
#define IDLE_STATE        0 // Does nothing
#define CYCLE_STATE       1 // Cycles through relay outputs
#define RESET_STATE       2 // Resets relay outputs and control variables
#define USB_PROGRAM_STATE 3 // Allows configuration of timer parameters

// Character codes
#define USB_PROG_CHAR 'P'

const unsigned int relayOutputsPreset[ 480 ] = {0b00000000,
                                     0b00000001,
                                     0b00000010,
                                     0b00000011,
                                     0b00000100,
                                     0b00000101,
                                     0b00000110,
                                     0b00000111,
                                     0b00001000,
                                     0b00001001,
                                     0b00001010,
                                     0b00001100,
                                     0b00001101,
                                     0b00001110,
                                     0b00001111,
                                     0b00010000,
                                     0b00010001,
                                     0b00010010,
                                     0b00010011,
                                     0b00010100,
                                     0b00010101,
                                     0b00010110,
                                     0b00010111,
                                     0b00011000,
                                     0b00011001,
                                     0b00011010,
                                     0b00011011,
                                     0b00011100,
                                     0b00011101,
                                     0b00011110,
                                     0b00011111,
                                     0b00100000,
                                     0b00100001,
                                     0b00100010,
                                     0b00100011,
                                     0b00100100,
                                     0b00100101,
                                     0b00100110,
                                     0b00100111,
                                     0b00101000,
                                     0b00101001,
                                     0b00101010,
                                     0b00101011,
                                     0b00101100,
                                     0b00101101,
                                     0b00101110,
                                     0b00101111,
                                     0b00110000,
                                     0b00110001,
                                     0b00110010,
                                     0b00110011,
                                     0b00110100,
                                     0b00110101,
                                     0b00110110,
                                     0b00110111,
                                     0b00111000,
                                     0b00111001,
                                     0b00111010,
                                     0b00111011,
                                     0b00111100,
                                     0b00111101,
                                     0b00111110,
                                     0b00111111,
                                     0b01000000,
                                     0b01000001,
                                     0b01000010,
                                     0b01000011,
                                     0b01000100,
                                     0b01000101,
                                     0b01000110,
                                     0b01000111,
                                     0b01001000,
                                     0b01001001,
                                     0b01001010,
                                     0b01001100,
                                     0b01001101,
                                     0b01001110,
                                     0b01001111,
                                     0b01010000,
                                     0b01010001,
                                     0b01010010,
                                     0b01010011,
                                     0b01010100,
                                     0b01010101,
                                     0b01010110,
                                     0b01010111,
                                     0b01011000,
                                     0b01011001,
                                     0b01011010,
                                     0b01011011,
                                     0b01011100,
                                     0b01011101,
                                     0b01011110,
                                     0b01011111,
                                     0b01100000,
                                     0b01100001,
                                     0b01100010,
                                     0b01100011,
                                     0b01100100,
                                     0b01100101,
                                     0b01100110,
                                     0b01100111,
                                     0b01101000,
                                     0b01101001,
                                     0b01101010,
                                     0b01101011,
                                     0b01101100,
                                     0b01101101,
                                     0b01101110,
                                     0b01101111,
                                     0b01110000,
                                     0b01110001,
                                     0b01110010,
                                     0b01110011,
                                     0b01110100,
                                     0b01110101,
                                     0b01110110,
                                     0b01110111,
                                     0b01111000,
                                     0b01111001,
                                     0b01111010,
                                     0b01111011,
                                     0b01111100,
                                     0b01111101,
                                     0b01111110,
                                     0b01111111,
                                     0b10000000,
                                     0b10000001,
                                     0b10000010,
                                     0b10000011,
                                     0b10000100,
                                     0b10000101,
                                     0b10000110,
                                     0b10000111,
                                     0b10001000,
                                     0b10001001,
                                     0b10001010,
                                     0b10001100,
                                     0b10001101,
                                     0b10001110,
                                     0b10001111,
                                     0b10010000,
                                     0b10010001,
                                     0b10010010,
                                     0b10010011,
                                     0b10010100,
                                     0b10010101,
                                     0b10010110,
                                     0b10010111,
                                     0b10011000,
                                     0b10011001,
                                     0b10011010,
                                     0b10011011,
                                     0b10011100,
                                     0b10011101,
                                     0b10011110,
                                     0b10011111,
                                     0b10100000,
                                     0b10100001,
                                     0b10100010,
                                     0b10100011,
                                     0b10100100,
                                     0b10100101,
                                     0b10100110,
                                     0b10100111,
                                     0b10101000,
                                     0b10101001,
                                     0b10101010,
                                     0b10101011,
                                     0b10101100,
                                     0b10101101,
                                     0b10101110,
                                     0b10101111,
                                     0b10110000,
                                     0b10110001,
                                     0b10110010,
                                     0b10110011,
                                     0b10110100,
                                     0b10110101,
                                     0b10110110,
                                     0b10110111,
                                     0b10111000,
                                     0b10111001,
                                     0b10111010,
                                     0b10111011,
                                     0b10111100,
                                     0b10111101,
                                     0b10111110,
                                     0b10111111,
                                     0b11000000,
                                     0b11000001,
                                     0b11000010,
                                     0b11000011,
                                     0b11000100,
                                     0b11000101,
                                     0b11000110,
                                     0b11000111,
                                     0b11001000,
                                     0b11001001,
                                     0b11001010,
                                     0b11001100,
                                     0b11001101,
                                     0b11001110,
                                     0b11001111,
                                     0b11010000,
                                     0b11010001,
                                     0b11010010,
                                     0b11010011,
                                     0b11010100,
                                     0b11010101,
                                     0b11010110,
                                     0b11010111,
                                     0b11011000,
                                     0b11011001,
                                     0b11011010,
                                     0b11011011,
                                     0b11011100,
                                     0b11011101,
                                     0b11011110,
                                     0b11011111,
                                     0b11100000,
                                     0b11100001,
                                     0b11100010,
                                     0b11100011,
                                     0b11100100,
                                     0b11100101,
                                     0b11100110,
                                     0b11100111,
                                     0b11101000,
                                     0b11101001,
                                     0b11101010,
                                     0b11101011,
                                     0b11101100,
                                     0b11101101,
                                     0b11101110,
                                     0b11101111,
                                     0b11110000,
                                     0b11110001,
                                     0b11110010,
                                     0b11110011,
                                     0b11110100,
                                     0b11110101,
                                     0b11110110,
                                     0b11110111,
                                     0b11111000,
                                     0b11111001,
                                     0b11111010,
                                     0b11111011,
                                     0b11111100,
                                     0b11111101,
                                     0b11111110,
                                     0b11111111,
                                     0b00000000,
                                     0b00000001,
                                     0b00000010,
                                     0b00000011,
                                     0b00000100,
                                     0b00000101,
                                     0b00000110,
                                     0b00000111,
                                     0b00001000,
                                     0b00001001,
                                     0b00001010,
                                     0b00001100,
                                     0b00001101,
                                     0b00001110,
                                     0b00001111,
                                     0b00010000,
                                     0b00010001,
                                     0b00010010,
                                     0b00010011,
                                     0b00010100,
                                     0b00010101,
                                     0b00010110,
                                     0b00010111,
                                     0b00011000,
                                     0b00011001,
                                     0b00011010,
                                     0b00011011,
                                     0b00011100,
                                     0b00011101,
                                     0b00011110,
                                     0b00011111,
                                     0b00100000,
                                     0b00100001,
                                     0b00100010,
                                     0b00100011,
                                     0b00100100,
                                     0b00100101,
                                     0b00100110,
                                     0b00100111,
                                     0b00101000,
                                     0b00101001,
                                     0b00101010,
                                     0b00101011,
                                     0b00101100,
                                     0b00101101,
                                     0b00101110,
                                     0b00101111,
                                     0b00110000,
                                     0b00110001,
                                     0b00110010,
                                     0b00110011,
                                     0b00110100,
                                     0b00110101,
                                     0b00110110,
                                     0b00110111,
                                     0b00111000,
                                     0b00111001,
                                     0b00111010,
                                     0b00111011,
                                     0b00111100,
                                     0b00111101,
                                     0b00111110,
                                     0b00111111,
                                     0b01000000,
                                     0b01000001,
                                     0b01000010,
                                     0b01000011,
                                     0b01000100,
                                     0b01000101,
                                     0b01000110,
                                     0b01000111,
                                     0b01001000,
                                     0b01001001,
                                     0b01001010,
                                     0b01001100,
                                     0b01001101,
                                     0b01001110,
                                     0b01001111,
                                     0b01010000,
                                     0b01010001,
                                     0b01010010,
                                     0b01010011,
                                     0b01010100,
                                     0b01010101,
                                     0b01010110,
                                     0b01010111,
                                     0b01011000,
                                     0b01011001,
                                     0b01011010,
                                     0b01011011,
                                     0b01011100,
                                     0b01011101,
                                     0b01011110,
                                     0b01011111,
                                     0b01100000,
                                     0b01100001,
                                     0b01100010,
                                     0b01100011,
                                     0b01100100,
                                     0b01100101,
                                     0b01100110,
                                     0b01100111,
                                     0b01101000,
                                     0b01101001,
                                     0b01101010,
                                     0b01101011,
                                     0b01101100,
                                     0b01101101,
                                     0b01101110,
                                     0b01101111,
                                     0b01110000,
                                     0b01110001,
                                     0b01110010,
                                     0b01110011,
                                     0b01110100,
                                     0b01110101,
                                     0b01110110,
                                     0b01110111,
                                     0b01111000,
                                     0b01111001,
                                     0b01111010,
                                     0b01111011,
                                     0b01111100,
                                     0b01111101,
                                     0b01111110,
                                     0b01111111,
                                     0b10000000,
                                     0b10000001,
                                     0b10000010,
                                     0b10000011,
                                     0b10000100,
                                     0b10000101,
                                     0b10000110,
                                     0b10000111,
                                     0b10001000,
                                     0b10001001,
                                     0b10001010,
                                     0b10001100,
                                     0b10001101,
                                     0b10001110,
                                     0b10001111,
                                     0b10010000,
                                     0b10010001,
                                     0b10010010,
                                     0b10010011,
                                     0b10010100,
                                     0b10010101,
                                     0b10010110,
                                     0b10010111,
                                     0b10011000,
                                     0b10011001,
                                     0b10011010,
                                     0b10011011,
                                     0b10011100,
                                     0b10011101,
                                     0b10011110,
                                     0b10011111,
                                     0b10100000,
                                     0b10100001,
                                     0b10100010,
                                     0b10100011,
                                     0b10100100,
                                     0b10100101,
                                     0b10100110,
                                     0b10100111,
                                     0b10101000,
                                     0b10101001,
                                     0b10101010,
                                     0b10101011,
                                     0b10101100,
                                     0b10101101,
                                     0b10101110,
                                     0b10101111,
                                     0b10110000,
                                     0b10110001,
                                     0b10110010,
                                     0b10110011,
                                     0b10110100,
                                     0b10110101,
                                     0b10110110,
                                     0b10110111,
                                     0b10111000,
                                     0b10111001,
                                     0b10111010,
                                     0b10111011,
                                     0b10111100,
                                     0b10111101,
                                     0b10111110,
                                     0b10111111,
                                     0b11000000,
                                     0b11000001,
                                     0b11000010,
                                     0b11000011,
                                     0b11000100,
                                     0b11000101,
                                     0b11000110,
                                     0b11000111,
                                     0b11001000,
                                     0b11001001,
                                     0b11001010,
                                     0b11001100,
                                     0b11001101,
                                     0b11001110,
                                     0b11001111,
                                     0b11010000,
                                     0b11010001,
                                     0b11010010,
                                     0b11010011,
                                     0b11010100,
                                     0b11010101,
                                     0b11010110,
                                     0b11010111,
                                     0b11011000,
                                     0b11011001,
                                     0b11011010,
                                     0b11011011,
                                     0b11011100,
                                     0b11011101,
                                     0b11011110,
                                     0b11011111,
                                     0b11100000,
                                     0b11100001,
                                     0b11100010,
                                     0b11100011,
                                     0b11100100,
                                     0b11100101,
                                     0b11100110,
                                     0b11100111};



/***** GLOBAL VARIABLE DEFINITIONS - THESE SHOULD NOT BE CHANGED *****/

// EEPROM Object
I2C_eeprom eeprom(EEPROM_ADDRESS, EEPROM_SIZE);

// System settings
unsigned int maxCycleState = MAX_CYCLE_TIME  * (1000 / UPDATE_INTERVAL); // Max number of states during timer run. Equals length of timer run divided by step size
byte relayOutputList[ MAX_CYCLE_TIME  * (1000 / UPDATE_INTERVAL) ];      // List of timer outputs - one for each state
byte timerLocks = 0;                                                     // Lockout bits for timer outputs - 1 indicates a lockout

// System state variables
byte state = IDLE_STATE; 
unsigned int currCycleState = 0;

// Timing variables
unsigned long currTime = 0;
unsigned long prevStateTime = 0;



void setup() {
  // Initializes USB and relay outputs
  initRelayOutputs();  
  SerialUSB.begin(9600);

  // Remove this soon - for DEBUG
 // while(!SerialUSB);
  SerialUSB.println("Hello!");

}

void loop() {

  // update our tick timer
  currTime = millis();

  // If we're ready to run the state machine, let's do it!
  if(currTime - prevStateTime >= UPDATE_INTERVAL) {
    prevStateTime = currTime;
    
    stateMachine();
  }

  // If there's serial data available, read it - check if we should enter programming mode
  if(SerialUSB.available()) {
    char data = SerialUSB.read();

    if(data == USB_PROG_CHAR) {
      state = USB_PROGRAM_STATE;
    }
  }
  
}

/* \brief Executes state machine. Currently DOES NOT include UI support.
 *  
 */
void stateMachine() {
  switch(state) {
    
    /* IDLE_STATE: Do nothing but wait for the cycle timer to start. If CYCLE_STATE exits prematurely,
     *             the cycle position is not reset, and we wait for it to restart in IDLE_STATE.
     */
    case IDLE_STATE: {     
      // Read old position
     // currCycleState = resumeCycle();
       
      // Start the timer!
      state = CYCLE_STATE;
    }break;

    /* CYCLE_STATE: Change the relay outputs accordingly. If exited prematurely, holds its position.
     *              Upon cycle completion, enters RESET_STATE.
     */
    case CYCLE_STATE: {
      // Fetch the next timer state, and configure the outputs
      setRelayOutputs(relayOutputList[currCycleState]);

      // Remove this soon! - DEBUG
      SerialUSB.print("Now on ");
      SerialUSB.print(currCycleState);
      SerialUSB.print(" of ");
      SerialUSB.println(maxCycleState - 1);

      // Store that this state is done
     // eeprom.writeByte(PAUSE_RESUME_START + currCycleState, STATE_FINISHED_CHAR);

      // Advance to next state - if we're done, reset!
      currCycleState++;
      
      if(currCycleState >= maxCycleState) {
        state = RESET_STATE;
      }
    }break;



    /* RESET_STATE: When entered, shuts off the relays and resets all state variables, sending the
     *              system to IDLE_STATE.
     */
    case RESET_STATE: {
      // Reset all constants
      currCycleState = 0;
      state = IDLE_STATE;
     // clearPauseData();

      // Turns off all outputs - this *may* turn off the timer, so do it last
      clearRelayOutputs();
      
    }break;


    /*  USB_PROGRAM_STATE: When entered, allows the device to be programmed over USB. Cycle timer
     *                     Cycle timer operation is paused. Entered by sending 'P' over USB.
     */
    case USB_PROGRAM_STATE:{
    
      // Turn off all relays - no operation & we have power!
      clearRelayOutputs();

      while(state == USB_PROGRAM_STATE) {
        // Wait for information to arrive
        while(!SerialUSB.available());

        // Process information
        char data = SerialUSB.read();

        switch(data) {
          // G - Get EEPROM Data - fetches EEPROM constants
          case 'G': {
            // Get settings from EEPROM
            getEEPROMSettings();
            
            // Say we're done
            SerialUSB.println("OK");
          }break;

          // S - Set EEPROM Data command - stores RAM constants in EEPROM
          case 'S': {
            // Set settings in EEPROM
            setEEPROMSettings();

            // Say we're done 
            SerialUSB.println("OK");
          }
          
          // F - Factory Reset command - loads default values from Flash
          case 'F': {
            // Set cycle length to factory default (120 seconds, 0.25 second update)
            maxCycleState = MAX_CYCLE_TIME  * (1000 / UPDATE_INTERVAL);
            
            // Load relay parameters
            for(int i = 0; i < maxCycleState; i++) {
              relayOutputList[i] = relayOutputsPreset[i];
            }

            // Say we're done
            SerialUSB.println("OK");
          }break;
          
          // C - Cycle Length Set command - sets cycle length
          case 'C': {
            // Get cycle length
            unsigned int cycleLength = SerialUSB.parseInt();

            // If it's within the acceptable range, set the length
            if((cycleLength < (MAX_CYCLE_TIME  * (1000 / UPDATE_INTERVAL))) && (cycleLength > 0)) {
              maxCycleState = cycleLength;

              SerialUSB.print("OK+"); SerialUSB.println(maxCycleState);
            // Print cycle length if requested
            } else if(cycleLength == 0) {
              SerialUSB.print("OK+"); SerialUSB.println(maxCycleState);
            // If it's out of range, throw an error
            } else {
              SerialUSB.println("ERROR");
            }
          }break;
          
          // R - Timer Output Read command - reads output for one interval of timer 
          case 'R': {
            // Get timer state
            unsigned int timerState = SerialUSB.parseInt();

            // If it's within the acceptable range, return the output
            if((timerState < maxCycleState) && (timerState >= 0)) {
              // Return output
              SerialUSB.print("OK+"); SerialUSB.println(relayOutputList[timerState], BIN);
              
            // If it's out of the range, throw an error
            } else {
              SerialUSB.println("ERROR");
            }  
          }break;
          
          // T - Timer Output Set command - sets output for one interval of timer 
          case 'T': {
            // Get timer state
            unsigned int timerState = SerialUSB.parseInt();

            // Get new output
            byte timerOutput = SerialUSB.parseInt();

            // If the state is within the acceptable range, set the new output
            // Will not change locked values
            if((timerState < maxCycleState) && (timerState >= 0)) {
              // Get old output
              byte oldOutput = relayOutputList[timerState];
              relayOutputList[timerState] = 0;
              
              // Set new output
              for(byte i = 0; i < NUM_OUTPUTS; i++) {
                // If timer is locked, skip
                if(timerLocks & (1<<i)) {
                  relayOutputList[timerState] |= (oldOutput & (1<<i));
                  
                // else, set the new value
                } else {
                  relayOutputList[timerState] |= (timerOutput & (1<<i));
                }
              }
              
              // Return output
              SerialUSB.print("OK+"); SerialUSB.println(relayOutputList[timerState], BIN);
              
            // If it's out of the range, throw an error
            } else {
              SerialUSB.println("ERROR");
            }  
            
            
          }break;
                    
          // Q - Quit command - returns from programming mode, begins running
          case 'Q': {
            state = CYCLE_STATE;

            SerialUSB.println("OK");
          }break;

          // L - Lockout command - locks a timer output, preventing changes
          case 'L': {
            // Get timer output number
            byte outputNumber = SerialUSB.parseInt();

            // If it's within the acceptable range, set the lock
            if((outputNumber < NUM_OUTPUTS) && (outputNumber >= 0)) {
              // Set lock
              timerLocks |= (1<<outputNumber);

              // Print lockout array
              SerialUSB.print("OK+"); SerialUSB.println(timerLocks, BIN);
              
            // If it's out of the range, throw an error
            } else {
              SerialUSB.println("ERROR");
            }            
          }break;

          // U - Unlock command - unlocks a timer output, allowing changes
          case 'U': {
            // Get timer output number
            byte outputNumber = SerialUSB.parseInt();

            // If it's within the acceptable range, clear the lock
            if((outputNumber < NUM_OUTPUTS) && (outputNumber >= 0)) {
              // Set lock
              timerLocks &= ~(1<<outputNumber);

              // Print lockout array
              SerialUSB.print("OK+"); SerialUSB.println(timerLocks, BIN);
              
            // If it's out of the range, throw an error
            } else {
              SerialUSB.println("ERROR");
            }            
          }break;

          // H - Help message - print the list of commands
          case '?':
          case 'h':
          case 'H': {
            // Send informational message - times in 1/4 second increments
            SerialUSB.println("USB Programing Mode - Firmware version v0.4");
            SerialUSB.println("The following messages are available:");
            SerialUSB.println("\t-T - Timer Output Set       - < cycle timer state > < timer output values >  - e.g. T 20 255");
            SerialUSB.println("\t-R - Timer Output Read      - < cycle timer state >                          - e.g. R 45");   
            SerialUSB.println("\t-C - Cycle Length Set       - < cycle length >                               - e.g. C 100");
            SerialUSB.println("\t-Q - Quit Programming Mode  - < no arguments >                               - e.g. Q");
            SerialUSB.println("\t-G - Get EEPROM Data        - < no arguments >                               - e.g. G");
            SerialUSB.println("\t-S - Set EEPROM Data        - < no arguments >                               - e.g. S");
            SerialUSB.println("\t-F - Factory Reset          - < no arguments >                               - e.g. F");
            SerialUSB.println("\t-L - Timer Output Lock      - < timer output # >                             - e.g. L 0");
            SerialUSB.println("\t-U - Timer Output Unlock    - < timer output # >                             - e.g. U 3");
            SerialUSB.println("\t-H - Help Message           - < no arguments >                               - e.g. H");
          }break;
        }
      }

      

      
    }break;
      
    
  }
}

/* \brief Sets relay pins to their proper state based on the binary values in a 32-bit integer
 *  
 */
void setRelayOutputs(byte pinUpdateList) {
  // Sets pins that should be on
  REG_PORT_OUTSET0 = (pinUpdateList<<2);

  // Clears pins that should be off
  pinUpdateList = ~pinUpdateList;
  REG_PORT_OUTCLR0 = (pinUpdateList<<2);

}

/* \brief Clears all pins listed in RELAY_PINS (sets them to output logical 0)
 *  
 */
void clearRelayOutputs(void) {
  setRelayOutputs(0);
}

/* \brief Sets all pins listed in RELAY_PINS to digital outputs. Sets values for relay output array.
 *  
 */
void initRelayOutputs(void) {
  // Declared relay pins are set as outputs
  REG_PORT_DIRSET0 = RELAY_PINS;

  // Relay state list is pulled from memory
  for(int i = 0; i < maxCycleState; i++) {
    relayOutputList[i] = relayOutputsPreset[i];
  }
  //getEEPROMSettings();
}

/* \brief Gets settings from EEPROM
 *  
 */
void getEEPROMSettings(void) {
  // fetch Cycle Length - start with low byte, then add high byte
  maxCycleState = eeprom.readByte(CYCLE_LENGTH_START + 1);
  maxCycleState |= (eeprom.readByte(CYCLE_LENGTH_START) << 8);

  // fetch timing parameters
  for(int i = 0; i < maxCycleState; i++) {
    relayOutputList[i] = eeprom.readByte(RELAY_TABLE_START + i);
  }
}

/* \brief Sets settings in EEPROM
 *  
 */
void setEEPROMSettings(void) {
  // Set Cycle Length - start with low byte, then store high byte
  eeprom.writeByte(CYCLE_LENGTH_START + 1, (uint8_t)maxCycleState);
  eeprom.writeByte(CYCLE_LENGTH_START, (uint8_t)(maxCycleState >> 8));

  // Set Timing Parameters
  for(int i = 0; i < maxCycleState; i++) {
    eeprom.writeByte(RELAY_TABLE_START + i, relayOutputList[i]);
  }
}

/* \brief Clears EEPROM pause/resume region
 *  
 */
void clearPauseData(void) {
  // Set all regions of memory to 0x00
  for(int i = 0; i < RELAY_TABLE_START; i++) {
    eeprom.writeByte(PAUSE_RESUME_START + i, 0x00);
  }
}

/* \brief Resume from shutdown
 *  
 */
int resumeCycle(void) {
  uint8_t searchFlag = 1;
  uint16_t lastCycleState = 0;
  uint8_t charBuffer = 0x00;
  
  // Search EEPROM for first non-printed character
  while(searchFlag) {
    // Read
    charBuffer = eeprom.readByte(PAUSE_RESUME_START + lastCycleState);

    // Compare - if there's a match, continue
    if(charBuffer == STATE_FINISHED_CHAR) {
      lastCycleState++;
      
    // Otherwise, exit
    } else {
      searchFlag = 0;
    }
  }

  return lastCycleState;
}

