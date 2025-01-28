/*
 * Like all Arduino code - copied from somewhere else :)
 * So don't claim it as your own
 * Based on the code from -> https://elektro.turanis.de/html/prj099/index.html
 * ---------------------------------
 Modified Version - 24/08 by skuydi likes kokofresko
 * adjusted matrix size from 8x8 to 10x10
 * changed matrix organization --> zigzag pattern
 * added a timer for the minimum time the player has to collect apples
 * added point tracking for scores and highscores
 * added a MAX712 Display to show the score and remaining time
 * added a boost mode and a turbo mode to allow the player to speed up the game on demand
 * added a potentiometer to adjust the display and LEDs brightness
 * added some sounds and animations
 *
 *  Arduino UNO
 *  -----------------------
 * CS D10   - CS  - Yellow
 * MOSI D11 - DIN - Brown
 * SCK D12  - CLK - Gray
 * GND GND  - GND - White
 * VCC 5V   - 5V  - Orange
 *  -----------------------
 */

//#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>
#include <Adafruit_NeoPixel.h>

#define PIN_BUTTON_UP    2  // UP PIN     - Orange
#define PIN_BUTTON_DOWN  3  // DOWN PIN   - Green
#define PIN_BUTTON_LEFT  4  // LEFT PIN   - Red
#define PIN_BUTTON_RIGHT 5  // Right PIN  - Blue

#define PIN_LED_MATRIX   6  // LED_PIN    - Yellow

#define PIN_BUTTON_TURBO 7  // TURBO_PIN  - White
#define PIN_BUTTON_BOOST 8  // BOOST_PIN  - Gray
#define BUZZER_PIN       9  // Buzzer
#define DEBOUNCE_TIME 300   // Debounce for buttons (in ms)

#define X_MAX 10
#define Y_MAX 10

#define GAME_DELAY 400      // Normal game speed (in ms)
#define BOOST_DELAY 100     // Boost game speed (in ms)
#define TURBO_DELAY 200     // Turbo game speed (in ms)
#define TIME_LIMIT 8000     // Time limit in milliseconds (8 seconds)
#define POINT_MULTIPLIER 30 // Score sensitivity

#define LED_TYPE_SNAKE 1
#define LED_TYPE_OFF   2
#define LED_TYPE_FOOD  3
#define LED_TYPE_BLOOD 4

#define DIRECTION_NONE  0
#define DIRECTION_UP    1
#define DIRECTION_DOWN  2
#define DIRECTION_LEFT  3
#define DIRECTION_RIGHT 4

int GAME_MODE_BOOST = 0;
int GAME_MODE_TURBO = 0;

#define GAME_STATE_RUNNING 1
#define GAME_STATE_END     2
#define GAME_STATE_INIT    3

#define MAX_TAIL_LENGTH X_MAX * Y_MAX
#define MIN_TAIL_LENGTH 2

struct Coords {
  int x;
  int y;
};

int pinCS = 10; // CS pin for SPI communication
int numberOfHorizontalDisplays = 4;
int numberOfVerticalDisplays = 1;
int wait = 25; // Delay time between displays
int spacer = 1;
int width = 5 + spacer; // The width of each character (5 pixels + spacing)

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(X_MAX*Y_MAX, PIN_LED_MATRIX, NEO_RGB + NEO_KHZ800);
Max72xxPanel matrixTime = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);
byte incomingByte = 0;
byte userDirection;
byte gameState;
Coords head;
Coords tail[MAX_TAIL_LENGTH];
Coords food;
unsigned long lastDrawUpdate = 0;
unsigned long lastButtonClick;
unsigned int wormLength = 0;
unsigned long timeRemaining; 
unsigned long foodTimer = 0;   // Timer for the food

// Add a variable to store the start time of the timer
unsigned long foodStartTime = 0;

// Add variables to store the score and the high score
int score;
int highScore;
bool scoreBattu;

int potValue;
int apple;

void setup()
{
  Serial.begin(9600);
  pixels.begin();
  pinMode(PIN_BUTTON_UP, INPUT_PULLUP);
  pinMode(PIN_BUTTON_DOWN, INPUT_PULLUP);
  pinMode(PIN_BUTTON_LEFT, INPUT_PULLUP);
  pinMode(PIN_BUTTON_RIGHT, INPUT_PULLUP);
  pinMode(PIN_BUTTON_TURBO, INPUT_PULLUP);
  pinMode(PIN_BUTTON_BOOST, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  // Configuration of the matrix positions
  matrixTime.setPosition(0, 0, 0);
  matrixTime.setPosition(1, 1, 0);
  matrixTime.setPosition(2, 2, 0);
  matrixTime.setPosition(3, 3, 0);

  // Rotation of each matrix for correct orientation
  matrixTime.setRotation(0, 1);
  matrixTime.setRotation(1, 1);
  matrixTime.setRotation(2, 1);
  matrixTime.setRotation(3, 1);
  
  // Adjust the brightness
  int potValue = analogRead(A0);                          // Read the value from the potentiometer
  int brightnessPixel = map(potValue, 0, 1023, 25, 150);  // Convert the value to brightness
  int brightnessMatrix = map(potValue, 0, 1023, 1, 10);   // Convert the value to brightness
  pixels.setBrightness(brightnessPixel);                  // Adjust the brightness 0-255
  matrixTime.setIntensity(brightnessMatrix);              // Adjust the brightness 0-15
  delay(10);                                              // Small pause to avoid oscillations

  resetLEDs();
  gameState = GAME_STATE_END;
  
  animation16();
  matrixTime.fillScreen(LOW);
  matrixTime.write();
}

void loop() {
  switch(gameState)
  {
    case GAME_STATE_INIT:
      initGame();
      break;
    case GAME_STATE_RUNNING:
      checkButtonPressed();
      updateGame();
      break;
    case GAME_STATE_END:
      checkButtonPressed();
      break;
  }
}

void resetLEDs()
{
  for(int i=0; i<X_MAX*Y_MAX; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
  }
  pixels.show();
}

void initGame()
{
    // Adjust the brightness
  int potValue = analogRead(A0);                          // Read the value from the potentiometer
  int brightnessPixel = map(potValue, 0, 1023, 25, 150);  // Convert the value to brightness
  int brightnessMatrix = map(potValue, 0, 1023, 1, 10);   // Convert the value to brightness
  pixels.setBrightness(brightnessPixel);                  // Adjust the brightness 0-255
  matrixTime.setIntensity(brightnessMatrix);              // Adjust the brightness 0-15
  delay(10);  
  
  Serial.println("Start GAME");
  apple = -1;
  score = -1;
  resetLEDs();
  head.x = 0;
  head.y = 0;
  food.x = -1;
  food.y = -1;
  wormLength = MIN_TAIL_LENGTH;
  userDirection = DIRECTION_LEFT;
  lastButtonClick = millis();

  for(int i=0; i<MAX_TAIL_LENGTH; i++) {
    tail[i].x = -1;
    tail[i].y = -1;
  }
  updateFood();
  gameState = GAME_STATE_RUNNING;
}

void updateGame()
{ // Game mode control (speed)
  int currentDelay = GAME_DELAY;  // Default use of normal speed

  if (GAME_MODE_TURBO == 1) {
    currentDelay = TURBO_DELAY;  // If the turbo button is pressed, use turbo speed
  } else if (GAME_MODE_BOOST == 1) {
    currentDelay = BOOST_DELAY;  // If the turbo button is pressed, use boost speed
  }
    
  if ((millis() - lastDrawUpdate) > currentDelay) {
    toggleLed(tail[wormLength-1].x, tail[wormLength-1].y, LED_TYPE_OFF);
    switch(userDirection) {
      case DIRECTION_RIGHT:
        if (head.x > 0) {
          head.x--;
        }
        break;
      case DIRECTION_LEFT:
        if (head.x < X_MAX-1) {
          head.x++;
        }
        break;
      case DIRECTION_DOWN:
        if (head.y > 0) {
          head.y--;
        }
        break;
      case DIRECTION_UP:
        if (head.y < Y_MAX-1) {
          head.y++;
        }
        break;
    }

    // Calculate the remaining time
    timeRemaining = TIME_LIMIT - (millis() - foodStartTime);
    showTime();
    
    // Display the remaining time on the serial console
    // Serial.print("Remaining time: ");
    // Serial.print(timeRemaining);  // Convert to seconds
    // Serial.println("s");

    // Check if the time to catch the food has expired
    if (millis() - foodStartTime > TIME_LIMIT) {
      Serial.println("Time's up");
      endGame(); // The player has lost because they did not eat the food in time
    return;
    }
        
    if (isCollision() == true) {
      endGame();
      return;
    }

    updateTail();

    if (head.x == food.x && head.y == food.y) {
      // Calculate the elapsed time
      unsigned long lastTimeElapsed; 
      unsigned long timeElapsed = millis() - foodTimer -lastTimeElapsed ;
      // Calculate points based on speed
      int pointsEarned = timeRemaining / POINT_MULTIPLIER;

      Serial.print("timeRemaining : ");
      Serial.print(timeRemaining);
      Serial.print(" - ");
      Serial.print("ApplesEated : ");
      Serial.print(apple);
      Serial.print(" - ");
      Serial.print("PointsEarned : ");
      Serial.print(pointsEarned);
      Serial.print(" - ");
      Serial.print("Score : ");
      Serial.println(score);
      score += max(pointsEarned, 1);  // Ensure that at least 1 point is added

      if (wormLength < MAX_TAIL_LENGTH) {
        wormLength++;
      }
      updateFood();
      foodStartTime = millis();  // Reset the timer
    }

    lastDrawUpdate = millis();
    pixels.show();
  }
}

void endGame()
{
  if (score > highScore) {
    highScore = score;
    scoreBattu = true;
  }
  toggleLed(head.x, head.y, LED_TYPE_BLOOD);
  pixels.show();
  Serial.println("GAME OVER");
  Serial.print("Score : ");
  Serial.println(score);
  Serial.print("High score : ");
  Serial.println(highScore);
  delay(2000);
  animation4();
  showApplesEated(); 
  delay(3000);
  showScore();
  delay(3000);
  showHighScore();
  if (scoreBattu) {   // Display the fireworks animation if the high score has been beaten.
    animation17();  
  }
  else {
    //animation2();  
    petitSifflet();
  }
  
  delay(500);
  matrixTime.fillScreen(LOW);
  matrixTime.write();
  scoreBattu = false;
  gameState = GAME_STATE_END;
}

void updateTail()
{
  for(int i=wormLength-1; i>0; i--) {
    tail[i].x = tail[i-1].x;
    tail[i].y = tail[i-1].y;
  }
  tail[0].x = head.x;
  tail[0].y = head.y;

  for(int i=0; i<wormLength; i++) {
    if (tail[i].x > -1) {
      toggleLed(tail[i].x, tail[i].y, LED_TYPE_SNAKE);
    }
  }
}

void updateFood()
{ apple = apple + 1;
  bool found = true;
  do {
    found = true;
    food.x = random(0, X_MAX);
    food.y = random(0, Y_MAX);
    for(int i=0; i<wormLength; i++) {
      if (tail[i].x == food.x && tail[i].y == food.y) {
         found = false;
      }
    }
  } while(found == false);
  foodStartTime = millis();  // Start the timer for this new food.
  toggleLed(food.x, food.y, LED_TYPE_FOOD);
}

bool isCollision()
{
  if (head.x < 0 || head.x >= X_MAX) {
    return true;
  }
  if (head.y < 0 || head.y >= Y_MAX) {
    return true;
  }
  for(int i=1; i<wormLength; i++) {
    if (tail[i].x == head.x && tail[i].y == head.y) {
       return true;
    }
  }
  return false;
}

void checkButtonPressed() {
    if (millis() - lastButtonClick < DEBOUNCE_TIME) {
        return;
    }
    if ((digitalRead(PIN_BUTTON_TURBO) == HIGH) && (digitalRead(PIN_BUTTON_BOOST) == LOW)) {
      if (gameState == GAME_STATE_RUNNING) {
        GAME_MODE_TURBO = 1;
        GAME_MODE_BOOST = 0;
      }
      lastButtonClick = millis();
    } 
    else if ((digitalRead(PIN_BUTTON_TURBO) == LOW) && (digitalRead(PIN_BUTTON_BOOST) == HIGH)) {
      if (gameState == GAME_STATE_RUNNING) {
        GAME_MODE_TURBO = 0;
        GAME_MODE_BOOST = 1;
      }
      lastButtonClick = millis();
    } 
    else {
      GAME_MODE_TURBO = 0;
      GAME_MODE_BOOST = 0;
    }
    if (digitalRead(PIN_BUTTON_UP) == LOW) {
        if (gameState == GAME_STATE_RUNNING) {
            userDirection = DIRECTION_UP;
        }
        lastButtonClick = millis();
    } 
    else if (digitalRead(PIN_BUTTON_DOWN) == LOW) {
        if (gameState == GAME_STATE_RUNNING) {
            userDirection = DIRECTION_DOWN;
        }
        lastButtonClick = millis();
    } 
    else if (digitalRead(PIN_BUTTON_RIGHT) == LOW) {
        if (gameState == GAME_STATE_RUNNING) {
            userDirection = DIRECTION_RIGHT;
        }
        lastButtonClick = millis();
    } 
    else if (digitalRead(PIN_BUTTON_LEFT) == LOW) {
        if (gameState == GAME_STATE_RUNNING) {
            userDirection = DIRECTION_LEFT;
        } 
        else if (gameState == GAME_STATE_END) {
            gameState = GAME_STATE_INIT;
        }
        lastButtonClick = millis();
    }
}

void toggleLed(int x, int y, byte type)
{
  int ledIndex = y * X_MAX + x;
  uint32_t color;

  // If the line is odd, reverse the X index (zigzag matrix) - Remove the if to disable zigzag
  if (y % 2 == 0) {
    ledIndex = y * X_MAX + x;
  } else {
    ledIndex = y * X_MAX + (X_MAX - 1 - x);
  }
 
  switch(type) {
    case LED_TYPE_SNAKE:
      color = pixels.Color(255, 50, 0);
      break;
    case LED_TYPE_OFF:
      color = pixels.Color(0, 0, 0);
      break;
    case LED_TYPE_FOOD:
      color = pixels.Color(0, 15, 0);
      break;
    case LED_TYPE_BLOOD:
      color = pixels.Color(15, 0, 0);
      break;
  }
  pixels.setPixelColor(ledIndex, color);
}

// Display the remaining time on the MAX7219
void showTime() {
  // Convert the score to a 4-character string
  String scoreStr = String(timeRemaining);
  
// Add leading zeros if the score is less than 1000
  while (scoreStr.length() < 4) {
    scoreStr = "0" + scoreStr;
  }

  matrixTime.fillScreen(LOW); // Clear the matrix

  // Display each digit on a matrix
  for (int i = 0; i < 4; i++) {
    matrixTime.drawChar(i * 8, 0, scoreStr[i], HIGH, LOW, 1);
  }

  matrixTime.write(); // Send the image to the matrices
}

// Display the number of apples eaten on the MAX7219 - (no scroll)
void showApplesEated() {
  // Convert the score to a 4-character string
  String appleStr = String(apple); //score
  
// Add leading zeros if the score is less than 1000
  while (appleStr.length() < 4) {
    appleStr = "0" + appleStr;
  }

  matrixTime.fillScreen(LOW); // Clear the matrix

  // Display each digit on a matrix
  for (int i = 0; i < 4; i++) {
    matrixTime.drawChar(i * 8, 0, appleStr[i], HIGH, LOW, 1);
  }

  matrixTime.write(); // Send the image to the matrice
}


// Display the score on the MAX7219 - (no scroll)
void showScore() {
  // Convert the score to a 4-character string
  String scoreStr = String(score); //score
  
// Add leading zeros if the score is less than 1000
  while (scoreStr.length() < 4) {
    scoreStr = "0" + scoreStr;
  }

  matrixTime.fillScreen(LOW); // Clear the matrix

  // Display each digit on a matrix
  for (int i = 0; i < 4; i++) {
    matrixTime.drawChar(i * 8, 0, scoreStr[i], HIGH, LOW, 1);
  }

  matrixTime.write(); // Send the image to the matrice
}


// Display the high score on the MAX7219 - (scroll)
void showHighScore() {
// Format the highScore with leading zeros if necessary
  char formattedScore[10];
  sprintf(formattedScore, "%04d", highScore); // Format the score with 4 digits, adding leading zeros if necessary

  String text = "hiScore:" + String(formattedScore); //highScore

  int textLength = text.length() * width; // Calculate the total length of the text

  for (int i = 0; i < textLength + matrixTime.width() - 1; i++) {
    matrixTime.fillScreen(LOW);

    int letter = i / width;
    int x = (matrixTime.width() - 1) - i % width;
    int y = (matrixTime.height() - 8) / 2; // Center the text vertically

    while (x + width - spacer >= 0 && letter >= 0) {
      if (letter < text.length()) {
        matrixTime.drawChar(x, y, text[letter], HIGH, LOW, 1);
      }
      letter--;
      x -= width;
    }
    matrixTime.write(); // Send the image to the matrice
    delay(wait);
  }
}

// Display a funny message - (scroll)
void petitSifflet() {
  String text = "...petit-e sifflet-te...";

  int textLength = text.length() * width; // Calculate the total length of the text

  for (int i = 0; i < textLength + matrixTime.width() - 1; i++) {
    matrixTime.fillScreen(LOW);

    int letter = i / width;
    int x = (matrixTime.width() - 1) - i % width;
    int y = (matrixTime.height() - 8) / 2; // Center the text vertically

    while (x + width - spacer >= 0 && letter >= 0) {
      if (letter < text.length()) {
        matrixTime.drawChar(x, y, text[letter], HIGH, LOW, 1);
      }
      letter--;
      x -= width;
    }
    matrixTime.write(); // Send the image to the matrice
    delay(5);
  }
}


// Pixel movement animation
void animation1(){
  for (int x = 0; x < matrixTime.width(); x++) {
    matrixTime.fillScreen(LOW);
    matrixTime.drawPixel(x % matrixTime.width(), random(matrixTime.height()), HIGH);
    matrixTime.write();
     delay(100);
  }
}

// Random pattern animation
void animation2(){
  for (int i = 0; i < 10; i++) {
  matrixTime.fillScreen(LOW);
    for (int j = 0; j < 10; j++) {
      matrixTime.drawPixel(random(matrixTime.width()), random(matrixTime.height()), HIGH);
    }
    matrixTime.write();
    delay(300);
  }
}

// Vertical sweep animation
void animation3(){
  for (int y = 0; y < matrixTime.height(); y++) {
    matrixTime.fillScreen(LOW);
    for (int x = 0; x < matrixTime.width(); x++) {
      matrixTime.drawPixel(x, y, HIGH);
    }
    matrixTime.write();
    delay(100);
  }
}

// Spiral animation
void animation4(){
  int xmin = 0, ymin = 0;
  int xmax = matrixTime.width() - 1;
  int ymax = matrixTime.height() - 1;
  while (xmin <= xmax && ymin <= ymax) {
    for (int x = xmin; x <= xmax; x++) {
      matrixTime.drawPixel(x, ymin, HIGH);
    }
    for (int y = ymin + 1; y <= ymax; y++) {
      matrixTime.drawPixel(xmax, y, HIGH);

    }
    for (int x = xmax - 1; x >= xmin; x--) {
      matrixTime.drawPixel(x, ymax, HIGH);
    }
    for (int y = ymax - 1; y > ymin; y--) {
      matrixTime.drawPixel(xmin, y, HIGH);
    }
    xmin++;
    ymin++;
    xmax--;
    ymax--;
    matrixTime.write();
    delay(500);
      }
}
 
// Wave animation
void animation5(){
  for (int x = 0; x < matrixTime.width(); x++) {
    matrixTime.fillScreen(LOW);

    int yOffset = 3 * sin(x * 0.5);  // Calcul de l'offset pour créer une vague

    for (int y = 0; y < matrixTime.height(); y++) {
      matrixTime.drawPixel(x, (y + yOffset) % matrixTime.height(), HIGH);
    }
    matrixTime.write();
    delay(100);
    }
}

// Increasing diagonal animation
void animation6(){
  for (int i = 0; i < matrixTime.width() + matrixTime.height(); i++) {
    matrixTime.fillScreen(LOW);
    for (int x = 0; x < matrixTime.width(); x++) {
      int y = i - x;
      if (y >= 0 && y < matrixTime.height()) {
        matrixTime.drawPixel(x, y, HIGH);
      }
    }
    matrixTime.write();
    delay(100);
  }
}

// Fast blinking animation
void animation7(){
  for (int i = 0; i < 20; i++) {
    matrixTime.fillScreen(i % 2 == 0 ? HIGH : LOW);  // Alterne entre ON et OFF
    matrixTime.write();
    delay(200);
  }
}

// Cross animation
void animation8(){
  for (int i = 0; i < matrixTime.width(); i++) {
    matrixTime.fillScreen(LOW);
    for (int j = 0; j < matrixTime.height(); j++) {
      if (i == j || i + j == matrixTime.width() - 1) {
        matrixTime.drawPixel(i, j, HIGH);  // Diagonales formant une croix
      }
    }
    matrixTime.write();
    delay(100);
  }
}

// Rising column animation
void animation9(){
  for (int y = matrixTime.height() - 1; y >= 0; y--) {
    matrixTime.fillScreen(LOW);
    for (int x = 0; x < matrixTime.width(); x++) {
      matrixTime.drawPixel(x, y, HIGH);
    }
    matrixTime.write();
    delay(100);
  }
}

// Inverted spiral animation
void animation10(){
  int xmin = 0, ymin = 0;
  int xmax = matrixTime.width() - 1;
  int ymax = matrixTime.height() - 1;
  while (xmin <= xmax && ymin <= ymax) {
    for (int y = ymax; y >= ymin; y--) {
      matrixTime.drawPixel(xmin, y, HIGH);
    }
    for (int x = xmin + 1; x <= xmax; x++) {
      matrixTime.drawPixel(x, ymin, HIGH);
    }
    for (int y = ymin + 1; y <= ymax; y++) {
      matrixTime.drawPixel(xmax, y, HIGH);
    }
    for (int x = xmax - 1; x > xmin; x--) {
      matrixTime.drawPixel(x, ymax, HIGH);
    }
    xmin++;
    ymin++;
    xmax--;
    ymax--;
    matrixTime.write();
    delay(500);
  }
}

// Random pixels animation with fading
void animation11(){
  for (int i = 0; i < 20; i++) {
    matrixTime.fillScreen(LOW);
    for (int j = 0; j < 15; j++) {
      matrixTime.drawPixel(random(matrixTime.width()), random(matrixTime.height()), HIGH);
    }
    matrixTime.write();
    delay(100);
  }
  for (int i = 0; i < 5; i++) {
    matrixTime.fillScreen(LOW);
    matrixTime.write();
    delay(200);
  }
}

// Scrolling line animation
void animation12(){
  for (int x = 0; x < matrixTime.width(); x++) {
    matrixTime.fillScreen(LOW);
    for (int y = 0; y < matrixTime.height(); y++) {
      matrixTime.drawPixel((x + y) % matrixTime.width(), y, HIGH);
    }
    matrixTime.write();
    delay(150);
  }
}

// Checkerboard animation
void animation13(){
  for (int i = 0; i < 10; i++) {
    matrixTime.fillScreen(LOW);
    for (int x = 0; x < matrixTime.width(); x++) {
      for (int y = 0; y < matrixTime.height(); y++) {
        if ((x + y + i) % 2 == 0) {
          matrixTime.drawPixel(x, y, HIGH);
        }
      }
    }
    matrixTime.write();
    delay(300);
  }
}

// Water drop animation
void animation14(){
  int centerX = matrixTime.width() / 2;
  int centerY = matrixTime.height() / 2;
  for (int radius = 0; radius <= matrixTime.width() / 2; radius++) {
    matrixTime.fillScreen(LOW);
    for (int angle = 0; angle < 360; angle += 15) {
      int x = centerX + radius * cos(radians(angle));
      int y = centerY + radius * sin(radians(angle));
      if (x >= 0 && x < matrixTime.width() && y >= 0 && y < matrixTime.height()) {
        matrixTime.drawPixel(x, y, HIGH);
      }
    }
    matrixTime.write();
    delay(200);
  }
}

// Horizontal ping-pong animation
void animation15(){
  int x = 0;
  bool direction = true;
  for (int i = 0; i < matrixTime.width() * 2; i++) {
    matrixTime.fillScreen(LOW);
    matrixTime.drawPixel(x, matrixTime.height() / 2, HIGH);
    matrixTime.write();
    delay(100);
    direction ? x++ : x--;
    if (x == matrixTime.width() || x < 0) {
      direction = !direction;
      x = direction ? 0 : matrixTime.width() - 1;
    }
  }
}

// Scrolling snake animation from left to right
void animation16(){
int snakeHeight = 8;  // Height of the matrix
int snakeWidth = 8;   // Width of the matrix

// Representation of the snake as a matrix (1 = pixel on, 0 = pixel off)

  int snake[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 1, 1},
    {0, 1, 0, 1, 0, 0, 1, 0},
    {0, 1, 1, 0, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}

  };

  for (int x = -snakeWidth; x < matrixTime.width(); x++) {
    matrixTime.fillScreen(LOW);
    
// Display the arrow
    for (int i = 0; i < snakeHeight; i++) {
      for (int j = 0; j < snakeWidth; j++) {
        int drawX = x + j;
        int drawY = (matrixTime.height() / 2 - snakeHeight / 2) + i;
        
// Draw the pixel only if it is within the matrix
        if (drawX >= 0 && drawX < matrixTime.width() && snake[i][j] == 1) {
          matrixTime.drawPixel(drawX, drawY, HIGH);
        }
      }
    }
    
    matrixTime.write();
    delay(150);
  }
}

// Fireworks and explosion animation
void animation17() {
    int numFireworks = 4;  // Number of fireworks
    int centerX[numFireworks], centerY[numFireworks];
    
// Generate random positions for each firework
    for (int i = 0; i < numFireworks; i++) {
        centerX[i] = random(matrixTime.width());
        centerY[i] = random(matrixTime.height() / 2);  // Limited to the upper half for the rising effect
    }

    // Phase 1 : Montée du feu d'artifice
    for (int y = matrixTime.height() - 1; y >= 0; y--) {
        matrixTime.fillScreen(LOW);

        for (int i = 0; i < numFireworks; i++) {
            if (y >= centerY[i]) {
                matrixTime.drawPixel(centerX[i], y, HIGH);  // Display the rising pixels
            }
        }
        matrixTime.write();
        delay(100);
    }

    // Phase 2 : Explosion
    int maxRadius = max(matrixTime.width(), matrixTime.height()) / 2;
    for (int radius = 1; radius < maxRadius; radius++) {
        matrixTime.fillScreen(LOW);

        for (int i = 0; i < numFireworks; i++) {
            // Move particles in a circle around the center of each firework
            for (int angle = 0; angle < 360; angle += 30) {
                int x = centerX[i] + radius * cos(radians(angle));
                int y = centerY[i] + radius * sin(radians(angle));
                
                if (x >= 0 && x < matrixTime.width() && y >= 0 && y < matrixTime.height()) {
                    matrixTime.drawPixel(x, y, HIGH);
                }
            }
        }
        matrixTime.write();
        delay(100);
    }
}

void playFireworkSound() {
// Rising phase: Frequency gradually increasing
    for (int i = 500; i < 2000; i += 10) {
        tone(BUZZER_PIN, i, 30);
        delay(10);
    }

// Break before the explosion
    delay(100);

// Explosion phase: Burst of rapid and random sounds
    for (int i = 0; i < 5; i++) {
        int freq = random(1000, 3000);
        tone(BUZZER_PIN, freq, 50);
        delay(100);
    }

// Dissipation phase: Frequency gradually decreasing
    for (int i = 2000; i > 500; i -= 10) {
        tone(BUZZER_PIN, i, 30);
        delay(20);
    }
}
