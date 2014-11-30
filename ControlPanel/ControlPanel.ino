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
RF24 radio(9,10);

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

// Data to send/receive
// state: 'n' = normal, 'l' = landing sequence, 'k' = kill
char curr_state;
// voltage: read using analogRead, 0-5V scaled to 0-1023
int curr_voltage;
const int voltage_in_pin = A0;

// This Arduino's role: control panel (role_cp == 1) or quad (role_cp == 0).
bool role_cp = 0;
// For testing, set which role this Arduino should be by grounding pin 7 (grounded means this is the control panel) 
const int role_cp_pin = 7;

/* Return the current state from the control panel */
char getState() {
    // TODO: switch to interrupts and debounce the buttons. For now, always be in the normal state.. Kill or landing should be the default.
    return 'n';
}

/* Return the current voltage from the quad */
int getVoltage() {
    return analogRead(voltage_in_pin); 
}

void setup() {
    // Set the role
    pinMode(role_cp_pin, INPUT);
    digitalWrite(role_cp_pin, HIGH);
    delay(20);    

    // Print the role for debugging
    Serial.begin(57600);
    printf_begin();
    printf("\nUSCAR Quad Control Panel\n");
    printf("Role: %s\n", role_cp ? "Control Panel" : "Quad");
    
    // Set up the radio
    radio.begin();
    radio.setRetries(15,15);
    if (digitalRead(role_cp_pin)) { // This is the quad
        radio.openWritingPipe(pipes[1]);
        radio.openReadingPipe(1, pipes[0]);

        // Initialize state/voltage
        curr_state = 'l';
        curr_voltage = getVoltage();
    }
    else { // This is the control panel
        role_cp = 1;
        radio.openWritingPipe(pipes[0]);
        radio.openReadingPipe(1, pipes[1]);

        // Initialize state/voltage
        curr_state = getState();
        curr_voltage = 0;
    }
    radio.startListening();

    // Print radio config to debug
    radio.printDetails();
}

void loop(void)
{
    // Now send/receive the state/voltage
    if (role_cp) { // Control Panel
        // Stop listening
        radio.stopListening();

        //TODO: Finish this. Update the state.
        curr_state = getState();
        printf("Sending state %c ...", curr_state);
        bool ok = radio.write(&curr_state, sizeof(char));

        if (ok) {
          printf("ok...\n");
        }
        else {
          printf("failed.\n");
        }

        // Resume listening
        radio.startListening();

        // Wait for a response, or until 200 ms have passed
        unsigned long start_time = millis();
        bool timeout = false;
        while (!radio.available() && !timeout) {
            if (millis() - start_time > 200 ) {
                timeout = true;
            }
        }

        // Handle received data
        if (!timeout) {
            // Success, so get the voltage
            radio.read(&curr_voltage, sizeof(int));
            printf("Got response. Voltage: %i\n", curr_voltage);
        }
        else {
            printf("Failed, response timed out.\n");
        }

        // Wait 1s
        delay(1000);
    }
    else { // Quad
        // Check if data is ready
        if (radio.available()) {
            bool done = false;
            while (!done) {
                done = radio.read(&curr_state, sizeof(char));
            }

            printf("Got payload, state is %c...", curr_state);
            delay(20);

            // Send the voltage back
            radio.stopListening();
            curr_voltage = getVoltage();
            radio.write(&curr_voltage, sizeof(int));
            printf("Sent response %i.\n", curr_voltage);

            radio.startListening();
        }
    }
}

