//code for the ESP8266
#include <SoftwareSerial.h>
#include <TinyGPS++.h> //http://arduiniana.org/libraries/tinygpsplus

//sender phone number with country code
//Note: Not enter the SIM800L Phone Number here
const String PHONE = "";//your phone number with country code

//GSM Module RX pin to NodeMCU D6
//GSM Module TX pin to NodeMCU D5
#define rxGSM 14
#define txGSM 12
SoftwareSerial sim800(rxGSM, txGSM);

//GPS Module RX pin to NodeMCU D3
//GPS Module TX pin to NodeMCU D2
#define rxGPS 4
#define txGPS 0
SoftwareSerial neogps(rxGPS, txGPS);
TinyGPSPlus gps;

// Define button pins
#define BUTTON1_PIN 5 // GPIO 5 for sending SMS
#define BUTTON2_PIN 16 // GPIO 16 for making a call

String smsStatus, senderNumber, receivedDate, msg;

void setup() {
    Serial.begin(115200);
    Serial.println("Arduino serial initialize");

    sim800.begin(9600);
    Serial.println("SIM800L serial initialize");

    neogps.begin(9600);
    Serial.println("NodeMCU serial initialize");

    sim800.listen();
    neogps.listen();

    smsStatus = "";
    senderNumber = "";
    receivedDate = "";
    msg = "";

    sim800.print("AT+CMGF=1\r"); // SMS text mode
    delay(1000);

    // Initialize button pins
    pinMode(BUTTON1_PIN, INPUT_PULLUP); // Button 1 for sending SMS
    pinMode(BUTTON2_PIN, INPUT_PULLUP); // Button 2 for making a call
}

void loop() {
    // Check button states
    if (digitalRead(BUTTON1_PIN) == HIGH) { // Button 1 pressed
        sendLocation(); // Send Google Maps link
        delay(2000); // Debounce delay
    }

    if (digitalRead(BUTTON2_PIN) == HIGH) { // Button 2 pressed
        makeCall(); // Make a call
        delay(2000); // Debounce delay
    }

    //--------------------------------------
    while (sim800.available()) {
        parseData(sim800.readString());
    }
    //--------------------------------------
    while (Serial.available()) {
        sim800.println(Serial.readString());
    }
    //--------------------------------------
} // main loop ends

// Function to make a call
void makeCall() {
    sim800.print("ATD" + PHONE + ";\r"); // Dial the number
    delay(5000);
    Serial.println("Calling " + PHONE);
}

// ... (rest of your existing code remains unchanged)

// Modify the parseData function to handle the "CALL" SMS
void parseData(String buff) {
    Serial.println(buff);

    unsigned int len, index;
    //-----------------------------------------------------
    // Remove sent "AT Command" from the response string.
    index = buff.indexOf("\r");
    buff.remove(0, index + 2);
    buff.trim();
    //-----------------------------------------------------

    //-----------------------------------------------------
    if (buff != "OK") {
        index = buff.indexOf(":");
        String cmd = buff.substring(0, index);
        cmd.trim();

        buff.remove(0, index + 2);

        if (cmd == "+CMTI") {
            // get newly arrived memory location and store it in temp
            index = buff.indexOf(",");
            String temp = buff.substring(index + 1, buff.length());
            temp = "AT+CMGR=" + temp + "\r";
            // get the message stored at memory location "temp"
            sim800.println(temp);
        } else if (cmd == "+CMGR") {
            extractSms(buff);

            if (senderNumber == PHONE) {
                if (msg == "get location") {
                    sendLocation();
                } else if (msg == "call") { // Check for " CALL" message
                    makeCall(); // Make a call if "CALL" SMS is received
                }
            } else {
                Serial.println(String(senderNumber) + ": Phone not registered");
            }
        }
    //-----------------------------------------------------
    } else {
        // The result of AT Command is "OK"
    }
}
//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM

// 
//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
void extractSms(String buff) {
    unsigned int index;

    index = buff.indexOf(",");
    smsStatus = buff.substring(1, index - 1);
    buff.remove(0, index + 2);

    senderNumber = buff.substring(0, 13);
    buff.remove(0, 19);

    receivedDate = buff.substring(0, 20);
    buff.remove(0, buff.indexOf("\r"));
    buff.trim();

    index = buff.indexOf("\n\r");
    buff = buff.substring(0, index);
    buff.trim();
    msg = buff;
    buff = "";
    msg.toLowerCase();

    //Serial.println("----------------------------------");
    //Serial.println(smsStatus);
    //Serial.println(senderNumber);
    //Serial.println(receivedDate);
    //Serial.println(msg);
    //Serial.println("----------------------------------");
}
//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM

//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
void sendLocation()
 {
    //-----------------------------------------------------------------
    // Can take up to 60 seconds
    boolean newData = false;

    for (unsigned long start = millis(); millis() - start < 2000;) {
        while (neogps.available()) {
            if (gps.encode(neogps.read())) {
                newData = true;
            }
        }
    }
    //-----------------------------------------------------------------

    //-----------------------------------------------------------------
    // If newData is true
    if (newData)
     {
        Serial.print("Latitude= ");
        Serial.print(gps.location.lat(), 6);
        Serial.print(" Longitude= ");
        Serial.println(gps.location.lng(), 6);
        newData = false;
        delay(300);
        sim800.print("AT+CMGF=1\r");
        delay(1000);
        sim800.print("AT+CMGS=\"" + PHONE + "\"\r");
        delay(1000);
        sim800.print("Live Location");
        sim800.print("http://maps.google.com/maps?q=loc:");
        sim800.print(gps.location.lat(), 6);
        sim800.print(",");
        sim800.print(gps.location.lng(), 6);
        delay(100);
        sim800.write(0x1A); // ascii code for ctrl-26
        delay(1000);
        Serial.println("GPS Location SMS Sent Successfully.");
    } 
    else
     {
        Serial.println("Invalid GPS data");
    }
    //-----------------------------------------------------------------
}
