#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <conio.h>
#include <unistd.h>
#include <math.h>

#define RETURN_TO_MAIN_MENU (-1)

int map_size, num_of_ships;

typedef struct Ship {
    struct Ship *next_ship;
    int len;
} Ship;

typedef struct {
    char name[100];
    Ship *ships;
    char **revealed_map;
    char **concealed_map;
    long score;
} Player;
Player player1, player2;

typedef struct {
    int row, column;
} Square;

enum {
    UP, RIGHT, DOWN, LEFT
};

void main_menu();

int get_choice();

void help();

void play_vs_bot();

void play_vs_a_friend();

void classic_mode();

void advanced_mode();

void equip_army();

void use_radar();

void bombard();

void load_map();

bool is_valid_placement();

void apply_changes();

bool is_valid_shoot();

void save_shoots();

void save_game();

void load_game();

void display_saved_games_list();

void replay_game();

void display_scoreboard();

void change_settings();

void main_menu() {
    for (;;) {
        printf("1.Play vs computer\n"
               "2.Play vs a friend\n"
               "3.Load a game\n"
               "4.Battle log\n"
               "5.Scoreboard\n"
               "6.Settings\n"
               "7.Help\n"
               "0.Exit\n");
        int choice = get_choice();
        system("cls");
        switch (choice) {
            case 1:
                play_vs_bot();
                break;
            case 2:
                play_vs_a_friend();
                break;
            case 3:
                load_game();
                break;
            case 4:
                replay_game();
                break;
            case 5:
                display_scoreboard();
                break;
            case 6:
                change_settings();
                break;
            case 0:
                exit(EXIT_SUCCESS);
            default:
                printf("Invalid syntax.\n");
        }
    }
}

int main() {
    printf("Welcome to sea battle!\n");
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

int get_choice() {
    int choice;
    scanf("%d", &choice);
    return choice;
}

void ask_to_save() {
    printf("1.Save game\n"
           "2.Surrender\n");
    switch (get_choice()) {
        case 1:
            save_game();
            break;
        case 2:
            printf("You surrender to the power of computer! Good luck to you!\n");
    }
}

int get_choice_from_pause_menu() {
    printf("1.Main menu\n"
           "2.Resume\n"
           "0.Quit game\n");
    switch (get_choice()) {
        case 0:
            ask_to_save();
            return 0;
        case 1:
            ask_to_save();
            return RETURN_TO_MAIN_MENU;
        case 2:
            return 2;
        default:
            printf("Invalid syntax.\n");
    }
}

void print_date() {
    time_t now = time(NULL);
    struct tm *date = localtime(&now);
    printf("%d/%d/%d\n", date->tm_mon + 1, date->tm_mday, date->tm_year + 1900);
}

void print_header() {
    int line_len = printf("Subject: Sea Battle%*sDate: ", map_size * 2, "");
    print_date();
    while (line_len--)
        printf("-");
    printf("\n\n");
}

void print_columns() {
    for (int i = 0; i < map_size; ++i)
        printf("%d ", i + 1);
    printf("\n");
}

void display_map(char **map) {
    print_columns();
    for (int i = 0; i < map_size; ++i) {
        putchar(i + 65);
        for (int j = 0; j < map_size; ++j)
            printf(" %c", map[i][j]);
        printf("\n");
    }
    printf("\n");
}


void print_page_in_bot_mode(char **map1, char **map2) {
    print_header();
    display_map(map1);
    display_map(map2);
}

void add_node(Ship **list, int len_of_ship) {
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

Ship *create_list_of_ships(FILE *settings) {
    Ship *ships = NULL;
    fscanf(settings, "num_of_ships = %d", &num_of_ships);
    for (int i = 0; i < num_of_ships; i++) {
        int ship_len;
        fscanf(settings, "ship%*d_len = %d", &ship_len);
        add_node(&ships, ship_len);
    }

    return ships;
}

bool load_settings(Ship **ships) {
    FILE *settings = fopen("Settings.txt", "r");

    if (settings == NULL) {
        perror("Error opening settings file");
        return false;
    }

    fscanf(settings, "map_size = %d", &map_size);

    *ships = create_list_of_ships(settings);

    // load other settings:

    return true;
}

void copy_list(Ship *source, Ship **dest) {
    while (source != NULL) {
        add_node(dest, source->len);
        source = source->next_ship;
        *dest = (*dest)->next_ship;
    }
}

void allocate_map(char ***map, char initial_value) {
    *map = (char **) malloc(sizeof(char *) * map_size);

    for (int i = 0; i < map_size; ++i) {
        (*map)[i] = (char *) calloc(map_size, sizeof(char));
        memset((*map)[i], initial_value, map_size * sizeof(char));
    }
}

void do_the_allocations() {
    allocate_map(&player1.revealed_map, '?');
    allocate_map(&player2.revealed_map, '?');
    allocate_map(&player1.concealed_map, '?');
    allocate_map(&player2.concealed_map, '?');
}

void display_page_for_placing(char **map) {
    system("cls");
    print_header();
    display_map(map);
}

void put_ship(Square bow, Square stern, char **map) {
    int min_row, max_row, min_col, max_col;
    min_row = fmin(bow.row, stern.row), min_col = fmin(bow.column, stern.column);
    max_row = fmax(bow.column, stern.column), max_col = fmax(bow.column, stern.column);
    for (int i = min_row - 1 >= 0 ? min_row - 1 : min_row;
         i < max_row + 1 < map_size ? max_row + 1 : max_row; ++i) {
        for (int j = min_col - 1 >= 0 ? min_col - 1 : min_col;
             j < max_col + 1 < map_size ? max_col + 1 : max_col; ++j) {
            if (i <= max_col && i >= min_col && j <= max_col && j >= min_col)
                map[i][j] = 'S';
            else
                map[i][j] = 'W';
        }
    }
}

void manually_place_ships(Player *player) {
    for (int i = 0; i < num_of_ships;) {
        display_page_for_placing(player->revealed_map);

        printf("Enter bow and stern of ship%d (e.g. 1A 3A): ", i + 1);
        char chosen_bow[20], chosen_stern[20];
        scanf("%*c%s %[^\n]s%*c", chosen_bow, chosen_stern);
        Square bow, stern;
        bow.row = chosen_bow[0] - '1', stern.row = chosen_stern[0] - '1';
        bow.column = chosen_bow[1] - 'A', bow.column = chosen_stern[1] - 'A';
        if (is_valid_placement(bow, stern))
            put_ship(bow, stern, player->revealed_map);
    }
}

void choose_a_random_square(char **map, int remained_squares, Square *square) {
    while (true) {
        int random_square = rand() % remained_squares;
        for (int j = 0; j < map_size; ++j) {
            for (int k = 0; k < map_size; ++k) {
                if (map[j][k] == '?') {
                    if (random_square == 0)
                        square->row = j, square->column = k;
                    else --remained_squares;
                }
            }
        }
        if (map[square->row][square->column] == '?') return;
    }
}

void find_placeable_directions(Square square, bool *directions, char **map, int len) {
    for (int i = UP; i < 4; ++i) {
        if (i == UP ? square.row - len >= 0
                    : i == RIGHT ? square.column + len < map_size
                                 : i == DOWN ? square.row + len < map_size
                                             : square.column - len >= 0) {
            for (int k = len, j = i == UP || i == DOWN ? square.row : square.column; k--;
                 i == UP || i == LEFT ? --j : ++j) {
                if (i == UP || i == DOWN ? map[j][square.column] != '?' : map[square.row][j] != '?') {
                    directions[i] = false;
                    break;
                }
            }
        } else directions[i] = false;
    }
}

int count_placeable_dirs(bool *directions) {
    int count = 0;
    for (int i = 0; i < 4; ++i)
        if (directions[i] == true) count++;

    return count;
}

int choose_direction(bool directions[], int placeable_dirs) {
    int i, random_dir = rand() % placeable_dirs + 1;
    for (i = 0; random_dir--; ++i)
        while (!directions[i]) ++i;

    return i - 1;
}

void find_stern(Square bow, int direction, Square *stern, int len) {
    switch (direction) {
        case UP:
            stern->column = bow.column;
            stern->row = bow.row - len;
            break;
        case RIGHT:
            stern->row = bow.row;
            stern->column = bow.column + len;
            break;
        case DOWN:
            stern->column = bow.column;
            stern->row = bow.row + len;
            break;
        case LEFT:
            stern->row = bow.row;
            stern->column = bow.column - len;
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

void replace_by_W(int remained_squares, char **map) {
    for (int i = 0; i < map_size; ++i) {
        for (int j = 0; j < map_size; ++j, --remained_squares) {
            if (remained_squares == 0) break;
            else if (map[i][j] == '?')
                map[i][j] = 'W';
        }
    }
}

void auto_place_ships(Player *player) {
    int remained_squares = map_size * map_size;
    for (int i = 0; i < num_of_ships; ++i, player->ships = player->ships->next_ship) {
        int len_of_ship = player->ships->len;
        if (len_of_ship > 1) {
            int num_of_placeable_dirs = 0;
            while (num_of_placeable_dirs == 0) {
                Square bow, stern;
                choose_a_random_square(player->revealed_map, remained_squares, &bow);
                bool placeable_dirs[4] = {true, true, true, true};
                find_placeable_directions(bow, placeable_dirs, player->revealed_map, --len_of_ship);
                num_of_placeable_dirs = count_placeable_dirs(placeable_dirs);
                if (num_of_placeable_dirs > 0) {
                    find_stern(bow, choose_direction(placeable_dirs, num_of_placeable_dirs), &stern, len_of_ship);
                    put_ship(bow, stern, player->revealed_map);
                    remained_squares = count_char(player->revealed_map, '?');
                }
            }
        } else {
            Square bow;
            choose_a_random_square(player->revealed_map, remained_squares, &bow);
            put_ship(bow, bow, player->revealed_map);
            remained_squares = count_char(player->revealed_map, '?');
        }
    }
    replace_by_W(remained_squares, player->revealed_map);
}

void place_ships(Player *player) {
    printf("Place ships:\n"
           "1.Auto\n"
           "2.Manually\n");
    int choice = get_choice();
    switch (choice) {
        case 1:
            auto_place_ships(player);
            break;
        case 2:
            manually_place_ships(player);
            break;
        default:
            printf("Invalid choice.\n");
    }
}

void display_page_for_guessing_vs_bot() {
    system("cls");
    print_header();
    display_map(player1.revealed_map);
    display_map(player2.concealed_map);
    printf("Choose a square to shoot (e.g. 1A) or cease fire by entering \"pause\": ");
}

void play_for_bot() {

}

int game_vs_bot_loop() {
    int turn = 1;
    while (player1.ships != NULL && player2.ships != NULL) {
        if (turn % 2) {
            display_page_for_guessing_vs_bot();
            char command[30];
            gets(command);
            if (strcmpi(command, "pause") == 0 && get_choice_from_pause_menu() == RETURN_TO_MAIN_MENU)
                return RETURN_TO_MAIN_MENU;
            else if (is_valid_shoot(command, player2.concealed_map)) /////
                apply_changes(turn % 2 ? player2.concealed_map : player1.concealed_map, command, &turn);/////
            else {
                printf("Invalid shoot or syntax. Please try again.\n");
                sleep(3);
            }
        } else {
            play_for_bot();
            turn++;
        }
    }

    return 0;
}

void end_game() {

}

void play_vs_bot() {
    player1.ships = NULL, player2.ships = NULL;
    bool is_settings_loaded = load_settings(&(player1.ships));
    if (!is_settings_loaded) return;

    copy_list(player1.ships, &player2.ships);

    do_the_allocations();

    place_ships(&player1);
    auto_place_ships(&player2);

    if (game_vs_bot_loop() == RETURN_TO_MAIN_MENU) return;

    end_game();////
}
