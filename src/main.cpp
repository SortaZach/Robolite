#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h> // for itoa() conversion

#define JOYSTICK_X PC2
#define JOYSTICK_Y PC1
#define JOYSTICK_PRESSED PD5
#define BUTTON_1 PD6

void setupADC();
uint16_t readADC(uint8_t channel);
void setupUART();
void uartTransmit(char data);
void uartPrint(const char *str);
void parseToJSON(uint16_t joyX1, uint16_t joyY1, uint16_t joySW1, uint16_t b1);

int main(){

    setupADC();
    setupUART();

    while(1) {
        uint16_t xValue = readADC(JOYSTICK_X); 
        uint16_t yValue = readADC(JOYSTICK_Y);
        uint16_t swValue = 0; // false or not pressed
        uint16_t b1Value = 0;

        // turn on the Input for pressed use & for pointer otherwise we overwrite the entire DDRD register instead of just where the pin is
        DDRD &= ~(1 << JOYSTICK_PRESSED);
        PORTD |= (1 << JOYSTICK_PRESSED);
        
        DDRD &= ~(1 << BUTTON_1);
        PORTD |= (1 << BUTTON_1);
        // Check if Joystick is pressed (LOW = Pressed)
        // PIND  is a hardware register for AVR that stores the current state of all digital input pins on PORT D
        // the & symbol is a bitwise and, meaning we can point to specific registers
        if(!(PIND & (1 << JOYSTICK_PRESSED))){
            swValue = 1;
        }

        if(!(PIND & (1 << BUTTON_1))){
            b1Value = 1;
        }
        
        parseToJSON(xValue, yValue, swValue, b1Value);
        _delay_ms(500); // Delay for readablity
    }
}


void parseToJSON(uint16_t joyX1, uint16_t joyY1, uint16_t joySW1, uint16_t b1){
    char buffer[10];
    //Indenting to indicate JSON Formatting
    uartPrint("{\"input\":{");
        uartPrint("\"js1\":{");
            uartPrint("\"X\":"); itoa(joyX1, buffer, 10); uartPrint(buffer); uartPrint(",");
            uartPrint("\"Y\":"); itoa(joyY1, buffer, 10); uartPrint(buffer); uartPrint(",");
            uartPrint("\"SW\":"); itoa(joySW1, buffer, 10); uartPrint(buffer); 
        uartPrint("},");
        uartPrint("\"buttons\":{");
            uartPrint("\"b1\":"); itoa(b1 ,buffer, 10); uartPrint(buffer);
        uartPrint("}");
    uartPrint("}}");

    uartPrint("\n");
}

// Analog to Digital Conversion (ADC)
void setupADC(){
    // ADMUX (ADC Multiplexer Selection Register: configuration register)
        // selects reference voltage
        // input channel
        // how ADC results are stored
    ADMUX |= (1 << REFS0); // Use AVCC (5V) as reference voltage (Ensures ADC reads between 0V and 5V)(REFS0/REFS1 controllers reference Voltage)
                           // setting REFS0 makes 0v = 0 and 5V = 1023 in ADC readings

    // ADCSRA (ADC Control and Status Register A)
        // ADC enable/disable
        // ADC start conversion
        // ADC clock prescaler
        // ADC interrupt settings
    ADCSRA |= (1 << ADEN); // Enable ADC (ADEN: bit 7 of ADCSRA, enables ADC if 1)
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // set prescaler to 128 (16 Mhz / 128 = 125khz ADC clock)
}

uint16_t readADC(uint8_t channel){
    ADMUX = (ADMUX & 0xF8) | channel; // Select ADC channel (A0 or A1 in this setup)
    ADCSRA |= (1 << ADSC); // Start conversion
    while (ADCSRA & (1 << ADSC)); // Wait for conversion to complete
    return ADC; // Return 10-bit ADC value (0-1023)
}

// Universal Asyncronous Reciver-Transmitter communication (currently can use the serial monitor in the Arduino IDE)
void setupUART() {
    UBRR0H = 0;   // UBRR0H is high byte of UART bit regist (16-bit so two registers)
    UBRR0L = 103; // 9600 baud rate for 16MHz clock (low byte, second half of 16-bit register)
    UCSR0B |= (1 << TXEN0); // Enable UART transmitter (UCSR0B is 3 bits, TXEN0 is the bit that enables it)
                            // This activates the TX pin allowing data transmission

    UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00); // 8-bit data format(UCSR0C configures UART Data format. setting it as 8 bit is default or standard)
}

void uartTransmit(char data) {
    while (!(UCSR0A & (1 << UDRE0))); // Wait for buffer to be empty (UDREO is USART Data Register Empty flag)
    UDR0 = data; // send data (this is the Data Register)
} 

void uartPrint(const char *str) {
    while(*str){
        uartTransmit(*str++);
    }
}