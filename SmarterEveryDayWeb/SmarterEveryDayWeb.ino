/*******************************************************************
    A scrolling text sign on 2 RGB LED matrixes where the text
    can be updated via an internally hosted webpage.

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

#include "emojiImages.h"

#include "emojiFunctions.h"

// ----------------------------
// Standard Libraries
// ----------------------------

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <ESPmDNS.h>

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

// ----------------------------
// Dependancy Libraries - each one of these will need to be installed.
// ----------------------------

// Adafruit GFX library is a dependancy for the matrix Library
// Can be installed from the library manager
// https://github.com/adafruit/Adafruit-GFX-Library



// -------------------------------------
// ------- Replace the following! ------
// -------------------------------------

// These are now set using the config mode

// Wifi network station credentials
//#define WIFI_SSID "YOUR_SSID"
//#define WIFI_PASSWORD "YOUR_PASSWORD"
//
//// Telegram BOT Token (Get from Botfather)
//#define BOT_TOKEN "XXXXXXXXX:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"

// -------------------------------------
// -------   Matrix Config   ------
// -------------------------------------

#define PANEL_RES_X 64      // Number of pixels wide of each INDIVIDUAL panel module. 
#define PANEL_RES_Y 32     // Number of pixels tall of each INDIVIDUAL panel module.
#define PANEL_CHAIN 4      // Total number of panels chained one to another

//------- ---------------------- ------

WebServer server(80);

MatrixPanel_I2S_DMA *dma_display = nullptr;

#define BUTTON_PIN 21

uint16_t myBLACK = dma_display->color565(0, 0, 0);
uint16_t myWHITE = dma_display->color565(255, 255, 255);
uint16_t myRED = dma_display->color565(255, 0, 0);
uint16_t myGREEN = dma_display->color565(0, 255, 0);
uint16_t myBLUE = dma_display->color565(0, 0, 255);

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

int textXPosition = PANEL_RES_X * PANEL_CHAIN; // Will start one pixel off screen
int textYPosition = PANEL_RES_Y / 2 - (FONT_SIZE * 8 / 2); // This will center the text

String messageToDisplay;

String shownText = "Hi!"; //Starting Text (this can't have Emojis at the moment!)
uint16_t textColour = myRED;
uint16_t chartColour = myRED;
//------- ---------------------- ------

#define EMOJI_NUM_TYPES 7

// T-Rex, Tall Dino, Rocket, Full Moon, Crecent Moon, Ring Planet, scope

const char emojiSymbolArray[EMOJI_NUM_TYPES][5] = {"🦖", "🦕", "🚀", "🌕", "🌙", "🪐", "🔭"};
const char emojiCodeArray[EMOJI_NUM_TYPES][6] = {"%1%", "%2%", "%3%", "%4%", "%5%", "%6%", "%%7%%"};
const unsigned short *emojiImageArray[EMOJI_NUM_TYPES] = {BrownDino, TallDino, Rocket, Moon, Cresent, Ring, webbSmall};


// For scrolling Text
unsigned long isAnimationDue;

unsigned long secondCountDown;

unsigned long telegramCoolDown;

unsigned long lastPanicButtonTimer;
unsigned long panicButtonTotal;

int countDownValue = 5;
int lastDisplayedCount;

bool updateFinished = true;

int goalCurrent = 0;

enum AnimationState { sScroll, sCount, sVert, sStatic, sGoal };

AnimationState screenState = sScroll;

void configDisplay() {
  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,   // module width
    PANEL_RES_Y,   // module height
    PANEL_CHAIN    // Chain length
  );


  mxconfig.double_buff = true;
  mxconfig.gpio.e = 18;

  // May or may not be needed depending on your matrix
  // Example of what needing it looks like:
  // https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA/issues/134#issuecomment-866367216
  mxconfig.clkphase = false;

  //mxconfig.driver = HUB75_I2S_CFG::FM6126A;

  // Display Setup
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->fillScreen(myBLACK);
}

const char *webpage =
#include "webPage.h"
  ;

void handleRoot() {

  server.send(200, "text/html", webpage);
}



void setup() {

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(115200);

  WiFiManager wm;

  //reset settings - wipe credentials for testing
  //wm.resetSettings();

  // Display Setup
  configDisplay();
  dma_display->setTextSize(FONT_SIZE);
  dma_display->setTextWrap(false); // N.B!! Don't wrap at end of line
  dma_display->setTextColor(textColour); // Can change the colour here

  if (!wm.autoConnect("SmarterDisplay", "everyday")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    // if we still have not connected restart and try all over again
    ESP.restart();
    delay(5000);
  }

  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/message", handleMessage);
  server.on("/count", handleCountDown);
  server.on("/goal", handleGoal);
  server.on("/static", handleStaticMessage);
  server.on("/vertical", handleVertical);
  server.enableCORS(true);
  server.begin();
  chatIdStr = String(chatId);

  shownText = WiFi.localIP().toString();

}


void drawEmoji(int x, int y, emoji *e)
{
  int xOffset = x + e->xOffset;
  // Check if the Emoji is on the screen
  if (xOffset > - (e->emojiWidth) && xOffset < dma_display->width())
  {
    if (y > - (e->emojiHeight) && y < dma_display->height())
    {
      int arrayIndex;
      for (int i = 0; i < e->emojiHeight; i++)
      {
        //    Serial.print("I: ");
        //    Serial.print(i);
        arrayIndex = i * e->emojiWidth;
        for (int j = 0; j < e->emojiWidth; j++) {

          // If this pixel is black, no need to draw it
          if (e->image[arrayIndex + j] > 0) // Don't need to draw black
          {
            // Check is this pixel on screen
            if (xOffset + j >= 0 && x + j < dma_display->width())
            {
              dma_display->drawPixel(xOffset + j, y + i, e->image[arrayIndex + j]);
            }
          }
        }
      }
    }
  }
}

bool checkTelegram = false;
bool displayReadyForDraw = false;

String hiddenVersion; //This string will be used to index the Emojis

int panicButtonCount = 0;
bool panicMode = false;

int wifiDownCount = 0;

void processEmoji() {
  bool hasEmoji = false;
  for (int i = 0; i < EMOJI_NUM_TYPES; i++) {
    //            Serial.print(i);
    //            Serial.print(" ");
    //            Serial.println(emojiSymbolArray[i]);
    if (i == 6) {
      // Extra Spacing for Webb
      hasEmoji = replaceEmojiText(hiddenVersion, shownText, emojiSymbolArray[i], "     ", emojiCodeArray[i]) || hasEmoji;
    } else {
      hasEmoji = replaceEmojiText(hiddenVersion, shownText, emojiSymbolArray[i], "   ", emojiCodeArray[i]) || hasEmoji;
    }
    //            Serial.println(hiddenVersion);
  }


  if (hasEmoji)
  {
    for (int i = 0; i < EMOJI_NUM_TYPES; i++) {
      parseEmoji(hiddenVersion, shownText, emojiCodeArray[i], emojiImageArray[i], dma_display);
    }
  }
}

void prepareAnimationStart() {
  // destroy the old Emoji objects
  destroyEmojiList();

  // blank the canvas
  dma_display->fillScreen(myBLACK);

  // Display
  displayReadyForDraw = false;
}

void prepareMessageAnimation(String msg) {

  prepareAnimationStart();

  // Takes the contents of the message, and sets it to be displayed.
  hiddenVersion = msg;
  shownText = String(msg); // Making a second copy of the text.

  //Stripping out new line
  replaceEmojiText(hiddenVersion, shownText, "\n", "", "");

  dma_display->setTextSize(FONT_SIZE);
  processEmoji();

  screenState = sScroll;
  textXPosition = dma_display->width();
  textYPosition = dma_display->height() / 2 - (FONT_SIZE * 8 / 2);
  delayBetweeenAnimations = HORIZONTAL_SCROLL;
}

void prepareVerticalMessageAnimation(String msg) {

  prepareAnimationStart();

  // Takes the contents of the message, and sets it to be displayed.
  hiddenVersion = msg;
  shownText = String(msg); // Making a second copy of the text.

  screenState = sVert;
  textYPosition = dma_display->height() + 7; // 7 to allow for Emoji
  delayBetweeenAnimations = VERTICAL_SCROLL;
  replaceEmojiText(hiddenVersion, shownText, "\n", "\n\n", "^^");
  dma_display->setTextSize(2);

  bool hasEmoji = true;
  for (int i = 0; i < EMOJI_NUM_TYPES; i++) {
    replaceEmojiText(hiddenVersion, shownText, emojiSymbolArray[i], "   ", emojiCodeArray[i]) || hasEmoji;
  }

  int lineCount = 0;
  //Serial.println(hiddenVersion);
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
    //Serial.print("tempHidden: ");
    //Serial.println(tempHidden);
    int yOffset = (dma_display->height() * lineCount) - 7;
    for (int i = 0; i < EMOJI_NUM_TYPES; i++) {
      parseEmoji(tempHidden, tempShown, emojiCodeArray[i], emojiImageArray[i], dma_display, yOffset);
    }
    lineCount ++;

  }
}

void prepareStaticMessageAnimation(String msg) {

  prepareAnimationStart();

  // Takes the contents of the message, and sets it to be displayed.
  hiddenVersion = msg;
  shownText = String(msg); // Making a second copy of the text.

  //Stripping out new line
  replaceEmojiText(hiddenVersion, shownText, "\n", "", "");

  dma_display->setTextSize(FONT_SIZE);
  processEmoji();

  screenState = sStatic;
  updateFinished = false;
  textYPosition = dma_display->height() / 2 - (FONT_SIZE * 8 / 2);
}

void prepareCountDownAnimation(String number) {

  prepareAnimationStart();

  // Takes the contents of the message, and sets it to be displayed.
  hiddenVersion = number;
  shownText = String(number); // Making a second copy of the text.

  countDownValue = hiddenVersion.toInt() + 1; // We will trigger the countdown mechanic imedialty,

  // so the plus one will get cut off.
  secondCountDown = 0; // will trigger straight away

  lastDisplayedCount = -1;

  screenState = sCount;
  dma_display->setTextSize(3);
}

void prepareGoalAnimation(int lower, int higher, int firstEmojiIndex = -1, int secondEmojiIndex = -1) {

  prepareAnimationStart();
  int emojiSpace = 0;
  if (firstEmojiIndex >= 0) {
    firstEmoji = new emoji;
    currentEmoji = firstEmoji;
    currentEmoji->xOffset = 0;
    currentEmoji->image = emojiImageArray[firstEmojiIndex];
    currentEmoji->next = NULL;
    if (firstEmojiIndex == 6) {
      currentEmoji->emojiWidth = 51;
    } else {
      currentEmoji->emojiWidth = 30;
    }

    currentEmoji->emojiHeight = 30;

    emojiSpace = emojiSpace + currentEmoji->emojiWidth;
  }

  if (secondEmojiIndex >= 0) {
    currentEmoji->next = new emoji;
    currentEmoji = currentEmoji->next;

    if (secondEmojiIndex == 6) {
      currentEmoji->emojiWidth = 51;
    } else {
      currentEmoji->emojiWidth = 30;
    }
    currentEmoji->emojiHeight = 30;

    emojiSpace = emojiSpace + currentEmoji->emojiWidth;

    currentEmoji->xOffset = dma_display->width() - currentEmoji->emojiWidth;
    currentEmoji->image = emojiImageArray[secondEmojiIndex];
    currentEmoji->next = NULL;
  }

  //Serial.print(lower);
  //Serial.print("/");
  //Serial.println(higher);

  if (lower <= higher) {
    screenState = sGoal;
    // map(value, fromLow, fromHigh, toLow, toHigh)
    goalCurrent = map(lower, 0, higher, 0, dma_display->width() - emojiSpace);
    updateFinished = false;
  }
}

void handleMessage() {

  String msg = server.arg("msg");
  prepareMessageAnimation(msg);


  server.send(200, "text/plain", "Ok");
}

void handleStaticMessage() {

  String msg = server.arg("msg");
  prepareStaticMessageAnimation(msg);


  server.send(200, "text/plain", "Ok");
}

void handleCountDown() {

  String number = server.arg("number");
  prepareCountDownAnimation(number);


  server.send(200, "text/plain", "Ok");
}

void handleGoal() {

  int lower = server.arg("lower").toInt();
  int higher = server.arg("higher").toInt();

  int firstEmojiIndex = -1;
  String temp = server.arg("firstEmojiIndex");
  if (temp != "") {
    firstEmojiIndex = temp.toInt();
  }

  int secondEmojiIndex = -1;
  temp = server.arg("secondEmojiIndex");
  if (temp != "") {
    secondEmojiIndex = temp.toInt();
  }

  prepareGoalAnimation(lower, higher, firstEmojiIndex, secondEmojiIndex);


  server.send(200, "text/plain", "Ok");
}

void handleVertical() {

  String msg = server.arg("msg");
  prepareVerticalMessageAnimation(msg);


  server.send(200, "text/plain", "Ok");
}


void loop() {
  if (!displayReadyForDraw) {

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
              dma_display->getTextBounds(String(countDownValue), 0, 0, &xOne, &yOne, &w, &h);

              // Set the cursor to center it
              dma_display->setCursor(dma_display->width() / 2 - w / 2 + 1, 4);
              dma_display->fillScreen(myBLACK);
              dma_display->print(countDownValue);
            } else {
              checkTelegram = true;
            }
          }
          displayReadyForDraw = true;
          break;
        }
      case sScroll:

        // Update X position of text
        textXPosition += scrollXMove;

        // Checking if the end of the text is off screen to the left
        dma_display->getTextBounds(shownText, textXPosition, textYPosition, &xOne, &yOne, &w, &h);
        if (textXPosition + w <= 0)
        {
          checkTelegram = true;

          // Reset text to scroll again
          textXPosition = dma_display->width();
        }

        dma_display->setCursor(textXPosition, textYPosition);

        dma_display->fillRect(0, 2, dma_display->width(), 30, myBLACK);

        dma_display->print(shownText);
        currentEmoji = firstEmoji;
        while (currentEmoji != NULL) {
          //Serial.println("Meow");
          drawEmoji(textXPosition, 2, currentEmoji);
          currentEmoji = currentEmoji->next;
        }

        displayReadyForDraw = true;
        break;

      case sVert:

        // Update Y position of text
        textYPosition += scrollYMove;

        // Checking if the end of the text is off screen to the left
        dma_display->getTextBounds(shownText, 0, textYPosition + 8, &xOne, &yOne, &w, &h);
        //Serial.print("Vert Check: ");
        //Serial.println(textYPosition + h);
        if (textYPosition + h + 7 <= 0)
        {
          checkTelegram = true;

          // Reset text to scroll again
          textYPosition = dma_display->height() + 7;
        }

        //textXPosition = (dma_display->width() / 2) - (w / 2) + 1;

        dma_display->setCursor(0, textYPosition);

        dma_display->fillScreen(myBLACK);

        dma_display->print(shownText);
        currentEmoji = firstEmoji;
        while (currentEmoji != NULL) {
          //Serial.println("Meow");
          drawEmoji(0, textYPosition + currentEmoji->yOffset, currentEmoji);
          currentEmoji = currentEmoji->next;
        }

        displayReadyForDraw = true;
        break;

      case sStatic:

        if (!updateFinished)
        {
          // Checking if the end of the text is off screen to the left
          dma_display->getTextBounds(shownText, textXPosition, textYPosition, &xOne, &yOne, &w, &h);

          textXPosition = dma_display->width() / 2 - w / 2 + 1;

          dma_display->setCursor(textXPosition, textYPosition);

          dma_display->fillRect(0, 2, dma_display->width(), 30, myBLACK);

          dma_display->print(shownText);
          currentEmoji = firstEmoji;
          while (currentEmoji != NULL) {
            //Serial.println("Meow");
            drawEmoji(textXPosition, 2, currentEmoji);
            currentEmoji = currentEmoji->next;
          }

        }
        checkTelegram = true;
        displayReadyForDraw = true;
        break;

      case sGoal:


        if (!updateFinished)
        {
          int leftOffset = 0;
          int rightOffset = 0;

          // Left Emoji
          if (firstEmoji != NULL)
          {

            drawEmoji(0, 2, firstEmoji);
            leftOffset = firstEmoji->emojiWidth;

            //Check for Right Emoji
            if (firstEmoji->next != NULL) {
              currentEmoji = firstEmoji->next;
              drawEmoji(0, 2, currentEmoji);
              rightOffset = currentEmoji->emojiWidth;
            }
          }

          dma_display->fillScreen(myBLACK);
          dma_display->drawRect(leftOffset, 4, dma_display->width() - (leftOffset + rightOffset), 24, chartColour);
          dma_display->drawRect(leftOffset + 1, 5, dma_display->width() - (leftOffset + rightOffset + 2), 22, chartColour);
          dma_display->fillRect(leftOffset, 4, goalCurrent, 24, chartColour);

          currentEmoji = firstEmoji;
          while (currentEmoji != NULL) {
            //Serial.println("Meow");
            drawEmoji(0, 2, currentEmoji);
            currentEmoji = currentEmoji->next;
          }
        }
        checkTelegram = true;
        displayReadyForDraw = true;
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
    } else if (screenState == sStatic && updateFinished) {
      // No need to display
    } else if (screenState == sGoal && updateFinished) {
      // No need to display
    } else {
      if (screenState == sCount)
      {
        lastDisplayedCount = countDownValue;
      }
      if (screenState == sStatic || screenState == sGoal) {
        updateFinished = true;
      }

      // This code swaps the second buffer to be visible (puts it on the display)
      dma_display->flipDMABuffer();
    }

    displayReadyForDraw = false;

  }

  unsigned long lastPanicButtonTimer;
  unsigned long panicButtonTotal;
  now = millis();
  //
  //  if (digitalRead(BUTTON_PIN) == LOW) {
  //    if (!panicMode)
  //    {
  //      panicButtonTotal = panicButtonTotal + (now - lastPanicButtonTimer);
  //      if (panicButtonTotal > 3000) {
  //        // we are panic-in skywalker
  //        panicMode = true;
  //        Serial.println("We are panic-in Skywalker right now");
  //
  //        //Set the display to countdown for 5 seconds
  //        countDownValue = 6;
  //        secondCountDown = 0; // will trigger straight away
  //
  //        lastDisplayedCount = -1;
  //
  //        screenState = sCount;
  //        dma_display->setTextSize(3);
  //        panicButtonTotal = 0;
  //      }
  //    }
  //  } else {
  //    panicButtonTotal = 0;
  //    panicMode = false;
  //  }

  server.handleClient();

  // Telegram will only be checked when there is no data on screen
  // as checking interfeers with scrolling.
  //    if (checkTelegram && now > telegramCoolDown)
  //    {
  //      telegramCoolDown = now + 1000;
  //      checkTelegram = false;
  //
  //      int skipMessage = (digitalRead(BUTTON_PIN) == LOW) ? 1 : 0;
  //      if (skipMessage > 0) {
  //        Serial.println("Skipping the next Telegram Message");
  //      }
  //
  //      Serial.print("WiFi Strength: ");
  //      if (WiFi.RSSI() == 0) {
  //        wifiDownCount++;
  //        if (wifiDownCount > 3) {
  //          Serial.println("Wfi seems to be disconnected, trying a restart");
  //          // if we still have not connected restart and try all over again
  //          ESP.restart();
  //          delay(5000);
  //        }
  //      }
  //      Serial.println(WiFi.RSSI());
  //      int numNewMessages = bot.getUpdates(bot.last_message_received + 1 + skipMessage);
  //      Serial.print("numNewMessages: ");
  //      Serial.println(numNewMessages);
  //      if (numNewMessages == 0)
  //        numNewMessages = bot.getUpdates(bot.last_message_received + 1 + skipMessage);
  //      if (numNewMessages > 0)
  //      {
  //        Serial.println("got response");
  //
  //        // This is where you would check is the message from a valid source
  //        // You can get your ID from myIdBot in the Telegram client
  //        //if(bot.messages[0].chat_id == "175753388")
  //        if (chatIdStr == "-1" || chatIdStr == bot.messages[0].chat_id )
  //
  //          // destroy the old Emoji objects
  //          destroyEmojiList();
  //          dma_display->fillScreen(myBLACK);
  //          displayReadyForDraw = false;
  //
  //          // Takes the contents of the message, and sets it to be displayed.
  //          hiddenVersion = bot.messages[0].text;
  //          shownText = String(hiddenVersion); // Making a second copy of the text.
  //          Serial.println(hiddenVersion);
  //
  //
  //          if (hiddenVersion.startsWith("/count")) {
  //            hiddenVersion.replace("/count", "");
  //            countDownValue = hiddenVersion.toInt() + 1; // We will trigger the countdown mechanic imedialty,
  //            // so the plus one will get cut off.
  //            secondCountDown = 0; // will trigger straight away
  //
  //            lastDisplayedCount = -1;
  //
  //            screenState = sCount;
  //            dma_display->setTextSize(3);
  //          } else if (hiddenVersion.startsWith("/goal")) {
  //            hiddenVersion.replace("/goal", "");
  //
  //            int firstEmojiRef = -1;
  //            int firstEmojiIndex = -1;
  //            int secondEmojiRef = -1;
  //            for (int i = 0; i < EMOJI_NUM_TYPES; i++) {
  //              int index = hiddenVersion.indexOf(emojiSymbolArray[i]);
  //              if (index >= 0) {
  //                if ( firstEmojiIndex == -1) {
  //                  firstEmojiIndex = index;
  //                  firstEmojiRef = i;
  //                } else if (index < firstEmojiIndex) {
  //                  secondEmojiRef = firstEmojiRef;
  //                  firstEmojiIndex = index;
  //                  firstEmojiRef = i;
  //                } else {
  //                  secondEmojiRef = i;
  //                }
  //              }
  //            }
  //
  //            if (firstEmojiRef >= 0) {
  //              firstEmoji = new emoji;
  //              currentEmoji = firstEmoji;
  //              currentEmoji->xOffset = 0;
  //              currentEmoji->image = emojiImageArray[firstEmojiRef];
  //              currentEmoji->next = NULL;
  //
  //              hiddenVersion.replace(emojiSymbolArray[firstEmojiRef], "");
  //
  //              if (secondEmojiRef >= 0) {
  //                currentEmoji->next = new emoji;
  //                currentEmoji = currentEmoji->next;
  //                currentEmoji->xOffset = dma_display->width() - 30;
  //                currentEmoji->image = emojiImageArray[secondEmojiRef];
  //                currentEmoji->next = NULL;
  //
  //                hiddenVersion.replace(emojiSymbolArray[secondEmojiRef], "");
  //              }
  //            }
  //
  //            int slashIndex = hiddenVersion.indexOf("/");
  //            if (slashIndex > 0) {
  //              int lower = hiddenVersion.substring(0, slashIndex).toInt();
  //              int higher = hiddenVersion.substring(slashIndex + 1).toInt();
  //              Serial.print(lower);
  //              Serial.print("/");
  //              Serial.println(higher);
  //
  //              if (lower <= higher) {
  //                screenState = sGoal;
  //                // map(value, fromLow, fromHigh, toLow, toHigh)
  //                goalCurrent = map(lower, 0, higher, 0, dma_display->width() - 60);
  //                updateFinished = false;
  //              }
  //            }
  //          } else if (hiddenVersion.startsWith("/static")) {
  //
  //            hiddenVersion.replace("/static ", "");
  //            hiddenVersion.replace("/static", "");
  //
  //            shownText.replace("/static ", "");
  //            shownText.replace("/static", "");
  //
  //            dma_display->setTextSize(FONT_SIZE);
  //
  //            updateFinished = false;
  //
  //            //Stripping out new line
  //            replaceEmojiText(hiddenVersion, shownText, "\n", "", "");
  //
  //            bool hasEmoji = true;
  //            for (int i = 0; i < EMOJI_NUM_TYPES; i++) {
  //              replaceEmojiText(hiddenVersion, shownText, emojiSymbolArray[i], "   ", emojiCodeArray[i]) || hasEmoji;
  //            }
  //
  //            if (hasEmoji)
  //            {
  //              for (int i = 0; i < EMOJI_NUM_TYPES; i++) {
  //                parseEmoji(hiddenVersion, shownText, emojiCodeArray[i], emojiImageArray[i], dma_display);
  //              }
  //            }
  //
  //            screenState = sStatic;
  //            dma_display->setTextSize(2);
  //            textYPosition = dma_display->height() / 2 - (FONT_SIZE * 8 / 2);
  //          } else if (hiddenVersion.indexOf("\n") >= 0) {
  //            screenState = sVert;
  //            textYPosition = dma_display->height() + 7; // 7 to allow for Emoji
  //            delayBetweeenAnimations = VERTICAL_SCROLL;
  //            replaceEmojiText(hiddenVersion, shownText, "\n", "\n\n", "^^");
  //            dma_display->setTextSize(2);
  //
  //            bool hasEmoji = true;
  //            for (int i = 0; i < EMOJI_NUM_TYPES; i++) {
  //              replaceEmojiText(hiddenVersion, shownText, emojiSymbolArray[i], "   ", emojiCodeArray[i]) || hasEmoji;
  //            }
  //
  //            int lineCount = 0;
  //            Serial.println(hiddenVersion);
  //            String hiddenTextTemp = String(hiddenVersion);
  //            String visibleTextTemp = String(shownText);
  //            bool keepChecking = true;
  //            while (keepChecking) {
  //              int endIndex = hiddenTextTemp.indexOf("^^");
  //              String tempHidden;
  //              String tempShown;
  //              if (endIndex > 0) {
  //                tempHidden = hiddenTextTemp.substring(0, endIndex);
  //                tempShown = visibleTextTemp.substring(0, endIndex);
  //
  //                hiddenTextTemp = hiddenTextTemp.substring(endIndex + 2);
  //                visibleTextTemp = visibleTextTemp.substring(endIndex + 2);
  //              } else {
  //                keepChecking = false;
  //                tempHidden = hiddenTextTemp;
  //                tempShown = visibleTextTemp;
  //              }
  //              Serial.print("tempHidden: ");
  //              Serial.println(tempHidden);
  //              int yOffset = (32 * lineCount) - 7;
  //              for (int i = 0; i < EMOJI_NUM_TYPES; i++) {
  //                parseEmoji(tempHidden, tempShown, emojiCodeArray[i], emojiImageArray[i], dma_display, yOffset);
  //              }
  //              lineCount ++;
  //
  //            }
  //
  //
  //          } else {
  //
  //            //Stripping out new line
  //            replaceEmojiText(hiddenVersion, shownText, "\n", "", "");
  //
  //            bool hasEmoji = false;
  //            for (int i = 0; i < EMOJI_NUM_TYPES; i++) {
  //              //            Serial.print(i);
  //              //            Serial.print(" ");
  //              //            Serial.println(emojiSymbolArray[i]);
  //              hasEmoji = replaceEmojiText(hiddenVersion, shownText, emojiSymbolArray[i], "   ", emojiCodeArray[i]) || hasEmoji;
  //              //            Serial.println(hiddenVersion);
  //            }
  //
  //            if (hasEmoji)
  //            {
  //              for (int i = 0; i < EMOJI_NUM_TYPES; i++) {
  //                parseEmoji(hiddenVersion, shownText, emojiCodeArray[i], emojiImageArray[i], dma_display);
  //              }
  //            }
  //
  //            screenState = sScroll;
  //            textXPosition = dma_display->width();
  //            textYPosition = dma_display->height() / 2 - (FONT_SIZE * 8 / 2);
  //            delayBetweeenAnimations = HORIZONTAL_SCROLL;
  //            dma_display->setTextSize(FONT_SIZE);
  //          }
  //        }
  //      }
  //    }
}
