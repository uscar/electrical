#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>

/*
  USCAR
  Control Panel
  Description: Transmit a state (nomal operation, kill power, or initiate the landing sequence) to the quad and read back the battery's voltage.
*/

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

/* Configuration */

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10 
RF24 radio(9, 10);

// Set up the button pins
int kButton = 4;
int lButton = 5;
int nButton = 6;

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

// Data to send/receive
// state: 'n' = normal, 'l' = landing sequence, 'k' = kill
char curr_state;
// voltage: read using analogRead, 0-5V scaled to 0-1023
int curr_voltage;
const int voltage_in_pin = A0;
// The max voltage of the battery, to scale curr_voltage
const float battery_capacity = 7.4;

/* Poll buttons (in order of priority) to check for state change */
void pollButtons() {
    if (digitalRead(kButton) == LOW) {
        curr_state = 'k';
    }
    else if (digitalRead(lButton) == LOW) {
        curr_state = 'l';
    }
    else if (digitalRead(nButton) == LOW) {
        curr_state = 'n'; 
    }
}

void setup() {  
    // Print the role for debugging
    Serial.begin(57600);
    printf_begin();
    
    // Set up the radio
    radio.begin();
    radio.setRetries(15, 15);
    radio.openWritingPipe(pipes[0]);
    radio.openReadingPipe(1, pipes[1]);

    // Initialize state/voltage. Start in land, since it's on the ground.
    curr_state = 'l';
    curr_voltage = 0;
        
    // Initialize button pins
    pinMode(kButton, INPUT_PULLUP);
    pinMode(lButton, INPUT_PULLUP);
    pinMode(nButton, INPUT_PULLUP);
     
    radio.startListening();
}

void loop() {
    // Now send the state and voltage to the receiver:
    // Stop listening
    radio.stopListening();

    // Poll buttons to update the state, then send it
    pollButtons();
    bool ok = radio.write(&curr_state, sizeof(char));

    if (ok) {
        printf("Sending State: %c, ", curr_state);
    }
    else {
        printf("Error: Could not send state.\n");
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
        printf("Receiving Voltage: ");
        // Scale the voltage by the battery capacity
        Serial.print(curr_voltage*battery_capacity/1023);
        printf("v\n", curr_voltage);
    }
    else {
        printf("Error: RF24 Timeout.\n");
    }

    // Wait 1s
    delay(1000);
}
