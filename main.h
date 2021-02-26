#ifndef MAIN_C_SEA_BATTLE_H
#define MAIN_C_SEA_BATTLE_H

#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#define MAX_LEN_OF_NAME 35
#define NOT_STARTED_GAME 0
#define RETURN_TO_MAIN_MENU (-1)
#define NO_PREVIOUS_VALUE (-1)

extern int map_size, ships_count;

typedef struct {
    int map_size;
    int ships_count;
    int lengths_of_ships[20];
} Settings;
typedef struct {
    int row;
    int col;
} Square;
typedef struct Ship {
    int          len;
    Square       bow;
    Square       stern;
    struct Ship *next_ship;
} Ship;
typedef struct {
    bool     is_bot;
    int      score;
    int      remaining_ships;
    Ship    *ships;
    char   **revealed_map;
    char   **concealed_map;
    char     name[MAX_LEN_OF_NAME];
    Settings settings;
} Player;
typedef struct {
  int       turn;
  struct tm date;
  char      name[MAX_LEN_OF_NAME];
  Player    player1;
  Player    player2;
  int       shoots_counter;
  Square    shoots[150];
} Game;

enum Menu {MAIN_MENU, PAUSE_MENU, SETTINGS_MENU};
enum Direction {UP, RIGHT, DOWN, LEFT};
enum Placeability {NOT_PLACEABLE, PLACEABLE};

void say_goodbye();

Player   load_player(const char *name, bool is_welcome_message_needed);
Player   create_player(const char *name);
Settings set_defaults();
int      seek_player_by_name(const char *name, FILE *saved_players);

void  main_menu(Player player1);
Game *allocate_an_initial_game(Player player1);
char  print_and_clear_menu(char choice, enum Menu menu);
char  print_menu(enum Menu);

void free_game_pointer(Game* game);
void free_ships(Ship* ships);
void free_map(char** map);

void    scoreboard();
void    copy_players_file(FILE *dest, FILE *source);
void    print_scoreboard(FILE *saved_players);
Player *find_player_with_max_score(FILE *saved_players);

Game       create_game(Player player1, bool is_vs_bot);
struct tm  get_date();
Ship      *load_settings(Settings settings);
void       set_map_size_and_ships_count(Settings settings);
Ship      *create_list_of_ships(const int *lengths_of_ships);
Ship      *copy_ships_list(Ship *source);
void       add_ship(Ship **ships_head, int len_of_ship);
void       do_the_maps_allocations(Player *player1, Player *player2);
char     **allocate_map(char initial_value);

void display_screen_for_placing(Player player);
void display_screen_for_guessing(Player player1, Player player2, bool is_replay);
void gotoxy(int x, int y);
void print_header();
void print_date();
void display_map(char **map, char *name, int score);
void print_columns();
void put_colored_char(char c);
void set_color(int color);

void  resume_previous_games(char *player_name);
Game *load_game(char *player1_name, bool is_replay);
int   display_saved_games(char *player1_name, bool is_replay);
int   get_chosen_game_num(int total_num_of_games);
Game *find_chosen_game(FILE *saved_games, char *player1_name, int choice, bool is_replay);
void  prepare_players_of_loaded_game(Game *game, FILE *saved_games, char *player1_name);
void  prepare_players_of_replay(Game *game, FILE *saved_games);
void  load_4_maps_of_game(Game *game, FILE *saved_games);
void  fread_map(char **map, FILE *saved_games);
void  restore_ships(Ship *ships_head, const char **map);
void  restore_ships_with_equal_length(Ship **ship, const char **map);
Ship *load_ships_list(FILE *saved_games, int remaining_ships);
int   load_score(const char *player1_name, const char *player_to_load_name);

void battle_log(char *player_name);
void replay(Game game);

void play_the_game(Game *game, bool is_a_loaded_game);
int  game_loop(Game *game);
bool is_game_ended(Ship *player1_ships, Ship *player2_ships);
bool is_human_turn(int turn, bool player2_is_human);
int  control_player_turn(Game *game);
void get_command(Player player1, Player player2, char command[], int turn);

void how_to_place_ships(Player *player);

void           auto_arrange_map(Player *player);
void           clear_map(char **map);
void           fill_empty_squares_with_water(char **map);
bool           is_refused();
void           auto_place_ships(Player *player);
void           auto_place_ship_longer_than_1(const Player *player, Ship *ship, int remained_squares);
Square         rand_square(char **map, int remained_squares);
void           reveal_ship(Ship ship, char **map, char S_or_C);
void           find_placeable_dirs(Square square, enum Placeability *directions, char **map, int len);
int            count_placeable_dirs(const enum Placeability directions[]);
enum Direction rand_dir(const enum Placeability directions[], int placeable_dirs_count);
Square         find_stern(Square bow, enum Direction chosen_direction, int len);
int            count_char(char **map, char c);

void manually_place_ships(Player *player);
void get_bow_and_stern(char bow[], char stern[], int ship_len, int ship_num);
bool is_valid_ship_placement(Ship *ship, char **map, char *chosen_bow, char *chosen_stern);
bool is_placeable_square(char **map, char *chosen_square, Square *square_coordinates);
int  square_compare(Square square1, Square square2);
void square_swap(Square *square1, Square *square2);

void   play_for_bot(Game *game);
Square shoot_for_bot(char **opponent_map);
Square find_explosion(char **map, int previous_row, int previous_col);
Square follow_explosions(char **map);
int    is_vertical(Square square1, Square square2);

void update_game(Game *game, Square shoot);
void apply_changes(Player *opponent, Square shoot, int *attacker_score, const int *lengths_of_ships);
bool is_ship_sunk(Ship attacked_ship, char **concealed_map);
Ship find_attacked_ship(Ship *opponents_ships, Square shoot);
void delete_ship(Ship **ships_head, Square bow);
int  win_bonus(const int *lengths_of_ships);
int  max_len_of_ships(const int *lengths_of_ships, bool is_the_game_ended);

void switch_turn(Game *game, Square shoot);

int  get_choice_from_pause_menu(Game *game);
void ask_to_save_game(Game game);
void save_game(Game game, bool is_replay);
void save_ships_list(Game game, FILE *saved_games);
void save_4_maps_of_game(Game game, FILE *saved_games);
void fwrite_map(char **map, FILE *file_to_write_to);

void end_game(Game *game);
void update_players_in_file(Player *player1, Player *player2);
void game_over_message(Game game);
void game_over_menu(Game *game);

void settings_menu(Player *player);
void change_map_size(Player *player);
void change_ships_settings(Player *player);
int  integer_compare_descending(const void *p_int1, const void *p_int2);
int  get_map_size_or_ships_count(int choice_from_settings_menu);

void hide_cursor();
void show_cursor();

void handle_error();

#endif //MAIN_C_SEA_BATTLE_H
