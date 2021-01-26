#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <windows.h>

int map_size, num_of_ships;

char **revealed_map1, **revealed_map2, **concealed_map1, **concealed_map2;

typedef struct Ship {
    struct Ship *next_ship;
    int len;
} Ship;

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

void auto_place_ships(char **map);

void save_map(char **map);

void load_ships_arrangement();

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

int main() {
    printf("Welcome to sea battle!\n");

    for (;;) {
        main_menu();
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
                exit(0);
            default:
                printf("Invalid syntax.\n");
        }
    }
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

void main_menu() {
    printf("1.Play vs computer\n"
           "2.Play vs a friend\n"
           "3.Load a game\n"
           "4.Replay a game\n"
           "5.Scoreboard\n"
           "6.Settings\n"
           "7.Help\n"
           "0.Exit\n");
}

void pause_menu() {
    printf("1.Arsenal\n"
           "2.Main menu\n"
           "0.Quit game\n");
}

int get_choice() {
    int choice;
    scanf("%d", &choice);
    return choice;
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
        printf("%d ");
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

void allocate_map(char **map, char initial_value) {
    map = (char **) malloc(sizeof(char *) * map_size);

    for (int i = 0; i < map_size; ++i) {
        map[i] = (char *) calloc(map_size, sizeof(char));
        memset(map[i], initial_value, map_size * sizeof(char));
    }
}

void display_page_for_placing(char **map) {
    system("cls");
    print_header();
    display_map(map);
}

void manually_place_ships(char **map) {
    for (int i = 0; i < num_of_ships;) {
        display_page_for_placing(map);

        printf("Enter bow and stern of ship%d (e.g. 1A 3A): ", i + 1);
        char chosen_bow[20], chosen_stern[20];
        scanf("%*c%s %[^\n]s%*c", chosen_bow, chosen_stern);
        is_valid_placement(chosen_bow, chosen_stern);
    }
}

void auto_place_ships(char **map) {

}

void place_ships(char **map) {
    printf("Place ships:\n"
           "1.Auto\n"
           "2.Manually\n");
    int choice = get_choice();
    switch (choice) {
        case 1:
            auto_place_ships(map);
            break;
        case 2:
            manually_place_ships(map);
            break;
        default:
            printf("Invalid choice.\n");
    }
}

void display_page_for_guessing_vs_bot() {
    system("cls");
    print_header();
    display_map(revealed_map1);
    display_map(concealed_map2);
    printf("Choose a square to shoot: ");
}

void play_vs_bot() {
    Ship *player1_ships = NULL, *player2_ships = NULL;
    bool is_settings_loaded = load_settings(&player1_ships);
    if (!is_settings_loaded) return;
    copy_list(player1_ships, &player2_ships);

    allocate_map(revealed_map1, 'W');
    allocate_map(revealed_map2, 'W');
    allocate_map(concealed_map1, '?');
    allocate_map(concealed_map2, '?');

    place_ships(revealed_map1);/////
    auto_place_ships(revealed_map2);/////

    int turn = 1;
    while (player1_ships != NULL && player2_ships != NULL) {
        display_page_for_guessing_vs_bot();
        char shot_square[20];
        gets(shot_square);
        if (is_valid_shoot(shot_square, concealed_map2)) {/////
            apply_changes(shot_square, turn % 2 ? concealed_map2 : concealed_map1);/////
            turn++;
        } else
            printf("Invalid shoot. Please try again.\n");
    }
}

