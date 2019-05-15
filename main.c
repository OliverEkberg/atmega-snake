/*
 * Snakespel.c
 *
 * Created: 2019-04-17 10:12:22
 * Author : Grupp 2
 */
#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <time.h>

#define JOYSTICK_NEUTRAL_HORIZONTAL 420
#define JOYSTICK_NEUTRAL_VERTICAL 500
#define JOYSTICK_THRESHOLD 150

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCALE 4 // Scale must divide SCREEN_WIDTH and SCREEN_HEIGHT
#define WIDTH ((SCREEN_WIDTH) / (SCALE))
#define HEIGHT ((SCREEN_HEIGHT) / (SCALE))

char virtual_display[8][128];

typedef enum Direction
{
  None,
  Left,
  Right,
  Up,
  Down
} Direction;

typedef struct Coordinate
{
  int x;
  int y;
} Coordinate;

typedef struct Snake
{
  int dx;
  int dy;
  int len;
  Coordinate parts[100];
} Snake;

Direction dir1 = None;
Direction dir2 = None;

// Get result from ADC
uint16_t adcRead(uint8_t ch)
{
  // Make sure input is 0-7
  ch &= 0b00000111;
  // Set MUX-channel
  ADMUX = (ADMUX & 0xF8) | ch;
  // Start ADC
  ADCSRA |= (1 << ADSC);
  // Wait for ADC to complete
  while (ADCSRA & (1 << ADSC))
    ;
  return (ADC);
}

// Return the direction created by the given joystick
Direction getDirectionForJoystick(int horizontalPotensometer, int verticalPotensometer)
{
  int horizontalMove = adcRead(horizontalPotensometer);
  int verticalMove = adcRead(verticalPotensometer);

  if (horizontalMove < JOYSTICK_NEUTRAL_HORIZONTAL - JOYSTICK_THRESHOLD)
  {
    return Left;
  }

  if (horizontalMove > JOYSTICK_NEUTRAL_HORIZONTAL + JOYSTICK_THRESHOLD)
  {
    return Right;
  }

  if (verticalMove < JOYSTICK_NEUTRAL_VERTICAL - JOYSTICK_THRESHOLD)
  {
    return Up;
  }

  if (verticalMove > JOYSTICK_NEUTRAL_VERTICAL + JOYSTICK_THRESHOLD)
  {
    return Down;
  }

  return None;
};

void cs2high()
{
  PORTD |= 0b00000010;
}

void cs2low()
{
  PORTD &= 0b11111101;
}

void cs1high()
{
  PORTD |= 0b00000001;
}

void cs1low()
{
  PORTD &= 0b11111110;
}

void resetHigh()
{
  PORTD |= 0b00000100;
}

void resetLow()
{
  PORTD &= 0b11111011;
}

void rwHigh()
{
  PORTD |= 0b00001000;
}

void rwLow()
{
  PORTD &= 0b11110111;
}

void rsHigh()
{
  PORTD |= 0b00010000;
}

void rsLow()
{
  PORTD &= 0b11101111;
}

void eHigh()
{
  PORTD |= 0b00100000;
}

void eLow()
{
  PORTD &= 0b11011111;
}

void startDisplay()
{
  PORTB = 0b00111111;

  cs1low();
  cs2low();

  resetHigh();
  rwLow();
  rsLow();
  eHigh();

  cs1high();
  cs2high();

  eLow();
  eHigh();
}

void setCell(char cell)
{
  rsLow();
  rwLow();

  PORTB = 0b10111000 | cell;

  eHigh();
  eLow();
}

void setX(char x)
{
  rsLow();
  rwLow();

  PORTB = 0b01000000 | x;

  eHigh();
  eLow();
}

// Wait for display to be ready
void waitForDisplay()
{
  PORTB &= 0x7F;
  DDRB = 0x7F;

  rsLow();
  rwHigh();

  while (PINB & 0x80)
    ;

  DDRB = 0xFF;
}

// Draw all set pixels
void draw()
{
  for (char cell = 0; cell < 8; cell++)
  {
    for (char x = 0; x < 128; x++)
    {
      waitForDisplay();
      char displayX;
      if (x < 64)
      {
        waitForDisplay();
        cs1high();
        waitForDisplay();
        cs2low();
        waitForDisplay();
        displayX = x;
      }
      else
      {
        waitForDisplay();
        cs2high();
        waitForDisplay();
        cs1low();
        waitForDisplay();
        displayX = x - 64;
      }

      waitForDisplay();
      setX(displayX);
      waitForDisplay();
      setCell(cell);
      waitForDisplay();
      waitForDisplay();
      rsHigh();
      rwLow();

      PORTB = virtual_display[cell][x];

      eHigh();
      eLow();

      virtual_display[cell][x] = 0; // Reset virtual_display afterwards
    }
  }
}

void init()
{
  // Set DDR
  DDRA = 0;
  DDRB = 0b11111111;
  DDRD = 0b11111111;

  startDisplay();

  // Init ADC
  ADMUX |= (1 << REFS0);                                            // Set the reference of ADC
  ADCSRA |= (1 << ADEN) | (1 < ADPS2) | (1 < ADPS1) | (1 << ADPS0); // Enable ADC, set prescaler to 128

  // Init random-generator
  srand(time(NULL));
}

int isEqual(Coordinate *c1, Coordinate *c2)
{
  return c1->x == c2->x && c1->y == c2->y;
}

// Checks if a coordinate is within window bounds
int isWithinBounds(Coordinate *c)
{
  return c->x >= 0 && c->x < WIDTH && c->y >= 0 && c->y < HEIGHT;
}

// Draws given snake
void drawSnake(Snake *s)
{
  for (int i = 0; i < s->len; i++)
  {
    setPixel(s->parts[i].x, s->parts[i].y, SCALE);
  }
}

// Sets the direction of given snake
void setDirection(Snake *s, enum Direction d)
{
  switch (d)
  {
  case Right:
    if (s->dx != -1)
    {
      s->dx = 1;
      s->dy = 0;
    }
    break;
  case Left:
    if (s->dx != 1)
    {
      s->dx = -1;
      s->dy = 0;
    }
    break;
  case Up:
    if (s->dy != 1)
    {
      s->dx = 0;
      s->dy = -1;
    }
    break;
  case Down:
    if (s->dy != -1)
    {
      s->dx = 0;
      s->dy = 1;
    }
    break;
  }
}

// Gives the food a new random position
void randomizeFoodPos(Coordinate *food)
{
  food->x = rand() % (WIDTH - 1) + 1;
  food->y = rand() % (HEIGHT - 1) + 1;
}

void update(Snake *s1, Snake *s2)
{
  for (int i = 0; i < s1->len; i++)
  {
    if (isEqual(&s2->parts[0], &s1->parts[i]))
    {
      endGame(1);
    }
  }

  for (int i = 0; i < s2->len; i++)
  {
    if (isEqual(&s1->parts[0], &s2->parts[i]))
    {
      endGame(2);
    }
  }
}

void update_snake(Snake *s, Coordinate *food, int num)
{
  Coordinate newHead = {.x = s->parts[0].x,
                        .y = s->parts[0].y};

  newHead.x += s->dx;
  newHead.y += s->dy;

  for (int i = s->len - 1; i >= 0; i--)
  {
    s->parts[i + 1] = s->parts[i];
  }

  s->parts[0] = newHead;

  // If the head has same position as the food it should grow
  if (isEqual(&s->parts[0], food))
  {
    s->len++;
    randomizeFoodPos(food);
  }

  // Check for collision with other body parts
  for (int i = 1; i < s->len; i++)
  {
    if (isEqual(&s->parts[0], &s->parts[i]))
    {
      endGame(num);
    }
  }

  // If the snake hits the walls its game over
  if (!isWithinBounds(&s->parts[0]))
  {
    endGame(num);
  }
}

// Sets a pixel to active for future drawing
void setPixel(char x, char y, int scale)
{
  x *= scale;
  y *= scale;
  for (int i = 0; i < scale; i++)
  {
    for (int j = 0; j < scale; j++)
    {
      // ABS to compensates for upside-down display :)
      char yDisplay = abs((y + i) - 63);
      char xDisplay = abs((x + j) - 127);
      virtual_display[yDisplay / 8][xDisplay] |= (1 << (yDisplay % 8));
    }
  }
}

void endGame(int winner)
{
  int x = 10;
  int y = 5;
  int s = 2;
  // P
  setPixel(x + 0, y + 0, s);
  setPixel(x + 0, y + 1, s);
  setPixel(x + 0, y + 2, s);
  setPixel(x + 0, y + 3, s);
  setPixel(x + 0, y + 4, s);
  setPixel(x + 1, y + 0, s);
  setPixel(x + 1, y + 2, s);
  setPixel(x + 2, y + 1, s);
  x += 4;
  // L
  setPixel(x + 0, y + 0, s);
  setPixel(x + 0, y + 1, s);
  setPixel(x + 0, y + 2, s);
  setPixel(x + 0, y + 3, s);
  setPixel(x + 0, y + 4, s);
  setPixel(x + 1, y + 4, s);
  setPixel(x + 2, y + 4, s);
  x += 4;
  // A
  setPixel(x + 0, y + 1, s);
  setPixel(x + 0, y + 2, s);
  setPixel(x + 0, y + 3, s);
  setPixel(x + 0, y + 4, s);
  setPixel(x + 1, y + 0, s);
  setPixel(x + 1, y + 2, s);
  setPixel(x + 2, y + 1, s);
  setPixel(x + 2, y + 2, s);
  setPixel(x + 2, y + 3, s);
  setPixel(x + 2, y + 4, s);
  x += 4;
  // Y
  setPixel(x + 0, y + 0, s);
  setPixel(x + 0, y + 1, s);
  setPixel(x + 1, y + 2, s);
  setPixel(x + 1, y + 3, s);
  setPixel(x + 1, y + 4, s);
  setPixel(x + 2, y + 0, s);
  setPixel(x + 2, y + 1, s);
  x += 4;
  // E
  setPixel(x + 0, y + 0, s);
  setPixel(x + 0, y + 1, s);
  setPixel(x + 0, y + 2, s);
  setPixel(x + 0, y + 3, s);
  setPixel(x + 0, y + 4, s);
  setPixel(x + 1, y + 0, s);
  setPixel(x + 1, y + 2, s);
  setPixel(x + 1, y + 4, s);
  setPixel(x + 2, y + 0, s);
  setPixel(x + 2, y + 4, s);
  x += 4;

  // R
  setPixel(x + 0, y + 0, s);
  setPixel(x + 0, y + 1, s);
  setPixel(x + 0, y + 2, s);
  setPixel(x + 0, y + 3, s);
  setPixel(x + 0, y + 4, s);
  setPixel(x + 1, y + 0, s);
  setPixel(x + 1, y + 2, s);
  setPixel(x + 2, y + 1, s);
  setPixel(x + 2, y + 3, s);
  setPixel(x + 2, y + 4, s);
  x += 4;
  x++; // Space between words

  if (winner == 1)
  {
    // Player 1 won
    setPixel(x + 0, y + 0, s);
    setPixel(x + 0, y + 4, s);
    setPixel(x + 1, y + 0, s);
    setPixel(x + 1, y + 1, s);
    setPixel(x + 1, y + 2, s);
    setPixel(x + 1, y + 3, s);
    setPixel(x + 1, y + 4, s);
    setPixel(x + 2, y + 4, s);
  }
  else
  {
    // Player 2 won
    setPixel(x + 0, y + 1, s);
    setPixel(x + 0, y + 4, s);
    setPixel(x + 1, y + 0, s);
    setPixel(x + 1, y + 3, s);
    setPixel(x + 1, y + 4, s);
    setPixel(x + 2, y + 1, s);
    setPixel(x + 2, y + 2, s);
    setPixel(x + 2, y + 4, s);
  }
  x += 4;
  x++; // Space between words

  // W
  setPixel(x + 0, y + 0, s);
  setPixel(x + 0, y + 1, s);
  setPixel(x + 0, y + 2, s);
  setPixel(x + 0, y + 3, s);
  setPixel(x + 1, y + 4, s);
  setPixel(x + 2, y + 0, s);
  setPixel(x + 2, y + 1, s);
  setPixel(x + 2, y + 2, s);
  setPixel(x + 2, y + 3, s);
  setPixel(x + 3, y + 4, s);
  setPixel(x + 4, y + 0, s);
  setPixel(x + 4, y + 1, s);
  setPixel(x + 4, y + 2, s);
  setPixel(x + 4, y + 3, s);
  x += 6;

  // O
  setPixel(x + 0, y + 1, s);
  setPixel(x + 0, y + 2, s);
  setPixel(x + 0, y + 3, s);
  setPixel(x + 1, y + 0, s);
  setPixel(x + 1, y + 4, s);
  setPixel(x + 2, y + 1, s);
  setPixel(x + 2, y + 2, s);
  setPixel(x + 2, y + 3, s);

  x += 4;

  // N
  setPixel(x + 0, y + 0, s);
  setPixel(x + 0, y + 1, s);
  setPixel(x + 0, y + 2, s);
  setPixel(x + 0, y + 3, s);
  setPixel(x + 0, y + 4, s);
  setPixel(x + 1, y + 1, s);
  setPixel(x + 2, y + 2, s);
  setPixel(x + 3, y + 0, s);
  setPixel(x + 3, y + 1, s);
  setPixel(x + 3, y + 2, s);
  setPixel(x + 3, y + 3, s);
  setPixel(x + 3, y + 4, s);

  x += 5;

  // !
  setPixel(x + 0, y + 0, s);
  setPixel(x + 0, y + 1, s);
  setPixel(x + 0, y + 2, s);
  setPixel(x + 0, y + 4, s);

  draw();
  exit(0);
}

int main(void)
{
  init();
  Snake s1 = {.parts = {{.x = 0, .y = 0}}, .len = 1};                  // Upper left corner
  Snake s2 = {.parts = {{.x = WIDTH - 1, .y = HEIGHT - 1}}, .len = 1}; // Lower right corner
  setDirection(&s1, Right);
  setDirection(&s2, Left);
  Coordinate food = {.x = 0, .y = 0};
  randomizeFoodPos(&food);

  while (1)
  {
    // Set direction from joystics
    setDirection(&s1, getDirectionForJoystick(2, 1));
    setDirection(&s2, dir2 = getDirectionForJoystick(5, 4));

    // Update
    update_snake(&s1, &food, 2);
    update_snake(&s2, &food, 1);
    update(&s1, &s2);

    // Drawing
    drawSnake(&s1);
    drawSnake(&s2);
    setPixel(food.x, food.y, SCALE);
    draw();

    // To make game not to fast
    _delay_ms(100);
  }
}