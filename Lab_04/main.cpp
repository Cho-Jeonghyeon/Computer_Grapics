#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include <string>
#include <windows.h>

using namespace std;

const int MAX_SIZE = 6;

char board[MAX_SIZE][MAX_SIZE];
bool opened[MAX_SIZE][MAX_SIZE];
int rowSize;
int colSize;
int totalCard;
int openCount;
int tryCount;
int maxTry;
int score;

void set_color(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void reset_color() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

void clear_screen() {
    system("cls");
}

void print_board(int r1 = -1, int c1 = -1, int r2 = -1, int c2 = -1, bool showAll = false) {
    clear_screen();

    cout << "try: " << tryCount << " / " << maxTry << "   score: " << score << endl;
    cout << "   ";

    for (int c = 0; c < colSize; c++) {
        cout << static_cast<char>('a' + c) << "   ";
    }
    cout << endl;

    for (int r = 0; r < rowSize; r++) {
        cout << r + 1 << "  ";

        for (int c = 0; c < colSize; c++) {
            bool selected = (r == r1 && c == c1) || (r == r2 && c == c2);

            if (opened[r][c] || selected || showAll) {
                char ch = board[r][c];

                if (opened[r][c] && ch != '@') {
                    ch = static_cast<char>(toupper(ch));
                    set_color(10);
                }
                else if (ch == '@') {
                    set_color(14);
                }
                else {
                    set_color(11);
                }

                cout << ch;
                reset_color();
                cout << "   ";
            }
            else {
                cout << "*   ";
            }
        }

        cout << endl;
    }
}

void shuffle_cards(char cards[], int count) {
    for (int i = 0; i < count; i++) {
        int randomIndex = rand() % count;

        char temp = cards[i];
        cards[i] = cards[randomIndex];
        cards[randomIndex] = temp;
    }
}

void init_game() {
    char cards[MAX_SIZE * MAX_SIZE];
    int cardIndex = 0;
    int pairCount = totalCard / 2;

    openCount = 0;
    tryCount = 0;
    score = 0;
    maxTry = totalCard * 2;

    for (int i = 0; i < pairCount; i++) {
        cards[cardIndex++] = static_cast<char>('a' + i);
        cards[cardIndex++] = static_cast<char>('a' + i);
    }

    if (totalCard % 2 == 1) {
        cards[cardIndex++] = '@';
        cout << "joker exists" << endl;
    }
    else {
        cout << "no joker" << endl;
    }

    shuffle_cards(cards, totalCard);

    cardIndex = 0;
    for (int r = 0; r < rowSize; r++) {
        for (int c = 0; c < colSize; c++) {
            board[r][c] = cards[cardIndex++];
            opened[r][c] = false;
        }
    }
}

bool read_position(const string& input, int& row, int& col) {
    if (input.length() < 2) {
        return false;
    }

    char colChar = static_cast<char>(tolower(input[0]));
    char rowChar = input[1];

    col = colChar - 'a';
    row = rowChar - '1';

    if (row < 0 || row >= rowSize || col < 0 || col >= colSize) {
        return false;
    }

    return true;
}

bool is_game_clear() {
    return openCount == totalCard;
}

void open_card(int row, int col) {
    if (!opened[row][col]) {
        opened[row][col] = true;
        openCount++;
    }
}

void check_card(int r1, int c1, int r2, int c2) {
    if (r1 == r2 && c1 == c2) {
        cout << "same card" << endl;
        return;
    }

    if (opened[r1][c1] || opened[r2][c2]) {
        cout << "already opened" << endl;
        return;
    }

    tryCount++;
    print_board(r1, c1, r2, c2);
    Sleep(1000);

    if (board[r1][c1] == board[r2][c2] || board[r1][c1] == '@' || board[r2][c2] == '@') {
        open_card(r1, c1);
        open_card(r2, c2);
        score += 10;
        print_board();
        cout << "match" << endl;
    }
    else {
        score -= 1;
        print_board();
        cout << "not match" << endl;
    }
}

int main() {
    srand(static_cast<unsigned int>(time(0)));

    cout << "input row col: ";
    cin >> rowSize >> colSize;

    if (rowSize < 3 || rowSize > 6 || colSize < 3 || colSize > 6) {
        cout << "size must be 3 to 6" << endl;
        return 0;
    }

    totalCard = rowSize * colSize;
    init_game();
    print_board();

    while (true) {
        string firstInput;
        cin >> firstInput;

        if (firstInput == "q") {
            break;
        }
        else if (firstInput == "r") {
            init_game();
            print_board();
        }
        else if (firstInput == "h") {
            print_board(-1, -1, -1, -1, true);
            Sleep(1000);
            print_board();
        }
        else {
            string secondInput;
            int r1, c1, r2, c2;
            cin >> secondInput;

            if (!read_position(firstInput, r1, c1) || !read_position(secondInput, r2, c2)) {
                cout << "wrong input" << endl;
                print_board();
            }
            else {
                check_card(r1, c1, r2, c2);
            }
        }

        if (is_game_clear()) {
            cout << "game clear" << endl;
            cout << "score = " << score << endl;
            break;
        }

        if (tryCount >= maxTry) {
            cout << "game over" << endl;
            cout << "score = " << score << endl;
            break;
        }
    }

    reset_color();
    return 0;
}
