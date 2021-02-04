/*******************************************************************
    A scrolling text sign on 2 RGB LED matrixes where the text
    can be updated via telegram.

    Parts Used:
    2 x 64x32 P3 Matrix display * - https://s.click.aliexpress.com/e/_dYz5DLt
    ESP32 D1 Mini * - https://s.click.aliexpress.com/e/_dSi824B
    ESP32 I2S Matrix Shield (From my Tindie) = https://www.tindie.com/products/brianlough/esp32-i2s-matrix-shield/

 *  * = Affilate

    If you find what I do useful and would like to support me,
    please consider becoming a sponsor on Github
    https://github.com/sponsors/witnessmenow/


    Written by Brian Lough
    YouTube: https://www.youtube.com/brianlough
    Tindie: https://www.tindie.com/stores/brianlough/
    Twitter: https://twitter.com/witnessmenow
 *******************************************************************/

#include "emojiiImages.h"

// ----------------------------
// Standard Libraries
// ----------------------------

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FS.h>
#include <SPIFFS.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <WiFiManager.h>
// Captive portal for configuring the WiFi

// Can be installed from the library manager (Search for WifiManager", install the Alhpa version)
// https://github.com/tzapu/WiFiManager

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
// This is the library for interfacing with the display

// Can be installed from the library manager (Search for "ESP32 MATRIX DMA")
// https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA

#include <UniversalTelegramBot.h>
// This is the library for connecting to Telegram

// Can be installed from the library manager (Search for "Universal Telegram")
// https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot/

// ----------------------------
// Dependancy Libraries - each one of these will need to be installed.
// ----------------------------

// Adafruit GFX library is a dependancy for the matrix Library
// Can be installed from the library manager
// https://github.com/adafruit/Adafruit-GFX-Library

// ArduinoJson is used for parsing Json from the API responses.
// It is required by the Telegram Library.
// Search for "Arduino Json" in the Arduino Library manager
// https://github.com/bblanchon/ArduinoJson


// -------------------------------------
// ------- Replace the following! ------
// -------------------------------------

// Wifi network station credentials
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASSWORD "YOUR_PASSWORD"

// Telegram BOT Token (Get from Botfather)
#define BOT_TOKEN "XXXXXXXXX:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"

//------- ---------------------- ------

// True enables dubble buffer, this reduces flicker
MatrixPanel_I2S_DMA dma_display(true);

WiFiClientSecure secured_client;
UniversalTelegramBot bot("12345", secured_client);

#define BUTTON_PIN 21

uint16_t myBLACK = dma_display.color565(0, 0, 0);
uint16_t myWHITE = dma_display.color565(255, 255, 255);
uint16_t myRED = dma_display.color565(255, 0, 0);
uint16_t myGREEN = dma_display.color565(0, 255, 0);
uint16_t myBLUE = dma_display.color565(0, 0, 255);

char botToken[40];
char chatId[20]  = "-1";
String chatIdStr;
char scrollSpeed[3] = "35";

//flag for saving data
bool shouldSaveConfig = false;

// -------------------------------------
// -------   Text Configuraiton   ------
// -------------------------------------

#define FONT_SIZE 2 // Text will be FONT_SIZE x 8 pixels tall.

#define VERTICAL_SCROLL 120
#define HORIZONTAL_SCROLL 35

int delayBetweeenAnimations = HORIZONTAL_SCROLL; // How fast it scrolls, Smaller == faster
int scrollXMove = -1; //If positive it would scroll right
int scrollYMove = -1; //If positive it would scroll down (would need to adjust start position)

int textXPosition = dma_display.width(); // Will start one pixel off screen
int textYPosition = dma_display.height() / 2 - (FONT_SIZE * 8 / 2); // This will center the text

String shownText = "Hi!"; //Starting Text (this can't have Emojiis at the moment!)
uint16_t textColour = myRED;
//------- ---------------------- ------

// For scrolling Text
unsigned long isAnimationDue;

unsigned long secondCountDown;

unsigned long telegramCoolDown;

int countDownValue = 5;
int lastDisplayedCount;

enum AnimationState { sScroll, sCount, sVert };

AnimationState screenState = sScroll;

struct emojii
{
  int xOffset;
  int yOffset;
  const unsigned short *image;
  emojii *next;
};

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setupSpiffs() {
  //clean FS, for testing
  // SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  // May need to make it begin(true) first time you are using SPIFFS
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        StaticJsonDocument<512> json;
        DeserializationError error = deserializeJson(json, configFile);
        serializeJsonPretty(json, Serial);
        if (!error) {
          Serial.println("\nparsed json");

          strcpy(botToken, json["botToken"]);
          strcpy(chatId, json["chatId"]);
          strcpy(scrollSpeed, json["scrollSpeed"]);

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
}

void setup() {

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(115200);

  setupSpiffs();

  WiFiManager wm;

  //set config save notify callback
  wm.setSaveConfigCallback(saveConfigCallback);

  WiFiManagerParameter custom_bot_token("botToken", "Bot Token", botToken, 60);
  WiFiManagerParameter custom_chat_id("chatId", "Chat Id", chatId, 20);
  WiFiManagerParameter custom_scroll_speed("scrollSpeed", "Scroll Speed", scrollSpeed, 3);

  //add all your parameters here
  wm.addParameter(&custom_bot_token);
  wm.addParameter(&custom_chat_id);
  wm.addParameter(&custom_scroll_speed);

  //reset settings - wipe credentials for testing
  //wm.resetSettings();

  // Display Setup
  dma_display.begin();
  dma_display.fillScreen(myBLACK);
  dma_display.showDMABuffer();
  dma_display.setTextSize(FONT_SIZE);
  dma_display.setTextWrap(false); // N.B!! Don't wrap at end of line
  dma_display.setTextColor(textColour); // Can change the colour here

  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Entering Forced Config Mode");
    if (!wm.startConfigPortal("SmarterDisplay", "everyday")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }
  } else {
    if (!wm.autoConnect("SmarterDisplay", "everyday")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      // if we still have not connected restart and try all over again
      ESP.restart();
      delay(5000);
    }
  }

  strcpy(botToken, custom_bot_token.getValue());
  strcpy(chatId, custom_chat_id.getValue());
  strcpy(scrollSpeed, custom_scroll_speed.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonDocument  json(200);
    json["botToken"] = botToken;
    json["chatId"]   = chatId;
    json["scrollSpeed"]   = scrollSpeed;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    serializeJsonPretty(json, Serial);
    if (serializeJson(json, configFile) == 0) {
      Serial.println(F("Failed to write to file"));
    }
    configFile.close();
    //end save
    shouldSaveConfig = false;
  }

  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  bot.updateToken(String(botToken));

  chatIdStr = String(chatId);

}


void drawEmojii(int x, int y, emojii *e)
{
  int xOffset = x + e->xOffset;
  // Check if the Emojii is on the screen
  if (xOffset > -30 && xOffset < dma_display.width())
  {
    if (y > - 30 && y < dma_display.height())
    {
      int arrayIndex;
      for (int i = 0; i < 30; i++)
      {
        //    Serial.print("I: ");
        //    Serial.print(i);
        arrayIndex = i * 30;
        for (int j = 0; j < 30; j++) {

          // If this pixel is black, no need to draw it
          if (e->image[arrayIndex + j] > 0) // Don't need to draw black
          {
            // Check is this pixel on screen
            if (xOffset + j >= 0 && x + j < dma_display.width())
            {
              dma_display.drawPixel(xOffset + j, y + i, e->image[arrayIndex + j]);
            }
          }
        }
      }
    }
  }
}

// Will be used in getTextBounds.
int16_t xOne, yOne;
uint16_t w, h;
bool checkTelegram = false;
bool displayReady = false;

String hiddenVersion; //This string will be used to index the Emojiis

int emojiiIndex = 0;

int panicButtonCount = 0;
bool panicMode = false;

emojii *firstEmojii = NULL;
emojii *currentEmojii = NULL;
emojii *tempEmojii = NULL;


// This function we will extract out the location of the Emojii and save it
// to the linked list
void parseEmojii(String stringToParse, String displayString, const char* emojiiChar, const unsigned short *emojiiImage, int yOffset = -1) {
  emojiiIndex = 0;

  emojiiIndex = stringToParse.indexOf(emojiiChar);
  while (emojiiIndex >= 0) {
    //Serial.println("Found");
    if (currentEmojii == NULL) {
      firstEmojii = new emojii;
      currentEmojii = firstEmojii;
    } else {
      currentEmojii->next = new emojii;
      tempEmojii = currentEmojii;
      currentEmojii = currentEmojii->next;
    }

    currentEmojii->yOffset = yOffset;
    currentEmojii->next = NULL;
    currentEmojii->image = emojiiImage;

    // Finding out where the Emojii should be
    if (emojiiIndex == 0) {
      currentEmojii->xOffset = 0;
    } else {
      dma_display.getTextBounds(displayString.substring(0, emojiiIndex), 0, 0, &xOne, &yOne, &w, &h);
      currentEmojii->xOffset = w;
    }

    emojiiIndex = stringToParse.indexOf(emojiiChar, emojiiIndex + 1);
  }
}

bool replaceEmojiiText(const char* emojiiChar, const char* visibleReplacement, const char* hiddenReplacement) {
  if (hiddenVersion.indexOf(emojiiChar) >= 0) {
    shownText.replace(emojiiChar, visibleReplacement);
    hiddenVersion.replace(emojiiChar, hiddenReplacement);
    return true;
  }

  return false;
}

void destroyEmojiiList() {
  currentEmojii = firstEmojii;
  while (currentEmojii != NULL) {
    tempEmojii = currentEmojii;
    currentEmojii = currentEmojii->next;
    delete tempEmojii;
  }
  firstEmojii = NULL;
}


void loop() {
  if (!displayReady) {

    // Because we are using a double buffer, we don't need to wait
    // for when the animation is due before drawing it to the buffer
    // it will only get swapped onto the display when the animation is due.


    switch ( screenState) {
      case sCount:
        {
          unsigned long countNow = millis();
          if (countNow > secondCountDown)
          {
            if (countDownValue > 0)
            {
              countDownValue--;
              secondCountDown = countNow + 1000;

              // Get the bounds of the text
              dma_display.getTextBounds(String(countDownValue), 0, 0, &xOne, &yOne, &w, &h);

              // Set the cursor to center it
              dma_display.setCursor(dma_display.width() / 2 - w / 2 + 1, 4);
              dma_display.fillScreen(myBLACK);
              dma_display.print(countDownValue);
            } else {
              checkTelegram = true;
            }
          }
          displayReady = true;
          break;
        }
      case sScroll:

        // Update X position of text
        textXPosition += scrollXMove;

        // Checking if the end of the text is off screen to the left
        dma_display.getTextBounds(shownText, textXPosition, textYPosition, &xOne, &yOne, &w, &h);
        if (textXPosition + w <= 0)
        {
          checkTelegram = true;

          // Reset text to scroll again
          textXPosition = dma_display.width();
        }

        dma_display.setCursor(textXPosition, textYPosition);

        dma_display.fillRect(0, 2, dma_display.width(), 30, myBLACK);

        dma_display.print(shownText);
        currentEmojii = firstEmojii;
        while (currentEmojii != NULL) {
          //Serial.println("Meow");
          drawEmojii(textXPosition, 2, currentEmojii);
          currentEmojii = currentEmojii->next;
        }

        displayReady = true;
        break;

      case sVert:

        // Update Y position of text
        textYPosition += scrollYMove;

        // Checking if the end of the text is off screen to the left
        dma_display.getTextBounds(shownText, 0, textYPosition + 8, &xOne, &yOne, &w, &h);
        Serial.print("Vert Check: ");
        Serial.println(textYPosition + h);
        if (textYPosition + h + 7 <= 0)
        {
          checkTelegram = true;

          // Reset text to scroll again
          textYPosition = dma_display.height() + 7;
        }

        //textXPosition = (dma_display.width() / 2) - (w / 2) + 1;

        dma_display.setCursor(0, textYPosition);

        dma_display.fillScreen(myBLACK);

        dma_display.print(shownText);
        currentEmojii = firstEmojii;
        while (currentEmojii != NULL) {
          //Serial.println("Meow");
          drawEmojii(0, textYPosition + currentEmojii->yOffset, currentEmojii);
          currentEmojii = currentEmojii->next;
        }

        displayReady = true;
        break;
    }

  }

  unsigned long now = millis();
  if (now > isAnimationDue)
  {
    // This sets the timer for when we should scroll again.
    isAnimationDue = now + delayBetweeenAnimations;

    if (screenState == sCount && lastDisplayedCount == countDownValue) {
      // No need to display
    } else {
      if (screenState == sCount)
      {
        lastDisplayedCount = countDownValue;
      }
      // This code swaps the second buffer to be visible (puts it on the display)
      dma_display.showDMABuffer();

      // This tells the code to update the second buffer
      dma_display.flipDMABuffer();
    }

    if (digitalRead(BUTTON_PIN) == LOW) {
      if (!panicMode)
      {
        panicButtonCount++;
        if (panicButtonCount > 60) {
          // we are panic-in skywalker
          panicMode = true;
          Serial.println("We are panic-in Skywalker right now");

          //Set the display to countdown for 5 seconds
          countDownValue = 6;
          secondCountDown = 0; // will trigger straight away

          lastDisplayedCount = -1;

          screenState = sCount;
          dma_display.setTextSize(3);
          panicButtonCount = 0;
        }
      }
    } else {
      panicButtonCount = 0;
      panicMode = false;
    }
    displayReady = false;

  }

  // Telegram will only be checked when there is no data on screen
  // as checking interfeers with scrolling.
  if (checkTelegram && now > telegramCoolDown)
  {
    telegramCoolDown = now + 1000;
    checkTelegram = false;

    int skipMessage = (digitalRead(BUTTON_PIN) == LOW) ? 1 : 0;
    if (skipMessage > 0) {
      Serial.println("Skipping the next Telegram Message");
    }
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1 + skipMessage);
    Serial.print("numNewMessages: ");
    Serial.println(numNewMessages);
    Serial.print("WiFi Strength: ");
    Serial.println(WiFi.RSSI());
    //    if(numNewMessages == 0)
    //      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    if (numNewMessages > 0)
    {
      Serial.println("got response");

      // This is where you would check is the message from a valid source
      // You can get your ID from myIdBot in the Telegram client
      //if(bot.messages[0].chat_id == "175753388")
      if (chatIdStr == "-1" || chatIdStr == bot.messages[0].chat_id ) {

        // destroy the old Emojii objects
        destroyEmojiiList();
        dma_display.fillScreen(myBLACK);

        // Takes the contents of the message, and sets it to be displayed.
        hiddenVersion = bot.messages[0].text;
        shownText = String(hiddenVersion); // Making a second copy of the text.
        Serial.println(hiddenVersion);
        if (hiddenVersion.startsWith("/count")) {
          hiddenVersion.replace("/count", "");
          countDownValue = hiddenVersion.toInt() + 1; // We will trigger the countdown mechanic imedialty,
          // so the plus one will get cut off.
          secondCountDown = 0; // will trigger straight away

          lastDisplayedCount = -1;

          screenState = sCount;
          dma_display.setTextSize(3);
        } else if (hiddenVersion.indexOf("\n") >= 0) {
          screenState = sVert;
          textYPosition = dma_display.height() + 7; // 7 to allow for Emoji
          delayBetweeenAnimations = VERTICAL_SCROLL;
          replaceEmojiiText("\n", "\n\n", "^^");
          dma_display.setTextSize(2);

          bool hasEmojii = replaceEmojiiText("🦖", "   ", "%1%"); // T-Rex
          hasEmojii = replaceEmojiiText("🦕", "   ", "%2%") || hasEmojii; // Tall Dino
          hasEmojii = replaceEmojiiText("🚀", "   ", "%3%") || hasEmojii; // Rocket
          hasEmojii = replaceEmojiiText("🌕", "   ", "%4%") || hasEmojii; // Full Moon
          hasEmojii = replaceEmojiiText("🌙", "   ", "%5%") || hasEmojii; // Crecent Moon
          hasEmojii = replaceEmojiiText("🪐", "   ", "%6%") || hasEmojii; // Crecent Moon

          int lineCount = 0;
          Serial.println(hiddenVersion);
          String hiddenTextTemp = String(hiddenVersion);
          String visibleTextTemp = String(shownText);
          bool keepChecking = true;
          while (keepChecking) {
            int endIndex = hiddenTextTemp.indexOf("^^");
            String tempHidden;
            String tempShown;
            if (endIndex > 0) {
              tempHidden = hiddenTextTemp.substring(0, endIndex);
              tempShown = visibleTextTemp.substring(0, endIndex);

              hiddenTextTemp = hiddenTextTemp.substring(endIndex + 2);
              visibleTextTemp = visibleTextTemp.substring(endIndex + 2);
            } else {
              keepChecking = false;
              tempHidden = hiddenTextTemp;
              tempShown = visibleTextTemp;
            }
            Serial.print("tempHidden: ");
            Serial.println(tempHidden);
            int yOffset = (32 * lineCount) - 7;
            parseEmojii(tempHidden, tempShown, "%1%", BrownDino, yOffset);
            parseEmojii(tempHidden, tempShown, "%2%", TallDino, yOffset);
            parseEmojii(tempHidden, tempShown, "%3%", Rocket, yOffset);
            parseEmojii(tempHidden, tempShown, "%4%", Moon, yOffset);
            parseEmojii(tempHidden, tempShown, "%5%", Cresent, yOffset);
            parseEmojii(tempHidden, tempShown, "%6%", Ring, yOffset);
            lineCount ++;

          }


        } else {


          //Stripping out new line
          replaceEmojiiText("\n", "", "");

          bool hasEmojii = replaceEmojiiText("🦖", "   ", "%1%"); // T-Rex
          hasEmojii = replaceEmojiiText("🦕", "   ", "%2%") || hasEmojii; // Tall Dino
          hasEmojii = replaceEmojiiText("🚀", "   ", "%3%") || hasEmojii; // Rocket
          hasEmojii = replaceEmojiiText("🌕", "   ", "%4%") || hasEmojii; // Full Moon
          hasEmojii = replaceEmojiiText("🌙", "   ", "%5%") || hasEmojii; // Crecent Moon
          hasEmojii = replaceEmojiiText("🪐", "   ", "%6%") || hasEmojii; // Crecent Moon

          if (hasEmojii)
          {
            // This will extract the correct position to draw each emoji
            parseEmojii(hiddenVersion, shownText, "%1%", BrownDino);
            parseEmojii(hiddenVersion, shownText, "%2%", TallDino);
            parseEmojii(hiddenVersion, shownText, "%3%", Rocket);
            parseEmojii(hiddenVersion, shownText, "%4%", Moon);
            parseEmojii(hiddenVersion, shownText, "%5%", Cresent);
            parseEmojii(hiddenVersion, shownText, "%6%", Ring);
          }

          screenState = sScroll;
          textYPosition = dma_display.height() / 2 - (FONT_SIZE * 8 / 2);
          delayBetweeenAnimations = HORIZONTAL_SCROLL;
          dma_display.setTextSize(FONT_SIZE);
        }
      }
    }
  }

}
