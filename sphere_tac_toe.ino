#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Keypad.h>

void(* resetFunc) (void) = 0;

const byte lcdAddress = 0x27,
           gyroAddress = 0x68;

const uint8_t neopixelPin = 2,
              buzzerPin = 3,
              neoPixelBrightnessPin = A6,
              activityLimitPin = A7;

const byte rowPins[] = {4, 5, 6, 7, 8}; // top -> bottom
const byte colPins[] = {9, 14, 15, 16, 17}; // left -> right

unsigned long startingTime;
unsigned long lastActivityTime = 0;

const char keys[][5] = {
  {'A', 'B', 'C', 'D', 'E'},
  {'F', 'G', 'H', 'I', 'J'},
  {'K', 'L', 'M', 'N', 'O'},
  {'P', 'Q', 'R', 'S', 'T'},
  {'U', 'V', 'W', 'X', 'Y'}
};

const byte colors[][3] = {
  {255, 0, 0}, // red
  {255, 127, 0}, // orange
  {255, 255, 0}, // yellow
  {127, 255, 0}, // lime
  {0, 255, 0}, // green
  {0, 255, 255}, // blue
  {75, 75, 255}, // blueish-purple
  {127, 0, 255}, // purple
  {255, 0, 255} // magenta
};

const byte playerColors[][3] = {
  {62, 95, 138}, // Brillant blue – player 1
  {155, 17, 30}, // Ruby red – player 2
  {78, 87, 84} // Basalt grey – tie/unknown
};

int states[][5] = {
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0}
};

bool player = 0;

int plays = 0;

LiquidCrystal_I2C lcd = LiquidCrystal_I2C(lcdAddress, 16, 2);
Keypad buttonGrid = Keypad(makeKeymap(keys), rowPins, colPins, 5, 5);
Adafruit_NeoPixel pixelGrid = Adafruit_NeoPixel(26, neopixelPin, NEO_GRB + NEO_KHZ800);

void setup() {
  pinMode(buzzerPin, OUTPUT);

  Serial.begin(9600);

  Serial.println("On/Reset");

  tone(buzzerPin, 750, 500);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(3, 0);
  lcd.print("Welcome to");
  lcd.setCursor(1, 1);
  lcd.print("SPHERE TAC TOE");

  pixelGrid.begin();

  onAnimation();

  delay(750);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Player 1's Turn!");

  setAllOnGrid(playerColors[2][0], playerColors[2][1], playerColors[2][2]);
  setIndicatorPixel(playerColors[0][0], playerColors[0][1], playerColors[0][2]);
}

void loop() {
  char key = buttonGrid.getKey();

  if (key) {
    lastActivityTime = millis();

    Serial.print("Key Pressed: ");
    Serial.print(key);
    Serial.print(" --> ");

    int rowCol[2];
    keyToRowCol(key, keys, rowCol);
    int row = rowCol[0];
    int col = rowCol[1];

    Serial.print(row);
    Serial.print(", ");
    Serial.println(col);

    if (states[row][col] == 0) {
      plays++;

      if (plays <= 1) {
        startingTime = millis();
      }

      noTone(buzzerPin);
      tone(buzzerPin, 1000, 250);

      setGridPixel(row, col, playerColors[player][0], playerColors[player][1], playerColors[player][2]);

      states[row][col] = player + 1;

      int winner = checkFourInARow(states, row, col);
      if (winner == 1) {
        announceEnding(msToString(getTime(startingTime, millis())), 0);
      } else if (winner == 2) {
        announceEnding(msToString(getTime(startingTime, millis())), 1);
      } else {
        player = !player;

        if (plays >= 25) {
          plays = 0;
          announceEnding(msToString(getTime(startingTime, millis())), 2);
        } else {
          lcd.setCursor(7, 0);
          lcd.print(String(player + 1));

          setIndicatorPixel(playerColors[player][0], playerColors[player][1], playerColors[player][2]);
        }
      }
    } else {
      noTone(buzzerPin);
      tone(buzzerPin, 500, 250);
    }

    if (plays > 0) {
      String time = msToString(getTime(startingTime, millis()));

      lcd.setCursor(0, 1);
      lcd.print(time);
    }
  }

  activityCheck();
  pixelGrid.setBrightness(map(analogRead(neoPixelBrightnessPin), 0, 1024, 0, 255));
  pixelGrid.show();

  delay(50);
}

int gridToPixel(int row, int col) {
  if (col % 2 > 0) {
    row = ((row - 2) * -1) + 2;
  }

  int pixel = row + (col * 5);

  return pixel;
}

void setIndicatorPixel (byte r, byte g, byte b) {
  pixelGrid.setPixelColor(25, pixelGrid.Color(r, g, b));
  pixelGrid.show();
}

void setGridPixel (int row, int col, byte r, byte g, byte b) {
  int pixel = gridToPixel(row, col);

  pixelGrid.setPixelColor(pixel, pixelGrid.Color(r, g, b));
  pixelGrid.show();
}

void setAllOnGrid (byte r, byte g, byte b) {
  for (int i = 0; i < 25; i++) {
    pixelGrid.setPixelColor(i, pixelGrid.Color(r, g, b));
    pixelGrid.show();
  }
}

int keyToRowCol (char key, char keys[5][5], int *rowCol) {
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 5; j++) {
      if (keys[i][j] == key) {
        rowCol[0] = i;
        rowCol[1] = j;
      }
    }
  }

  return NULL;
}

void onAnimation() {
  for (int i = 0; i < 9; i++) {
    byte color[3];
    color[0] = colors[i][0];
    color[1] = colors[i][1];
    color[2] = colors[i][2];

    setIndicatorPixel(color[0], color[1], color[2]);

    setGridPixel(i, 0, color[0], color[1], color[2]);
    setGridPixel(i - 1, 1, color[0], color[1], color[2]);
    setGridPixel(i - 2, 2, color[0], color[1], color[2]);
    setGridPixel(i - 3, 3, color[0], color[1], color[2]);
    setGridPixel(i - 4, 4, color[0], color[1], color[2]);

    delay(1000 / 36);
  }

  for (int i = 0; i < 9; i++) {
    int white = map(i, 0, 8, 255, 0);
    setIndicatorPixel(white, white, white);

    setGridPixel(i, 0, 0, 0, 0);
    setGridPixel(i - 1, 1, 0, 0, 0);
    setGridPixel(i - 2, 2, 0, 0, 0);
    setGridPixel(i - 3, 3, 0, 0, 0);
    setGridPixel(i - 4, 4, 0, 0, 0);

    delay(1000 / 36);
  }

  setIndicatorPixel(0, 0, 0);
}

void announceEnding(String time, int winner) {
  if (winner == 2) {
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("It was a tie!!");

    setIndicatorPixel(playerColors[2][0], playerColors[2][1], playerColors[2][2]);
  } else {
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Player ");
    lcd.setCursor(8, 0);
    lcd.print(String(winner + 1));
    lcd.setCursor(10, 0);
    lcd.print("Won!!");

    setIndicatorPixel(playerColors[winner][0], playerColors[winner][1], playerColors[winner][2]);
  }

  lcd.setCursor(0, 1);
  lcd.print(time);

  tone(buzzerPin, 750, 250);
  delay(500);
  tone(buzzerPin, 750, 250);
  delay(500);
  tone(buzzerPin, 750, 250);
}

unsigned long getTime(unsigned long start, unsigned long end) {
  return end - start;
}

String msToString(unsigned long time) {
  float m, s, ms;
  unsigned long over;
  String string, msString;

  m = time / 60000;
  over = time % 60000;
  s = over / 1000;
  ms = over % 1000;

  msString = String(int(ms));

  while (msString.length() < 3) {
    msString = "0" + msString;
  }

  string = String(int(m)) + ":" + String(int(s)) + "." + msString;

  string = spacePad(string);

  return string;
}

String spacePad(String in) {
  String out, FspaceString, BspaceString;
  int column;

  column = round((16 - in.length()) / 2);
  FspaceString = "";
  for (int i = 0; i < column; i++) {
    FspaceString += " ";
  }

  column = 16 - in.length();
  BspaceString = "";
  for (int i = 0; i < column; i++) {
    BspaceString += " ";
  }

  out = FspaceString + in + BspaceString;

  return out;
}

void activityCheck() {
  if (millis() - lastActivityTime > map(analogRead(activityLimitPin), 0, 1024, 5000, 90000)) {
    tone(buzzerPin, 500, 1000);
  }
}

float inRange(int num, int minimum, int maximum) {
  return num > minimum && num < maximum;
}

int checkFourInARow(int board[][5], int lastMoveRow, int lastMoveCol) {
  int i, j, count;
  int player = board[lastMoveRow][lastMoveCol];

  // check horizontal
  for (i = 0; i < 5; i++) {
    count = 0;
    for (j = 0; j < 5; j++) {
      int col = (lastMoveCol + j) % 5;  // wrap around to beginning if necessary
      if (board[i][col] == player) {
        count++;
        if (count == 4) {
          return player;
        }
      } else {
        count = 0;
      }
    }
  }

  // check vertical
  for (j = 0; j < 5; j++) {
    count = 0;
    for (i = 0; i < 5; i++) {
      int row = (lastMoveRow + i) % 5;  // wrap around to beginning if necessary
      if (board[row][j] == player) {
        count++;
        if (count == 4) {
          return player;
        }
      } else {
        count = 0;
      }
    }
  }

  // check diagonal (top-left to bottom-right)
  for (i = 0; i < 2; i++) {
    for (j = 0; j < 2; j++) {
      count = 0;
      int r = lastMoveRow - i;
      int c = lastMoveCol - j;
      while (r < 5 && c < 5) {
        if (board[r][c] == player) {
          count++;
          if (count == 4) {
            return player;
          }
        } else {
          count = 0;
        }
        r++;
        c++;
      }
    }
  }

  // check diagonal (bottom-left to top-right)
  for (i = 0; i < 2; i++) {
    for (j = 0; j < 2; j++) {
      count = 0;
      int r = lastMoveRow + i;
      int c = lastMoveCol - j;
      while (r >= 0 && c < 5) {
        if (board[r][c] == player) {
          count++;
          if (count == 4) {
            return player;
          }
        } else {
          count = 0;
        }
        r--;
        c++;
      }
    }
  }

  return 0;
}
