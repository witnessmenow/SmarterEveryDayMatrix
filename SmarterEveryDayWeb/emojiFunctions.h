#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

struct emoji
{
  int xOffset;
  int yOffset;
  const unsigned short *image;
  int emojiWidth;
  int emojiHeight;
  emoji *next;
};

SemaphoreHandle_t emojiSemaphore;

emoji *firstEmoji = NULL;
emoji *currentEmoji = NULL;

// Will be used in getTextBounds.
int16_t xOne, yOne;
uint16_t w, h;

// This function we will extract out the location of the Emoji and save it
// to the linked list
void parseEmoji(String stringToParse, String displayString, const char* emojiChar, const unsigned short *emojiImage, MatrixPanel_I2S_DMA *matrixDispaly, int yOffset = -1) {
  int emojiIndex = 0;
  int found = 1;
  //Serial.println(emojiChar);
  emojiIndex = stringToParse.indexOf(emojiChar);
  xSemaphoreTake( emojiSemaphore, portMAX_DELAY );
  while (emojiIndex >= 0) { 
    //Serial.print(found);
    //Serial.println(" Found");
    found++;
    if (currentEmoji == NULL) {
      firstEmoji = new emoji;
      currentEmoji = firstEmoji;
    } else {
      currentEmoji->next = new emoji;
      currentEmoji = currentEmoji->next;
    }

    if(strncmp(emojiChar, "%%7%%", 5) == 0){
      currentEmoji->emojiWidth = 51;
      currentEmoji->emojiHeight = 30;
    } else if(strncmp(emojiChar, "%8%", 3) == 0){
      // Mars Emoji is 32x32
      currentEmoji->emojiWidth = 32;
      currentEmoji->emojiHeight = 32;
    }
    else {
      currentEmoji->emojiWidth = 30;
      currentEmoji->emojiHeight = 30;
    }


    currentEmoji->yOffset = yOffset;
    currentEmoji->next = NULL;
    currentEmoji->image = emojiImage;

    // Finding out where the Emoji should be
    if (emojiIndex == 0) {
      currentEmoji->xOffset = 0;
    } else {
      matrixDispaly->getTextBounds(displayString.substring(0, emojiIndex), 0, 0, &xOne, &yOne, &w, &h);
      currentEmoji->xOffset = w;
    }

    emojiIndex = stringToParse.indexOf(emojiChar, emojiIndex + 1);
  }
  xSemaphoreGive( emojiSemaphore );
}

bool replaceEmojiText(String &hiddenString, String &shownString, const char* emojiChar, const char* visibleReplacement, const char* hiddenReplacement) {
  //Serial.print("replace: ");
  //Serial.println(hiddenString);
  if (hiddenString.indexOf(emojiChar) >= 0) {
    //Serial.println("found");
    shownString.replace(emojiChar, visibleReplacement);
    hiddenString.replace(emojiChar, hiddenReplacement);
    return true;
  }
  //Serial.println("not found");
  return false;
}

void destroyEmojiList() {
  xSemaphoreTake( emojiSemaphore, portMAX_DELAY );
  emoji *tempEmoji = NULL;
  currentEmoji = firstEmoji;
  while (currentEmoji != NULL) {
    tempEmoji = currentEmoji;
    currentEmoji = currentEmoji->next;
    delete tempEmoji;
  }
  firstEmoji = NULL;
  xSemaphoreGive( emojiSemaphore );
}
