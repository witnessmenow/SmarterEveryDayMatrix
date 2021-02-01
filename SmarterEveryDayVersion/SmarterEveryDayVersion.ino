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

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

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
UniversalTelegramBot bot(BOT_TOKEN, secured_client);


uint16_t myBLACK = dma_display.color565(0, 0, 0);
uint16_t myWHITE = dma_display.color565(255, 255, 255);
uint16_t myRED = dma_display.color565(255, 0, 0);
uint16_t myGREEN = dma_display.color565(0, 255, 0);
uint16_t myBLUE = dma_display.color565(0, 0, 255);

// -------------------------------------
// -------   Text Configuraiton   ------
// -------------------------------------

#define FONT_SIZE 2 // Text will be FONT_SIZE x 8 pixels tall.

int delayBetweeenAnimations = 35; // How fast it scrolls, Smaller == faster
int scrollXMove = -1; //If positive it would scroll right

int textXPosition = dma_display.width(); // Will start one pixel off screen
int textYPosition = dma_display.height() / 2 - (FONT_SIZE * 8 / 2); // This will center the text

String shownText = "Hello Smarter Every Day!"; //Starting Text (this can't have Emojiis at the moment!)
uint16_t textColour = myRED;
//------- ---------------------- ------

// For scrolling Text
unsigned long isAnimationDue;

struct emojii
{
  int xOffset;
  const unsigned short *image;
  emojii *next;
};

void setup() {

  Serial.begin(115200);

  // Display Setup
  dma_display.begin();
  dma_display.fillScreen(myBLACK);
  dma_display.showDMABuffer();
  dma_display.setTextSize(FONT_SIZE);
  dma_display.setTextWrap(false); // N.B!! Don't wrap at end of line
  dma_display.setTextColor(myRED); // Can change the colour here

  // attempt to connect to Wifi network:
  Serial.print("Connecting to Wifi SSID ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());


}


void drawEmojii(int x, int y, emojii *e)
{
  int xOffset = x + e->xOffset;
  // Check if the Emojii is on the screen
  if (xOffset > -30 && xOffset < dma_display.width())
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

// Will be used in getTextBounds.
int16_t xOne, yOne;
uint16_t w, h;
bool checkTelegram = false;
bool displayReady = false;

String hiddenVersion; //This string will be used to index the Emojiis

int emojiiIndex = 0;

emojii *firstEmojii = NULL;
emojii *currentEmojii = NULL;
emojii *tempEmojii = NULL;


// This function we will extract out the location of the Emojii and save it
// to the linked list
void parseEmojii(const char* emojiiChar, const unsigned short *emojiiImage) {
  emojiiIndex = 0;

  emojiiIndex = hiddenVersion.indexOf(emojiiChar);
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

    currentEmojii->next = NULL;
    currentEmojii->image = emojiiImage;

    // Finding out where the Emojii should be
    if (emojiiIndex == 0) {
      currentEmojii->xOffset = 0;
    } else {
      dma_display.getTextBounds(shownText.substring(0, emojiiIndex), 0, 0, &xOne, &yOne, &w, &h);
      currentEmojii->xOffset = w;
    }

    emojiiIndex = hiddenVersion.indexOf(emojiiChar, emojiiIndex + 1);
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
}


void loop() {
  if (!displayReady) {

    // Because we are using a double buffer, we don't need to wait
    // for when the animation is due before drawing it to the buffer
    // it will only get swapped onto the display when the animation is due.

    textXPosition += scrollXMove;

    // Checking if the very right of the text off screen to the left
    dma_display.getTextBounds(shownText, textXPosition, textYPosition, &xOne, &yOne, &w, &h);
    if (textXPosition + w <= 0)
    {
      checkTelegram = true;
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
  }

  unsigned long now = millis();
  if (now > isAnimationDue)
  {

    // This sets the timer for when we should scroll again.
    isAnimationDue = now + delayBetweeenAnimations;

    // This code swaps the second buffer to be visible (puts it on the display)
    dma_display.showDMABuffer();

    // This tells the code to update the second buffer
    dma_display.flipDMABuffer();

    displayReady = false;

  }

  // Telegram will only be checked when there is no data on screen
  // as checking interfeers with scrolling.
  if (checkTelegram)
  {
    checkTelegram = false;
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    if (numNewMessages > 0)
    {
      Serial.println("got response");

      // This is where you would check is the message from a valid source
      // You can get your ID from myIdBot in the Telegram client
      //if(bot.messages[0].chat_id == "175753388")
      if (true) {

        // destroy the old Emojii objects
        destroyEmojiiList();

        // Takes the contents of the message, and sets it to be displayed.
        hiddenVersion = bot.messages[0].text;
        shownText = String(hiddenVersion); // Making a second copy of the text.

        bool hasEmojii = replaceEmojiiText("🦖", "   ", "%1%"); // T-Rex
        hasEmojii = replaceEmojiiText("🦕", "   ", "%2%") || hasEmojii; // Tall Dino
        hasEmojii = replaceEmojiiText("🚀", "   ", "%3%") || hasEmojii; // Rocket
        hasEmojii = replaceEmojiiText("🌕", "   ", "%4%") || hasEmojii; // Full Moon
        hasEmojii = replaceEmojiiText("🌙", "   ", "%5%") || hasEmojii; // Crecent Moon

        if (hasEmojii)
        {
          parseEmojii("%1%", BrownDino);
          parseEmojii("%2%", TallDino);
          parseEmojii("%3%", Rocket);
          parseEmojii("%4%", Moon);
          parseEmojii("%5%", Cresent);
        }

      }
    }
  }

}
