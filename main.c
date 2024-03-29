#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef WIN32
#include <windows.h>
#include <conio.h>
#endif

#include "main.h"

int map_size, ships_count;

int main() {
    system(CLEAR_SCREEN);
    atexit(say_goodbye);
    srand(time(NULL));

    printf("Sea battle\n"
           "By Armin Ghorbanian\n\n"
           "What shall the history call you? ");
    main_menu(load_player(get_player_name(), true));
    return 0;
}

void say_goodbye() {
    system(CLEAR_SCREEN);
    printf("Hope to see you soon.\n");
    SLEEP(2000);
}

char *get_player_name() {
    char *name = (char *) malloc(sizeof(char) * MAX_LEN_OF_NAME);
    if (name == NULL) handle_error();
    fflush(stdin);
    gets(name);
    return name;
}


Player load_player(char *name, bool is_welcome_message_needed) {
    FILE *saved_players = fopen(PLAYERS_FILENAME, "r+b");
    if (saved_players == NULL || seek_player_by_name(name, saved_players) == -1) {
        if (saved_players != NULL) fclose(saved_players);
        printf("Welcome to sea battle %s!\n", name);
        SLEEP(2000);
        return create_player(name);
    }

    Player player;
    fread(&player, sizeof(Player), 1, saved_players);
    fclose(saved_players);
    if (is_welcome_message_needed) printf("Welcome back %s.\n", name);
    SLEEP(2000);

    free(name);
    return player;
}

Player create_player(const char *name) {
    Player player = {};
    strcpy(player.name, name);
    player.settings = set_defaults();

    FILE *saved_players = fopen(PLAYERS_FILENAME, "ab");
    if (saved_players == NULL) handle_error();
    fwrite(&player, sizeof(player), 1, saved_players);
    fclose(saved_players);

    return player;
}

Settings set_defaults() {
    Settings settings = {.map_size = 10, .ships_count = 10};

    settings.lengths_of_ships[0] = 4;
    settings.lengths_of_ships[1] = settings.lengths_of_ships[2] = 3;
    settings.lengths_of_ships[3] = settings.lengths_of_ships[4] = settings.lengths_of_ships[5] = 2;
    settings.lengths_of_ships[6] = settings.lengths_of_ships[7] = settings.lengths_of_ships[8]
            = settings.lengths_of_ships[9] = 1;

    return settings;
}

int seek_player_by_name(const char *name, FILE *saved_players) {
    rewind(saved_players);
    Player player;
    for (int offset = 0; fread(&player, sizeof(Player), 1, saved_players) == 1; ++offset) {
        if (strcmp(player.name, name) == 0) {
            fseek(saved_players, (long) -sizeof(Player), SEEK_CUR);
            fflush(saved_players);
            return offset * (int) sizeof(Player);
        }
    }

    return -1;
}


void main_menu(Player player1) {
    Game *game = allocate_an_initial_game(player1);
    int choice = 1;
    for (;;) {
        switch (choice = print_menu(choice, MAIN_MENU)) {
            case 1:
            case 2:
                *game = create_game(game->player1, choice == 1 ? true : false);
                play_the_game(game, false);
                break;
            case 3: resume_previous_games(game->player1.name); break;
            case 4: battle_log(game->player1.name);            break;
            case 5: scoreboard();                              break;
            case 6: settings_menu(&game->player1);             break;
            case 0: free_game_pointer(game);                   return;
        }
    }
}

Game *allocate_an_initial_game(Player player1) {
    Game *game = (Game *) malloc(sizeof(Game));
    if (game == NULL) handle_error();

    game->player1 = player1;
    game->turn = NOT_STARTED_GAME;

    return game;
}

int print_menu(int choice, enum Menu menu) {
    if (1 <= choice && choice <= (menu == MAIN_MENU ? 6 : menu == PAUSE_MENU ? 4 : 3)) {
        system(CLEAR_SCREEN);
        printf(menu == MAIN_MENU ? "1.Play vs computer\n"
                                   "2.Play vs a friend\n"
                                   "3.Load a game\n"
                                   "4.Battle log\n"
                                   "5.Scoreboard\n"
                                   "6.Settings\n"
                                   "0.Exit\n"
                                 : menu == PAUSE_MENU ? "1.Main menu\n"
                                                        "2.Save\n"
                                                        "3.Resume\n"
                                                        "4.Quit game\n"
                                                      : "1.Change map size.\n"
                                                        "2.Change ships settings.\n"
                                                        "3.Restore default settings.\n"
                                                        "4.Main menu.\n");
        if ('1' <= (choice = GET_CHAR()) && choice <= (menu == MAIN_MENU ? '6' : '4')) system(CLEAR_SCREEN);
        return choice - '0';
    }

    return GET_CHAR() - '0';
}

void free_game_pointer(Game *game) {
    if (game == NULL) return;
    if (game->turn != NOT_STARTED_GAME) {
        free_ships(game->player1.ships);
        free_ships(game->player2.ships);

        free_map(game->player1.revealed_map);
        free_map(game->player2.revealed_map);
        free_map(game->player1.concealed_map);
        free_map(game->player2.concealed_map);
    }

    free(game);
}

void free_ships(Ship *ships) {
    Ship *temp;
    while (ships != NULL) {
        temp = ships;
        ships = ships->next_ship;
        free(temp);
    }
}

void free_map(char **map) {
    if (map == NULL) return;
    for (int row = 0; row < map_size; ++row)
        free(map[row]);
    free(map);
}


void scoreboard() {
    FILE *saved_players = fopen(PLAYERS_FILENAME, "r+b");
    FILE *temp = fopen("tmp.bin", "w+b");
    if (saved_players == NULL || temp == NULL) handle_error();

    copy_players_file(temp, saved_players);
    print_scoreboard(saved_players);
    copy_players_file(saved_players, temp);

    fclose(temp);
    fclose(saved_players);
    remove("tmp.bin");

    printf("Press any key to return to main menu. ");
    GET_CHAR();
}

void copy_players_file(FILE *dest, FILE *source) {
    fflush(dest);
    fflush(source);
    rewind(dest);
    rewind(source);

    for (Player tmp; fread(&tmp, sizeof(Player), 1, source) == 1; fwrite(&tmp, sizeof(Player), 1, dest));
}

void print_scoreboard(FILE *saved_players) {
    Player *player;
    for (int i = 1; (player = find_player_with_max_score(saved_players)) != NULL; ++i) {
        printf("%2d- %-*s score: %d\n", i, MAX_LEN_OF_NAME, player->name, player->score);

        player->score = -1;
        seek_player_by_name(player->name, saved_players);
        fwrite(player, sizeof(Player), 1, saved_players);
        fflush(saved_players);
    }
}

Player *find_player_with_max_score(FILE *saved_players) {
    rewind(saved_players);

    int max_score = -1;
    static Player player_with_max_score;
    for (Player temp; fread(&temp, sizeof(Player), 1, saved_players) == 1;) {
        if (max_score < temp.score) {
            max_score = temp.score;
            player_with_max_score = temp;
        }
    }

    return max_score == -1 ? NULL : &player_with_max_score;
}


Game create_game(Player player1, bool is_vs_bot) {
    Game game = {.turn = 1, .date = get_date(), .player1 = player1};
    if (is_vs_bot) {
        strcpy(game.player2.name, "Computer");
        game.player2.is_bot = true;
    } else {
        printf("What shall your friend be called throughout the history? ");
        game.player2 = load_player(get_player_name(), true);
    }

    game.player1.ships = load_settings(player1.settings);
    game.player2.ships = copy_ships_list(game.player1.ships);
    game.player1.remaining_ships = game.player2.remaining_ships = ships_count;

    do_the_maps_allocations(&game.player1, &game.player2);

    return game;
}

struct tm get_date() {
    time_t now = time(NULL);
    return *localtime(&now);
}

Ship *load_settings(Settings settings) {
    set_map_size_and_ships_count(settings);
    return create_list_of_ships(settings.lengths_of_ships);
}

void set_map_size_and_ships_count(Settings settings) {
    map_size = settings.map_size;
    ships_count = settings.ships_count;
}

Ship *create_list_of_ships(const int *lengths_of_ships) {
    Ship *ships = NULL;
    for (int i = 0; i < ships_count; ++i)
        add_ship(&ships, lengths_of_ships[i]);

    return ships;
}

void add_ship(Ship **ships_head, int len_of_ship) {
    if (*ships_head == NULL) {
        if ((*ships_head = (Ship *) malloc(sizeof(Ship))) == NULL) handle_error();
        (*ships_head)->len = len_of_ship;
        (*ships_head)->next_ship = NULL;
    } else {
        Ship *ship = *ships_head;
        while (ship->next_ship) ship = ship->next_ship;

        if ((ship->next_ship = (Ship *) malloc(sizeof(Ship))) == NULL) handle_error();
        ship->next_ship->len = len_of_ship;
        ship->next_ship->next_ship = NULL;
    }
}

void do_the_maps_allocations(Player *player1, Player *player2) {
    player1->revealed_map = allocate_map(NOT_REVEALED);
    player2->revealed_map = allocate_map(NOT_REVEALED);
    player1->concealed_map = allocate_map(NOT_REVEALED);
    player2->concealed_map = allocate_map(NOT_REVEALED);
}

char **allocate_map(char initial_value) {
    char **map = (char **) malloc(map_size * sizeof(char *));
    if (map == NULL) handle_error();
    for (int row = 0; row < map_size; ++row) {
        if ((map[row] = (char *) malloc(map_size * sizeof(char))) == NULL) handle_error();
        memset(map[row], initial_value, map_size * sizeof(char));
    }

    return map;
}

Ship *copy_ships_list(Ship *source) {
    Ship *dest = NULL;
    while (source != NULL) {
        add_ship(&dest, source->len);
        source = source->next_ship;
    }

    return dest;
}


void display_screen_for_placing(Player player) {
    hide_cursor();
    gotoxy(0, 0);
    print_header();
    display_map(player.revealed_map, player.name, player.score);
    printf("%100s\n%100s\n%100s", "", "", "");                  // Deletes three previously printed lines.
    gotoxy(0, map_size + 6);
    show_cursor();
}

void display_screen_for_guessing(Player player1, Player player2, bool is_replay) {
    if (!is_replay) hide_cursor();
    gotoxy(0, 0);
    print_header();
    display_map(player1.concealed_map, player1.name, is_replay ? -1 : player1.score);
    display_map(player2.concealed_map, player2.name, player2.is_bot || is_replay ? -1 : player2.score);
    printf("%100s\n%100s\n%100s", "", "", "");                  // Deletes three previously printed lines.
    gotoxy(0, 2 * map_size + 9);
    if (!is_replay) show_cursor();
}

void gotoxy(int x, int y) {
#ifdef WIN32
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), (COORD) {x, y});
#endif
}

void print_header() {
    int line_len = printf("Subject: Sea Battle%*sDate: ", map_size * 2, "") + 9;
    print_date(get_date());
    putchar('\n');
    while (line_len--) putchar('-');
    printf("\n\n");
}

void print_date(struct tm date) {
    printf("%d/%d/%d", date.tm_mon + 1, date.tm_mday, date.tm_year + 1900);
}

void display_map(char **map, char *name, int score) {
    printf(score == -1 ? "%s\n" : "%s%*s score: %d\n", name, 3 * map_size - (int) strlen(name) - 7, "", score);
    print_columns();
    for (int row = 0; row < map_size; ++row) {
        putchar(row + 'A');
        for (int col = 0; col < map_size; ++col) {
            printf("  ");
            put_colored_char(map[row][col]);
        }
        putchar('\n');
    }
    putchar('\n');
}

void print_columns() {
    printf("   ");
    for (int col = 1; col <= map_size; ++col)
        printf(col >= 10 ? "%d " : "%d  ", col);
    putchar('\n');
}

void put_colored_char(char c) {
#ifdef WIN32
    enum ColorCmd {
        BLUE = 1, GREEN = 10, RED = 4, YELLOW = 6, GREY = 8, WHITE = 7
    };
    switch (c) {
        case WATER: set_text_color(BLUE); break;
        case SHIP: set_text_color(GREEN); break;
        case EXPLOSION: set_text_color(RED); break;
        case CAPSIZED_SHIP: set_text_color(YELLOW); break;
        case NOT_REVEALED: set_text_color(GREY); break;
        default: set_text_color(WHITE);
    }
    putchar(c);
    set_text_color(WHITE);
#endif
}

void set_text_color(int color) {
#ifdef WIN32
    WORD wColor;
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hStdOut, &csbi)) {
        wColor = (csbi.wAttributes & 0xF0) + (color & 0x0F);
        SetConsoleTextAttribute(hStdOut, wColor);
    }
#endif
}


void resume_previous_games(char *player_name) {
    Game *game = (Game *) malloc(sizeof(Game));
    if ((game = load_game(player_name, false)) != NULL) {
        play_the_game(game, true);
        free_game_pointer(game);
    }
}

Game *load_game(char *player1_name, bool is_replay) {
    int total_num_of_games = display_saved_games(player1_name, is_replay);
    if (total_num_of_games == 0) {
        printf(is_replay ? "You haven't completed any battle yet.\n" : "No saved games were found.\n");
        SLEEP(1000);
        return NULL;
    }

    int choice = get_chosen_game_num(total_num_of_games);
    system(CLEAR_SCREEN);

    FILE *saved_games = fopen(is_replay ? REPLAYS_FILENAME : GAMES_FILENAME, "rb");
    if (saved_games == NULL) handle_error();
    Game *game = find_chosen_game(saved_games, player1_name, choice, is_replay);

    set_map_size_and_ships_count(game->player1.settings);
    do_the_maps_allocations(&game->player1, &game->player2);
    is_replay ? prepare_players_of_replay(game, saved_games)
              : prepare_players_of_loaded_game(game, saved_games, player1_name);

    fclose(saved_games);
    return game;
}

int display_saved_games(char *player1_name, bool is_replay) {
    FILE *saved_games = fopen(is_replay ? REPLAYS_FILENAME : GAMES_FILENAME, "rb");
    if (saved_games == NULL) return 0;

    int game_counter = 1;
    for (Game game;
         fread(&game, sizeof(Game), 1, saved_games) == 1;
         fseek_to_next_game(&game, saved_games, is_replay)) {
        if (strcmp(game.player1.name, player1_name) == 0 || strcmp(game.player2.name, player1_name) == 0) {
            printf("%2d-%-*s ", game_counter++, MAX_LEN_OF_NAME, game.name);
            print_date(game.date);
            printf(" - %s vs %s\n", game.player1.name, game.player2.name);
        }
    }
    putchar('\n');

    fclose(saved_games);
    return game_counter - 1;
}

int get_chosen_game_num(int total_num_of_games) {
    int choice;
    for (;;) {
        printf("Choose number of a game: ");
        scanf("%d", &choice);
        if (1 <= choice && choice <= total_num_of_games) return choice;
        printf("The choice must be between 1 and %d\n", total_num_of_games);
    }
}

Game *find_chosen_game(FILE *saved_games, char *player1_name, int choice, bool is_replay) {
    Game *game = (Game *) malloc(sizeof(Game));
    if (game == NULL) handle_error();
    for (int i = 0; fread(game, sizeof(Game), 1, saved_games); fseek_to_next_game(game, saved_games, is_replay)) {
        if (strcmp(game->player1.name, player1_name) == 0 || strcmp(game->player2.name, player1_name) == 0)
            if (++i == choice) return game;
    }
}

void fseek_to_next_game(Game *game, FILE *saved_games, bool is_replay) {
    long sizeof_map = (long) sizeof(char) * game->player1.settings.map_size * game->player1.settings.map_size;
    long offset = is_replay ? 2 * sizeof_map
                            : 4 * sizeof_map
                              + (long) sizeof(Ship) * (game->player1.remaining_ships + game->player2.remaining_ships);
    fseek(saved_games, offset, SEEK_CUR);
}

void prepare_players_of_loaded_game(Game *game, FILE *saved_games, char *player1_name) {
    game->player1.score = load_score(player1_name, game->player1.name);
    if (!game->player2.is_bot) game->player2.score = load_score(player1_name, game->player2.name);

    load_4_maps_of_game(game, saved_games);

    game->player1.ships = load_ships_list(saved_games, game->player1.remaining_ships);
    game->player2.ships = load_ships_list(saved_games, game->player2.remaining_ships);
}

void prepare_players_of_replay(Game *game, FILE *saved_games) {
    game->player1.remaining_ships = game->player2.remaining_ships = game->player1.settings.ships_count;
    game->turn = 1;
    game->shoots_counter = 0;

    fread_map(game->player1.revealed_map, saved_games);
    fread_map(game->player2.revealed_map, saved_games);

    game->player1.ships = create_list_of_ships(game->player1.settings.lengths_of_ships);
    game->player2.ships = copy_ships_list(game->player1.ships);

    restore_ships(game->player1.ships, (const char **) game->player1.revealed_map);
    restore_ships(game->player2.ships, (const char **) game->player2.revealed_map);
}

void load_4_maps_of_game(Game *game, FILE *saved_games) {
    fread_map(game->player1.revealed_map, saved_games);
    fread_map(game->player2.revealed_map, saved_games);
    fread_map(game->player1.concealed_map, saved_games);
    fread_map(game->player2.concealed_map, saved_games);
}

void fread_map(char **map, FILE *saved_games) {
    for (int i = 0; i < map_size; ++i)
        fread(map[i], sizeof(char), map_size, saved_games);
}

void restore_ships(Ship *ships_head, const char **map) {
    for (; ships_head != NULL; restore_ships_with_equal_length(&ships_head, map));
}

void restore_ships_with_equal_length(Ship **ship, const char **map) {
    for (int row = 0; row < map_size; ++row) {
        for (int col = 0; col < map_size; ++col) {
            if (map[row][col] == SHIP) {
                if ((row - 1 >= 0 && map[row - 1][col] == SHIP) || (col - 1 >= 0 && map[row][col - 1] == SHIP))
                    continue;
                if ((*ship)->len == 1) {
                    if ((row + 1 > map_size - 1 || map[row + 1][col] == WATER)
                            && (col + 1 > map_size - 1 || map[row][col + 1] == WATER)) {
                        (*ship)->bow = (*ship)->stern = (Square) {row, col};
                        (*ship) = (*ship)->next_ship;
                        if (*ship == NULL) return;
                    }
                    continue;
                }
                if (row + (*ship)->len - 1 < map_size && map[row + (*ship)->len - 1][col] == SHIP
                        && (row + (*ship)->len > map_size - 1 || map[row + (*ship)->len][col] == WATER)) {
                    int k = row + 1, found = 1;
                    for (int l = (*ship)->len - 2; l--; ++k) {
                        if (map[k][col] != SHIP) found = 0;
                    }
                    if (found) {
                        (*ship)->bow = (Square) {row, col};
                        (*ship)->stern = (Square) {k, col};

                        if ((*ship)->next_ship->len != (*ship)->len) {
                            (*ship) = (*ship)->next_ship;
                            return;
                        }
                        (*ship) = (*ship)->next_ship;
                        continue;
                    }
                }
                if (col + (*ship)->len - 1 < map_size && map[row][col + (*ship)->len - 1] == SHIP
                        && (col + (*ship)->len > map_size - 1 || map[row][col + (*ship)->len] == WATER)) {
                    int copy_of_col = col++, found = 1;
                    for (int l = (*ship)->len - 2; l--; ++col) {
                        if (map[row][col] != SHIP) found = 0;
                    }
                    if (!found) {
                        col = copy_of_col;
                        continue;
                    }

                    (*ship)->bow = (Square) {row, copy_of_col};
                    (*ship)->stern = (Square) {row, col};

                    if ((*ship)->next_ship->len != (*ship)->len) {
                        (*ship) = (*ship)->next_ship;
                        return;
                    }
                    (*ship) = (*ship)->next_ship;
                }
            }
        }
    }
}

Ship *load_ships_list(FILE *saved_games, int remaining_ships) {
    Ship *ships, *temp;
    if ((ships = temp = (Ship *) malloc(sizeof(Ship))) == NULL) handle_error();
    fread(ships, sizeof(Ship), 1, saved_games);

    for (int i = 1; i < remaining_ships; ++i, temp = temp->next_ship) {
        if ((temp->next_ship = (Ship *) malloc(sizeof(Ship))) == NULL) handle_error();
        fread(temp->next_ship, sizeof(Ship), 1, saved_games);
    }
    temp->next_ship = NULL;

    return ships;
}

int load_score(char *player1_name, char *player_to_load_name) {
    return load_player(player_to_load_name, strcmp(player_to_load_name, player1_name) == 0 ? false : true).score;
}


void battle_log(char *player_name) {
    Game *game = load_game(player_name, true);
    if (game != NULL) replay(*game);
}

void replay(Game game) {
    hide_cursor();
    while (!is_game_ended(game.player1.ships, game.player2.ships)) {
        display_screen_for_guessing(game.player1, game.player2, true);
        SLEEP(100);

        update_game(&game, game.shoots[game.shoots_counter]);
        switch_turn(&game, game.shoots[game.shoots_counter++]);
    }
    display_screen_for_guessing(game.player1, game.player2, true);

    show_cursor();
    game_over_message(game);
    printf("\nPress any key to return to main menu. ");
    GET_CHAR();
}


void play_the_game(Game *game, bool is_a_loaded_game) {
    if (!is_a_loaded_game) {
        how_to_place_ships(&game->player1);
        game->player2.is_bot ? auto_arrange_map(&game->player2) : how_to_place_ships(&game->player2);
    }
    if (game_loop(game) == RETURN_TO_MAIN_MENU) return;
    end_game(game);
}

int game_loop(Game *game) {
    while (!is_game_ended(game->player1.ships, game->player2.ships)) {
        if (is_human_turn(game->turn, !game->player2.is_bot)) {
            if (control_player_turn(game) == RETURN_TO_MAIN_MENU) return RETURN_TO_MAIN_MENU;
        } else
            play_for_bot(game);
    }
    display_screen_for_guessing(game->player1, game->player2, false);

    return 0;
}

bool is_game_ended(Ship *player1_ships, Ship *player2_ships) {
    return player1_ships == NULL || player2_ships == NULL;
}

bool is_human_turn(int turn, bool player2_is_human) {
    return turn % 2 == 1 || player2_is_human;
}

int control_player_turn(Game *game) {
    display_screen_for_guessing(game->player1, game->player2, false);

    char command[50];
    get_command(game->player1, game->player2, command, game->turn);

    if (strcmp(command, "pause") == 0) {
        if (get_choice_from_pause_menu(game) == RETURN_TO_MAIN_MENU) return RETURN_TO_MAIN_MENU;
    } else if (is_placeable_square(game->turn % 2 ? game->player2.concealed_map : game->player1.concealed_map,
                                   command, &game->shoots[game->shoots_counter])) {
        update_game(game, game->shoots[game->shoots_counter]);
        switch_turn(game, game->shoots[game->shoots_counter++]);
    } else {
        printf("Invalid shoot or syntax. Please try again.\n");
        SLEEP(1000);
    }

    return 0;
}

void get_command(Player player1, Player player2, char command[], int turn) {
    if (!player2.is_bot) printf("%s's turn.\n", turn % 2 ? player1.name : player2.name);
    printf("Choose a square to shoot (e.g. A1) or cease fire by entering \"pause\": ");
    scanf(" %[^\n]s", command);
}

void how_to_place_ships(Player *player) {
    system(CLEAR_SCREEN);
    printf("Place ships %s:\n"
           "1.Auto\n"
           "2.Manually\n", player->name);
    for (;;) {
        int choice = GET_CHAR();
        switch (choice) {
            case '1':
                system(CLEAR_SCREEN);
                auto_arrange_map(player);
                system(CLEAR_SCREEN);
                return;
            case '2':
                system(CLEAR_SCREEN);
                manually_place_ships(player);
                system(CLEAR_SCREEN);
                return;
        }
    }
}

void auto_arrange_map(Player *player) {
    do {
        clear_map(player->revealed_map);

        auto_place_ships(player);
        fill_empty_squares_with_water(player->revealed_map);

        if (player->is_bot) return;

        display_screen_for_placing(*player);
    } while (is_refused());
}

void clear_map(char **map) {
    if (map[0][0] != NOT_REVEALED) {
        for (int row = 0; row < map_size; ++row)
            memset(map[row], NOT_REVEALED, sizeof(char) * map_size);
    }
}

void fill_empty_squares_with_water(char **map) {
    for (int row = 0; row < map_size; ++row) {
        for (int col = 0; col < map_size; ++col)
            if (map[row][col] == NOT_REVEALED) map[row][col] = WATER;
    }
}

bool is_refused() {
    printf("Confirm? (y/n) ");
    for (;;) {
        int choice = GET_CHAR();
        if (choice == 'y' || choice == 'Y') return false;
        if (choice == 'n' || choice == 'N') return true;
    }
}

void auto_place_ships(Player *player) {
    int remained_squares = map_size * map_size;
    Ship *ship = player->ships;
    for (int i = 0; i < ships_count; ++i, ship = ship->next_ship) {
        if (ship->len == 1) {
            ship->stern = ship->bow = rand_square(player->revealed_map, remained_squares);
            reveal_ship(*ship, player->revealed_map, SHIP);
        } else
            auto_place_ship_longer_than_1(player, ship, remained_squares);

        remained_squares = count_char(player->revealed_map, NOT_REVEALED);
    }
}

void auto_place_ship_longer_than_1(const Player *player, Ship *ship, int remained_squares) {
    int placeable_dirs_count;
    do {
        ship->bow = rand_square(player->revealed_map, remained_squares);

        // I have assumed that the stern is always righter or downer than the bow.
        enum Placeability placeable_dirs[] = {NOT_PLACEABLE, PLACEABLE, PLACEABLE, NOT_PLACEABLE};
        find_placeable_dirs(ship->bow, placeable_dirs, player->revealed_map, ship->len - 1);

        if ((placeable_dirs_count = count_placeable_dirs(placeable_dirs)) > 0) {
            ship->stern =
                    find_stern(ship->bow, rand_dir(placeable_dirs, placeable_dirs_count), ship->len - 1);
            reveal_ship(*ship, player->revealed_map, SHIP);
        }
    } while (placeable_dirs_count == 0);
}

Square rand_square(char **map, int remained_squares) {
    int random_num = rand() % remained_squares;
    for (int row = 0; row < map_size; ++row) {
        for (int col = 0; col < map_size; ++col)
            if (map[row][col] == NOT_REVEALED && random_num-- == 0) return (Square) {row, col};
    }
}

void reveal_ship(Ship ship, char **map, enum MapCell ship_sign) {
    int from_row = (ship.bow.row - 1 >= 0) ? ship.bow.row - 1 : ship.bow.row;
    int from_col = (ship.bow.col - 1 >= 0) ? ship.bow.col - 1 : ship.bow.col;
    int to_row = (ship.stern.row + 1 < map_size) ? ship.stern.row + 1 : ship.stern.row;
    int to_col = (ship.stern.col + 1 < map_size) ? ship.stern.col + 1 : ship.stern.col;
    for (int row = from_row; row <= to_row; ++row) {
        for (int col = from_col; col <= to_col; ++col) {
            if (ship.bow.row <= row && row <= ship.stern.row
                    && ship.bow.col <= col && col <= ship.stern.col)
                map[row][col] = ship_sign;
            else
                map[row][col] = WATER;
        }
    }
}

void find_placeable_dirs(Square square, enum Placeability *directions, char **map, int len) {
    if (directions[UP] == PLACEABLE && square.row - len >= 0) {
        for (int l = len, row = square.row - 1; l--; --row)
            if (map[row][square.col] != NOT_REVEALED) directions[UP] = NOT_PLACEABLE;
    } else
        directions[UP] = NOT_PLACEABLE;

    if (directions[RIGHT] == PLACEABLE && square.col + len < map_size) {
        for (int l = len, col = square.col + 1; l--; ++col)
            if (map[square.row][col] != NOT_REVEALED) directions[RIGHT] = NOT_PLACEABLE;
    } else
        directions[RIGHT] = NOT_PLACEABLE;

    if (directions[DOWN] == PLACEABLE && square.row + len < map_size) {
        for (int l = len, row = square.row + 1; l--; ++row)
            if (map[row][square.col] != NOT_REVEALED) directions[DOWN] = NOT_PLACEABLE;
    } else
        directions[DOWN] = NOT_PLACEABLE;

    if (directions[LEFT] == PLACEABLE && square.col - len >= 0) {
        for (int l = len, col = square.col - 1; l--; --col)
            if (map[square.row][col] != NOT_REVEALED) directions[LEFT] = NOT_PLACEABLE;
    } else
        directions[LEFT] = NOT_PLACEABLE;
}

int count_placeable_dirs(const enum Placeability directions[]) {
    int count = 0;
    for (enum Direction i = 0; i < 4; ++i)
        if (directions[i] == PLACEABLE) ++count;

    return count;
}

enum Direction rand_dir(const enum Placeability directions[], int placeable_dirs_count) {
    int random_dir = rand() % placeable_dirs_count + 1;

    enum Direction i;
    for (i = UP; random_dir != 0; ++i)
        if (directions[i] == PLACEABLE) random_dir--;

    return i - 1;
}

Square find_stern(Square bow, enum Direction chosen_direction, int len) {
    switch (chosen_direction) {
        case UP: return (Square) {bow.row - len, bow.col};
        case RIGHT: return (Square) {bow.row, bow.col + len};
        case DOWN: return (Square) {bow.row + len, bow.col};
        case LEFT: return (Square) {bow.row, bow.col - len};
    }
}

int count_char(char **map, char c) {
    int count = 0;
    for (int row = 0; row < map_size; ++row) {
        for (int col = 0; col < map_size; ++col)
            if (map[row][col] == c) ++count;
    }

    return count;
}


void manually_place_ships(Player *player) {
    do {
        clear_map(player->revealed_map);
        Ship *ship = player->ships;
        for (int i = 0; i < ships_count; ++i) {
            display_screen_for_placing(*player);

            char chosen_bow[30], chosen_stern[30];
            get_bow_and_stern(chosen_bow, chosen_stern, ship->len, i + 1);

            if (is_valid_ship_placement(ship, player->revealed_map, chosen_bow, chosen_stern)) {
                if (square_compare(ship->stern, ship->bow) == 1) square_swap(&ship->bow, &ship->stern);
                reveal_ship(*ship, player->revealed_map, SHIP);
                ship = ship->next_ship;
            } else {
                printf("Invalid placement or syntax. Please try again.\n");
                SLEEP(2000);
                i--;
            }
        }
        fill_empty_squares_with_water(player->revealed_map);
        display_screen_for_placing(*player);
    } while (is_refused());
}

void get_bow_and_stern(char bow[], char stern[], int ship_len, int ship_num) {
    printf(ship_len == 1 ? "Enter a square to place ship%d with length %d (e.g. A2): "
                         : "Enter bow and stern of ship%d with length %d (e.g. A1 A%d): ",
           ship_num, ship_len, ship_len);
    scanf(ship_len == 1 ? "%s" : "%s %s", bow, stern);
    if (ship_len == 1) strcpy(stern, bow);
}

bool is_valid_ship_placement(Ship *ship, char **map, char *chosen_bow, char *chosen_stern) {
    if (!is_placeable_square(map, chosen_bow, &ship->bow)
            ||!is_placeable_square(map, chosen_stern, &ship->stern))
        return false;

    if (square_compare(ship->stern, ship->bow) == 1) square_swap(&ship->stern, &ship->bow);

    if (is_vertical(ship->bow, ship->stern) == 1) {
        if (ship->stern.row - ship->bow.row != ship->len - 1) return false;
        for (int row = ship->bow.row + 1; row < ship->stern.row; ++row)
            if (map[row][ship->bow.col] != NOT_REVEALED) return false;
    } else if (is_vertical(ship->bow, ship->stern) == -1) {
        if (ship->stern.col - ship->bow.col != ship->len - 1) return false;
        for (int col = ship->bow.col + 1; col < ship->stern.col; ++col)
            if (map[ship->bow.row][col] != NOT_REVEALED) return false;
    } else
        return false;

    return true;
}

bool is_placeable_square(char **map, char *chosen_square, Square *square_coordinates) {
    square_coordinates->row = isupper(chosen_square[0]) ? chosen_square[0] - 'A' : chosen_square[0] - 'a';
    sscanf(chosen_square + 1, "%d", &square_coordinates->col);

    if (strlen(chosen_square) > 3
            ||!isalpha(chosen_square[0])
            ||!isdigit(chosen_square[1])
            || (strlen(chosen_square) == 3 &&!isdigit(chosen_square[2]))
            || square_coordinates->row >= map_size
            || --square_coordinates->col >= map_size
            || square_coordinates->row < 0
            || square_coordinates->col < 0
            || map[square_coordinates->row][square_coordinates->col] != NOT_REVEALED)
        return false;

    return true;
}

int square_compare(Square square1, Square square2) {
    return (square1.row < square2.row || square1.col < square2.col) ? 1 : -1;
}

void square_swap(Square *square1, Square *square2) {
    Square temp = *square1;
    *square1 = *square2;
    *square2 = temp;
}


void play_for_bot(Game *game) {
    Square shoot = shoot_for_bot(game->player1.concealed_map);
    game->shoots[game->shoots_counter++] = shoot;

    switch_turn(game, shoot);

    display_screen_for_guessing(game->player1, game->player2, false);
    apply_changes(&game->player1, shoot, NULL, game->player1.settings.lengths_of_ships);
    SLEEP(1000);
    display_screen_for_guessing(game->player1, game->player2, false);
    printf("Computer shot at %c%d.\n", shoot.row + 65, shoot.col + 1);
    SLEEP(1000);
}

Square shoot_for_bot(char **opponent_map) {
    switch (count_char(opponent_map, EXPLOSION)) {
        case 0: return rand_square(opponent_map, count_char(opponent_map, NOT_REVEALED));
        case 1: {
            Square explosion_square = find_explosion(opponent_map, NO_PREVIOUS_VALUE, NO_PREVIOUS_VALUE);

            enum Placeability placeable_dirs[] = {PLACEABLE, PLACEABLE, PLACEABLE, PLACEABLE};
            find_placeable_dirs(explosion_square, placeable_dirs, opponent_map, 1);

            return find_stern(explosion_square, rand_dir(placeable_dirs, count_placeable_dirs(placeable_dirs)), 1);
        }
        default: return follow_explosions(opponent_map);
    }
}

Square find_explosion(char **map, int previous_row, int previous_col) {
    for (int row = 0; row < map_size; ++row) {
        for (int col = 0; col < map_size; ++col)
            if (map[row][col] == EXPLOSION && (row != previous_row || col != previous_col)) return (Square) {row, col};
    }
}

Square follow_explosions(char **map) {
    Square explos1 = find_explosion(map, NO_PREVIOUS_VALUE, NO_PREVIOUS_VALUE);
    Square explos2 = find_explosion(map, explos1.row, explos1.col);

    if (is_vertical(explos1, explos2) == 1) {
        for (; explos2.row + 1 < map_size && map[explos2.row + 1][explos1.col] == EXPLOSION; ++explos2.row);

        if (explos1.row - 1 < 0 || map[explos1.row - 1][explos1.col] != NOT_REVEALED)
            return (Square) {explos2.row + 1, explos1.col};
        else if (explos2.row + 1 > map_size - 1 || map[explos2.row + 1][explos2.col] != NOT_REVEALED)
            return (Square) {explos1.row - 1, explos1.col};
        else
            return (rand() % 2 ? (Square) {explos2.row + 1, explos1.col} : (Square) {explos1.row - 1, explos1.col});
    } else {
        for (; explos2.col + 1 < map_size && map[explos1.row][explos2.col + 1] == EXPLOSION; ++explos2.col);

        if (explos1.col - 1 < 0 || map[explos1.row][explos1.col - 1] != NOT_REVEALED)
            return (Square) {explos1.row, explos2.col + 1};
        else if (explos2.col + 1 > map_size - 1 || map[explos2.row][explos2.col + 1] != NOT_REVEALED)
            return (Square) {explos1.row, explos1.col - 1};
        else
            return (rand() % 2 ? (Square) {explos1.row, explos2.col + 1} : (Square) {explos1.row, explos1.col - 1});
    }
}

int is_vertical(Square square1, Square square2) {
    return square1.col == square2.col ? 1 : square1.row == square2.row ? -1 : 0;
}


void update_game(Game *game, Square shoot) {
    game->turn % 2
    ? apply_changes(&game->player2, shoot, &game->player1.score, game->player1.settings.lengths_of_ships)
    : apply_changes(&game->player1, shoot, game->player2.is_bot ? NULL : &game->player2.score,
                    game->player1.settings.lengths_of_ships);
}

void apply_changes(Player *opponent, Square shoot, int *attacker_score, const int *lengths_of_ships) {
    if (opponent->revealed_map[shoot.row][shoot.col] == WATER)
        opponent->concealed_map[shoot.row][shoot.col] = WATER;
    else {
        opponent->concealed_map[shoot.row][shoot.col] = EXPLOSION;
        if (attacker_score != NULL) (*attacker_score)++;

        Ship attacked_ship = find_attacked_ship(opponent->ships, shoot);
        if (is_ship_sunk(attacked_ship, opponent->concealed_map)) {
            if (attacker_score != NULL)
                *attacker_score += (5 * max_len_of_ships(lengths_of_ships, false) / attacked_ship.len) / 2;

            reveal_ship(attacked_ship, opponent->concealed_map, CAPSIZED_SHIP);
            delete_ship(&opponent->ships, attacked_ship.bow);
            opponent->remaining_ships--;

            if (opponent->ships == NULL && attacker_score != NULL)
                *attacker_score += win_bonus(lengths_of_ships);
        }
    }
}

bool is_ship_sunk(Ship attacked_ship, char **concealed_map) {
    for (int row = attacked_ship.bow.row; row <= attacked_ship.stern.row; ++row) {
        for (int col = attacked_ship.bow.col; col <= attacked_ship.stern.col; ++col)
            if (concealed_map[row][col] != EXPLOSION) return false;
    }

    return true;
}

Ship find_attacked_ship(Ship *opponents_ships, Square shoot) {
    for (;; opponents_ships = opponents_ships->next_ship) {
        if (opponents_ships->bow.row <= shoot.row && shoot.row <= opponents_ships->stern.row
                && opponents_ships->bow.col <= shoot.col && shoot.col <= opponents_ships->stern.col)
            return *opponents_ships;
    }
}

void delete_ship(Ship **ships_head, Square bow) {
    Ship *temp;
    if ((*ships_head)->bow.row == bow.row && (*ships_head)->bow.col == bow.col) {
        temp = *ships_head;
        *ships_head = (*ships_head)->next_ship == NULL ? NULL : (*ships_head)->next_ship;
    } else {
        Ship *ship = *ships_head;
        for (; ship->next_ship->bow.row != bow.row || ship->next_ship->bow.col != bow.col; ship = ship->next_ship);

        temp = ship->next_ship;
        ship->next_ship = ship->next_ship->next_ship == NULL ? NULL : ship->next_ship->next_ship;
    }
    free(temp);
}

int win_bonus(const int *lengths_of_ships) {
    int bonus = 0, max_len = max_len_of_ships(lengths_of_ships, false);
    for (int i = 0; i < ships_count; ++i)
        bonus += (5 * max_len / lengths_of_ships[i]) / 2;

    return bonus;
}

int max_len_of_ships(const int *lengths_of_ships, bool is_the_game_ended) {
    static int max_len = 0;
    static bool is_max_len_calculated = false;
    if (is_the_game_ended) {
        is_max_len_calculated = false;
        return 0;
    }
    if (!is_max_len_calculated) {
        for (int i = 0; i < ships_count; ++i) {
            if (max_len < lengths_of_ships[i])
                max_len = lengths_of_ships[i];
        }
        is_max_len_calculated = true;
    }

    return max_len;
}

void switch_turn(Game *game, Square shoot) {
    if (game->turn % 2 ? game->player2.revealed_map[shoot.row][shoot.col] == WATER
                       : game->player1.revealed_map[shoot.row][shoot.col] == WATER)
        game->turn++;
}


int get_choice_from_pause_menu(Game *game) {
    int choice = 1;
    for (;;) {
        choice = print_menu(choice, PAUSE_MENU);
        if (1 <= choice && choice <= 4) update_players_in_file(&game->player1, &game->player2);
        switch (choice) {
            case 1:
                max_len_of_ships(game->player1.settings.lengths_of_ships, true);
                ask_to_save_game(*game);
                return RETURN_TO_MAIN_MENU;
            case 2: save_game(*game, false); break;
            case 3: return 3;
            case 4:
                ask_to_save_game(*game);
                free_game_pointer(game);
                exit(EXIT_SUCCESS);
        }
    }
}

void ask_to_save_game(Game game) {
    printf("1.Save game\n"
           "2.Surrender\n");
    for (;;) {
        int choice = GET_CHAR();
        if (choice == '1') {
            save_game(game, false);
            return;
        } else if (choice == '2') {
            system(CLEAR_SCREEN);
            printf(game.player2.is_bot ? "You surrendered to the power of computer!\n"
                                       : "You surrendered to the power of the rival!\n");
            SLEEP(2000);
            return;
        }
    }
}

void save_game(Game game, bool is_replay) {
    printf("Enter a name for the game: ");
    fflush(stdin);
    gets(game.name);

    FILE *saved_games = fopen(is_replay ? REPLAYS_FILENAME : GAMES_FILENAME, "ab");
    if (saved_games == NULL) handle_error();
    fwrite(&game, sizeof(game), 1, saved_games);

    if (is_replay) {
        fwrite_map(game.player1.revealed_map, saved_games);
        fwrite_map(game.player2.revealed_map, saved_games);
    } else {
        save_4_maps_of_game(game, saved_games);
        save_ships_list(game, saved_games);
    }

    fclose(saved_games);
    printf("Game saved.\n");
    SLEEP(1000);
}

void save_ships_list(Game game, FILE *saved_games) {
    for (Ship *ship = game.player1.ships; ship != NULL; ship = ship->next_ship)
        fwrite(ship, sizeof(Ship), 1, saved_games);
    for (Ship *ship = game.player2.ships; ship != NULL; ship = ship->next_ship)
        fwrite(ship, sizeof(Ship), 1, saved_games);
}

void save_4_maps_of_game(Game game, FILE *saved_games) {
    fwrite_map(game.player1.revealed_map, saved_games);
    fwrite_map(game.player2.revealed_map, saved_games);
    fwrite_map(game.player1.concealed_map, saved_games);
    fwrite_map(game.player2.concealed_map, saved_games);
}

void fwrite_map(char **map, FILE *file_to_write_to) {
    for (int row = 0; row < map_size; ++row)
        fwrite(map[row], sizeof(char), map_size, file_to_write_to);
}

void end_game(Game *game) {
    game_over_message(*game);

    max_len_of_ships(game->player1.settings.lengths_of_ships, true);
    update_players_in_file(&game->player1, &game->player2);
    save_game(*game, true);

    game_over_menu(game);
}

void update_players_in_file(Player *player1, Player *player2) {
    FILE *saved_players = fopen(PLAYERS_FILENAME, "r+b");
    if (saved_players == NULL) handle_error();
    seek_player_by_name(player1->name, saved_players);
    fwrite(player1, sizeof(Player), 1, saved_players);

    if (player2 == NULL || player2->is_bot) {
        fclose(saved_players);
        return;
    }
    seek_player_by_name(player2->name, saved_players);
    fwrite(player2, sizeof(Player), 1, saved_players);
    fclose(saved_players);
}

void game_over_message(Game game) {
    printf(game.player2.is_bot ? game.player1.ships == NULL ? "You were beaten by the power of computer!\n"
                                                            : "Well done captain! You won against computer by chance!\n"
                               : "%s wins!\n", game.player1.ships == NULL ? game.player2.name : game.player1.name);
}

void game_over_menu(Game *game) {
    printf("Press any key to continue. ");
    GET_CHAR();
    system(CLEAR_SCREEN);
    printf("1.Main menu\n"
           "2.Quit\n");
    for (;;) {
        int choice = GET_CHAR();
        if (choice == '1') {
            return;
        } else if (choice == '2') {
            free_game_pointer(game);
            exit(EXIT_SUCCESS);
        }
    }
}


void settings_menu(Player *player) {
    int choice = 1;
    for (;;) {
        switch (choice = print_menu(choice, SETTINGS_MENU)) {
            case 1: change_map_size(player); break;
            case 2: change_ships_settings(player); break;
            case 3:
                player->settings = set_defaults();
                printf("Done.\n");
                SLEEP(1000);
                break;
            case 4: return;
        }
        if (1 <= choice && choice <= 3) update_players_in_file(player, NULL);
    }
}

void change_map_size(Player *player) {
    player->settings.map_size = get_map_size_or_ships_count(1);
    printf("Length of map changed successfully.\n");
    SLEEP(1000);
}

void change_ships_settings(Player *player) {
    player->settings.ships_count = get_map_size_or_ships_count(2);

    for (int i = 0; i < player->settings.ships_count; ++i) {
        printf("Enter length of ship%d: ", i + 1);
        scanf("%d", &player->settings.lengths_of_ships[i]);
        if (player->settings.lengths_of_ships[i] > 7 || player->settings.lengths_of_ships[i] < 1) {
            printf(player->settings.lengths_of_ships[i] > 7
                   ? "Maximum length of ship is 7.\n"
                   : "As you might know, length of a ship is a natural number!\n");
            SLEEP(2000);
            system(CLEAR_SCREEN);
            i--;
        }
    }
    qsort(player->settings.lengths_of_ships, player->settings.ships_count, sizeof(int), integer_compare_descending);

    printf("Settings changed successfully.\n");
    SLEEP(1000);
}

int integer_compare_descending(const void *p_int1, const void *p_int2) {
    int int1 = *((int *) p_int1);
    int int2 = *((int *) p_int2);
    return (int1 > int2) ? -1 : (int1 == int2) ? 0 : 1;
}

int get_map_size_or_ships_count(int choice_from_settings_menu) {
    int value_to_get;
    for (;;) {
        system(CLEAR_SCREEN);
        printf(choice_from_settings_menu == 1 ? "Enter new length of map: " : "Enter new number of ships: ");
        scanf("%d", &value_to_get);

        if (value_to_get < 7 || value_to_get > 20) {
            printf(value_to_get > 20 ? "Maximum value is 20.\n" : "Minimum value is 7.\n");
            SLEEP(1000);
        } else
            return value_to_get;
    }
}


void hide_cursor() {
#ifdef WIN32
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &(CONSOLE_CURSOR_INFO) {.dwSize = 100, .bVisible = FALSE});
#endif
}

void show_cursor() {
#ifdef WIN32
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &(CONSOLE_CURSOR_INFO) {.dwSize = 10, .bVisible = TRUE});
#endif
}


void handle_error() {
    perror("Error");
    exit(EXIT_FAILURE);
}
