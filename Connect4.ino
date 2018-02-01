#include <Adafruit_FONA.h>

#include <LinkedList.h>	// https://github.com/ivanseidel/LinkedList

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#ifndef PSTR
#define PSTR // Make Arduino Due happy
#endif

#define MATRIX_PIN 6
#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4


// this is a large buffer for replies
char replybuffer[255];
#include <SoftwareSerial.h>
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);


// Convenient way to hold data
struct Point {
  byte x;
  byte y;
};

// Init the matrix with size and shape arguments
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(7, 6, MATRIX_PIN,
                            NEO_MATRIX_BOTTOM  + NEO_MATRIX_LEFT +
                            NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
                            NEO_GRB         + NEO_KHZ800);



void setup() {
  matrix.begin();	// Get the matrix going
  matrix.setBrightness(16);	// Make it not blinding
  redraw();

  Serial.begin(115200);
  Serial.println(F("FONA SMS caller ID test"));
  Serial.println(F("Initializing....(May take 3 seconds)"));

  // make it slow so its easy to read!
  fonaSerial->begin(4800);
  if (! fona.begin(*fonaSerial)) {
    Serial.println(F("Couldn't find FONA"));
    while (1);
  }
  Serial.println(F("FONA is OK"));

  // Print SIM card IMEI number.
  char imei[16] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = fona.getIMEI(imei);
  if (imeiLen > 0) {
    Serial.print("SIM card IMEI: "); Serial.println(imei);
  }

  fonaSerial->print("AT+CNMI=2,1\r\n");  //set up the FONA to send a +CMTI notification when an SMS is received

  Serial.println("FONA Ready");

}

char fonaNotificationBuffer[64];          //for notifications from the FONA
char smsBuffer[250];
int incomingByte = 0;   // for incoming serial data
byte turn = 0;	// Who's turn it is, either 0 or 1
byte board[7][6];	// Init the board (top left of board is 0, 0)
boolean showed = false;
void loop() {

  if (!showed) {
    showed = true;
  }

  char* bufPtr = fonaNotificationBuffer;    //handy buffer pointer

  if (fona.available())      //any data available from the FONA?
  {
    int slot = 0;            //this will be the slot number of the SMS
    int charCount = 0;
    //Read the notification into fonaInBuffer
    do  {
      *bufPtr = fona.read();
      Serial.write(*bufPtr);
      delay(1);
    } while ((*bufPtr++ != '\n') && (fona.available()) && (++charCount < (sizeof(fonaNotificationBuffer) - 1)));

    //Add a terminal NULL to the notification string
    *bufPtr = 0;

    //Scan the notification string for an SMS received notification.
    //  If it's an SMS message, we'll get the slot number in 'slot'
    if (1 == sscanf(fonaNotificationBuffer, "+CMTI: " FONA_PREF_SMS_STORAGE ",%d", &slot)) {
      Serial.print("slot: "); Serial.println(slot);

      char callerIDbuffer[32];  //we'll store the SMS sender number in here

      // Retrieve SMS sender address/phone number.
      if (! fona.getSMSSender(slot, callerIDbuffer, 31)) {
        Serial.println("Didn't find SMS message in slot!");
      }
      Serial.print(F("FROM: ")); Serial.println(callerIDbuffer);

      // Retrieve SMS value.
      uint16_t smslen;
      if (fona.readSMS(slot, smsBuffer, 250, &smslen)) { // pass in buffer and max len!
        char selection = smsBuffer[0];
        byte selectionByte = 10;
        switch (selection) {
          case 'P': selectionByte = 0; break;
          case 'p': selectionByte = 0; break;
          case 'H': selectionByte = 1; break;
          case 'h': selectionByte = 1; break;
          case 'Y': selectionByte = 2; break;
          case 'y': selectionByte = 2; break;
          case 'S': selectionByte = 3; break;
          case 's': selectionByte = 3; break;
          case 'I': selectionByte = 4; break;
          case 'i': selectionByte = 4; break;
          case 'C': selectionByte = 5; break;
          case 'c': selectionByte = 5; break;
          case 'Z': selectionByte = 6; break;
          case 'z': selectionByte = 6; break;
          default: break;
        }

        if (selectionByte != 10) {
          if (runTurn(turn, selectionByte)) {
            turn = turn == 1 ? 0 : 1;
            showed = false;
          }
        }
        Serial.println(smsBuffer);
      }


      // delete the original msg after it is processed
      //   otherwise, we will fill up all the slots
      //   and then we won't be able to receive SMS anymore
      if (fona.deleteSMS(slot)) {
        Serial.println(F("OK!"));
      } else {
        Serial.print(F("Couldn't delete SMS in slot ")); Serial.println(slot);
        fona.print(F("AT+CMGD=?\r\n"));
      }
    }
  }
}

boolean runTurn(byte turn, byte pos) {
  if (board[pos][0]) {	// If the stack was full, return false
    return false;
  }

  byte coord = 0;
  // Loop from top to bottom of the board, this is to animate the token falling
  for (int i = 0; i < 6; i++) {
    if (i != 0) {	// If this is not the top of the board, clear the previous slot
      board[pos][i - 1] = 0;
    }
    board[pos][i] = turn + 1; // Set the current position to the correct players token (0 is empty, 1 is p1, 2 is p2)
    if (board[pos][i + 1] > 0 || i == 5) {	// If the token hits the top of the stack or the bottom of the board, stop
      coord = i;
      //break; // breaking ironicially breaks the arduino, don't use it
      i = 10;	// This is sort of a break
    }
    redraw();
    delay(50);
  }
  checkWin(turn + 1, (Point) {
    pos, coord
  });	// If the new token causes a win then this function resets the board
  return true;
}

void checkWin(byte player, Point pos) {
  LinkedList<Point> winList;
  LinkedList<Point> tempWinList;

  // Check for horizontal wins
  byte counter = 0;
  for (int i = 0; i < 8; i++) {
    if (board[i][pos.y] == player && i < 7) {
      counter++;
      tempWinList.add((Point) {
        i, pos.y
      });
    } else {
      if (counter >= 4) {
        i = 9;
        while (tempWinList.size() > 0) {
          winList.add(tempWinList.shift());
        }
      }
      tempWinList.clear();
      counter = 0;
    }
  }

  // Check for vertical wins
  counter = 0;
  for (int i = 0; i < 7; i++) {
    if (board[pos.x][i] == player && i < 6) {
      counter++;
      tempWinList.add((Point) {
        pos.x, i
      });
    } else {
      if (counter >= 4) {
        i = 9;
        while (tempWinList.size() > 0) {
          winList.add(tempWinList.shift());
        }
      }
      tempWinList.clear();
      counter = 0;
    }
  }

  // Check for -1 slope wins
  counter = 0;
  Point startPos = {pos.x - min(pos.x, pos.y), pos.y - min(pos.x, pos.y)};
  for (int i = 0; i < 7; i++) {
    if (board[startPos.x + i][startPos.y + i] == player && (startPos.x + i) < 8 && (startPos.x + i) < 7) {
      counter++;
      tempWinList.add((Point) {
        startPos.x + i, startPos.y + i
      });
    } else {
      if (counter >= 4) {
        i = 9;
        while (tempWinList.size() > 0) {
          winList.add(tempWinList.shift());
        }
      }
      tempWinList.clear();
      counter = 0;
    }
  }

  // Check for +1 slope wins
  counter = 0;
  startPos = {pos.x - min(pos.x, 7 - pos.y), pos.y + min(pos.x, 7 - pos.y)};
  for (int i = 0; i < 7; i++) {
    if (board[startPos.x + i][startPos.y - i] == player && (startPos.x + i) < 8 && (startPos.x - i) < 7) {
      counter++;
      tempWinList.add((Point) {
        startPos.x + i, startPos.y - i
      });
    } else {
      if (counter >= 4) {
        i = 9;
        while (tempWinList.size() > 0) {
          winList.add(tempWinList.shift());
        }
      }
      tempWinList.clear();
      counter = 0;
    }
  }

  // If the player wins, display win and reset the board
  if (winList.size() > 0) {
    for (int i = 0; i < 30; i++) {
      for (int j = 0; j < winList.size(); j++) {
        Point curPoint = winList.get(j);
        if (i % 2 == 0) {
          board[curPoint.x][curPoint.y] = 3;
        } else {
          board[curPoint.x][curPoint.y] = player;
        }
      }
      redraw();
      delay(40);
    }
    for (int x = 0; x < 7; x++) {
      for (int y = 0; y < 6; y++) {
        board[x][y] = 0;
      }
    }
    turn = 0;
    redraw();
  }
}

// Converts the byte board into an image and draws it
void redraw() {
  int16_t tempColor = matrix.Color(0, 0, 0);
  matrix.fillScreen(tempColor);
  for (int y = 0; y < 6; y++) {
    for (int x = 0; x < 7; x++) {
      if (board[x][y] == 0) {
        tempColor = matrix.Color(0, 0, 0);
      } else if (board[x][y] == 1) {
        tempColor = matrix.Color(255, 0, 0);
      } else if (board[x][y] == 2) {
        tempColor = matrix.Color(0, 0, 255);
      } else if (board[x][y] == 3) {
        tempColor = matrix.Color(255, 255, 0);
      }
      matrix.drawPixel(x, y, tempColor);
    }
  }
  matrix.show();
}
