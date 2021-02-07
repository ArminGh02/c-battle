#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "hicpp-multiway-paths-covered"
#pragma ide diagnostic ignored "cert-err34-c"
#pragma ide diagnostic ignored "bugprone-branch-clone"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <windows.h>
#include <conio.h>
#include <unistd.h>
#include "sea battle.h"

#define RETURN_TO_MAIN_MENU (-1)
#define NO_PREVIOUS_VALUE (-1)

int map_size, num_of_ships;

int main() {
    atexit(exit_message);
    printf("Sea battle\n"
           "By Armin Ghorbanian\n\n"
           "What shall the history call you? ");
    char name[35];
    gets(name);
    Player player1 = load_player(name);

    srand(time(NULL));
    main_menu(player1);
}

void exit_message() {
    system("cls");
    printf("Hope to see you back.\n");
    sleep(2);
}

Player load_player(char *name) {
    FILE *players_f;
    int offset;
    if ((players_f = fopen("Players.bin", "rb")) == NULL || (offset = seek_player_by_name(name, players_f)) == -1) {
        printf("Welcome to sea battle captain!\n");
        sleep_and_cls(2);
        return create_player(name, players_f);
    }

    Player player;
    fread(&player, sizeof(Player), 1, players_f);
    printf("Welcome back captain.\n");
    sleep_and_cls(2);

    return player;
}

Player create_player(char *name, FILE *players_f) {
    players_f = freopen("Player.txt", "ab", players_f);
    Player player_to_save = {.is_bot = false, .score = 0};
    strcpy(player_to_save.name, name);
    fwrite(&player_to_save, sizeof(player_to_save), 1, players_f);

    return player_to_save;
}

int seek_player_by_name(char *name, FILE *players_f) {
    Player player;
    for (int offset = 0; fread(&player, sizeof(Player), 1, players_f) == 1; ++offset)
        if (strcmpi(player.name, name) == 0) return offset * sizeof(Player);

    return -1;
}

void main_menu(Player player1) {
    for (;;) {
        printf("1.Play vs computer\n"
               "2.Play vs a friend\n"
               "3.Load a game\n"
               "4.Battle log\n"
               "5.Scoreboard\n"
               "6.Settings\n"
               "7.Help\n"
               "0.Exit\n");
        char choice = (char) getch();
        if (48 <= choice && choice <= 55) system("cls");
        switch (choice) {
            case '1':
                play_the_game(create_game(player1, choice));
                break;
            case '2':
                play_the_game(create_game(player1, choice));
                break;
//            case '3':
//                load_game();
//                break;
//            case '4':
//                replay_game();
//                break;
            case '5':
                display_scoreboard();
                break;
            case '6':
                settings_menu();
                break;
            case 7:
                help();
                break;
            case '0':
                exit(EXIT_SUCCESS);
        }
    }
}

void display_scoreboard() {
    FILE *players_f = fopen("Players.bin", "r+b"),
            *copy_of_players = fopen("Players copy.bin", "wb");
    copy_file(copy_of_players, players_f);

    Player *player;
    for (int i = 1;; ++i) {
        player = find_player_with_max_score(players_f);
        if (player == NULL) break;

        printf("%2d- %-25s%d\n", i, player->name, player->score);
        fflush(players_f);
        player->score = -1;
        fwrite(&player, sizeof(Player), 1, players_f);
    }

    copy_file(players_f, copy_of_players);
    remove("Players copy.bin");
    fclose(copy_of_players);
    fclose(players_f);

    printf("Press any key to return to main menu. ");
    getch();
}

void copy_file(FILE *dest, FILE *source) {
    char c;
    while ((c = fgetc(source)) != EOF)
        fputc(c, dest);
    fclose(dest);
}

Player *find_player_with_max_score(FILE *players_f) {
    int max_score = -1;
    Player temp;
    static Player player_with_max_score;

    while (fread(&temp, sizeof(Player), 1, players_f) == 1) {
        if (max_score < temp.score) {
            max_score = temp.score;
            player_with_max_score = temp;
        }
    }

    return max_score == -1 ? NULL : &player_with_max_score;
}


Game create_game(Player player1, char is_vs_friend) {
    Game game = {.turn = 1, .is_finished = false, .date = get_date(), .player1 = player1};
    game.player2.is_bot = is_vs_friend == '1' ? true : false;
    game.player1.ships = load_settings();
    game.player2.ships = copy_ships_list(game.player1.ships);
    do_the_allocations(&game.player1, &game.player2);
    if (is_vs_friend == '2') {
        printf("What shall your friend be called throughout the history? ");
        char name[35];
        gets(name);
        game.player2 = load_player(name);
    } else
        strcpy(game.player2.name, "Computer");

    return game;
}

struct tm get_date() {
    time_t now = time(NULL);
    return *localtime(&now);
}

Ship *load_settings() {
    FILE *settings = fopen("Settings.txt", "r");

    if (settings == NULL) {
        set_defaults();
        settings = fopen("Settings.txt", "r");
    }

    fscanf(settings, "map_size = %d", &map_size);

    return create_list_of_ships(settings);
}

void set_defaults() {
    FILE *settings = fopen("Settings.txt", "w");
    fprintf(settings,
            "size = 10 \nnum_of_ships = 10 \nship1_len = 4\nship2_len = 3\nship3_len = 3\nship4_len = 2\n"
            "ship5_len = 2\nship6_len = 2\nship7_len = 1\nship8_len = 1\nship9_len = 1\nship10_len = 1\n");
    fclose(settings);
}

Ship *create_list_of_ships(FILE *settings) {
    Ship *ships = NULL;
    fscanf(settings, " num_of_ships = %d", &num_of_ships);
    for (int i = 0; i < num_of_ships; i++) {
        int ship_len;
        fscanf(settings, " ship%*d_len = %d", &ship_len);
        add_ship(&ships, ship_len);
    }

    return ships;
}

void add_ship(Ship **list, int len_of_ship) {
    if (*list == NULL) {
        *list = (Ship *) malloc(sizeof(Ship));
        (*list)->len = len_of_ship;
        (*list)->next_ship = NULL;
    } else {
        Ship *new_node = *list;
        while (new_node->next_ship)
            new_node = new_node->next_ship;

        new_node->next_ship = (Ship *) malloc(sizeof(Ship));
        new_node = new_node->next_ship;
        new_node->len = len_of_ship;
        new_node->next_ship = NULL;
    }
}

void do_the_allocations(Player *player1, Player *player2) {
    player1->revealed_map = allocate_map('?');
    player2->revealed_map = allocate_map('?');
    player1->concealed_map = allocate_map('?');
    player2->concealed_map = allocate_map('?');
}

char **allocate_map(char initial_value) {
    char **map = (char **) malloc(map_size * sizeof(char *));
    for (int i = 0; i < map_size; ++i) {
        map[i] = (char *) malloc(map_size * sizeof(char));
        memset(map[i], initial_value, map_size * sizeof(char));
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
    gotoxy(0, 0);
    print_header();
    display_map(player.revealed_map, player.name);
    printf("%100s\n%100s", "", "");
    gotoxy(0, map_size + 6);
}

void display_screen_for_guessing(char **map1, char **map2, char *name1, char *name2) {
    gotoxy(0, 0);
    print_header();
    display_map(map1, name1);
    display_map(map2, name2);
    printf("%100s\n%100s", "", "");
    gotoxy(0, 2 * (map_size) + 9);
}

void gotoxy(int x, int y) {
    COORD coordinates = {x, y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coordinates);
}

void print_header() {
    int line_len = printf("Subject: Sea Battle%*sDate: ", map_size * 2, "") + 9;
    print_date();
    while (line_len--)
        printf("-");
    printf("\n\n");
}

void print_date() {
    struct tm date = get_date();
    printf("%d/%d/%d\n", date.tm_mon + 1, date.tm_mday, date.tm_year + 1900);
}

void display_map(char **map, char *name) {
    printf("%s\n", name);
    print_columns();
    for (int i = 0; i < map_size; ++i) {
        putchar(i + 65);
        for (int j = 0; j < map_size; ++j) {
            printf("  ");
            put_colored_char(map[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void print_columns() {
    printf("   ");
    for (int i = 1; i <= map_size; ++i)
        printf(i >= 10 ? "%d " : "%d  ", i);
    printf("\n");
}

void put_colored_char(char c) {
    set_color(c == 'W' ? 1 : c == 'S' ? 10 : c == 'E' ? 4 : c == 'C' ? 6 : 8);
    putchar(c);
    set_color(7);
}

void set_color(int color) {
    WORD wColor;
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hStdOut, &csbi)) {
        wColor = (csbi.wAttributes & 0xF0) + (color & 0x0F);
        SetConsoleTextAttribute(hStdOut, wColor);
    }
}


void play_the_game(Game game) {
    how_to_place_ships(&game.player1);
    game.player2.is_bot ? auto_arrange_map(&game.player2) : how_to_place_ships(&game.player2);

    if (game_loop(&game) == RETURN_TO_MAIN_MENU) return;
    end_game(game);
}

int game_loop(Game *game) {
    Player *player1 = &game->player1, *player2 = &game->player2;
    while (player1->ships != NULL && player2->ships != NULL) {
        if (game->turn % 2 || !player2->is_bot) {
            display_screen_for_guessing(player1->concealed_map, player2->concealed_map, player1->name, player2->name);
            char command[50];
            get_command(command, *player1, *player2, game->turn);
            Square shoot;
            if (strcmpi(command, "pause") == 0) {
                if (get_choice_from_pause_menu(*game) == RETURN_TO_MAIN_MENU)
                    return RETURN_TO_MAIN_MENU;
            } else if (is_placeable_square(command, player2->concealed_map, &shoot)) {
                game->turn % 2 ? apply_changes(player2, shoot, &player1->score)
                               : apply_changes(player1, shoot, &player2->score);
                if (player2->revealed_map[shoot.row][shoot.col] == 'W') game->turn++;
            } else {
                printf("Invalid shoot or syntax. Please try again.\n");
                sleep(2);
            }
        } else
            play_for_bot(game);
    }

    return 0;
}

void get_command(char command[], Player player1, Player player2, int turn) {
    if (!player2.is_bot)
        printf("%s's turn.\n", turn % 2 ? player1.name : player2.name);
    printf("Choose a square to shoot (e.g. A1) or cease fire by entering \"pause\": ");
    scanf(" %s", command);
}

void how_to_place_ships(Player *player) {
    printf("Place ships:\n"
           "1.Auto\n"
           "2.Manually\n");
    for (;;) {
        char choice = (char) getch();
        switch (choice) {
            case '1':
                system("cls");
                auto_arrange_map(player);
                return;
            case '2':
                system("cls");
                manually_place_ships(player);
                return;
        }
    }
}

void auto_arrange_map(Player *player) {
    enum confirmation is_confirmed;
    do {
        clear_map(player->revealed_map);

        int empty_squares = map_size * map_size;
        auto_place_ships(player, &empty_squares);
        fill_empty_squares_with_water(empty_squares, player->revealed_map);

        if (player->is_bot) return;

        gotoxy(0, 0);
        print_header();
        display_map(player->revealed_map, player->name);
        if ((is_confirmed = get_confirmation()) == CONFIRMED) {
            ask_to_save_map(*player);
            system("cls");
            return;
        }
    } while (is_confirmed == REFUSED);
}

void clear_map(char **map) {
    if (map[0][0] != '?') {
        for (int i = 0; i < map_size; ++i)
            memset(map[i], '?', sizeof(char) * map_size);
    }
}

void fill_empty_squares_with_water(int remained_squares, char **map) {
    for (int i = 0; i < map_size; ++i) {
        for (int j = 0; j < map_size; ++j) {
            if (remained_squares == 0) return;
            else if (map[i][j] == '?') {
                map[i][j] = 'W';
                --remained_squares;
            }
        }
    }
}

enum confirmation get_confirmation() {
    printf("Confirm? (y/n) ");
    for (;;) {
        char choice = (char) getch();
        if (choice == 'y' || choice == 'Y') return CONFIRMED;
        else if (choice == 'n' || choice == 'N') return REFUSED;
    }
}

void ask_to_save_map(Player player) {
    printf("%100s\rWould you like to save this map? (y/n) ", "");
    for (;;) {
        char choice = (char) getch();
        if (choice == 'y' || choice == 'Y') {
            save_map(player);
            return;
        } else if (choice == 'n' || choice == 'N') return;
    }
}

void save_map(Player player) {
    SavedMap saved_map = {.size = map_size, .date = get_date(), .player_map = player.revealed_map};
    strcpy(saved_map.player_name, player.name);

    FILE *saved_maps_file = fopen("Saved maps.bin", "ab");
    fwrite(&saved_map, sizeof(saved_map), 1, saved_maps_file);
    fclose(saved_maps_file);
    printf("\nMap saved successfully.");
    sleep(1);
}

void auto_place_ships(Player *player, int *remained_squares) {
    Ship *ship = player->ships;
    for (int i = 0; i < num_of_ships; ++i, ship = ship->next_ship) {
        if (ship->len == 1) {
            ship->bow = rand_square(player->revealed_map, *remained_squares);
            ship->stern = ship->bow;
            reveal_ship(*ship, player->revealed_map, 'S');
        } else {
            int num_of_placeable_dirs;
            do {
                ship->bow = rand_square(player->revealed_map, *remained_squares);
                // I have assumed that the stern is always righter or downer than the bow.
                enum placeability placeable_dirs[] = {NOT_PLACEABLE, PLACEABLE, PLACEABLE, NOT_PLACEABLE};
                find_placeable_dirs(ship->bow, placeable_dirs, player->revealed_map, ship->len - 1);
                num_of_placeable_dirs = count_placeable_dirs(placeable_dirs);
                if (num_of_placeable_dirs > 0) {
                    ship->stern =
                            find_stern(ship->bow, rand_dir(placeable_dirs, num_of_placeable_dirs), ship->len - 1);
                    reveal_ship(*ship, player->revealed_map, 'S');
                }
            } while (num_of_placeable_dirs == 0);
        }
        *remained_squares = count_char(player->revealed_map, '?');
    }
}

Square rand_square(char **map, int remained_squares) {
    Square square;
    int random_num = rand() % remained_squares;
    for (int i = 0; i < map_size; ++i) {
        for (int j = 0; j < map_size; ++j) {
            if (map[i][j] == '?') {
                if (random_num == 0) {
                    square.row = i, square.col = j;
                    return square;
                } else --random_num;
            }
        }
    }
}

void reveal_ship(Ship ship, char **map, char S_or_C) {
    for (int i = (ship.bow.row - 1 >= 0 ? ship.bow.row - 1 : ship.bow.row);
         i <= (ship.stern.row + 1 < map_size ? ship.stern.row + 1 : ship.stern.row); ++i) {
        for (int j = (ship.bow.col - 1 >= 0 ? ship.bow.col - 1 : ship.bow.col);
             j <= (ship.stern.col + 1 < map_size ? ship.stern.col + 1 : ship.stern.col); ++j) {
            if (ship.bow.row <= i && i <= ship.stern.row && ship.bow.col <= j && j <= ship.stern.col)
                map[i][j] = S_or_C;
            else map[i][j] = 'W';
        }
    }
}

void find_placeable_dirs(Square square, enum placeability *directions, char **map, int len) {
    if (directions[UP] == PLACEABLE && square.row - len >= 0) {
        for (int i = len, j = square.row - 1; i--; --j)
            if (map[j][square.col] != '?') directions[UP] = NOT_PLACEABLE;
    } else directions[UP] = NOT_PLACEABLE;

    if (directions[RIGHT] == PLACEABLE && square.col + len < map_size) {
        for (int i = len, j = square.col + 1; i--; ++j)
            if (map[square.row][j] != '?') directions[RIGHT] = NOT_PLACEABLE;
    } else directions[RIGHT] = NOT_PLACEABLE;

    if (directions[DOWN] == PLACEABLE && square.row + len < map_size) {
        for (int i = len, j = square.row + 1; i--; ++j)
            if (map[j][square.col] != '?') directions[DOWN] = NOT_PLACEABLE;
    } else directions[DOWN] = NOT_PLACEABLE;

    if (directions[LEFT] == PLACEABLE && square.col - len >= 0) {
        for (int i = len, j = square.col - 1; i--; --j)
            if (map[square.row][j] != '?') directions[LEFT] = NOT_PLACEABLE;
    } else directions[LEFT] = NOT_PLACEABLE;
}

int count_placeable_dirs(const enum placeability directions[]) {
    int count = 0;
    for (enum directions i = 0; i < 4; ++i)
        if (directions[i] == PLACEABLE) count++;

    return count;
}

enum directions rand_dir(const enum placeability *directions, int num_of_placeable_dirs) {
    enum directions i;
    int random_dir = rand() % num_of_placeable_dirs + 1;
    for (i = UP; random_dir--; ++i)
        while (directions[i] == NOT_PLACEABLE) ++i;

    return i - 1;
}

Square find_stern(Square bow, enum directions direction, int len) {
    Square stern;
    switch (direction) {
        case UP:
            stern.col = bow.col, stern.row = bow.row - len;
            return stern;
        case RIGHT:
            stern.row = bow.row, stern.col = bow.col + len;
            return stern;
        case DOWN:
            stern.col = bow.col, stern.row = bow.row + len;
            return stern;
        case LEFT:
            stern.row = bow.row, stern.col = bow.col - len;
            return stern;
    }
}

int count_char(char **map, char c) {
    int count = 0;
    for (int i = 0; i < map_size; ++i) {
        for (int j = 0; j < map_size; ++j)
            if (map[i][j] == c) ++count;
    }

    return count;
}


void manually_place_ships(Player *player) {
    enum confirmation is_confirmed;
    do {
        Ship *ship = player->ships;
        for (int i = 0; i < num_of_ships;) {
            display_screen_for_placing(*player);
            char chosen_bow[30], chosen_stern[30];
            get_bow_and_stern(chosen_bow, chosen_stern, ship->len, i + 1);
            if (is_valid_ship_placement(chosen_bow, chosen_stern, ship, player->revealed_map)) {
                if (square_compare(ship->stern, ship->bow) == 1) square_swap(&ship->bow, &ship->stern);
                reveal_ship(*ship, player->revealed_map, 'S');
                i++;
                ship = ship->next_ship;
            } else {
                printf("Invalid placement or syntax. Please try again.\n");
                sleep(2);
            }
        }
        fill_empty_squares_with_water(count_char(player->revealed_map, '?'), player->revealed_map);
        display_screen_for_placing(*player);
        if ((is_confirmed = get_confirmation()) == CONFIRMED) {
            ask_to_save_map(*player);
            return;
        }
    } while (is_confirmed == REFUSED);
}

void get_bow_and_stern(char bow[], char stern[], int ship_len, int ship_num) {
    printf(ship_len == 1 ? "Enter a square to place ship%d with length %d (e.g. A2): "
                         : "Enter bow and stern of ship%d with length %d (e.g. A1 A%d): ",
           ship_num, ship_len, ship_len);
    scanf(ship_len == 1 ? "%s" : "%s %s", bow, stern);
    if (ship_len == 1) strcpy(stern, bow);
}

bool is_valid_ship_placement(char *chosen_bow, char *chosen_stern, Ship *ship, char **map) {
    if (!is_placeable_square(chosen_bow, map, &ship->bow) ||
        !is_placeable_square(chosen_stern, map, &ship->stern))
        return false;

    if (square_compare(ship->stern, ship->bow) == 1) square_swap(&ship->stern, &ship->bow);

    if (ship->bow.row == ship->stern.row) {
        if (ship->stern.col - ship->bow.col != ship->len - 1)
            return false;
        for (int i = ship->bow.col + 1; i < ship->stern.col; ++i)
            if (map[ship->bow.row][i] != '?') return false;
    } else if (ship->bow.col == ship->stern.col) {
        if (ship->stern.row - ship->bow.row != ship->len - 1)
            return false;
        for (int i = ship->bow.row + 1; i < ship->stern.row; ++i)
            if (map[i][ship->bow.col] != '?') return false;
    } else return false;

    return true;
}

bool is_placeable_square(char *square, char **map, Square *square_coordinates) {
    square_coordinates->row = isupper(square[0]) ? square[0] - 'A' : square[0] - 'a';
    sscanf(square + 1, "%d", &square_coordinates->col);

    if (strlen(square) > 3 || !isalpha(square[0]) ||
        !isdigit(square[1]) || (strlen(square) == 3 && !isdigit(square[2])) ||
        square_coordinates->row >= map_size || --square_coordinates->col >= map_size ||
        square_coordinates->row < 0 || square_coordinates->col < 0 ||
        map[square_coordinates->row][square_coordinates->col] != '?')
        return false;

    return true;
}

int square_compare(Square square1, Square square2) {
    if (square1.row < square2.row || square1.col < square2.col)
        return 1;
    return -1;
}

void square_swap(Square *square1, Square *square2) {
    Square temp = *square1;
    *square1 = *square2;
    *square2 = temp;
}


void play_for_bot(Game *game) {
    Square shoot = shoot_for_bot(game->player1.concealed_map);
    if (game->player1.revealed_map[shoot.row][shoot.col] == 'W') game->turn++;
    apply_changes(&game->player1, shoot, NULL);
    display_screen_for_guessing(game->player1.concealed_map, game->player2.concealed_map,
                                game->player1.name, game->player2.name);
    printf("Computer chose %c%d.\n", shoot.row + 65, shoot.col + 1);
    sleep(2);
}

Square shoot_for_bot(char **opponent_map) {
    int num_of_explosions = count_char(opponent_map, 'E');
    if (num_of_explosions == 0) {
        return rand_square(opponent_map, count_char(opponent_map, '?'));
    } else if (num_of_explosions == 1) {
        Square explosion_square = find_explosion(opponent_map, NO_PREVIOUS_VALUE, NO_PREVIOUS_VALUE);
        enum placeability placeable_dirs[] = {PLACEABLE, PLACEABLE, PLACEABLE, PLACEABLE};
        find_placeable_dirs(explosion_square, placeable_dirs, opponent_map, 1);
        return find_stern(explosion_square, rand_dir(placeable_dirs, count_placeable_dirs(placeable_dirs)), 1);
    } else
        return follow_explosions(opponent_map);
}

Square find_explosion(char **map, int previous_row, int previous_col) {
    Square explosion;
    for (int row = 0; row < map_size; ++row) {
        for (int col = 0; col < map_size; ++col) {
            if (map[row][col] == 'E' && (row != previous_row || col != previous_col)) {
                explosion.row = row, explosion.col = col;
                return explosion;
            }
        }
    }
}

Square follow_explosions(char **map) {
    Square explos1 = find_explosion(map, NO_PREVIOUS_VALUE, NO_PREVIOUS_VALUE),
            explos2 = find_explosion(map, explos1.row, explos1.col),
            follower_explos;

    if (is_vertical(explos1, explos2)) {
        for (; explos2.row + 1 < map_size && map[explos2.row + 1][explos1.col] == 'E'; ++explos2.row);

        if (explos1.row - 1 < 0 || map[explos1.row - 1][explos1.col] != '?')
            follower_explos.row = explos2.row + 1;
        else if (explos2.row + 1 > map_size - 1 || map[explos2.row + 1][explos2.col] != '?')
            follower_explos.row = explos1.row - 1;
        else
            rand() % 2 ? (follower_explos.row = explos2.row + 1) : (follower_explos.row = explos1.row - 1);

        follower_explos.col = explos1.col;
    } else {
        for (; explos2.col + 1 < map_size && map[explos1.row][explos2.col + 1] == 'E'; ++explos2.col);

        if (explos1.col - 1 < 0 || map[explos1.row][explos1.col - 1] != '?')
            follower_explos.col = explos2.col + 1;
        else if (explos2.col + 1 > map_size - 1 || map[explos2.row][explos2.col + 1] != '?')
            follower_explos.col = explos1.col - 1;
        else
            rand() % 2 ? (follower_explos.col = explos2.col + 1) : (follower_explos.col = explos1.col - 1);

        follower_explos.row = explos1.row;
    }

    return follower_explos;
}

bool is_vertical(Square square1, Square square2) {
    return square1.row < square2.row;
}


void apply_changes(Player *opponent, Square shoot, int *attacker_score) {
    if (opponent->revealed_map[shoot.row][shoot.col] == 'W')
        opponent->concealed_map[shoot.row][shoot.col] = 'W';
    else {
        opponent->concealed_map[shoot.row][shoot.col] = 'E';
        (*attacker_score)++;

        Ship attacked_ship = find_attacked_ship(opponent->ships, shoot);
        if (is_ship_sunk(attacked_ship, *opponent)) {
            if (attacker_score != NULL)
                *attacker_score += (5 * max_len_of_ships() / attacked_ship.len) / 2;

            reveal_ship(attacked_ship, opponent->concealed_map, 'C');
            delete_ship(opponent, attacked_ship.bow);

            if (opponent->ships == NULL && attacker_score != NULL)
                *attacker_score += win_bonus();
        }
    }
}

bool is_ship_sunk(Ship attacked_ship, Player opponent) {
    for (int i = attacked_ship.bow.row; i <= attacked_ship.stern.row; ++i) {
        for (int j = attacked_ship.bow.col; j <= attacked_ship.stern.col; ++j)
            if (opponent.concealed_map[i][j] != 'E') return false;
    }

    return true;
}

Ship find_attacked_ship(Ship *opponent_ships, Square shoot) {
    for (;; opponent_ships = opponent_ships->next_ship) {
        if (opponent_ships->bow.row <= shoot.row && shoot.row <= opponent_ships->stern.row &&
            opponent_ships->bow.col <= shoot.col && shoot.col <= opponent_ships->stern.col)
            return *opponent_ships;
    }
}

void delete_ship(Player *player, Square bow) {
    Ship *temp;
    if (player->ships->bow.row == bow.row && player->ships->bow.col == bow.col) {
        temp = player->ships;
        player->ships = player->ships->next_ship;
    } else {
        Ship *ship = player->ships;
        for (; ship->next_ship->bow.row != bow.row || ship->next_ship->bow.col != bow.col;
               ship = ship->next_ship);

        temp = ship->next_ship;
        if (ship->next_ship->next_ship == NULL)
            ship->next_ship = NULL;
        else
            ship->next_ship = ship->next_ship->next_ship;
    }
    free(temp);
}

int win_bonus() {
    int bonus = 0, max_len = max_len_of_ships();
    FILE *settings = fopen("Settings.txt", "r");
    fscanf(settings, "%*[^\n]s\n%*[^\n]s\n");
    for (int i = 0; i < num_of_ships; ++i) {
        int len_of_ship;
        fscanf(settings, "ship%*d_len = %d", &len_of_ship);
        bonus += (5 * max_len / len_of_ship) / 2;
    }
    fclose(settings);

    return bonus;
}

int max_len_of_ships() {
    static int max_len = 0;
    static bool is_calculated = false;
    if (is_calculated == false) {
        FILE *settings = fopen("Settings.txt", "r");
        fscanf(settings, "%*[^\n]s\n%*[^\n]s\n");
        for (int i = 0; i < num_of_ships; ++i) {
            int len_of_ship;
            fscanf(settings, "ship%*d_len = %d", &len_of_ship);
            if (max_len < len_of_ship)
                max_len = len_of_ship;
        }
        fclose(settings);
        is_calculated = true;
    }

    return max_len;
}


int get_choice_from_pause_menu(Game game) {
    system("cls");
    for (;;) {
        printf("1.Main menu\n"
               "2.Resume\n"
               "3.Quit game\n");
        char choice = (char) getch();
        system("cls");
        switch (choice) {
            case '1':
                ask_to_save_game(game);
                return RETURN_TO_MAIN_MENU;
            case '2':
                return 2;
            case '3':
                ask_to_save_game(game);
                exit(EXIT_SUCCESS);
        }
    }
}

void ask_to_save_game(Game game) {
    printf("1.Save game\n"
           "2.Surrender\n");
    for (;;) {
        char choice = (char) getch();
        if (choice == '1') {
            save_game(game);
            return;
        } else if (choice == '2') {
            system("cls");
            printf("You surrendered to the power of computer! Good luck to you!\n");
            sleep_and_cls(2);
            return;
        }
    }
}

void save_game(Game game) {
    FILE *saved_games = fopen("Saved games.bin", "ab");
    fwrite(&game, sizeof(game), 1, saved_games);
    fclose(saved_games);
    printf("Game saved.\n");
    sleep(1);
}

void end_game(Game game) {
    game.is_finished = true;
    update_players_in_file(game.player1, game.player2);
    save_game(game);
    printf(game.player2.is_bot ? game.player1.ships == NULL ? "You were beaten by the power of computer!\n"
                                                            : "Well done captain! You beat computer by chance!\n"
                               : "%s wins!\n", game.player1.ships == NULL ? game.player2.name : game.player1.name);
    printf("Press any key to continue. ");
    getch();
    system("cls");
    for (;;) {
        printf("1.Main menu.\n"
               "2.Quit.\n");
        char choice = (char) getch();
        if (choice == '1' || choice == '2') system("cls");
        if (choice == '1') return;
        else if (choice == '2') exit(EXIT_SUCCESS);
    }
}

void update_players_in_file(Player player1, Player player2) {
    FILE *players_f = fopen("Players.bin", "r+");
    seek_player_by_name(player1.name, players_f);
    fwrite(&player1, sizeof(Player), 1, players_f);
    seek_player_by_name(player2.name, players_f);
    fwrite(&player2, sizeof(Player), 1, players_f);
    fclose(players_f);
}

void settings_menu() {
    for (;;) {
        printf("1.Change map size, number of ships and length of ships.\n"
               "2.Restore default settings\n"
               "3.Main menu.\n");
        char choice = (char) getch();
        system("cls");
        switch (choice) {
            case '1':
                change_settings();
                break;
            case '2':
                set_defaults();
                printf("Done.\n");
                sleep_and_cls(1);
                break;
            case '3':
                return;
        }
    }
}

void change_settings() {
    FILE *settings = fopen("Settings.txt", "w");
    int new_ships_num;
    for (;;) {
        change_map_size_and_num_of_ships(settings, &new_ships_num);

        for (int i = 1; i <= new_ships_num; ++i) {
            printf("Enter length of ship%d: ", i);
            int ship_len;
            scanf("%d", &ship_len);
            if (ship_len >= 10 || ship_len < 1) {
                printf(ship_len >= 10 ? "Maximum length of ship is 9.\n"
                                      : "As you might know, length of a ship is a natural number!\n");
                i--;
                sleep_and_cls(1);
                continue;
            }
            fprintf(settings, "ship%d_len = %d\n", i, ship_len);
        }

        fclose(settings);
        printf("Settings changed successfully.\n");
        sleep_and_cls(1);

        return;
    }
}

void change_map_size_and_num_of_ships(FILE *settings, int *new_ships_num) {
    int new_map_size;
    for (int i = 0; i < 2;) {
        system("cls");
        printf(i == 0 ? "Enter new length of map: " : "Enter ships number: ");
        scanf("%d", i == 0 ? &new_map_size : new_ships_num);
        int scanned_value = i == 0 ? new_map_size : *new_ships_num;
        if (scanned_value < 7 || scanned_value > 20) {
            printf(new_map_size > 20 ? "Maximum value is 20.\n" : "Minimum value is 7.\n");
            sleep(1);
            continue;
        } else {
            fprintf(settings, i == 0 ? "size = %-2d \n" : "num_of_ships = %-2d \n",
                    i == 0 ? new_map_size : *new_ships_num);
            i++;
        }
    }
}

void sleep_and_cls(int sleep_time) {
    sleep(sleep_time);
    system("cls");
}

#pragma clang diagnostic pop