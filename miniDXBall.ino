#include "LedControl.h"    //  Library for Matrix.
#include <LiquidCrystal.h> // Library for LCD.
#define Bottom 7 
// Arduino pin numbers
#define SW_pin  2 // digital pin connected to the joystick button
#define X_pin  0  // analog pin connected to X output of joystick
#define Y_pin  1  // analog pin connected to Y output of joystick
LedControl lc = LedControl(12, 11, 10, 1); //DIN, CLK, LOAD, No. DRIVER
#define V0_PIN 9  // Using PWM instead of potentiometer
LiquidCrystal lcd(8, 3, 4, 5, 6, 7);
#define nr_col_LCD 16 // number of collums on the LCD.
#define nr_row_LCD 2  // number of rows on the LCD.
//This is for showing the heart caracter on the LCD.
byte heart_h[] = {
  B00000,
  B01010,
  B11111,
  B11111,
  B01110,
  B00100,
  B00000,
  B00000
};

//This is a structure to easely manipulate the coords for the board.
struct coords_1
{
  int y1;
  int y2;
  int y3;
} board;
//This is a structure to easely manipulate the individual LEDs that needs moving
struct coords_2
{
  int x;
  int y;
} ball, prev_pos, my_direction, magnet;

//These are info to show on the LCD.
int score = 0, lives = 3, level = 1;
unsigned long t_update_LCD = 0, t_start = 0;
//These are the timings needed for manipulating the LEDs.
unsigned long t0_board = 0, t1 = 0, t0_ball = 0, t0_magnet = 0, t0_magnet_start;
bool magnet_on = false, sticky = false;// if sticky is true, then the ball stays on the board.
int brick_number;//Number of bricks left in the current level.

void Merry_Christmas()
{
  lcd.clear();
  lcd.write("  Ho, ho, ho...");
  lcd.setCursor(0,1);
  lcd.write("Merry Christmas!");
  delay(5000);
}


void update_LCD (char s)
{
  //I'm not using "lcd.clear()" because it will crash really fast.
  if (s == 't')//t = time
  {
    lcd.setCursor(6, 1);
    int tt = (t_update_LCD - t_start) / 1000;
    char ss[5];
    //I'm transforming the int to a string for avoiding errors on the LCD.
    itoa(tt, ss, 10);
    Serial.println(ss);
    lcd.write(ss);
    lcd.write(" s");
  }
  else if (s == 'l')// l = life
  {
    lcd.setCursor(0, 0);
    lcd.write("   ");
    lcd.setCursor(0, 0);
    //Writing the number of lives left.
    for ( int i = 0; i < lives; i++)
      lcd.write(byte(0));
  }
  else // s = score
  {
    lcd.setCursor(12, 0);
    char ss[5];
    //Using a string for the same reasong as the time.
    itoa(score, ss, 10);
    lcd.write(ss);
  }
}
//Clear LCD + prepare it for the game.
void start_LLCD()
{
  //I'm not using "lcd.clear()" because it often crashes.
  lcd.setCursor(0, 0);
  lcd.write("                ");
  lcd.setCursor(0, 1);
  lcd.write("                ");
  lcd.setCursor(0, 0);
  for ( int i = 0; i < lives; i++)
    lcd.write(byte(0));
  lcd.setCursor(4, 0);
  lcd.write(" Score: ");
  char ss[5];
  itoa(score, ss, 10);
  lcd.write(ss);
  lcd.setCursor(0, 1);
  lcd.write("Time: ");
}

void start()
{
  //This function allows you to move the board before the ball starts moving
  ball_off();
  t1 = millis();
  prev_pos.x = 7;
  ball.x = 6;
  ball.y = board.y2;
  my_direction.x = -1;
  my_direction.y = 0;
  //While you don't press the joystick you can move the board along with the board
  while (digitalRead(SW_pin) == true)
  {
    t1 = millis();
    if (t1 - t0_board > 150 && analogRead(X_pin) < 200)
      board_to_left(t1);
    if (t1 - t0_board > 150 && analogRead(X_pin) > 820)
      board_to_right(t1);
    ball_off();
    ball.y = board.y2;
    ball_on();
  }
  //The sticky boolean means the ball is "glued" to the board so they move along
  sticky = false;
  update_ball(t1);
}
//This matrix is for the current bricks left on the level.
// 1 means there is a brick.
// 0 means there's no brick.
int brick_matrix[8][8];
// menu_nav is to know where the "^" is placed
// 0 means left. 1 means right side.
// dificulty_level default is 0 ( Easy ).
// 1 means the dificulty is hard. ( ball updates faster ).
int menu_nav = 0, dificulty_level = 0, to_go = 0;
char opt = 's';//DEfault option is start.
//there are 4 options: start, dificulty, easy, hard.
unsigned long t_menu = 0;// this is used to know when to update the menu.
//notice: update it too frequently and the LCD may crash.
         //you should wait at least 1 second before doing another change.
bool menu_1 = false;//Which menu is selected.
// false = "start dificulty"
// true  = "easy hard"
int ref_time = 400;//Dificulty
void meniu ()
{
  lcd.setCursor(0, 0);
  lcd.write("Start Dificulty");
  lcd.setCursor(0, 1);
  lcd.write("^");
  //While start was not selected
  while (to_go == 0)
  {
    while (digitalRead(SW_pin) == true )
    {
      if (menu_1 == false)
      {
        t1 = millis();
        if (menu_nav == 1 && t1 - t_menu > 150 && analogRead(X_pin) < 200 )
        {
          //Cursor is on start.
          lcd.setCursor(0, 1);
          lcd.write("^");
          lcd.write("       ");
          menu_nav = 0;
          t_menu = t1;
          opt =  's';
        }
        if (menu_nav == 0 && t1 - t_menu > 150 && analogRead(X_pin) > 820)
        {
          //Cursor is on dificulty
          lcd.setCursor(0, 1);
          lcd.write("      ");
          lcd.write("^");
          menu_nav = 1;
          t_menu = t1;
          opt = 'd';
        }
      }
      else
      {
        t1 = millis();
        if (dificulty_level == 1 && t1 - t_menu > 150 && analogRead(X_pin) < 200)
        {
          //Cursor is on easy.
          lcd.setCursor(0, 1);
          lcd.write("^");
          lcd.write("          ");
          dificulty_level = 0;
          t_menu = t1;
          opt = 'e';
        }
        if (dificulty_level == 0 && t1 - t_menu > 150 && analogRead(X_pin) > 820)
        {
          //Cursor is on hard
          lcd.setCursor(0, 1);
          lcd.write("         ");
          lcd.write("^");
          dificulty_level = 1;
          t_menu = t1;
          opt = 'h';
        }
      }

    }
    if (menu_1 == false)
    {
      if (opt == 's')
      {
        to_go = 1;
      }
      else
      {
        //Show the other menu.
        lcd.setCursor(0, 0);
        lcd.write("Easy     Hard  ");
        lcd.setCursor(0, 1);
        lcd.write("^         ");
        menu_1 = true;
      }
    }
    else
    {
      //Set the dificulty.
      menu_1 = false;
      if (opt == 'h')
      {
        ref_time = 200;
      }
      else
      {
        ref_time = 400;
      }
      //Change menu.
      lcd.setCursor(0, 0);
      lcd.write("Start Dificulty");
      lcd.setCursor(0, 1);
      lcd.write("^");
      lcd.write("           ");
    }
    opt = 's';
    //using delay not to crash the LCD.
    delay(200);
  }
}
void game_over()
{
  //print_GG();
  //this function will be added to print GG on 
    //the LED matrix after you finish all levels.

  //this function is called either when you win a level
    // or you fail.
  to_go = 0;
  for ( int i = 0; i < 8; i++)
    for (int j = 0; j < 8 ; j++)
    {
      brick_matrix[i][j] = 0;
      led_off(i, j);
    }
  board.y1  = 3;
  board.y2  = 4;
  board.y3  = 5;
  board_on();
  lcd.clear();
  delay(200);
  lcd.setCursor(1, 0);
  lcd.print("Score: ");
  char a[10];
  itoa(score, a, 10);
  lcd.print(a);
  if( brick_number == 0)
  {
    //Here you finished a level
      //this will be changed in the next update of the game.
    load_level_2();
    lcd.setCursor(0,1);
    lcd.write("   GG, mah, gg!   ");
    //this is a refference to a Romanian guy that plays games.
      // Do you know him?
  }
  else
  {
    //here you lost the game so we reset stuff back to the start.
    load_level_test();
    score = 0;
    lives = 3;
    lcd.setCursor(0,1);
    lcd.write(" Get good, noob!   ");
  }
  //Get the LCD menu back.
  while ( digitalRead(SW_pin) == true );
  //using delay not to crash the LCD.
  delay(300);
  lcd.setCursor(0,1);
  lcd.write("                  ");
  meniu();
  //using delay not to crash the LCD.
  delay(300);
  start_LLCD();
  //using delay not to crash the LCD.
  delay(300);
  start();
}
void setup()
{
  //LCD
  lcd.createChar(0, heart_h);
  lcd.begin(nr_col_LCD, nr_row_LCD);
  pinMode(V0_PIN, OUTPUT);
  analogWrite(V0_PIN, 40);

  //Matrice
  pinMode(SW_pin, INPUT);
  digitalWrite(SW_pin, HIGH);
  lc.shutdown(0, false); // does much stuff :))
                         // check here: https://playground.arduino.cc/Main/LedControl
                         // while you're there if you don't believe me on the next two
                         // function, search them too!
  lc.setIntensity(0, 2); // set the brightness.
  lc.clearDisplay(0);    // turns every led off.
  Serial.begin(9600);
  Merry_Christmas();//Christmas special.
  
  //Make the brick_matrix without bricks
  for (int i = 0; i < 8; i++)
    for (int j = 0; j < 8; j++)
      brick_matrix[i][j] = 0;

  //Set the board to start in the middle.
  board.y1  = 3;
  board.y2  = 4;
  board.y3  = 5;
  board_on();
  meniu();
  //delay so the LCD won't crash.
  delay(300);
  start_LLCD();
  load_level_test();
  start();
  t_start = millis();
}

void loop()
{
  
  t1 = millis();
  //Update the time on the LCD once every second.
  if (t1 - t_update_LCD > 1000)
  {
    t_update_LCD = t1;
    update_LCD('t');
  }
  if (sticky == true)
    start();
  else
  {
    //Moving the ball depending on the dificulty.
    if (t1 - t0_ball > ref_time)
      update_ball(t1);
  }
  if (t1 - t0_board > 150 && analogRead(X_pin) < 200)
    board_to_left(t1);
  if (t1 - t0_board > 150 && analogRead(X_pin) > 820)
    board_to_right(t1);

  //Spawn a "magnet superpower" once every 30 seconds.
  if (t1 - t0_magnet > 30000)
    spawn_magnet(t1);

  //Moving the magnet once every 800 ms.
  if (magnet_on == true && t1 - t0_magnet_start > 800)
    update_magnet(t1);
}

//Move the magnet downwards
void update_magnet(unsigned long t)
{
  //Actualize the time the magnet was last moved.
  t0_magnet_start = t;
  if (magnet.x != 7)
  {
    //I know this looks weird, but hear me out:
      // I dunno why it wasn't working when I wrote the negation of that condition
      // so... I used a dumb trick :))
    if ( ball.x == magnet.x && ball.y == magnet.y)
    {
      // I dunno
    }
    else
      led_off(magnet.x, magnet.y);
    
    magnet.x++;
    led_on(magnet.x, magnet.y);
  }
  else
  {
    //If you caught the magnet, your ball will teleport to the board becoming sticky.
    if ( magnet.y == board.y1 || magnet.y == board.y2 || magnet.y == board.y3 )
      sticky = true;
    else
      led_off(magnet.x, magnet.y);
    magnet_on = false;
  }
}
//Spawning the magnet in the middle of the map
void spawn_magnet(unsigned long t)
{
  t0_magnet = t;
  t0_magnet_start = t;
  magnet_on = true;
  magnet.y  = 4;
  magnet.x  = 4;
  led_on(magnet.x, magnet.y);
}
void update_ball(unsigned long t)
{
  t0_ball = t;
  //First conditions mean it's in one of he corners.
  //The spacing between the "(" are purposely made like that for easier reading.
  if ( (ball.x == 6 && ball.y == 0) || (ball.x == 6 && ball.y == 7) || (ball.x == 0 && ball.y == 7) || (ball.x == 0 && ball.y == 0) )
  {
    update_ball_corner();
  }
  //If the ball is on the 6th line it needs a special update to determine if you lose a life or not.
  else if (ball.x == 6)
  {
    update_ball_is_down();
  }
  //This is for when a ball hits the ball.
  else if (ball.y == 0 || ball.y == 7 || ball.x == 0)
  {
    update_ball_hit_wall();
  }
  //And the normal one.
  else
  {
    update_ball_normal();
  }
}
void update_ball_is_down()
{
  if (ball.y != board.y1 && ball.y != board.y2 && ball.y != board.y3)
    // Ball misses the board. Lose a life.
  {
    if (lives == 1)
    {
      game_over();
    }
    else
    {
      lives --;
      t_update_LCD = millis();
      //Delay for not crashing the LCD, but also for a better feedback that you lost a life.
      delay(1000);
      update_LCD('l');
      start();
    }
  }
  // You hit the board
  else if (ball.y == board.y1)
    // To the left a!
  {
    ball_move(-1, -1);
  }
  else if (ball.y == board.y3)
    // To the right a!
  {
    ball_move(-1, +1);
  }
  else
    // Jump around 'cause it's alright!
  {
    ball_move(-1, 0);
  }
  //Can you guess it without searching on Google?
    //I tought this is funny. Let me know if it isnt :))  
}
void update_ball_corner()
{
  //Bottom left/right corner.
  if ( (board.y3 == ball.y || board.y1 == ball.y) && ball.x == 6)
  {
    my_direction.x = -my_direction.x;
    my_direction.y = -my_direction.y;
    ball_move(my_direction.x, my_direction.y);
  }
  else
  {
    //Top left/right corner
    if ( ( ball.y == 0 && ball.x == 0) || (ball.y == 7 && ball.x == 0) )
    {
      my_direction.x = -my_direction.x;
      my_direction.y = -my_direction.y;
      ball_move(my_direction.x, my_direction.y);
    }
    //This else is weird here, should have been outside.
    //If you are at the bottom and you don't hit the board you lose a life.
    else
    {
      if ( lives == 1 )
      {
        game_over();
      }
      else
      {
        lives--;
        t_update_LCD = millis();
        //The delay is for not chrashing and feedback purposes
        delay(1000);
        update_LCD('l');
        start();
      }
    }
  }
}
void update_ball_hit_wall()
{
  //Left/right wall. The y axis changes.
  if (ball.y == 0 || ball.y == 7)
  {
    my_direction.y = -my_direction.y;
    ball_move(my_direction.x, my_direction.y);
  }
  //Top border. The x axis changes.
  else
  {
    my_direction.x = -my_direction.x;
    ball_move(my_direction.x, my_direction.y);
  }
}
void update_ball_normal()
{
  //Nothing to see here, just normal movement.
  ball_move(my_direction.x, my_direction.y);
}

// Turn on a led at x, y
void led_on(int x, int y)
{
  if ( x < 8 && x >= 0 && y < 8 && y >= 0)
    lc.setLed(0, x, y, true);
  else
    Serial.println("Can't turn off that led. Exit!");
}
// Turn off led at x, y
void led_off(int x, int y)
{
  if ( x < 8 && x >= 0 && y < 8 && y >= 0)
    lc.setLed(0, x, y, false);
  else
    Serial.println("Can't turn off that led. Exit!");
}

// Turn on the ball
void ball_on()
{
  led_on(ball.x , ball.y);
}
// Turn off the ball
void ball_off()
{
  led_off(ball.x , ball.y);
}

// Turn on the board.
void board_on()
{
  led_on(Bottom, board.y1);
  led_on(Bottom, board.y2);
  led_on(Bottom, board.y3);
}
// Turn off the board.
void board_off()
{
  led_off(Bottom, board.y1);
  led_off(Bottom, board.y2);
  led_off(Bottom, board.y3);
}

// Move board to the right
void board_to_right(unsigned long t)
{
  // Change the time of last movement
  t0_board = t;
  // If it's not on the far right:
  if ( board.y3 != 7 )
  {
    board_off();
    board.y1++;
    board.y2++;
    board.y3++;
    board_on();
  }
}
// Move board to the left
void board_to_left(unsigned long t)
{
  // Change the time of last movement
  t0_board = t;
  // If it's not on the far left:
  if (board.y1 != 0)
  {
    board_off();
    board.y1--;
    board.y2--;
    board.y3--;
    board_on();
  }
}

void remove_brick(int x, int y)
{
  brick_number--;
  if ( brick_number == 0 )
  //If there are no more bricks on the map, gg.
  {
    score++;
    game_over();
  }
  //Erase the brick and update LCD.
  else
  {
    brick_matrix[x][y] = 0;
    led_off(x, y);
    t_update_LCD = millis();
    score++;
    update_LCD('s');
  }
}

void ball_move_brick_collision(int x, int y)
{
  if (ball.x == 2 && x == 1 && ball.y == 6)//I honestly don't know why ball.y == 6 is there.(prob a bug)
  //If it's going top hitting a ball on second row.
  {
    my_direction.x = -my_direction.x;
    my_direction.y = -my_direction.y;
    prev_pos.x = x;
    prev_pos.y = y;
    ball_move(my_direction.x, my_direction.y);
  }
  else if (ball.x == 0  && x == 1)
  // If it's going to hit a brick on second row and it comes from the top.
  {
    //tbh I think this is a really, really rare case.
    //I turn off the ball and move it manually where it's supposed to be next.
    ball_off();
    ball.x = prev_pos.x;
    ball.y = prev_pos.y;
    prev_pos.x += my_direction.x;
    prev_pos.y -= my_direction.y;
    my_direction.y = -my_direction.y;
    ball_on();
  }
  else if ( (y <= 5 && y >= 2) && ( x >= 2) || (ball.y == 2 || ball.y == 5) || (ball.y == 6 && y == 7) || (ball.y == 1 && y == 0) )
    /* You're fine, do your normal thing. Spacing in this if is for better understanding of the conditions.
     *  (y <= 5 && y >= 2) && ( x >= 2) if the brick is not on the outter 2 layers.
     *   I consider a layer the squares counting from the middle to outside.
     *  (ball.y == 2 || ball.y == 5) if the ball is on the edges, but not on the last layer.
     *  (ball.y == 6 && y == 7) if the ball is going to hit a brick on the last layer and it's comming from the inside.
     *  (ball.y == 1 && y == 0) ----------------------------------------||--------------------------------------------
     */
  {
    //It goes in the opposite way.
    my_direction.x = -my_direction.x;
    my_direction.y = -my_direction.y;
    prev_pos.x = x;
    prev_pos.y = y;
    ball_move(my_direction.x, my_direction.y);
  }
  else
    // Too close to the border, mate!
    // Again, I manually move the ball.
  {
    if ( ball.y == 7 && y == 6)
      // Being on the right side
    {
      ball_off();
      ball.x = prev_pos.x;
      ball.y = prev_pos.y;
      prev_pos.x += my_direction.x;
      prev_pos.y -= my_direction.y;
      my_direction.x = -my_direction.x;
      ball_on();
    }
    else
      //Being on the left side
    {
      ball_off();
      ball.x = prev_pos.x;
      ball.y = prev_pos.y;
      prev_pos.x += my_direction.x;
      prev_pos.y -= my_direction.y;
      my_direction.x = -my_direction.x;
      ball_on();
    }
  }
}

void ball_move(int x, int y)
{
  if (brick_matrix[ball.x + x][ball.y + y] == 1)
  //If it's gonna hit a brick, remove the brick and deal with the collision.
  {
    remove_brick(ball.x + x, ball.y + y);
    ball_move_brick_collision(ball.x + x, ball.y + y);
  }
  else
  {
    //Just moving it normally.
    ball_off();
    prev_pos.x = ball.x;
    prev_pos.y = ball.y;
    my_direction.x = x;
    my_direction.y = y;
    ball.x += my_direction.x;
    ball.y += my_direction.y;
    ball_on();
  }
}
//Levele:
void load_level_test()
{
  brick_number = 3;
  brick_matrix[1][0] = 1; led_on(1, 0);
  brick_matrix[1][1] = 1; led_on(1, 1);
  brick_matrix[1][3] = 1; led_on(1, 3);
}
//Nivel Diana
void load_level_2()
{
  brick_number = 8;
  for (int i = 0; i < 3; i++)
    for (int j = i + 1; j <= 6 - i; j++)
    {
      brick_matrix[i][j] = 1;
      led_on(i, j);
    }
  brick_matrix[0][5] = brick_matrix[0][6] = 0;
  led_off(0, 5);
  led_off(0, 6);
  brick_matrix[0][1] = brick_matrix[0][2] = 0;
  led_off(0, 1);
  led_off(0, 2);
}
// Nivel Razvan.
void load_level_3()
{
  brick_number = 12;

  brick_matrix[0][3] = 1; led_on(0, 3);
  brick_matrix[0][5] = 1; led_on(0, 5);
  brick_matrix[0][6] = 1; led_on(0, 6);
  brick_matrix[1][0] = 1; led_on(1, 0);
  brick_matrix[1][1] = 1; led_on(1, 1);
  brick_matrix[1][3] = 1; led_on(1, 3);
  brick_matrix[1][6] = 1; led_on(1, 6);
  brick_matrix[2][1] = 1; led_on(2, 1);
  brick_matrix[2][3] = 1; led_on(2, 3);
  brick_matrix[3][4] = 1; led_on(3, 4);
  brick_matrix[3][6] = 1; led_on(3, 6);
  brick_matrix[3][7] = 1; led_on(3, 7);
}
//Nivel Serafim.
void load_level_1()
{
  brick_number = 12;
  for (int j = 1; j < 3; j++)
    for (int i = 0; i < 7; i += 3)
    {
      brick_matrix[j][i] = 1;
      brick_matrix[j][i + 1] = 1;
      led_on(j, i);
      led_on(j, i + 1);
    }
}
