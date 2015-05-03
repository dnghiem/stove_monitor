/*************************************************** 
  This is the stove monitor program. It initializes the CC3000 wifi module, connects the wifi
  and starts up the temperature monitor.
 ****************************************************/
 
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>

#include <TembooAccount.h>
#include <Temboo.h>

#include "utility/debug.h"

/* Include the dht11 library to make use of the temperature sensor. */

#include <dht11.h>

/*-----( Declare objects )-----*/
dht11 DHT11;

/*-----( Declare Constants, Pin Numbers )-----*/
#define DHT11PIN 2

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11

Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed
Adafruit_CC3000_Client client;
                                         
                                         

#define WLAN_SSID       "Disrupt"           // cannot be longer than 32 characters!

#define WLAN_PASS       ""

// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define IDLE_TIMEOUT_MS  3000      // Amount of time to wait (in milliseconds) with no data 
                                   // received before closing the connection.  If you know the server
                                   // you're accessing is quick to respond, you can reduce this value.

// What page to grab!
#define WEBSITE      "www.temboo.com"
#define WEBPAGE      "index.html"


/**************************************************************************/
/*!
    @brief  Sets up the HW and the CC3000 module (called automatically
            on startup)
*/
/**************************************************************************/

uint32_t ip;


// TEMBO specific setup
// We limit this so you won't use all of your Temboo calls while testing
int maxCalls = 10;

// The number of times this Choreo has been run so far in this sketch
int calls = 0;
int outputPin = 6;
int choreoInterval = 30000; // Choreo execution interval in milliseconds
uint32_t lastChoreoRunTime = 0; // store the time of the last Choreo execution

// led pin
const int led_pin = 13;

#define SMS_NOT_SENT    0
#define SMS_SENT        1

int sms_state = SMS_NOT_SENT;
unsigned long sms_state_timer;

// 1m timeout
#define SMS_SENT_TIMEOUT  60000

void setup(void)
{
  Serial.begin(115200);
  Serial.println(F("Hello, CC3000!\n")); 

  // configure pins
  // initialize the digital pin as an output.
  pinMode( led_pin, OUTPUT );

  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  
  // Initialise the module 
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  
  // Optional SSID scan
  // listSSIDResults();
  
  Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));
  
  /* Wait for DHCP to complete */

  Serial.println(F("Request DHCP"));

  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  

  /* Display the IP address DNS, Gateway, etc. */  

  while (! displayConnectionDetails()) {
    delay(1000);
  }

  ip = 0;

  // Try looking up the website's IP address


  Serial.print(WEBSITE); Serial.print(F(" -> "));
  while (ip == 0) {
    if (! cc3000.getHostByName(WEBSITE, &ip)) {
      Serial.println(F("Couldn't resolve!"));
    }
    delay(500);
  }



  cc3000.printIPdotsRev(ip);
  
  // Optional: Do a ping test on the website
  //
  //Serial.print(F("\n\rPinging ")); cc3000.printIPdotsRev(ip); Serial.print("...");  
  //replies = cc3000.ping(ip, 5);
  //Serial.print(replies); Serial.println(F(" replies"));
    

  /* Try connecting to the website.
     Note: HTTP/1.1 protocol is used to keep the server from closing the connection before all data is read.
  */
   
  
  Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 80);
    if (www.connected()) {

	// send a text message using Twilio via Temboo 
        
//        Serial.println("Attempting to send text to twilio");
//	runSendSMS();


    www.fastrprint(F("GET "));
    www.fastrprint(WEBPAGE);
    www.fastrprint(F(" HTTP/1.1\r\n"));
    www.fastrprint(F("Host: ")); www.fastrprint(WEBSITE); www.fastrprint(F("\r\n"));
    www.fastrprint(F("\r\n"));
    www.println();
    }

//  } else {
//    Serial.println(F("Connection failed"));    
//    return;
//  }


  Serial.println(F("-------------------------------------"));



  /* Read data until either the connection is closed, or the idle timeout is reached. */ 

/*
  unsigned long lastRead = millis();
  while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (www.available()) {
      char c = www.read();
      Serial.print(c);
      lastRead = millis();
    }
  }
  www.close();
  Serial.println(F("-------------------------------------"));
  */
  
  /* You need to make sure to clean up after yourself or the CC3000 can freak out */
  /* the next time your try to connect ... */
  
  /*
  Serial.println(F("\n\nDisconnecting"));
  cc3000.disconnect();
  */
 
  /* Indicate that the DH11 sensor is being activated, and the versions of the library being used */
  Serial.println("DHT11 TEST PROGRAM ");
  Serial.print("LIBRARY VERSION: ");
  Serial.println(DHT11LIB_VERSION);
  Serial.println();
 
}

void loop(void) {
  
  Serial.println("\n");

  int chk = DHT11.read(DHT11PIN);
  
  Serial.print("Read sensor: ");
  switch (chk)
  {
    case 0: Serial.println("OK"); break;
    case -1: Serial.println("Checksum error"); break;
    case -2: Serial.println("Time out error"); break;
    default: Serial.println("Unknown error"); break;
  }
 
  Serial.print("Temperature (oC): ");
  Serial.println((float)DHT11.temperature, 2);

  Serial.print("Temperature (oF): ");
  Serial.println(Fahrenheit(DHT11.temperature), 2);

  Serial.print("Temperature (K): ");
  Serial.println(Kelvin(DHT11.temperature), 2); 

   if (DHT11.temperature >= 45) {
      Serial.println("Now here is where we'd push out an sms to the phone that the stove is on.");
    
      if( sms_state == SMS_NOT_SENT ) {
          runSendSMS();
          sms_state = SMS_SENT;
          sms_state_timer = millis();
      }
   }
 
  delay(2000);

  if( sms_state == SMS_SENT )
      if( millis() - sms_state_timer > SMS_SENT_TIMEOUT )
          sms_state = SMS_NOT_SENT;
  
}

//void loop(void)
//{
//  if (calls < maxCalls) {
//    Serial.println("Calling SendSMS Choreo...");
//    runSendSMS();
//    calls++;
//  } else {
//    Serial.println("Skipping to save Temboo calls. Adjust maxCalls as required.");
//  }
//  delay(30000);
//}

void runSendSMS() {
  TembooChoreo SendSMSChoreo(client);

  // Set Temboo account credentials
  SendSMSChoreo.setAccountName(TEMBOO_ACCOUNT);
  SendSMSChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
  SendSMSChoreo.setAppKey(TEMBOO_APP_KEY);

  // Set Choreo inputs
  String AuthTokenValue = "ffc7c1a48023818440037303e7c2d038";
  SendSMSChoreo.addInput("AuthToken", AuthTokenValue);
  String BodyValue = "The stove is on!!!";
  SendSMSChoreo.addInput("Body", BodyValue);
  String ToValue = "+19179755336";
  SendSMSChoreo.addInput("To", ToValue);
  String AccountSIDValue = "AC230fe39213081cd4a8472de2da826186";
  SendSMSChoreo.addInput("AccountSID", AccountSIDValue);
  String FromValue = "+13474921928";
  SendSMSChoreo.addInput("From", FromValue);
  // Identify the Choreo to run
  SendSMSChoreo.setChoreo("/Library/Twilio/SMSMessages/SendSMS");

  Serial.println(F("We should've gotten past the inits"));
  // Run the Choreo
  unsigned int returnCode = SendSMSChoreo.run();

  // A return code of zero means everything worked
  Serial.println(F("Return code"));
  Serial.println(returnCode);
  if (returnCode == 0) {
    while (SendSMSChoreo.available()) {
      String name = SendSMSChoreo.readStringUntil('\x1F');
      name.trim();

      if (name == "Response") {
        if (SendSMSChoreo.findUntil("", "\x1E")) {
          digitalWrite(outputPin, HIGH);
          SendSMSChoreo.find("\x1E");
        }
      }
      else {
        SendSMSChoreo.find("\x1E");
      }
    }
  }

  SendSMSChoreo.close();
}


/**************************************************************************/
/*!
    @brief  Begins an SSID scan and prints out all the visible networks
*/
/**************************************************************************/

void listSSIDResults(void)
{
  uint32_t index;
  uint8_t valid, rssi, sec;
  char ssidname[33]; 

  if (!cc3000.startSSIDscan(&index)) {
    Serial.println(F("SSID scan failed!"));
    return;
  }

  Serial.print(F("Networks found: ")); Serial.println(index);
  Serial.println(F("================================================"));

  while (index) {
    index--;

    valid = cc3000.getNextSSID(&rssi, &sec, ssidname);
    
    Serial.print(F("SSID Name    : ")); Serial.print(ssidname);
    Serial.println();
    Serial.print(F("RSSI         : "));
    Serial.println(rssi);
    Serial.print(F("Security Mode: "));
    Serial.println(sec);
    Serial.println();
  }
  Serial.println(F("================================================"));

  cc3000.stopSSIDscan();
}

/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}

/*-----( Declare User-written Functions )-----*/
//
//Celsius to Fahrenheit conversion
double Fahrenheit(double celsius)
{
	return 1.8 * celsius + 32;
}

//Celsius to Kelvin conversion
double Kelvin(double celsius)
{
	return celsius + 273.15;
}
