#include<soc/uart_struct.h>
#include<esp_timer.h>
#include<graphics.h>
#include<fonts.h>
#include<driver/gpio.h>


//Float vector (used for coordinates and velocities)
typedef struct { float x; float y; } vec2f;

//game pieces
typedef struct Piece {

  //Using 2d vector for ship allows me to reuse similar functions for projectiles
  //even though the main player piece will only move side to side
  //the position marks the 'nose' of the ship (top center)
  vec2f dimensions;
  vec2f position;
  vec2f velocity;
  vec2f accel;

} Piece;

  //creating a linked list to hold enemies
struct Node {
  Piece enemy;
  struct Node* next;
};
struct enemies {
  struct Node* first;
  uint16_t count;
};

void add_node(struct Node* list, struct Node* new) {

  while(list->next != NULL) {
    list= list->next;
  }
  list->next = new;

}

void delete_node(struct Node* head, struct Node* delete) {

  struct Node* temp = head;
  struct Node* prev = head;

  if (head == delete) {

    head = head->next;
    free(temp);

  } else {

    //iterate over the nodes
    while(temp->next != NULL) {

      prev = temp;
      temp = temp->next;

      if (temp == delete) {

        prev->next = temp->next;
        free(temp);

      }  

    }
  }




}


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

vec2f random_start(int screen_width, vec2f dim) {
  return (vec2f) {rand() % (int) screen_width+1-dim.x,0-dim.y};
}

void draw_ship(Piece ship) {

  draw_rectangle(ship.position.x - ship.dimensions.x/2, ship.position.y, ship.dimensions.x, ship.dimensions.y, rgbToColour(255,0,0));

}

bool test_enemy(Piece enemy, float screen_height) {
    return (enemy.dimensions.y >= screen_height);  
  
}
//adapted from https://levelup.gitconnected.com/2d-collision-detection-8e50b6b8b5c0
bool test_collision(Piece a, Piece b) {
  return a.position.x < b.position.x + b.dimensions.x //if a's left most position is less than b's right most
  && a.position.x + a.dimensions.x > b.position.x // while a's right most is greater than b's left most, they must be overlapping on x
  && a.position.y < b.position.y + b.dimensions.y // if a's top most dimension in less than b's bottom most dimension
  && a.position.y + a.dimensions.y > b.position.y;//  while a's bottom most dimension is greater than b's top most then they overlap
}

void draw_enemy(Piece enemy) {

  draw_rectangle(enemy.position.x - enemy.dimensions.x/2, enemy.position.y, enemy.dimensions.x, enemy.dimensions.y, rgbToColour(0,255,0));

}


void app_main() { 

  // =============================================
  // Configurations
  // =============================================

  // configurations for GPIO and graphics initialisations
  gpio_set_direction(0,GPIO_MODE_INPUT);
  gpio_set_direction(35,GPIO_MODE_INPUT);
  graphics_init();
  set_orientation(PORTRAIT);
  
  //game variables
  u_int16_t level = 0;
  u_int32_t score = 0;
  bool crashed;

  // game piece configurations, should be able to be modified in menu
  // position is always the top left corner of the hit box! 
  vec2f ship_dimensions = (vec2f) {20,40};
  vec2f min_ship_pos = (vec2f) {0, 0};
  vec2f max_ship_pos = (vec2f) {135 - ship_dimensions.x, 240 - ship_dimensions.y};
  vec2f max_velocity = (vec2f) {100,0}; //pixels per second essentially
  float thrust_accel = 200; //pixels per second per second
  float drag_decel = 100;

  vec2f enemy_dimesions = (vec2f) {6,20};
  int first_level_enemies = 5;
  int max_enemies = 20;
  vec2f first_level_velocity = (vec2f) {0,10};

  // timer declarations
  u_int64_t current_time, last_enemy_time ,last_frame_time = esp_timer_get_time();
      last_enemy_time = last_frame_time;

  float dt; //declare as float for the vector calcs.


  // =============================================
  // Game
  // =============================================

  // infinite loop through the game menus 
  for(;;){
    crashed = false;
    //draws menu screen and awaits user press of either key
    cls(rgbToColour(75,125,200));
    setFont(FONT_UBUNTU16);
    flip_frame();
    while(gpio_get_level(0));
    //delay start
    while(esp_timer_get_time() < last_frame_time+500000);
    // create ships

    Piece ship;
    ship.velocity = (vec2f) {0,0};
    ship.dimensions = ship_dimensions;
    ship.position = (vec2f) {135/2+1-ship.dimensions.x/2, max_ship_pos.y};

    struct enemies enemies;
    enemies.count = 0;
    enemies.first = NULL;
   
    last_frame_time = esp_timer_get_time();
    srand(last_frame_time);
    // operating game loop continues endlessly until lose condition
    while(!crashed) {

      cls(0);

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
      current_time = esp_timer_get_time();
      dt = (esp_timer_get_time() - last_frame_time)/1.0e6f; //converted to seconds
      ship.velocity = add_vec(ship.velocity,mul_vec_by_float(ship.accel,dt));
      //test that the ship is not going faster than it's max velocity
      ship.velocity = min_vector(ship.velocity,max_velocity);
      //move the ship by it's speed and time travelled
      ship.position = add_vec(ship.position,mul_vec_by_float(ship.velocity,dt));
      ship.position = max_vector(ship.position, min_ship_pos);
      ship.position = min_vector(ship.position, max_ship_pos);

      draw_ship(ship);

      //create enemies if enough time has passed
      if (last_enemy_time+4000000 < current_time) {
          //check there aren't too many on the board
          if (enemies.count <= first_level_enemies + level && enemies.count <= max_enemies) {
            
            struct Node* temp = NULL;
            temp = (struct Node*)malloc(sizeof(struct Node));

            temp->enemy.position = random_start(135,enemy_dimesions);
            temp->enemy.velocity = first_level_velocity;
            temp->enemy.dimensions = enemy_dimesions;
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
        while(temp->next != NULL) {

          //move
          temp->enemy.position = add_vec(temp->enemy.position, mul_vec_by_float(first_level_velocity,dt));
          
          //this has to be set as a result, otherwise it just swaps between states as it scans through each item in the LL
          if (test_collision(temp->enemy,ship)) crashed = true;
          draw_enemy(temp->enemy);
          temp = temp->next;

        }
      }




      //draw scoreboard
      draw_rectangle(0,0,240,16,rgbToColour(30,30,100));
      if (crashed) gprintf("Crash!!");
      last_frame_time = current_time;
      flip_frame();


    }




    //finish screen

    cls(rgbToColour(255,0,0));
    flip_frame();
    while(esp_timer_get_time() < last_frame_time + 2000000);












  }








}