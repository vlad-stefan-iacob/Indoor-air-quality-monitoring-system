#include <DHT.h>
#include <Adafruit_GFX.h>  
#include <Adafruit_ST7735.h>  
#include <SPI.h>
#include "ThingSpeak.h"
#include <ESP8266WiFi.h>
#include <Wire.h>
#include "Adafruit_SGP30.h"
#include <ESP_Mail_Client.h>
#include <Arduino.h>
#include <string>

#define DHTPIN 12     
#define DHTTYPE DHT22  

const char *ssid = "######"; // Your wifi ssid
const char *password = "######"; //Your wifi password

unsigned long myChannelNumber = ######;
const char * myWriteAPIKey = "###########";

const char* server = "api.thingspeak.com";

WiFiClient client;

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

/* The sign in credentials */
#define AUTHOR_EMAIL "smp655433@gmail.com"
#define AUTHOR_PASSWORD "fszzradbzwkbsvdb"

/* Recipient's email*/
#define RECIPIENT_EMAIL "vladiacob14@gmail.com"

/* The SMTP Session object used for Email sending */
SMTPSession smtp;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SGP30 sgp;

// Declare pins for the display:
#define TFT_CS     D8
#define TFT_RST    D4  
#define TFT_DC     D3

// Create display:
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// current temperature & humidity, updated in loop()
float temp = 0.0;
float hum = 0.0;
float hi = 0.0; //heat index
int TVOC = 0;
int CO2 = 0;

int asapEmail_hi = 1;
int asapEmail_tvoc = 1;

String string_hi;
String string_tvoc;

void updateThingSpeak(float temp, float hum, float hi, int TVOC, int CO2){
    ThingSpeak.setField(1,temp);
    ThingSpeak.setField(2,hum);
    ThingSpeak.setField(3,hi);
    ThingSpeak.setField(4,TVOC);
    ThingSpeak.setField(5,CO2);
    ThingSpeak.writeFields(myChannelNumber,myWriteAPIKey);
    delay(20000);
}

void setup()
{   
    Serial.begin(115200);
    Wire.begin();
    delay(10);
	  dht.begin();
    
    Serial.println("SGP30 test");

    sgp.begin(&Wire);
    Serial.print("Found SGP30 serial #");
    Serial.print(sgp.serialnumber[0], HEX);
    Serial.print(sgp.serialnumber[1], HEX);
    Serial.println(sgp.serialnumber[2], HEX);

    ThingSpeak.begin(client);
    WiFi.begin(ssid, password);
    Serial.println("Waiting for WiFi connection ");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("################################ ");
    Serial.println();

     // Display setup:
    tft.initR(INITR_BLACKTAB);  // Initialize a ST7735S chip, black tab

    tft.fillScreen(ST7735_BLACK);  // Fill screen with black

    tft.setRotation(1);   
    tft.setTextColor(ST7735_WHITE);  
    tft.setTextSize(2);  

    smtp.debug(1);
      /* Set the callback function to get the sending results */
    smtp.callback(smtpCallback);

    /* Declare the session config data */
    ESP_Mail_Session session;

    /* Set the session config */
    session.server.host_name = SMTP_HOST;
    session.server.port = SMTP_PORT;
    session.login.email = AUTHOR_EMAIL;
    session.login.password = AUTHOR_PASSWORD;
    session.login.user_domain = "";

    /* Declare the message class */
    SMTP_Message message;

    /* Set the message headers */
    message.sender.name = "SMP";
    message.sender.email = AUTHOR_EMAIL;
    message.subject = "Connection established";
    message.addRecipient("Vlad", RECIPIENT_EMAIL);

    /*Send HTML message*/
    String htmlMsg = "<div style=\"color:#2f4468;\"><h1>Welcome!</h1><p>SMTP connection established!</p></div>";
    message.html.content = htmlMsg.c_str();
    message.html.content = htmlMsg.c_str();
    message.text.charSet = "us-ascii";
    message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

    if (!smtp.connect(&session))
    return;

    /* Start sending Email and close the session */
    if (!MailClient.sendMail(&smtp, &message))
      Serial.println("Error sending Email, " + smtp.errorReason());
    
}

void loop()
{
    //Read data and store it to variables hum and temp
    hum = dht.readHumidity();
    temp= dht.readTemperature();
    hi = -8.78469475556 + 1.61139411*temp + 2.33854883889*hum + -0.14611605*temp*hum + -0.012308094*temp*temp + -0.0164248277778*hum*hum + 2.211732*pow(10,-3)*temp*temp*hum + 7.2546*pow(10,-4)*temp*hum*hum + -3.582*pow(10,-6)*temp*temp*hum*hum;
    
    if (! sgp.IAQmeasure()) {
      return;
    }

    TVOC = sgp.TVOC;
    CO2 = sgp.eCO2;

    Serial.print("TVOC: "); 
    Serial.print(TVOC); 
    Serial.println(" ppb\t");
    Serial.print("CO2: "); 
    Serial.print(CO2); 
    Serial.println(" ppm");

    if (isnan(hum) || isnan(temp) || isnan(hi))
    {
        return;
    }

    //Print temp and humidity values to serial monitor
    Serial.print("Humidity: ");
    Serial.print(hum);
    Serial.println(" %");
    Serial.print("Temperature: ");
    Serial.print(temp);
    Serial.println(" °C");
    Serial.print("Heat index: ");
    Serial.print(hi);
    Serial.print(" °C => ");
    if (hi < 27){
      Serial.println("GOOD");
    }
    if (hi >= 27 && hi <= 32){
      Serial.println("CAUTION");
    }
    if (hi > 32 && hi <= 41){
      Serial.println("EXTREME CAUTION");
    }
    if (hi > 41 && hi <= 54){
      Serial.println("DANGER");
    }
    if (hi > 54){
      Serial.println("EXTREME DANGER");
    }
    Serial.println("===========================");
    delay(2000); //Delay 2 sec.

    // Table 
    tft.drawRect(0, 0, 160, 128, ST7735_WHITE);
    tft.drawLine(0, 15, 160, 15, ST7735_WHITE);
    tft.drawLine(0, 30, 160, 30, ST7735_WHITE);
    tft.drawLine(0, 60, 160, 60, ST7735_WHITE);
    tft.drawLine(0, 75, 160, 75, ST7735_WHITE);
    tft.drawLine(0, 90, 160, 90, ST7735_WHITE);

    temperature_to_lcd (temp, 4);
    humidity_to_lcd (hum, 19);
    hi_to_lcd (hi, 34);
    TVOC_to_lcd (TVOC, 64);
    CO2_to_lcd (CO2, 79);

    if(client.connect(server,80)){
        updateThingSpeak(temp,hum,hi,TVOC,CO2);
    }

    if(hi >= 27 && asapEmail_hi == 1){
    auto string_hi = std::to_string(hi);

    smtp.debug(1);

    /* Set the callback function to get the sending results */
    smtp.callback(smtpCallback);

    /* Declare the session config data */
    ESP_Mail_Session session;

    /* Set the session config */
    session.server.host_name = SMTP_HOST;
    session.server.port = SMTP_PORT;
    session.login.email = AUTHOR_EMAIL;
    session.login.password = AUTHOR_PASSWORD;
    session.login.user_domain = "";

    /* Declare the message class */
    SMTP_Message message;

    /* Set the message headers */
    message.sender.name = "SMP";
    message.sender.email = AUTHOR_EMAIL;
    message.subject = "ALERT - Heat index is too high !!!";
    message.addRecipient("Vlad", RECIPIENT_EMAIL);

    //Send raw text message
    String htmlMsg = "Heat index value is: ";
    message.text.content = htmlMsg.c_str()+string_hi+" %";
    message.text.charSet = "us-ascii";
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

    /* Connect to server with the session config */
    if (!smtp.connect(&session))
      return;

    /* Start sending Email and close the session */
    if (!MailClient.sendMail(&smtp, &message))
      Serial.println("Error sending Email, " + smtp.errorReason());

    asapEmail_hi = 0;
    
    }

    if(hi<27){
      asapEmail_hi = 1;  
    }

    if(TVOC > 600 && asapEmail_tvoc == 1){
    auto string_tvoc = std::to_string(TVOC);

    smtp.debug(1);

    /* Set the callback function to get the sending results */
    smtp.callback(smtpCallback);

    /* Declare the session config data */
    ESP_Mail_Session session;

    /* Set the session config */
    session.server.host_name = SMTP_HOST;
    session.server.port = SMTP_PORT;
    session.login.email = AUTHOR_EMAIL;
    session.login.password = AUTHOR_PASSWORD;
    session.login.user_domain = "";

    /* Declare the message class */
    SMTP_Message message;

    /* Set the message headers */
    message.sender.name = "SMP";
    message.sender.email = AUTHOR_EMAIL;
    message.subject = "ALERT - TVOC is too high !!!";
    message.addRecipient("Vlad", RECIPIENT_EMAIL);

    //Send raw text message
    String htmlMsg = "TVOC value is: ";
    message.text.content = htmlMsg.c_str()+string_tvoc+" ppb";
    message.text.charSet = "us-ascii";
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

    /* Connect to server with the session config */
    if (!smtp.connect(&session))
      return;

    /* Start sending Email and close the session */
    if (!MailClient.sendMail(&smtp, &message))
      Serial.println("Error sending Email, " + smtp.errorReason());

    asapEmail_tvoc = 0;
    
    }

    if(TVOC<600){
      asapEmail_tvoc = 1;  
  }
     
}

// outputs temperature to LCD
void temperature_to_lcd (float temperature, unsigned char text_position )
{
  tft.setCursor(4,text_position);       
  tft.setTextColor(ST7735_WHITE,ST7735_BLACK);
  tft.setTextSize(1);
  tft.print("Temperature:");

  tft.setTextSize(1);

  tft.setCursor(73,text_position);
  fix_number_position(temperature);
  tft.setTextColor(0xF800,ST7735_BLACK);
  tft.print(temperature,1);

  tft.setCursor(112,text_position);
  tft.print("C"); 
  tft.drawChar(105,text_position, 247, 0xF800, ST7735_BLACK, 1); //degree symbol

}

//outputs humidity to LCD

void humidity_to_lcd (float humidity, unsigned char text_position )
{
  tft.setTextColor(ST7735_WHITE,ST7735_BLACK);
  tft.setCursor(4,text_position); 
  tft.setTextSize(1); 
  tft.println("Humidity:");
  tft.setTextSize(1);
  tft.setCursor(58,text_position);

  fix_number_position(humidity);
  tft.setTextColor(0x001F,ST7735_BLACK);

  tft.print(humidity,1);
  tft.print(" %");       

}

//outputs heat index to LCD

void hi_to_lcd (float hi, unsigned char text_position )
{
  tft.setCursor(4,text_position);       
  tft.setTextColor(ST7735_WHITE,ST7735_BLACK);
  tft.setTextSize(1);
  tft.print("Heat index:");

  tft.setTextSize(1);

  tft.setCursor(70,text_position);
  fix_number_position(hi);
  tft.setTextColor(0x77FF33,ST7735_BLACK);
  tft.print(hi,1);

  tft.setCursor(110,text_position);
  tft.print("C"); 
  tft.drawChar(103,text_position, 247, 0x77FF33, ST7735_BLACK, 1); //degree symbol

  if (hi < 27){
    tft.setTextColor(ST7735_BLACK,ST7735_BLACK);
    tft.setTextSize(1);
    tft.setCursor(55,text_position+15);
    tft.setTextColor(ST7735_GREEN,ST7735_BLACK);
    tft.print("GOOD           "); 
  }
  if (hi >= 27 && hi <= 32){
    tft.setTextColor(ST7735_BLACK,ST7735_BLACK);
    tft.setTextSize(1);
    tft.setCursor(55,text_position+15);
    tft.setTextColor(ST7735_MAGENTA,ST7735_BLACK);
    tft.print("CAUTION         "); 
  }
  if (hi > 32 && hi <= 41){
    tft.setTextColor(ST7735_BLACK,ST7735_BLACK);
    tft.setTextSize(1);
    tft.setCursor(55,text_position+15);
    tft.setTextColor(0x001F,ST7735_BLACK);
    tft.print("EXTREME CAUTION");
  }
  if (hi > 41 && hi <= 54){
    tft.setTextColor(ST7735_BLACK,ST7735_BLACK);
    tft.setTextSize(1);
    tft.setCursor(55,text_position+15);
    tft.setTextColor(0x001F,ST7735_YELLOW);
    tft.print("DANGER         "); 
  }
  if (hi > 54){
    tft.setTextColor(ST7735_BLACK,ST7735_BLACK);
    tft.setTextSize(1);
    tft.setCursor(55,text_position+15);
    tft.setTextColor(0x001F,ST7735_YELLOW);
    tft.print("EXTREME DANGER"); 
  }
}

void TVOC_to_lcd (int TVOC, unsigned char text_position )
{
  tft.setTextColor(ST7735_WHITE,ST7735_BLACK);
  tft.setCursor(4,text_position); 
  tft.setTextSize(1); 
  tft.println("TVOC [ppb]:");
  
  tft.setTextSize(1);
  tft.setCursor(70,text_position);
  tft.print("       ");  
  tft.setTextSize(1);
  tft.setCursor(70,text_position);
  fix_number_position(TVOC);
  tft.setTextColor(0xF800,ST7735_BLACK);

  tft.print(TVOC,1);     

}

void CO2_to_lcd (int CO2, unsigned char text_position )
{
  tft.setTextColor(ST7735_WHITE,ST7735_BLACK);
  tft.setCursor(4,text_position); 
  tft.setTextSize(1); 
  tft.println("CO2 [ppm]:");

  tft.setTextSize(1);
  tft.setCursor(65,text_position);
  tft.print("       ");  
  tft.setTextSize(1);
  tft.setCursor(65,text_position);

  fix_number_position(CO2);
  tft.setTextColor(0xF800,ST7735_BLACK);

  tft.print(CO2,1);
}

void fix_number_position(float number)
{
  if ((number >= -40)&&(number < -9.9))
  {
    ;
  } 

  if ((number >= -9.9)&&(number < 0.0))
  {
    tft.print(" ");
  }

  if ((number >= 0.0 )&&(number < 9.9))
  {
    tft.print("  ");
  }

  if ((number >= 9.9 )&&(number < 99.9))
  {
    tft.print(" ");
  }

  if ((number >= 99.9 )&&(number < 151))
  {
    tft.print("");
  }
}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status){
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()){
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++){
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");
  }
}
