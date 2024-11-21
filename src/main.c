#include<soc/uart_struct.h>
#include<esp_timer.h>
#include<graphics.h>
#include<fonts.h>
#include<driver/gpio.h>
#include<inttypes.h>
#include<stdio.h>
#include<math.h>

/* 
====================================================
Struct Definitions
----------------------------------------------------

vec2f is a utility vector with 2 floats, used for all
the X/Y type calcs (position, velocity, acceleration)

Piece is a struct made up of mostly vec2f and a 
utility flag, and represents any moving piece

linked_list and Node work together, linked_list is the 
head, and holds a incrementing counter and the pointer
to the first LL element 

====================================================
*/


//Float vector (used for coordinates and velocities)
typedef struct { float x; float y; } vec2f;

//game pieces
typedef struct Piece {
  //Using 2d vector for pieces allows me to use a lot of generic code
  //its designed with 2d movement in mind, but at present player only
  //moves side to side, and enemies top to bottom
  vec2f dimensions;
  vec2f position;
  vec2f velocity;
  vec2f accel;
  bool flag;

} Piece;

typedef struct Node {
  Piece piece;
  struct Node* next;
} Node;
typedef struct linked_list {
  struct Node* first;
  uint16_t count;
} linked_list;

void add_node(struct Node *list, struct Node *new) {

  while(list->next != NULL) {
    list= list->next;
  }
  list->next = new;

}

void delete_node(struct linked_list* head, struct Node* target) {

  struct Node* temp = head->first;
  struct Node* previous = NULL;

  while(temp != target) {
    previous = temp;
    temp = temp->next;
  }

  if(previous == NULL) {
    head->first = temp->next;
    head->count--;
    free(temp);
  } else {
    previous->next = temp->next;
    head->count--;
    free(temp);
  }

}

void clear_list(struct linked_list *head) {

  struct Node* temp = head->first;
  struct Node* previous = NULL;

  while (temp != NULL) {

    previous = temp;
    temp = temp->next;
    free(previous);

  }

  head->first = NULL;
  head->count = 0;


}

/* 
====================================================
Float Vector manipulations
----------------------------------------------------

with add, multiply, min, max just about anything can be 
managed relating to movement. I removed for now the 
mul_vec_by_vec calculation as I wasn't using it with 
only side to side/up down motion. Woudl be needed
of course to manage any diagonals.

====================================================
*/
//vector manipulation functions
static inline vec2f add_vec(vec2f v, vec2f d) {
  return (vec2f) {v.x + d.x, v.y + d.y};
}

static inline vec2f mul_vec_by_float(vec2f v, float i) {
  return (vec2f) {v.x * i, v.y * i};
}

static inline vec2f max_vector(vec2f v, vec2f min) {
  if(v.x < min.x) v.x = min.x;
  if(v.y < min.y) v.y = min.y;
  return (vec2f) v;
}
static inline vec2f min_vector(vec2f v, vec2f max) {
  if(v.x > max.x) v.x = max.x;
  if(v.y > max.y) v.y = max.y;
  return (vec2f) v;
}

/* 
====================================================
Piece related functions 
----------------------------------------------------

random_start a random starting position for enemies
draw_ship does what it suggests on the tin, and 
draw circle is an extension to have circle drawing

====================================================
*/

vec2f random_start(int screen_width, vec2f dim) {
  return (vec2f) {rand() % (int) screen_width+1-dim.x,0-dim.y};
}

void draw_circle(int radius, int center_x, int center_y, uint16_t colour) {
 
  int radius_sq = radius * radius, height;
  //adapted from an answer I made for drawing the japanese flag in 159.102 to a .ppm
  for (int i = 0; i <=radius; i++) {
    height = sqrt(radius_sq - (i*i));
    for (int j = 0;  j<=height; j++) {
      draw_pixel(center_x + i, center_y+j,colour);
      draw_pixel(center_x + i, center_y-j,colour);
      draw_pixel(center_x - i, center_y+j,colour);
      draw_pixel(center_x - i, center_y-j,colour);
    }
  }



}
void draw_enemy(Piece enemy) {

  draw_rectangle(enemy.position.x, enemy.position.y, enemy.dimensions.x, enemy.dimensions.y, rgbToColour(0,255,0));

}

void draw_ship(Piece ship) {

  //white side stripes
  draw_rectangle(ship.position.x, ship.position.y, 1, ship.dimensions.y, rgbToColour(150,200,200));
  draw_rectangle(ship.position.x + ship.dimensions.x-1, ship.position.y, 1, ship.dimensions.y, rgbToColour(150,200,200));

  //next stripes
  draw_rectangle(ship.position.x+1, ship.position.y, 2, ship.dimensions.y, rgbToColour(0,255,185));
  draw_rectangle(ship.position.x + ship.dimensions.x-3, ship.position.y, 2, ship.dimensions.y, rgbToColour(0,255,185));

  //next stripes
  draw_rectangle(ship.position.x+3, ship.position.y, 2, ship.dimensions.y, rgbToColour(72,103,103));
  draw_rectangle(ship.position.x + ship.dimensions.x-5, ship.position.y, 2, ship.dimensions.y, rgbToColour(72,103,103));

  //middle block 
  draw_rectangle(ship.position.x+5, ship.position.y+25, 10, ship.dimensions.y-25, rgbToColour(72,95,95));




}


/* 
====================================================
Piece tests
----------------------------------------------------

Collision tests, and a test to see if the enemy is out of screen 
====================================================
*/


//adapted from https://levelup.gitconnected.com/2d-collision-detection-8e50b6b8b5c0
bool test_collision(Piece a, Piece b) {
  return a.position.x < b.position.x + b.dimensions.x //if a's left most position is less than b's right most
  && a.position.x + a.dimensions.x > b.position.x // while a's right most is greater than b's left most, they must be overlapping on x
  && a.position.y < b.position.y + b.dimensions.y // if a's top most dimension in less than b's bottom most dimension
  && a.position.y + a.dimensions.y > b.position.y;//  while a's bottom most dimension is greater than b's top most then they overlap
}

bool test_enemy(Piece enemy, float screen_height) {
    return (enemy.dimensions.y >= screen_height);  
  
}

/* 
====================================================
Main
----------------------------------------------------
Three sections:
for(;;)
  while() - intro screen, press A to run
  while() - game runs until collision detected
  while() - finish screen runs until A pressed

====================================================
*/

void app_main() { 

/* 
====================================================
Configurations
----------------------------------------------------
Graphics initialisations and gpio directions
Game overall variables
Game piece configurations
Timer variables
====================================================
*/
  // configurations for GPIO and graphics initialisations
  gpio_set_direction(0,GPIO_MODE_INPUT);
  gpio_set_direction(35,GPIO_MODE_INPUT);
  graphics_init();
  set_orientation(PORTRAIT);
  
  //game variables
  uint16_t level;
  char level_string[100]; //
  char score_string[100]; //
  uint32_t score;
  bool crashed;

  // game piece configurations, variables to allow tuning and eventual expansion by adding menus
  // position is always the top left corner of the hit box
  vec2f ship_dimensions = (vec2f) {20,40};
  vec2f min_ship_pos = (vec2f) {0, 0};
  vec2f max_ship_pos = (vec2f) {135 - ship_dimensions.x, 240 - ship_dimensions.y};
  vec2f max_velocity = (vec2f) {100,0}; //pixels per second essentially

  float thrust_accel = 200; //pixels per second per second
  float drag_decel = 100;

  vec2f enemy_dimesions = (vec2f) {6,16};
  int first_level_enemies = 5;
  int max_enemies = 20;
  vec2f first_level_velocity = (vec2f) {0,10};

  // timer variables
  uint64_t current_time, last_level_time, last_enemy_time ,last_frame_time = esp_timer_get_time();
  float dt; //declare as float for the vector calcs.



  //procedural backgrounds
  struct linked_list bubbles;
  struct linked_list sidewalls;
  bubbles.count = 0;
  bubbles.first = NULL;
  sidewalls.count = 0;
  sidewalls.first = NULL;

/* 
====================================================
Game
----------------------------------------------------

====================================================
*/
  for(;;){

  crashed = false;
  level = 1;
  score = 0;


  last_enemy_time = esp_timer_get_time();

  //draws menu screen and awaits user press of the A key
  while(gpio_get_level(0)){

    current_time = esp_timer_get_time();
      
    cls(rgbToColour(100,0,0));

    //animate the bubbles, every half a second a new bubble
    //is added to the linked list with a random x position
    //and a random size range
    if (last_enemy_time + 500000 < current_time) {

      struct Node* temp = NULL;
      temp = (struct Node*)malloc(sizeof(struct Node));
      temp->piece.position = random_start(135,(vec2f) {20,20});
      temp->piece.velocity = (vec2f) {0,20};
      temp->piece.dimensions = (vec2f) {rand()%10+5,0};
      temp->next = NULL;

      if (bubbles.count == 0) {
          bubbles.first = temp;
        } else {
          add_node(bubbles.first,temp);
        }
      last_enemy_time = current_time;
      bubbles.count += 1;
    }

    //iterate over the linked list of bubbles moving them, then drawing them
    //test to ensure some actually exist before running.
    if (bubbles.first != NULL){
      struct Node* temp = bubbles.first;
      while(temp != NULL) {
        dt = (esp_timer_get_time() - last_frame_time)/1.0e6f;

        temp->piece.position = add_vec(temp->piece.position, mul_vec_by_float(temp->piece.velocity,dt));
    
        draw_circle(temp->piece.dimensions.x,temp->piece.position.x,temp->piece.position.y,rgbToColour(120,0,0));
        draw_circle(temp->piece.dimensions.x-2,temp->piece.position.x,temp->piece.position.y,rgbToColour(135,0,0));
        temp = temp->next;


      }
    }

    //clean up the bubbles that have exited the board
    if (bubbles.first != NULL) {
      struct Node* temp = bubbles.first;
      struct Node* to_delete = NULL;

      while(temp != NULL) {

        //advance the temp BEFORE deletion (otherwise you delete it then can't get ->next!)
        to_delete = temp;
        temp = temp->next;

        if (to_delete->piece.position.y >= 240) {
          delete_node(&bubbles,to_delete);
        }
        
      }
    }

    setFont(FONT_DEJAVU18);
    setFontColour(255,255,0);
    print_xy("BLOODSTREAM",CENTER,CENTER);
    setFontColour(255,255,255);
    setFont(FONT_SMALL);
    print_xy("Use A to veer left",CENTER,LASTY+25);
    print_xy("Use B to veer right",CENTER,LASTY+18);
    setFont(FONT_UBUNTU16);
    print_xy("PRESS A to BEGIN",CENTER,LASTY+20);
    
    
    setFontColour(0,0,0);
    draw_circle(15,20,220,rgbToColour(255,255,255));
    draw_circle(15,115,220,rgbToColour(255,255,255));
    print_xy("A",15,212);
    print_xy("B",111,212);

    
    flip_frame();
    last_frame_time = current_time;
  }

  //delay start to allow for button release (otherwise the ship just skites off to screen left!)
   while(esp_timer_get_time() < last_frame_time+500000);


  /* 
====================================================
Game Startup
----------------------------------------------------
First checks if there is a sidewall drawn, if not 
procedurally generates one

Then it creates the ships and sets up other timer 
variables
====================================================
*/

  //if there are no circles in the procedural sidewalls, seed the sidewalls
  if(sidewalls.count < 1) {
    
    struct Node* temp = NULL;
    temp = (struct Node*)malloc(sizeof(struct Node));

    temp->piece.position = (vec2f) {0,-10};
    temp->piece.dimensions = (vec2f) {rand()%10+5,0};
    temp->piece.flag = true;
    temp->next = NULL;
    sidewalls.first = temp;
    
    temp = (struct Node*)malloc(sizeof(struct Node));
    temp->piece.position = (vec2f) {135,-10};
    temp->piece.dimensions = (vec2f) {rand()%10+5,0};
    temp->piece.flag = true;
    temp->next = NULL;

    add_node(sidewalls.first,temp);
    
  }
      
  // create ships

  Piece ship;
  ship.velocity = (vec2f) {0,0};
  ship.dimensions = ship_dimensions;
  ship.position = (vec2f) {135/2+1-ship.dimensions.x/2, max_ship_pos.y};
  setFontColour(255,255,255);
  struct linked_list enemies;
  enemies.count = 0;
  enemies.first = NULL;
  
  last_frame_time = esp_timer_get_time();
  last_level_time = last_frame_time;

    //set the seed each game so the random generation changes 
  srand(last_frame_time);

  /* 
====================================================
Game
----------------------------------------------------
Draws sidewalls, then takes player inputs
Moves and draws player piece
Moves and draws enemies
Checks collisions and checks for enemies that have
moved off the game board

====================================================
*/
  while(!crashed) {
      
      cls(rgbToColour(35,0,0));
      
      current_time = esp_timer_get_time();
      dt = (esp_timer_get_time() - last_frame_time)/1.0e6f; //converted to seconds


      //Sidewall animation starts here
      struct Node* temp = sidewalls.first;
      while(temp != NULL) {
      //add circles if needed, only if the flag is TRUE otherwise it infinitely creates circles and crashes!
        if(temp->piece.position.y >= 0 && temp->piece.flag) {
          temp->piece.flag = false; //the bubble can only pass Y=0 once!
          struct Node* new = (struct Node*)malloc(sizeof(struct Node));
          float x_start = 0;
          if (temp->piece.position.x > 75) x_start = 135; 
          new->piece.position = (vec2f) {x_start,-10};
          new->piece.dimensions = (vec2f) {rand()%10+5,0};
          new->piece.flag = true;
          new->next = NULL;

          add_node(sidewalls.first,new);

        }
        temp = temp->next;
      }

      temp = sidewalls.first;
      while (temp != NULL) {
        //move bubbles
        temp->piece.velocity = add_vec(first_level_velocity,(vec2f){0,5*level});
        temp->piece.position = add_vec(temp->piece.position, mul_vec_by_float(temp->piece.velocity,dt));
        //draw bubbles
        draw_circle(temp->piece.dimensions.x,temp->piece.position.x,temp->piece.position.y,rgbToColour(50,0,0));
        draw_circle(temp->piece.dimensions.x-2,temp->piece.position.x,temp->piece.position.y,rgbToColour(60,0,0));
        draw_circle(temp->piece.dimensions.x-4,temp->piece.position.x,temp->piece.position.y,rgbToColour(100,0,0));

        temp = temp->next;
      }

      //delete and clean up if required
      temp = sidewalls.first;
      while(temp != NULL) {
        struct Node* to_delete = NULL;
        to_delete = temp;
        temp = temp->next;
        //remove bubbles if needed
        if (to_delete->piece.position.y>240+to_delete->piece.dimensions.x) {
          delete_node(&sidewalls,to_delete);
        }
      }
    
      //accelerate the ship based on the status of the buttons
      //left thrusts left, right right and both together thrusts forwards
      if(!gpio_get_level(0) && !gpio_get_level(35)) {

          // ship.accel = (vec2f) {0,-1*thrust_accel}; to be replaced with shooting mechanism

      } else if (!gpio_get_level(0)) {

          ship.accel = (vec2f) {-1*thrust_accel,0};

      } else if (!gpio_get_level(35)) {

          ship.accel = (vec2f) {thrust_accel,0};

      } else {

        //what to do if no buttons are pressed!
        if (ship.velocity.x < 0) { 
          ship.accel.x = drag_decel;
        } else if (ship.velocity.x > 0) {
          ship.accel.x = -1*drag_decel;
        } else {
          ship.accel.x = 0;
        }
        

      }

      //move the ship - accelerates, checks against terminal velocities, then moves and tests if within boundaries.
      //acceleration is velocity + accel * delta time


      ship.velocity = add_vec(ship.velocity,mul_vec_by_float(ship.accel,dt));
      //test that the ship is not going faster than it's max velocity
      ship.velocity = min_vector(ship.velocity,max_velocity);
      //move the ship by it's speed and time travelled
      ship.position = add_vec(ship.position,mul_vec_by_float(ship.velocity,dt));
      ship.position = max_vector(ship.position, min_ship_pos);
      ship.position = min_vector(ship.position, max_ship_pos);

      draw_ship(ship);

      //create enemies if enough time has passed, time between enemies gets tighter each time
      if (last_enemy_time+(4000000-level*10000) < current_time) {
          //check there aren't too many on the board
          if (enemies.count < first_level_enemies + level && enemies.count <= max_enemies) {
            
            struct Node* temp = NULL;
            temp = (struct Node*)malloc(sizeof(struct Node));

            temp->piece.position = random_start(135,enemy_dimesions);
            temp->piece.velocity = first_level_velocity;
            temp->piece.dimensions = enemy_dimesions;
            temp->next = NULL;

            if (enemies.count == 0) {
              enemies.first = temp;
            } else {
              add_node(enemies.first,temp);
            }
            enemies.count++;
            last_enemy_time = current_time;
          }

      }

      // //iterate over the linked list of enemies moving them, then drawing them
      //test to ensure some actually exist before running.
      if (enemies.first != NULL){
        struct Node* temp = enemies.first;
        while(temp != NULL) {
          dt = (esp_timer_get_time() - last_frame_time)/1.0e6f; //recalculating to handle slight time changes making jerky movements
          //test if out of screen yet and if so delete from the ll
          //this occurs here so that temp can become the next in the list before all the moves etc
          //level adds some acceleration
          temp->piece.velocity = add_vec(temp->piece.velocity,mul_vec_by_float((vec2f){0,level*10},dt));
          //up to a scaling max velocity
          temp->piece.velocity = min_vector(temp->piece.velocity,add_vec(max_velocity,(vec2f){0,5*level}));
          //then this is moved
          temp->piece.position = add_vec(temp->piece.position, mul_vec_by_float(temp->piece.velocity,dt));
          //this has to be set as a result, otherwise it just swaps between states as it scans through each item in the LL
            if (test_collision(temp->piece,ship)) crashed = true;
            draw_enemy(temp->piece);
            temp = temp->next;

          

        }
      }

      //clean up the pieces that have exited the board and increment score
      if (enemies.first != NULL) {
        struct Node* temp = enemies.first;
        struct Node* to_delete = NULL;

        while(temp != NULL) {

          //advance the temp BEFORE deletion (otherwise you delete it then can't get ->next!)
          to_delete = temp;
          temp = temp->next;

          if (to_delete->piece.position.y >= 240) {
            delete_node(&enemies,to_delete);
            score+=100;
          }
          
        }

      }

      //increment level every 10 seconds, this is too short for a real game, but for demo purposes of the speed changing etc
      //works quite well

      if (last_level_time+10000000 < current_time) {

        level += 1;
        last_level_time = current_time;
        
      }

      //draw scoreboard
      draw_rectangle(0,0,240,16,rgbToColour(30,30,100));
      gprintf("Score: %d",score);

      //display level change
      if (last_level_time + 1000000 > current_time && level > 1) {
        draw_rectangle(0,105,240,30,rgbToColour(255,0,0));
        snprintf(level_string,sizeof(level_string),"LEVEL %d",level);
        print_xy(level_string,CENTER,CENTER);
      }

      last_frame_time = current_time;
      flip_frame();


    }

/* 
====================================================
Game Over
----------------------------------------------------
Clears enemies LL, and restarts the bubble background
awaits user input showing them game over, and their 
score
====================================================
*/
  //delete any enemies remaining in the linked list
  clear_list(&enemies);
 // clear_list(&bubbles);

  while(gpio_get_level(0)){


    current_time = esp_timer_get_time();
      
      cls(rgbToColour(100,0,0));

      //animate the bubbles
      if (last_enemy_time + 500000 < current_time) {

        struct Node* temp = NULL;
        temp = (struct Node*)malloc(sizeof(struct Node));

        temp->piece.position = random_start(135,(vec2f) {20,20});
        temp->piece.velocity = (vec2f) {0,20};
        temp->piece.dimensions = (vec2f) {rand()%10+5,0};
        temp->next = NULL;

        if (bubbles.count == 0) {
            bubbles.first = temp;
          } else {
            add_node(bubbles.first,temp);
          }
        last_enemy_time = current_time;
        bubbles.count += 1;
      }

      // //iterate over the linked list of bubbles moving them, then drawing them
      //test to ensure some actually exist before running.
      if (bubbles.first != NULL){
        struct Node* temp = bubbles.first;
        while(temp != NULL) {
          dt = (esp_timer_get_time() - last_frame_time)/1.0e6f;

          temp->piece.position = add_vec(temp->piece.position, mul_vec_by_float(temp->piece.velocity,dt));
      
          draw_circle(temp->piece.dimensions.x,temp->piece.position.x,temp->piece.position.y,rgbToColour(120,0,0));
          draw_circle(temp->piece.dimensions.x-2,temp->piece.position.x,temp->piece.position.y,rgbToColour(135,0,0));
          temp = temp->next;


        }
      }

      //clean up the pieces that have exited the board
      if (bubbles.first != NULL) {
        struct Node* temp = bubbles.first;
        struct Node* to_delete = NULL;

        while(temp != NULL) {

          //advance the temp BEFORE deletion (otherwise you delete it then can't get ->next!)
          to_delete = temp;
          temp = temp->next;

          if (to_delete->piece.position.y >= 240) {
            delete_node(&bubbles,to_delete);
          }
          
        }
      }


    setFont(FONT_DEJAVU24);
    print_xy("GAME",CENTER,20);
    print_xy("OVER",CENTER,LASTY+25);
    setFont(FONT_UBUNTU16);
    
    snprintf(score_string,sizeof(score_string),"Your score: %" PRIu32, score);



    print_xy(score_string,CENTER,LASTY+30);
    
    print_xy("Press A",CENTER,LASTY+30);
    last_frame_time = current_time;
    flip_frame();
   
  }
    //delay to stop it immediately starting a new game
    last_frame_time = esp_timer_get_time();
    while(esp_timer_get_time() < last_frame_time+1000000);


  }

}