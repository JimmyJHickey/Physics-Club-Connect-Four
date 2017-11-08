#include <Adafruit_FONA.h>

#include <LinkedList.h>	// https://github.com/ivanseidel/LinkedList

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#ifndef PSTR
 #define PSTR // Make Arduino Due happy
#endif

#define MATRIX_PIN 6

struct Point{
  byte x;
  byte y;
};


Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(7, 6, MATRIX_PIN,
  NEO_MATRIX_BOTTOM  + NEO_MATRIX_LEFT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB         + NEO_KHZ800);


void setup() {
  matrix.begin();
  matrix.setBrightness(16);

	Serial.begin(9600);
  redraw();
}
int incomingByte = 0;   // for incoming serial data
byte turn = 0;
byte board[7][6];
boolean showed = false;
void loop() {
  if(!showed){
    showed = true;  
  }
  
  if (Serial.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial.read();

    // say what you got:
    if(incomingByte >= '1' && incomingByte <= '7'){
      if(runTurn(turn, incomingByte - '1')){
         turn = turn == 1 ? 0 : 1;
         showed = false;
      }
    }
  }
}

boolean runTurn(byte turn, byte pos){
  if(board[pos][0]){
    return false;
  }
  byte coord = 0;
  for(int i = 0; i < 6; i++){
    if (i != 0){
      board[pos][i-1] = 0;
    }
    board[pos][i] = turn + 1;
    if(board[pos][i+1] > 0 || i == 5){
      coord = i;
      //break;
      i = 10;
    }
    redraw();
    delay(50);
  }
  checkWin(turn + 1, (Point){pos, coord});
  return true;
}

void checkWin(byte player, Point pos){
  LinkedList<Point> winList;
  LinkedList<Point> tempWinList;
  byte counter = 0;
  for(int i = 0; i < 8; i++){
    if(board[i][pos.y] == player && i < 7){
      counter++;
      tempWinList.add((Point){i, pos.y});      
    }else{
      if(counter >= 4){
        i = 9;
        while(tempWinList.size() > 0){
          winList.add(tempWinList.shift());
        }
      }
      tempWinList.clear();
      counter = 0;
    }
  }
  
  counter = 0;
  for(int i = 0; i < 7; i++){
    if(board[pos.x][i] == player && i < 6){
      counter++;
      tempWinList.add((Point){pos.x, i});      
    }else{
      if(counter >= 4){
        i = 9;
        while(tempWinList.size() > 0){
          winList.add(tempWinList.shift());
        }
      }
      tempWinList.clear();
      counter = 0;
    }
  }

  counter = 0;
  Point startPos = {pos.x - min(pos.x, pos.y), pos.y - min(pos.x, pos.y)};
  for(int i = 0; i < 7; i++){
    if(board[startPos.x + i][startPos.y + i] == player && (startPos.x + i) < 8 && (startPos.x + i) < 7){
      counter++;
      tempWinList.add((Point){startPos.x + i, startPos.y + i});      
    }else{
      if(counter >= 4){
        i = 9;
        while(tempWinList.size() > 0){
          winList.add(tempWinList.shift());
        }
      }
      tempWinList.clear();
      counter = 0;
    }
  }

  counter = 0;
  startPos = {pos.x - min(pos.x, 7 - pos.y), pos.y + min(pos.x, 7 - pos.y)};
  for(int i = 0; i < 7; i++){
    if(board[startPos.x + i][startPos.y - i] == player && (startPos.x + i) < 8 && (startPos.x - i) < 7){
      counter++;
      tempWinList.add((Point){startPos.x + i, startPos.y - i});      
    }else{
      if(counter >= 4){
        i = 9;
        while(tempWinList.size() > 0){
          winList.add(tempWinList.shift());
        }
      }
      tempWinList.clear();
      counter = 0;
    }
  }

  if(winList.size() > 0){
    for(int i = 0; i < 10; i++){
      for(int j = 0; j < winList.size(); j++){
        Point curPoint = winList.get(j);
        if(i%2==0){
          board[curPoint.x][curPoint.y] = 3;
        }else{
          board[curPoint.x][curPoint.y] = player;
        }
      }
      redraw();
      delay(40);
    }
    for(int x = 0; x < 7; x++){
      for(int y = 0; y < 6; y++){
        board[x][y] = 0;
      }  
    }
    turn = 0;
    redraw();
  }
}

void redraw(){
  int16_t tempColor = matrix.Color(0, 0, 0);
  matrix.fillScreen(tempColor);
  for(int y = 0; y < 6; y++){
    for(int x = 0; x < 7; x++){
      if(board[x][y] == 0){
        tempColor = matrix.Color(0, 0, 0);
      }else if(board[x][y] == 1){
        tempColor = matrix.Color(255, 0, 0);
      }else if(board[x][y] == 2){
        tempColor = matrix.Color(0, 0, 255);
      }else if(board[x][y] == 3){
        tempColor = matrix.Color(255, 255, 0);
      }
      matrix.drawPixel(x, y, tempColor);    
    }  
  }
  matrix.show();
}
