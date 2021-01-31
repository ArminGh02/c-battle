#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <windows.h>
#include <conio.h>
#include <unistd.h>
#include <math.h>

#define RETURN_TO_MAIN_MENU (-1)

int map_size, num_of_ships;

typedef struct {
    int row, col;
} Square;

typedef struct Ship {
    struct Ship *next_ship;
    int len;
    Square bow, stern;
} Ship;

typedef struct {
    char name[100];
    Ship *ships;
    char **revealed_map;
    char **concealed_map;
    long score;
    bool is_bot;
} Player;

enum directions {
    UP, RIGHT, DOWN, LEFT
};

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

void invalid_choice() {
    printf("Invalid choice.\n");
    sleep(1);
    system("cls");
}

//void help();

//void classic_mode();
//void advanced_mode();
//void equip_army();
//void use_radar();
//void bombard();

//void load_map();

//void save_shoots();

//void save_game();

//void load_game();

//void display_saved_games_list();

//void replay_game();

//void display_scoreboard();

//void change_settings();

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
        new_node->next_ship->len = len_of_ship;
        new_node->next_ship->next_ship = NULL;
    }
}

Ship *create_list_of_ships(FILE *settings) {
    Ship *ships = NULL;
    fscanf(settings, "\nnum_of_ships = %d", &num_of_ships);
    for (int i = 0; i < num_of_ships; i++) {
        int ship_len;
        fscanf(settings, "\nship%*d_len = %d", &ship_len);
        add_ship(&ships, ship_len);
    }

    return ships;
}

int load_settings(Ship **ships) {
    FILE *settings = fopen("Settings.txt", "r");

    if (settings == NULL) {
        perror("Error opening settings file");
        sleep(3);
        exit(EXIT_FAILURE);
    }

    fscanf(settings, "map_size = %d", &map_size);

    *ships = create_list_of_ships(settings);

    // load other settings:

    return 0;
}

void copy_list(Ship **dest, Ship *source) {
    while (source != NULL) {
        add_ship(dest, source->len);
        source = source->next_ship;
    }
}

void allocate_map(char ***map, char initial_value) {
    *map = (char **) malloc(map_size * sizeof(char *));
    for (int i = 0; i < map_size; ++i) {
        (*map)[i] = (char *) malloc(map_size * sizeof(char));
        memset((*map)[i], initial_value, map_size * sizeof(char));
    }
}

void do_the_allocations(Player *player1, Player *player2) {
    allocate_map(&player1->revealed_map, '?');
    allocate_map(&player2->revealed_map, '?');
    allocate_map(&player1->concealed_map, '?');
    allocate_map(&player2->concealed_map, '?');
}

void prepare_players(Player *player1, Player *player2) {
    strcpy(player1->name, "Player1");
    strcpy(player2->name, "Player2");
    player1->ships = NULL, player2->ships = NULL;

    int is_settings_loaded = load_settings(&player1->ships);
    if (is_settings_loaded == -1) return;

    copy_list(&player2->ships, player1->ships);

    do_the_allocations(player1, player2);
}

void play_the_game(Player *player1, Player *player2);

void main_menu() {
    Player player1, player2;
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
        system("cls");
        switch (choice) {
            case '1':
                player1.is_bot = false, player2.is_bot = true;
                prepare_players(&player1, &player2);
                play_the_game(&player1, &player2);
                break;
            case '2':
                player1.is_bot = false, player2.is_bot = false;
                prepare_players(&player1, &player2);
                play_the_game(&player1, &player2);
                break;
//            case '3':
//                load_game();
//                break;
//            case '4':
//                replay_game();
//                break;
//            case '5':
//                display_scoreboard();
//                break;
//            case '6':
//                change_settings();
//                break;
            case '0':
                exit(EXIT_SUCCESS);
            default:
                invalid_choice();
        }
    }
}

int main() {
    printf("Welcome to sea battle!\n"
           "By Armin Ghorbanian\n");
    srand(time(NULL));
    main_menu();
}

void set_color(int ForgC) {
    WORD wColor;
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hStdOut, &csbi)) {
        wColor = (csbi.wAttributes & 0xF0) + (ForgC & 0x0F);
        SetConsoleTextAttribute(hStdOut, wColor);
    }
}

void print_date() {
    time_t now = time(NULL);
    struct tm *date = localtime(&now);
    printf("%d/%d/%d\n", date->tm_mon + 1, date->tm_mday, date->tm_year + 1900);
}

void print_header() {
    int line_len = printf("Subject: Sea Battle%*sDate: ", map_size * 2, "") + 9;
    print_date();
    while (line_len--)
        printf("-");
    printf("\n\n");
}

void print_columns() {
    printf("   ");
    for (int i = 0; i < map_size; ++i)
        printf("%d  ", i + 1);
    printf("\n");
}

void put_colored_char(char c) {
    set_color(c == 'W' ? 1 : c == 'S' ? 10 : c == 'E' ? 4 : c == 'C' ? 6 : 8);
    putchar(c);
    set_color(7);
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

void display_page_for_placing(Player player) {
    system("cls");
    print_header();
    display_map(player.revealed_map, player.name);
}

void display_page_for_guessing(char **map1, char **map2, char *name1, char *name2) {
    system("cls");
    print_header();
    display_map(map1, name1);
    display_map(map2, name2);
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

bool
is_valid_ship_placement(char *chosen_bow, char *chosen_stern, Square *bow, Square *stern, int len_of_ship, char **map) {
    if (!is_placeable_square(chosen_bow, map, bow) ||
        !is_placeable_square(chosen_stern, map, stern))
        return false;
    if (bow->row == stern->row) {
        if ((int) fabs((double) (bow->col - stern->col)) != len_of_ship - 1)
            return false;
        for (int i = (int) fmin(bow->col, stern->col) + 1; i < fmax(bow->col, stern->col); ++i)
            if (map[bow->row][i] != '?') return false;
    } else if (bow->col == stern->col) {
        if ((int) fabs((double) (bow->row - stern->row)) != len_of_ship - 1)
            return false;
        for (int i = (int) fmin(bow->row, stern->row) + 1; i < fmax(bow->row, stern->row); ++i)
            if (map[i][bow->col] != '?') return false;
    } else return false;

    return true;
}

void reveal_ship(Square bow, Square stern, char **map, char S_or_C) {
    for (int i = (bow.row - 1 >= 0 ? bow.row - 1 : bow.row);
         i <= (stern.row + 1 < map_size ? stern.row + 1 : stern.row); ++i) {
        for (int j = (bow.col - 1 >= 0 ? bow.col - 1 : bow.col);
             j <= (stern.col + 1 < map_size ? stern.col + 1 : stern.col); ++j) {
            if (bow.row <= i && i <= stern.row && bow.col <= j && j <= stern.col)
                map[i][j] = S_or_C;
            else map[i][j] = 'W';
        }
    }
}

void replace_by_W(int remained_squares, char **map) {
    for (int i = 0; i < map_size; ++i) {
        for (int j = 0; j < map_size; ++j) {
            if (remained_squares == 0) break;
            else if (map[i][j] == '?') {
                map[i][j] = 'W';
                --remained_squares;
            }
        }
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
    Ship *ship = player->ships;
    for (int i = 0; i < num_of_ships;) {
        display_page_for_placing(*player);
        printf(ship->len > 1 ? "Enter bow and stern of ship%d with length %d (e.g. A1 A%d): "
                             : "Enter a square to place ship%d with length %d (e.g. A2): ",
               i + 1, ship->len, ship->len);
        char chosen_bow[20], chosen_stern[20];
        scanf(ship->len > 1 ? "\n%s %s" : "\n%s", chosen_bow, chosen_stern);
        if (ship->len == 1) strcpy(chosen_stern, chosen_bow);
        Square bow, stern;
        if (is_valid_ship_placement(chosen_bow, chosen_stern, &bow, &stern, ship->len, player->revealed_map)) {
            if (square_compare(stern, bow) == 1) square_swap(&bow, &stern);
            reveal_ship(bow, stern, player->revealed_map, 'S');
            i++;
            ship = ship->next_ship;
        } else {
            printf("Invalid placement or syntax. Please try again.\n");
            sleep(2);
        }
    }
    replace_by_W(count_char(player->revealed_map, '?'), player->revealed_map);
    display_page_for_placing(*player);
    sleep(3);   // ask to save map and confirm
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

void find_placeable_dirs(Square square, bool *directions, char **map, int len) {
    for (enum directions i = UP; i < 4; ++i) {
        if (i == UP ? square.row - len >= 0
                    : i == RIGHT ? square.col + len < map_size
                                 : i == DOWN ? square.row + len < map_size
                                             : square.col - len >= 0) {
            for (int k = len, j = i == UP ? square.row - 1
                                          : i == RIGHT ? square.col + 1
                                                       : i == DOWN ? square.row + 1
                                                                   : square.col - 1;
                 k--; i == UP || i == LEFT ? --j : ++j) {
                if (i == UP || i == DOWN ? map[j][square.col] != '?' : map[square.row][j] != '?') {
                    directions[i] = false;
                    break;
                }
            }
        } else directions[i] = false;
    }
}

int count_placeable_dirs(const bool directions[]) {
    int count = 0;
    for (enum directions i = 0; i < 4; ++i)
        if (directions[i] == true) count++;

    return count;
}

enum directions rand_dir(const bool *directions, int num_of_placeable_dirs) {
    enum directions i;
    int random_dir = rand() % num_of_placeable_dirs + 1;
    for (i = UP; random_dir--; ++i)
        while (!directions[i]) ++i;

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

int auto_place_ships(Player *player) {
    int remained_squares = map_size * map_size;
    Ship *ship = player->ships;
    for (int i = 0; i < num_of_ships; ++i, ship = ship->next_ship) {
        int len_of_ship = ship->len;
        if (len_of_ship > 1) {
            int num_of_placeable_dirs = 0;
            while (num_of_placeable_dirs == 0) {
                Square stern, bow = rand_square(player->revealed_map, remained_squares);
                bool placeable_dirs[4] = {true, true, true, true};
                find_placeable_dirs(bow, placeable_dirs, player->revealed_map, --len_of_ship);
                num_of_placeable_dirs = count_placeable_dirs(placeable_dirs);
                if (num_of_placeable_dirs > 0) {
                    stern = find_stern(bow, rand_dir(placeable_dirs, num_of_placeable_dirs), len_of_ship);
                    if (square_compare(stern, bow) == 1) square_swap(&bow, &stern);
                    ship->bow = bow, ship->stern = stern;
                    reveal_ship(bow, stern, player->revealed_map, 'S');
                }
            }
        } else {
            Square bow = rand_square(player->revealed_map, remained_squares);
            ship->bow = bow, ship->stern = bow;
            reveal_ship(bow, bow, player->revealed_map, 'S');
        }
        remained_squares = count_char(player->revealed_map, '?');
    }
    replace_by_W(remained_squares, player->revealed_map);
    print_header();
    display_map(player->revealed_map, player->name);

    // show map and ask to confirm and save if player is not a bot:

    if (player->is_bot) return 1;
    
    printf("Confirm? (y/n) ");
    fflush(stdin);
    char choice = (char) getche();
    if (choice == 'y' || choice == 'Y') return 1;
    system("cls");
    return -1;
}

void how_to_place_ships(Player *player) {
    for (;;) {
        printf("Place ships:\n"
               "1.Auto\n"
               "2.Manually\n");
        char choice = (char) getch();
        system("cls");
        int is_confirmed = -1;
        switch (choice) {
            case '1':
                while (is_confirmed == -1) {
                    allocate_map(&player->revealed_map, '?');
                    is_confirmed = auto_place_ships(player);
                }
                return;
            case '2':
                manually_place_ships(player);
                return;
            default:
                printf("Invalid choice.\n");
                sleep(3);
                system("cls");
        }
    }
}

void move_to_attacked_ship(Player *player, Square shoot) {
    for (int i = 0; i < num_of_ships; ++i) {
        if (player->ships->bow.row <= shoot.row && shoot.row <= player->ships->stern.row &&
            player->ships->bow.col <= shoot.col && shoot.col <= player->ships->stern.col)
            return;
        else
            player->ships = player->ships->next_ship;
    }
}

bool is_ship_sunk(Square shoot, Player opponent) {
    move_to_attacked_ship(&opponent, shoot);

    for (int i = opponent.ships->bow.row; i <= opponent.ships->stern.row; ++i) {
        for (int j = opponent.ships->bow.col; j <= opponent.ships->stern.col; ++j)
            if (opponent.concealed_map[i][j] != 'E') return false;
    }

    return true;
}

void find_sunk_ship(Square *bow, Square *stern, Square shoot, Player opponent) {
    move_to_attacked_ship(&opponent, shoot);
    *bow = opponent.ships->bow, *stern = opponent.ships->stern;
}

void delete_ship(Player *player, Square bow) {
    Ship *temp;
    if (player->ships->bow.row == bow.row && player->ships->bow.col == bow.col) {
        temp = player->ships;
        player->ships = player->ships->next_ship;
    } else {
        Ship *ship = player->ships;
        for (; (!(ship->next_ship->bow.row == bow.row) || !(ship->next_ship->bow.col == bow.col));
               ship = ship->next_ship);

        temp = ship->next_ship;
        if (ship->next_ship->next_ship == NULL)
            ship->next_ship = NULL;
        else
            ship->next_ship = ship->next_ship->next_ship;
    }
    free(temp);
}

void apply_changes(Player *opponent, Square shoot) {
    if (opponent->revealed_map[shoot.row][shoot.col] == 'W')
        opponent->concealed_map[shoot.row][shoot.col] = 'W';
    else {
        opponent->concealed_map[shoot.row][shoot.col] = 'E';

        if (is_ship_sunk(shoot, *opponent)) {
            Square bow, stern;
            find_sunk_ship(&bow, &stern, shoot, *opponent);
            reveal_ship(bow, stern, opponent->concealed_map, 'C');
            delete_ship(opponent, bow);
        }
    }
}

Square find_explosion(char **map, int previous_row, int previous_col) {
    Square explosion;
    for (int i = 0; i < map_size; ++i) {
        for (int j = 0; j < map_size; ++j) {
            if (map[i][j] == 'E' && (i != previous_row || j != previous_col)) {
                explosion.row = i, explosion.col = j;
                return explosion;
            }
        }
    }
}

Square follow_explosions(char **map) {
    Square explos1 = find_explosion(map, -1, -1),
            explos2 = find_explosion(map, explos1.row, explos1.col),
            follower_explos;

    if (explos1.row < explos2.row) {
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

Square shoot_for_bot(char **opponent_map) {
    int num_of_explosions = count_char(opponent_map, 'E');
    if (num_of_explosions == 0)
        return rand_square(opponent_map, count_char(opponent_map, '?'));
    else if (num_of_explosions == 1) {
        Square explosion_square = find_explosion(opponent_map, -1, -1);
        bool placeable_dirs[4] = {true, true, true, true};
        find_placeable_dirs(explosion_square, placeable_dirs, opponent_map, 1);
        return find_stern(explosion_square, rand_dir(placeable_dirs, count_placeable_dirs(placeable_dirs)), 1);
    } else
        return follow_explosions(opponent_map);
}

void ask_to_save() {
    for (;;) {
        printf("1.Save game\n"
               "2.Surrender\n");
        char choice = (char) getch();
        system("cls");
        switch (choice) {
            case '1':
//            save_game();
                return;
            case '2':
                printf("You surrendered to the power of computer! Good luck to you!\n");
                sleep(2);
                return;
            default:
                invalid_choice();
        }
    }
}

int get_choice_from_pause_menu() {
    for (;;) {
        printf("1.Main menu\n"
               "2.Resume\n"
               "0.Quit game\n");
        char choice = (char) getch();
        system("cls");
        switch (choice) {
            case '0':
                ask_to_save();
                exit(EXIT_SUCCESS);
            case '1':
                ask_to_save();
                return RETURN_TO_MAIN_MENU;
            case '2':
                return 2;
            default:
                invalid_choice();
        }
    }
}

int game_loop(Player *player1, Player *player2) {
    int turn = 1;
    while (player1->ships != NULL && player2->ships != NULL) {
        if (turn % 2 || !player2->is_bot) {
            display_page_for_guessing(player1->concealed_map, player2->concealed_map, player1->name, player2->name);
            if (!player2->is_bot) printf("%s's turn.\n", turn % 2 ? player1->name : player2->name);
            printf("Choose a square to shoot (e.g. A1) or cease fire by entering \"pause\": ");
            char command[50];
            fflush(stdin);
            gets(command);
            Square shoot;
            if (strcmpi(command, "pause") == 0) {
                if (get_choice_from_pause_menu() == RETURN_TO_MAIN_MENU)
                    return RETURN_TO_MAIN_MENU;
            } else if (is_placeable_square(command, player2->concealed_map, &shoot)) {
                apply_changes(turn % 2 ? player2 : player1, shoot);
                if (player2->revealed_map[shoot.row][shoot.col] == 'W')
                    turn++;
            } else {
                printf("Invalid shoot or syntax. Please try again.\n");
                sleep(3);
            }
        } else {
            Square shoot = shoot_for_bot(player1->concealed_map);
            if (player1->revealed_map[shoot.row][shoot.col] == 'W') turn++;
            apply_changes(player1, shoot);
            display_page_for_guessing(player1->concealed_map, player2->concealed_map, player1->name, player2->name);
            printf("Computer chose %c%d.\n", shoot.row + 65, shoot.col + 1);
            sleep(2);
        }
    }

    return 0;
}

void end_game(Player player1, Player player2) {
    printf(player2.is_bot ? player1.ships == NULL ? "You were beaten by the power of computer!\n"
                                                  : "Well done captain!\n"
                          : "%s wins!\n", player1.ships == NULL ? player2.name : player1.name);
}

void play_the_game(Player *player1, Player *player2) {
    how_to_place_ships(player1);
    player2->is_bot ? auto_place_ships(player2) : how_to_place_ships(player2);

    if (game_loop(player1, player2) == RETURN_TO_MAIN_MENU) return;
    end_game(*player1, *player2);
}
