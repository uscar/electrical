/*
  USCAR
  Control Panel
  Description: Transmit a state (nomal operation, kill power, or initiate the landing sequence) to the quad and read back the battery's voltage.
  For now, both Arduinos are programmed with the same sketch to make debugging easier. They'll be separated later.
*/

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

/* Configuration */

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10 
RF24 radio(9, 10);

// Set up output pins:
int landingSeqOut = 4;
int killOut = 5;

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

// Data to send/receive
// state: 'n' = normal, 'l' = landing sequence, 'k' = kill
char curr_state;
// voltage: read using analogRead, 0-5V scaled to 0-1023
int curr_voltage;
const int voltage_in_pin = A0;

/* Handle a button pushe interrupt to change the current state */
// Debounce via checking when the last interrupt service was
unsigned long last_call_time = 0;

/* Return the current voltage from the quad */
int getVoltage() {
    return analogRead(voltage_in_pin); 
}
  
void setup(){
    // Print the role for debugging(remember to set buad rate to 57600);
    Serial.begin(57600);
    printf_begin();
    printf("KillSwitchReceiver");
    
    // Set up the radio
    radio.begin();
    radio.setRetries(15,15);
    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1, pipes[0]);

    // Initialize the output
     pinMode(landingSeqOut, OUTPUT);
     pinMode(killOut, OUTPUT);
     
    // Initialize state/voltage
    curr_state = 'l';
    curr_voltage = getVoltage();
    radio.startListening();

    // Print radio config to debug
    radio.printDetails();
}

void loop()
{
        // Check if data is ready
        if (radio.available()) {
            bool done = false;
            while (!done) {
                done = radio.read(&curr_state, sizeof(char));
            }

            printf("Receiving State: %c, ", curr_state);
            delay(20);

            // Send the voltage back
            radio.stopListening();
            curr_voltage = getVoltage();
            radio.write(&curr_voltage, sizeof(int));
            printf("Sending Response: %i.\n", curr_voltage);

            //Translate the received state into the correct outputs:
            digitalWrite(landingSeqOut, LOW);
            digitalWrite(killOut, LOW);
            if(curr_state == 'l'){
              printf("here landing");
              digitalWrite(landingSeqOut, HIGH);
              delay(1000);
            }
            else if(curr_state == 'k'){
              printf("here kill");
              digitalWrite(killOut, HIGH);
              delay(1000);
            }
            radio.startListening();
        }
}

