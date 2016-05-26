/*
  This is PONG, a simple Arcade game.
  You will need a 8x8 LED matriz with a MAX7219 driver, a piezo speaker and 2 potentiometer.
  Adapt the LedControl, piezo and potentiometer pin's to your needs.
*/

#include "LedControl.h"
#include <binary.h>

/*
 * MAX7219 connections
 * pin 12 is connected to the MAX7219 pin 1
 * pin 11 is connected to the MAX7219 CLK pin 13
 * pin 10 is connected to MAX7219 LOAD pin 12
 * 1 as we are only using 1 MAX7219
*/
LedControl lc = LedControl(12,11,10,1);
/* Potentiometer for player A */
const int potA = A0;
/* Potentiometer for player B */
const int potB = A1;
/* Piezo speaker */
const int piezo = 9;


const int minPos = B00000111;
const int maxPos = B11100000;
int potALastValue = 0;
int potBLastValue = 0;
byte ballPositionX = 3; // 0 - 7
byte ballPositionY = 3; // 0 - 7
int ballDirectionX = 0; // -1 (left) ,0 (still) ,1 (right)
int ballDirectionY = 1; // -1 (up) ,0 (still) ,1 (down)
byte ballTimig = 0;
boolean gameOver = false;

/*
 --> 0-7
  O O X X X O O O O  (racketB)
  O O O O O O O O O
  O O O O O O O O O
  O O O O @ O O O O
  O O O O O O O O O
  O O O O O O O O O
  O O O O O O O O O
  O O O O X X X O O  (racketA)
*/

struct Racket {
  byte row;
  byte rowBeforeRacket;
  int position;
};

Racket racketA = {0, 1, 0};
Racket racketB = {7, 6, 0};

void setup()
{
  // the zero refers to the MAX7219 number, it is zero for 1 chip
  lc.shutdown(0,false);// turn off power saving, enables display
  lc.setIntensity(0,8);// sets brightness (0~15 possible values)
  lc.clearDisplay(0);// clear screen
  lc.setLed(0, ballPositionY, ballPositionX, 1);
  pinMode(piezo, OUTPUT); // Piezo
}

void loop()
{
  // As long as the game is not over, move the ball and the rackets
  if (!gameOver)
  {
    // Read pot values and move the racket accordingly 
    moveRacket(potA, &racketA, &potALastValue);
    moveRacket(potB, &racketB, &potBLastValue);
  
    // Update the ball only every 1s
    if (ballTimig == 5)
    {
      beep(50);
      ballTimig = 0;
      // Clear current ball location
      lc.setLed(0, ballPositionY, ballPositionX, 0);
      // Move the ball one step to the current direction
      ballPositionX = ballPositionX + ballDirectionX;
      ballPositionY = ballPositionY + ballDirectionY;
      // Racket B hit? then adjust ball direction
      if (!adjustBallDirectionIfRacketHit(&racketB))
      {
        // Racket A hit? then adjust ball direction
        if (!adjustBallDirectionIfRacketHit(&racketA))
        {
          // Ball on the same row as the racket? -> game over
          if (ballPositionY == racketA.row)
          {
            // Game over for player A
            winner(&racketB);
          }
          // Ball on the same row as the racket? -> game over
          else if (ballPositionY == racketB.row)
          {
            // Game over for player B
            winner(&racketA);
          }
          // Hit the left or right border? Then bounce back in the opposite directrion
          else if (ballPositionX == 0 && ballDirectionX == -1)
          {
            ballDirectionX = 1;
          }
          else if (ballPositionX == 7 && ballDirectionX == 1)
          {
            ballDirectionX = -1;
          }
        }
      }
      if (!gameOver)
      {
        // Show the ball in the new position
        lc.setLed(0, ballPositionY, ballPositionX, 1);
      }
    } else {
      ballTimig += 1;
    }
    
    delay(50);
  }
}

/*
 * Beeps (1kHz for the given time)
 */
void beep(unsigned char delayms){
  tone(piezo, 1000);
  delay(delayms);
  noTone(piezo);
  delay(delayms);
}  

/*
 * Show who's the winner
 */
void winner(Racket* winningRacket)
{
  gameOver = true;
  for(int i = 0; i<3; i++) {
    setAllLights(B11111111);
    beep(100);
    delay(200);
    setAllLights(B00000000);
    beep(100);
    delay(200);
  }
  lc.setRow(0, winningRacket->row, B11111111);
}

/*
 * Set LED's of a row with the given pattern
 */
void setAllLights(int lights)
{
  for(int i = 0; i<8; i++) {
    lc.setRow(0, i, lights);
  }
}

/*
 * Checks if the ball has hit the given racket and adjusts the ballDirection accordingly
 */
boolean adjustBallDirectionIfRacketHit(Racket* racket)
{
  if (racketHit(racket))
  {
    // ball has hit the racket => change moving direction
    ballDirectionY *= -1;
    // Move the ball diagonally if the ball hits the racket on one of the outermost "led"
    // If we shift right the racket an the ball doesn't anymore hit the racket, then ball hit the left outermost "led"
    if (racketHitLeft(racket))
    {
      // If the ball hits at the left outermost racket "led" and the ball is at the left border, then 
      if (ballPositionX == 0)
      {
        ballDirectionX = 1;
      } else
      {
        ballDirectionX = -1;
      }
    }
    // If we left right the racket an the ball doesn't anymore hit the racket, then ball hit the right outermost "led"
    else if(racketHitRight(racket))
    {
      // If the ball hits at the left outermost racket "led" and the ball is at the left border, then 
      if (ballPositionX == 7)
      {
        ballDirectionX = -1;
      } else
      {
        ballDirectionX = 1;
      }
    }
    // ball hit the middle "led"
    else {
      ballDirectionX = 0;
    }
    return true;
  } 
  return false;
}

/*
  * Does the ball hit the racket?
  *   --> 0-7
  *   O X X X O O O O  (racketB)
  *   O O O O O O O O
  *   O O O O O O O O
  *   O O O O O O O O
  *   O O O O O O O O
  *   O O O O O O O O
  *   O O O @ O O O O  ball
  *   O O O X X X O O  (racketA)
 *   
 * Ball position (binary):   00010000 (1 << 4)
 * Racket position (binary): 00011100
 *               binary AND: 00010000 = ball position
 */
boolean racketHit(Racket* racket)
{
  return ballPositionY == racket->rowBeforeRacket && ((1 << (7 - ballPositionX)) & racket->position);
}

/*
 * Same as racketHit but first shift the racket one position to the right. If the boolean AND with the
 * ball is still > 0 then the ball hit the left part of the racket.
 * Prior to this function call the racketHit function for the same racket has to return true.
 */
boolean racketHitLeft(Racket* racket)
{
  return !((1 << (7 - ballPositionX)) & racket->position >> 1);
}

/*
 * Same as racketHitLeft but for the right part of the racket.
 */
boolean racketHitRight(Racket* racket)
{
  return !((1 << (7 - ballPositionX)) & racket->position << 1);
}

/*
 * Moves the racket according to the position of the potetionmeter
 */
void moveRacket(int potX, Racket* racket, int* lastPotValue)
{
  int potValue = analogRead(potX);
  // Filter out some analog to digital noise
  if (abs(potValue - *lastPotValue) > 50)
  {
    *lastPotValue = potValue;
    int steps = (int) (potValue / 170);
    int newPos = maxPos >> steps;
    if (newPos < minPos) {
      newPos = minPos;
    }
    lc.setRow(0, racket->row, newPos);
    racket->position = newPos;
  }
}

