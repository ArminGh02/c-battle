#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <windows.h>
#include <conio.h>
#include <unistd.h>
#include "main.h"

int map_size, num_of_ships;

int main() {
    atexit(say_goodbye);

    printf("Sea battle\n"
           "By Armin Ghorbanian\n\n"
           "What shall the history call you? ");
    char name[MAX_LEN_OF_NAME];
    gets(name);
    Player player1 = load_player(name, true);

    srand(time(NULL));
    main_menu(player1);
}

void say_goodbye() {
    system("cls");
    printf("Hope to see you soon.\n");
    sleep(2);
}


Player load_player(const char *name, bool is_welcome_message_needed) {
    FILE *saved_players = fopen("Saved players.bin", "r+b");
    if (saved_players == NULL || seek_player_by_name(name, saved_players) == -1) {
        fclose(saved_players);
        printf("Welcome to sea battle %s!\n", name);
        sleep(1);
        return create_player(name);
    }

    Player player;
    fread(&player, sizeof(Player), 1, saved_players);
    if (is_welcome_message_needed) printf("Welcome back %s.\n", name);
    sleep(1);

    return player;
}

Player create_player(const char *name) {
    Player player = {};
    strcpy(player.name, name);
    player.settings = set_defaults();

    FILE *saved_players = fopen("Saved players.bin", "ab");
    fwrite(&player, sizeof(player), 1, saved_players);
    fclose(saved_players);

    return player;
}

Settings set_defaults() {
    Settings settings = {.map_size = 10, .num_of_ships = 10};

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
    for (int offset = 0; fread(&player, sizeof(Player), 1, saved_players) == 1; ++offset)
        if (strcmp(player.name, name) == 0) {
            fseek(saved_players, (long) -sizeof(Player), SEEK_CUR);
            fflush(saved_players);
            return offset * (int) sizeof(Player);
        }

    return -1;
}


void main_menu(Player player1) {
    Game *game = (Game *) malloc(sizeof(Game));
    game->player1 = player1;
    char choice = '1';
    for (;;) {
        choice = clear_menu(choice, MAIN_MENU);
        switch (choice) {
            case '1':
            case '2':
                *game = create_game(game->player1, choice == '1' ? true : false);
                play_the_game(game, false);
                break;
            case '3': {
                Game *temp = game;
                if ((game = load_game(game->player1.name, false)) != NULL)
                    play_the_game(game, true);
                game = temp;
                break;
            }
            case '4':
                battle_log(game->player1.name);
                break;
            case '5':
                scoreboard();
                break;
            case '6':
                settings_menu(&game->player1);
                break;
            case '0':
                exit(EXIT_SUCCESS);
        }
    }
}

char clear_menu(char choice, enum menu menu) {
    if ('1' <= choice && choice <= menu == MAIN_MENU ? '6' : menu == PAUSE_MENU ? '4' : '3') {
        system("cls");
        return (char) print_menu(menu);
    } else
        return (char) getch();
}

char print_menu(enum menu menu) {
    char choice;
    switch (menu) {
        case MAIN_MENU:
            printf("1.Play vs computer\n2.Play vs a friend\n3.Load a game\n4.Battle log\n5.Scoreboard\n6.Settings"
                   "\n0.Exit\n");
            choice = (char) getch();
            if ('1' <= choice && choice <= '6') system("cls");
            return choice;
        case PAUSE_MENU:
        case SETTINGS_MENU:
            printf(menu == PAUSE_MENU
                   ? "1.Main menu\n2.Save\n3.Resume\n4.Quit game\n"
                   : "1.Change map size.\n2.Change ships settings.\n3.Restore default settings.\n4.Main menu.\n");
            choice = (char) getch();
            if ('1' <= choice && choice <= '4') system("cls");
            return choice;
    }
}


void scoreboard() {
    FILE *saved_players = fopen("Saved players.bin", "r+b"),
            *temp = fopen("tmp.bin", "w+b");
    copy_file(temp, saved_players);
    rewind(saved_players);

    print_scoreboard(saved_players);

    copy_file(saved_players, temp);
    fclose(temp);
    remove("tmp.bin");
    fclose(saved_players);

    printf("Press any key to return to main menu. ");
    getch();
    system("cls");
}

void copy_file(FILE *dest, FILE *source) {
    fflush(dest);
    fflush(source);
    rewind(dest);
    rewind(source);

    Player tmp;
    while (fread(&tmp, sizeof(Player), 1, source) == 1)
        fwrite(&tmp, sizeof(Player), 1, dest);
}

void print_scoreboard(FILE *saved_players) {
    Player *player;
    for (int i = 1; (player = find_player_with_max_score(saved_players)) != NULL; ++i) {
        printf("%2d- %-25s score: %d\n", i, player->name, player->score);

        player->score = -1;
        seek_player_by_name(player->name, saved_players);
        fwrite(player, sizeof(Player), 1, saved_players);
        fflush(saved_players);
    }
}

Player *find_player_with_max_score(FILE *saved_players) {
    int max_score = -1;
    Player temp;
    static Player player_with_max_score;

    rewind(saved_players);
    while (fread(&temp, sizeof(Player), 1, saved_players) == 1) {
        if (max_score < temp.score) {
            max_score = temp.score;
            player_with_max_score = temp;
        }
    }

    return max_score == -1 ? NULL : &player_with_max_score;
}


Game create_game(Player player1, bool is_vs_bot) {
    Game game = {.turn = 1, .date = get_date(), .player1 = player1, .shoots_counter = 0};
    if (is_vs_bot) {
        strcpy(game.player2.name, "Computer");
        game.player2.is_bot = true;
    } else {
        printf("What shall your friend be called throughout the history? ");
        char name[35];
        fflush(stdin);
        gets(name);
        game.player2 = load_player(name, true);
    }

    game.player1.ships = load_settings(player1.settings);
    game.player2.ships = copy_ships_list(game.player1.ships);
    game.player1.remaining_ships = game.player2.remaining_ships = num_of_ships;

    do_the_maps_allocations(&game.player1, &game.player2);

    return game;
}

struct tm get_date() {
    time_t now = time(NULL);
    return *localtime(&now);
}

Ship *load_settings(Settings settings) {
    set_map_size_and_num_of_ships(settings);
    return create_list_of_ships(settings);
}

void set_map_size_and_num_of_ships(Settings settings) {
    map_size = settings.map_size;
    num_of_ships = settings.num_of_ships;
}

Ship *create_list_of_ships(Settings settings) {
    Ship *ships = NULL;
    for (int i = 0; i < num_of_ships; i++)
        add_ship(&ships, settings.lengths_of_ships[i]);

    return ships;
}

void add_ship(Ship **list, int len_of_ship) {
    if (*list == NULL) {
        *list = (Ship *) malloc(sizeof(Ship));
        (*list)->len = len_of_ship;
        (*list)->next_ship = NULL;
    } else {
        Ship *node = *list;
        while (node->next_ship)
            node = node->next_ship;

        node->next_ship = (Ship *) malloc(sizeof(Ship));
        node->next_ship->len = len_of_ship;
        node->next_ship->next_ship = NULL;
    }
}

void do_the_maps_allocations(Player *player1, Player *player2) {
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
    gotoxy(0, 2 * (map_size) + 9);
    if (!is_replay) show_cursor();
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

void display_map(char **map, char *name, int score) {
    printf(score == -1 ? "%s\n" : "%s%*s score: %d\n", name, 3 * map_size - strlen(name) - 7, "", score);
    print_columns();
    for (int i = 0; i < map_size; ++i) {
        putchar(i + 65);
        for (int j = 0; j < map_size; ++j) {
            printf("  ");
            put_colored_char(map[i][j]);
        }
        putchar('\n');
    }
    putchar('\n');
}

void print_columns() {
    printf("   ");
    for (int i = 1; i <= map_size; ++i)
        printf(i >= 10 ? "%d " : "%d  ", i);
    putchar('\n');
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


Game *load_game(char *player1_name, bool is_replay) {
    int total_num_of_games = display_saved_games(player1_name, is_replay);
    if (total_num_of_games == 0) {
        printf(is_replay ? "You haven't completed any battle yet.\n" : "No saved games were found.\n");
        sleep(1);
        return NULL;
    }

    int choice = get_chosen_game_num(total_num_of_games);
    system("cls");

    FILE *saved_games = fopen(is_replay ? "Saved replays.bin" : "Saved games.bin", "rb");
    Game *game = find_chosen_game(choice, player1_name, saved_games, is_replay);

    set_map_size_and_num_of_ships(game->player1.settings);
    do_the_maps_allocations(&game->player1, &game->player2);
    is_replay ? prepare_players_of_replay(game, saved_games)
              : prepare_players_of_loaded_game(game, saved_games, player1_name);

    fclose(saved_games);
    return game;
}

int display_saved_games(char *player1_name, bool is_replay) {
    FILE *saved_games = fopen(is_replay ? "Saved replays.bin" : "Saved games.bin", "rb");
    if (saved_games == NULL) return 0;

    Game game;
    int game_counter = 1;
    while (fread(&game, sizeof(Game), 1, saved_games) == 1) {
        if (strcmp(game.player1.name, player1_name) == 0 || strcmp(game.player2.name, player1_name) == 0)
            printf("%2d- %-15s %d/%d/%d\n\t%s vs %s\n", game_counter++, game.name, game.date.tm_mon + 1,
                   game.date.tm_mday,
                   game.date.tm_year + 1900, game.player1.name, game.player2.name);

        fseek(saved_games,
              is_replay
              ? 2 * (long) sizeof(char) * game.player1.settings.map_size * game.player1.settings.map_size
              : 4 * (long) sizeof(char) * game.player1.settings.map_size * game.player1.settings.map_size
                + (long) sizeof(Ship) * (game.player1.remaining_ships + game.player2.remaining_ships), SEEK_CUR);
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
        else printf("The choice must be between 1 and %d\n", total_num_of_games);
    }
}

Game *find_chosen_game(int choice, char *player1_name, FILE *saved_games, bool is_replay) {
    Game *game = (Game *) malloc(sizeof(Game));
    for (int i = 0; i < choice;) {
        fread(game, sizeof(Game), 1, saved_games);
        if (strcmp(game->player1.name, player1_name) == 0 || strcmp(game->player2.name, player1_name) == 0) i++;

        if (i != choice)
            fseek(saved_games,
                  is_replay
                  ? 2 * (long) sizeof(char) * game->player1.settings.map_size * game->player1.settings.map_size
                  : 4 * (long) sizeof(char) * game->player1.settings.map_size * game->player1.settings.map_size
                    + (long) sizeof(Ship) * (game->player1.remaining_ships + game->player2.remaining_ships), SEEK_CUR);
    }

    return game;
}

void prepare_players_of_loaded_game(Game *game, FILE *saved_games, char *player1_name) {
    game->player1.score = load_score(player1_name, game->player1.name);
    if (!game->player2.is_bot) game->player2.score = load_score(player1_name, game->player2.name);

    load_4_maps_of_game(game, saved_games);

    game->player1.ships = load_ships_list(saved_games, game->player1.remaining_ships);
    game->player2.ships = load_ships_list(saved_games, game->player2.remaining_ships);
}

void prepare_players_of_replay(Game *game, FILE *saved_games) {
    game->player1.remaining_ships = game->player2.remaining_ships = game->player1.settings.num_of_ships;
    game->turn = 1;
    game->shoots_counter = 0;

    fread_map(game->player1.revealed_map, saved_games);
    fread_map(game->player2.revealed_map, saved_games);

    game->player1.ships = create_list_of_ships(game->player1.settings);
    game->player2.ships = copy_ships_list(game->player1.ships);

    restore_ships(&game->player1.ships, (const char **) game->player1.revealed_map);
    restore_ships(&game->player2.ships, (const char **) game->player2.revealed_map);
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

void restore_ships(Ship **ships, const char **map) {
    for (Ship *node = *ships;;) {
        restore_ships_with_equal_length(&node, map);
        if (node == NULL) return;
    }
}

void restore_ships_with_equal_length(Ship **node, const char **map) {
    for (int i = 0; i < map_size; ++i) {
        for (int j = 0; j < map_size; ++j) {
            if (map[i][j] == 'S') {
                if (i - 1 >= 0 && map[i - 1][j] == 'S') continue;
                if (j - 1 >= 0 && map[i][j - 1] == 'S') continue;

                if ((*node)->len == 1) {
                    if ((i + 1 > map_size - 1 || map[i + 1][j] == 'W') &&
                        (j + 1 > map_size - 1 || map[i][j + 1] == 'W')) {
                        (*node)->bow = (*node)->stern = (Square) {i, j};
                        (*node) = (*node)->next_ship;
                        if (*node == NULL) return;
                        continue;
                    } else continue;
                }

                if (i + (*node)->len - 1 < map_size && map[i + (*node)->len - 1][j] == 'S' &&
                    (i + (*node)->len > map_size - 1 || map[i + (*node)->len][j] == 'W')) {
                    int k = i + 1, found = 1;
                    for (int l = (*node)->len - 2; l--; ++k) {
                        if (map[k][j] != 'S') found = 0;
                    }
                    if (!found) goto there;

                    (*node)->bow = (Square) {i, j};
                    (*node)->stern = (Square) {k, j};

                    if ((*node)->next_ship->len != (*node)->len) {
                        (*node) = (*node)->next_ship;
                        return;
                    }
                    (*node) = (*node)->next_ship;
                    continue;
                    there:;
                }
                if (j + (*node)->len - 1 < map_size && map[i][j + (*node)->len - 1] == 'S' &&
                    (j + (*node)->len > map_size - 1 || map[i][j + (*node)->len] == 'W')) {
                    int copy_of_j = j++, found = 1;
                    for (int l = (*node)->len - 2; l--; ++j) {
                        if (map[i][j] != 'S') found = 0;
                    }
                    if (!found) continue;

                    (*node)->bow = (Square) {i, copy_of_j};
                    (*node)->stern = (Square) {i, j};

                    if ((*node)->next_ship->len != (*node)->len) {
                        (*node) = (*node)->next_ship;
                        return;
                    }
                    (*node) = (*node)->next_ship;
                }
            }
        }
    }
}

Ship *load_ships_list(FILE *saved_games, int remaining_ships) {
    Ship *ships, *temp;
    ships = temp = (Ship *) malloc(sizeof(Ship));
    fread(ships, sizeof(Ship), 1, saved_games);

    for (int i = 1; i < remaining_ships; ++i, temp = temp->next_ship) {
        temp->next_ship = (Ship *) malloc(sizeof(Ship));
        fread(temp->next_ship, sizeof(Ship), 1, saved_games);
    }
    temp->next_ship = NULL;

    return ships;
}

int load_score(const char *player1_name, const char *player_to_load_name) {
    return load_player(player_to_load_name, strcmp(player_to_load_name, player1_name) == 0 ? false : true).score;
}


void battle_log(char *name) {
    Game *game = load_game(name, true);
    if (game != NULL) replay(*game);
}

void replay(Game game) {
    hide_cursor();
    while (!is_game_ended(game.player1.ships, game.player2.ships)) {
        display_screen_for_guessing(game.player1, game.player2, true);
        sleep(1);

        update_game(&game, game.shoots[game.shoots_counter]);
        control_turns(&game, game.shoots[game.shoots_counter++]);
    }
    display_screen_for_guessing(game.player1, game.player2, true);

    show_cursor();
    game_over_message(game);
    printf("\nPress any key to return to main menu. ");
    getch();
}

void play_the_game(Game *game, bool is_a_loaded_game) {
    if (!is_a_loaded_game) {
        how_to_place_ships(&game->player1);
        game->player2.is_bot ? auto_arrange_map(&game->player2) : how_to_place_ships(&game->player2);
    }
    if (game_loop(game) == RETURN_TO_MAIN_MENU) return;
    end_game(*game);
}

int game_loop(Game *game) {
    while (!is_game_ended(game->player1.ships, game->player2.ships)) {
        if (game->turn % 2 || !game->player2.is_bot) {
            display_screen_for_guessing(game->player1, game->player2, false);

            char command[50];
            get_command(command, game->player1, game->player2, game->turn);

            Square shoot;
            if (strcmpi(command, "pause") == 0) {
                if (get_choice_from_pause_menu(game) == RETURN_TO_MAIN_MENU) return RETURN_TO_MAIN_MENU;
            } else if (is_placeable_square(game->turn % 2 ? game->player2.concealed_map : game->player1.concealed_map,
                                           command, &shoot)) {
                update_game(game, shoot);
                game->shoots[game->shoots_counter++] = shoot;
                control_turns(game, shoot);
            } else {
                printf("Invalid shoot or syntax. Please try again.\n");
                sleep(1);
            }
        } else
            play_for_bot(game);
    }
    display_screen_for_guessing(game->player1, game->player2, false);

    return 0;
}

bool is_game_ended(Ship *player1_ships, Ship *player2_ships) {
    return player1_ships == NULL || player2_ships == NULL;
}

void get_command(char command[], Player player1, Player player2, int turn) {
    if (!player2.is_bot)
        printf("%s's turn.\n", turn % 2 ? player1.name : player2.name);
    printf("Choose a square to shoot (e.g. A1) or cease fire by entering \"pause\": ");
    scanf(" %[^\n]s", command);
}

void how_to_place_ships(Player *player) {
    system("cls");
    printf("Place ships:\n"
           "1.Auto\n"
           "2.Manually\n");
    for (;;) {
        char choice = (char) getch();
        switch (choice) {
            case '1':
                system("cls");
                auto_arrange_map(player);
                system("cls");
                return;
            case '2':
                system("cls");
                manually_place_ships(player);
                system("cls");
                return;
        }
    }
}

void auto_arrange_map(Player *player) {
    bool is_confirmed;
    do {
        clear_map(player->revealed_map);

        int empty_squares = map_size * map_size;
        auto_place_ships(player, &empty_squares);
        fill_empty_squares_with_water(empty_squares, player->revealed_map);

        if (player->is_bot) return;

        hide_cursor();
        gotoxy(0, 0);
        print_header();
        display_map(player->revealed_map, player->name, player->score);
        show_cursor();
        is_confirmed = get_confirmation();
    } while (!is_confirmed);
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

bool get_confirmation() {
    printf("Confirm? (y/n) ");
    for (;;) {
        char choice = (char) getch();
        if (choice == 'y' || choice == 'Y') return true;
        else if (choice == 'n' || choice == 'N') return false;
    }
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

                if ((num_of_placeable_dirs = count_placeable_dirs(placeable_dirs)) > 0) {
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

enum directions rand_dir(const enum placeability directions[], int num_of_placeable_dirs) {
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
    bool is_confirmed;
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
        is_confirmed = get_confirmation();
    } while (!is_confirmed);
}

void get_bow_and_stern(char bow[], char stern[], int ship_len, int ship_num) {
    printf(ship_len == 1 ? "Enter a square to place ship%d with length %d (e.g. A2): "
                         : "Enter bow and stern of ship%d with length %d (e.g. A1 A%d): ",
           ship_num, ship_len, ship_len);
    scanf(ship_len == 1 ? "%s" : "%s %s", bow, stern);
    if (ship_len == 1) strcpy(stern, bow);
}

bool is_valid_ship_placement(char *chosen_bow, char *chosen_stern, Ship *ship, char **map) {
    if (!is_placeable_square(map, chosen_bow, &ship->bow) ||
        !is_placeable_square(map, chosen_stern, &ship->stern))
        return false;

    if (square_compare(ship->stern, ship->bow) == 1) square_swap(&ship->stern, &ship->bow);

    if (is_vertical(ship->bow, ship->stern) == 1) {
        if (ship->stern.row - ship->bow.row != ship->len - 1)
            return false;
        for (int i = ship->bow.row + 1; i < ship->stern.row; ++i)
            if (map[i][ship->bow.col] != '?') return false;
    } else if (is_vertical(ship->bow, ship->stern) == -1) {
        if (ship->stern.col - ship->bow.col != ship->len - 1)
            return false;
        for (int i = ship->bow.col + 1; i < ship->stern.col; ++i)
            if (map[ship->bow.row][i] != '?') return false;
    } else return false;

    return true;
}

bool is_placeable_square(char **map, char *chosen_square, Square *square_coordinates) {
    square_coordinates->row = isupper(chosen_square[0]) ? chosen_square[0] - 'A' : chosen_square[0] - 'a';
    sscanf(chosen_square + 1, "%d", &square_coordinates->col);

    if (strlen(chosen_square) > 3 || !isalpha(chosen_square[0]) ||
        !isdigit(chosen_square[1]) || (strlen(chosen_square) == 3 && !isdigit(chosen_square[2])) ||
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
    game->shoots[game->shoots_counter++] = shoot;

    control_turns(game, shoot);

    display_screen_for_guessing(game->player1, game->player2, false);
    apply_changes(&game->player1, shoot, NULL, game->player1.settings.lengths_of_ships);
    sleep(1);
    display_screen_for_guessing(game->player1, game->player2, false);
    printf("Computer shot at %c%d.\n", shoot.row + 65, shoot.col + 1);
    sleep(1);
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
    } else return follow_explosions(opponent_map);
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

    if (is_vertical(explos1, explos2) == 1) {
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
    if (opponent->revealed_map[shoot.row][shoot.col] == 'W')
        opponent->concealed_map[shoot.row][shoot.col] = 'W';
    else {
        opponent->concealed_map[shoot.row][shoot.col] = 'E';
        if (attacker_score != NULL) (*attacker_score)++;

        Ship attacked_ship = find_attacked_ship(opponent->ships, shoot);
        if (is_ship_sunk(attacked_ship, *opponent)) {
            if (attacker_score != NULL)
                *attacker_score += (5 * max_len_of_ships(lengths_of_ships, false) / attacked_ship.len) / 2;

            reveal_ship(attacked_ship, opponent->concealed_map, 'C');
            delete_ship(opponent, attacked_ship.bow);
            opponent->remaining_ships--;

            if (opponent->ships == NULL && attacker_score != NULL)
                *attacker_score += win_bonus(lengths_of_ships);
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

Ship find_attacked_ship(Ship *opponents_ships, Square shoot) {
    for (;; opponents_ships = opponents_ships->next_ship) {
        if (opponents_ships->bow.row <= shoot.row && shoot.row <= opponents_ships->stern.row &&
            opponents_ships->bow.col <= shoot.col && shoot.col <= opponents_ships->stern.col)
            return *opponents_ships;
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

int win_bonus(const int lengths_of_ships[]) {
    int bonus = 0, max_len = max_len_of_ships(lengths_of_ships, false);
    for (int i = 0; i < num_of_ships; ++i)
        bonus += (5 * max_len / lengths_of_ships[i]) / 2;

    return bonus;
}

int max_len_of_ships(const int lengths_of_ships[], bool is_the_game_ended) {
    static int max_len = 0;
    static bool is_max_len_calculated = false;
    if (is_the_game_ended) {
        is_max_len_calculated = false;
        return 0;
    }
    if (!is_max_len_calculated) {
        for (int i = 0; i < num_of_ships; ++i) {
            if (max_len < lengths_of_ships[i])
                max_len = lengths_of_ships[i];
        }

        is_max_len_calculated = true;
    }

    return max_len;
}

void control_turns(Game *game, Square shoot) {
    if (game->turn % 2 ? game->player2.revealed_map[shoot.row][shoot.col] == 'W'
                       : game->player1.revealed_map[shoot.row][shoot.col] == 'W')
        game->turn++;
}


int get_choice_from_pause_menu(Game *game) {
    max_len_of_ships(game->player1.settings.lengths_of_ships, true);
    char choice = '1';
    for (;;) {
        choice = clear_menu(choice, PAUSE_MENU);
        if ('1' <= choice && choice <= '4') update_players_in_file(&game->player1, &game->player2);
        switch (choice) {
            case '1':
                ask_to_save_game(*game);
                return RETURN_TO_MAIN_MENU;
            case '2':
                save_game(*game, false);
                break;
            case '3':
                return 3;
            case '4':
                ask_to_save_game(*game);
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
            save_game(game, false);
            return;
        } else if (choice == '2') {
            system("cls");
            printf(game.player2.is_bot ? "You surrendered to the power of computer!\n"
                                       : "You surrendered to the power of the rival!\n");
            sleep(2);
            return;
        }
    }
}

void save_game(Game game, bool is_replay) {
    printf("Enter a name for the game: ");
    fflush(stdin);
    gets(game.name);

    FILE *saved_games = fopen(is_replay ? "Saved replays.bin" : "Saved games.bin", "ab");
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
    sleep(1);
}

void save_ships_list(Game game, FILE *saved_games) {
    for (int i = 0; i < game.player1.remaining_ships; ++i, game.player1.ships = game.player1.ships->next_ship)
        fwrite(game.player1.ships, sizeof(Ship), 1, saved_games);
    for (int i = 0; i < game.player2.remaining_ships; ++i, game.player2.ships = game.player2.ships->next_ship)
        fwrite(game.player2.ships, sizeof(Ship), 1, saved_games);
}

void save_4_maps_of_game(Game game, FILE *saved_games) {
    fwrite_map(game.player1.revealed_map, saved_games);
    fwrite_map(game.player2.revealed_map, saved_games);
    fwrite_map(game.player1.concealed_map, saved_games);
    fwrite_map(game.player2.concealed_map, saved_games);
}

void fwrite_map(char **map, FILE *file_to_write_to) {
    for (int i = 0; i < map_size; ++i)
        fwrite(map[i], sizeof(char), map_size, file_to_write_to);
}


void end_game(Game game) {
    max_len_of_ships(game.player1.settings.lengths_of_ships, true);
    update_players_in_file(&game.player1, &game.player2);
    game_over_message(game);
    save_game(game, true);
    game_over_menu();
}

void update_players_in_file(Player *player1, Player *player2) {
    FILE *saved_players = fopen("Saved players.bin", "r+b");
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

void game_over_menu() {
    printf("Press any key to continue. ");
    getch();
    system("cls");
    printf("1.Main menu\n"
           "2.Quit\n");
    for (;;) {
        char choice = (char) getch();
        if (choice == '1') {
            system("cls");
            return;
        } else if (choice == '2') exit(EXIT_SUCCESS);
    }
}


void settings_menu(Player *player) {
    char choice = '1';
    for (;;) {
        choice = clear_menu(choice, SETTINGS_MENU);
        switch (choice) {
            case '1':
                change_map_size(player);
                break;
            case '2':
                change_ships_settings(player);
                break;
            case '3':
                player->settings = set_defaults();
                printf("Done.\n");
                sleep(1);
                break;
            case '4':
                return;
        }
        if ('1' <= choice && choice <= '3') update_players_in_file(player, NULL);
    }
}

void change_map_size(Player *player) {
    player->settings.map_size = get_map_size_or_num_of_ships(1);
    printf("Length of map changed successfully.\n");
    sleep(1);
}

void change_ships_settings(Player *player) {
    player->settings.num_of_ships = get_map_size_or_num_of_ships(2);

    for (int i = 1; i <= player->settings.num_of_ships; ++i) {
        printf("Enter length of ship%d: ", i);
        scanf("%d", &player->settings.lengths_of_ships[i]);
        if (player->settings.lengths_of_ships[i] >= 10 || player->settings.lengths_of_ships[i] < 1) {
            printf(player->settings.lengths_of_ships[i] >= 7
                   ? "Maximum length of ship is 7.\n"
                   : "As you might know, length of a ship is a natural number!\n");
            i--;
            sleep(2);
            system("cls");
            continue;
        }
    }

    qsort(player->settings.lengths_of_ships, player->settings.num_of_ships, sizeof(int), integer_compare_descending);

    printf("Settings changed successfully.\n");
    sleep(1);
}

int integer_compare_descending(const void *integer1, const void *integer2) {
    return (*((int *) integer1) > *((int *) integer2)) ? -1 : (*((int *) integer1) == *((int *) integer2)) ? 0 : 1;
}

int get_map_size_or_num_of_ships(int choice_from_settings_menu) {
    int new_map_size, new_num_of_ships;
    for (;;) {
        system("cls");
        printf(choice_from_settings_menu == 1 ? "Enter new length of map: " : "Enter new number of ships: ");
        scanf("%d", choice_from_settings_menu == 1 ? &new_map_size : &new_num_of_ships);

        int scanned_value = choice_from_settings_menu == 1 ? new_map_size : new_num_of_ships;
        if (scanned_value < 7 || scanned_value > 20) {
            printf(scanned_value > 20 ? "Maximum value is 20.\n" : "Minimum value is 7.\n");
            sleep(1);
            continue;
        }

        return choice_from_settings_menu == 1 ? new_map_size : new_num_of_ships;
    }
}

void hide_cursor() {
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = FALSE;
    SetConsoleCursorInfo(console_handle, &info);
}

void show_cursor() {
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 10;
    info.bVisible = TRUE;
    SetConsoleCursorInfo(console_handle, &info);
}

// THIS WAS FOR SAVING A MAP THAT PLAYER HAS CONFIRMED BUT ISN'T FINISHED:
//
//void ask_to_save_map(Player player) {
//    printf("%100s\rWould you like to save this map? (y/n) ", "");
//    for (;;) {
//        char choice = (char) getch();
//        if (choice == 'y' || choice == 'Y') {
//            save_map(player);
//            return;
//        } else if (choice == 'n' || choice == 'N') return;
//    }
//}
//
//void save_map(Player player) {
//    SavedMapInfo saved_map = {.size = map_size, .date = get_date()};
//    strcpy(saved_map.player_name, player.name);
//
//    FILE *saved_maps_file = fopen("Saved maps.bin", "ab");
//    fwrite(&saved_map, sizeof(saved_map), 1, saved_maps_file);
//    fclose(saved_maps_file);
//    printf("\nMap saved successfully.");
//    sleep(1);
//}