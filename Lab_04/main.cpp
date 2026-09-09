#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include <string>

using namespace std;

const int MAX_SIZE = 6;

char board[MAX_SIZE][MAX_SIZE];
bool opened[MAX_SIZE][MAX_SIZE];
int rowSize;
int colSize;
int totalCard;
int openCount;

void print_board(int r1 = -1, int c1 = -1, int r2 = -1, int c2 = -1) {
    cout << "   ";

    for (int c = 0; c < colSize; c++) {
        cout << static_cast<char>('a' + c) << "   ";
    }
    cout << endl;

    for (int r = 0; r < rowSize; r++) {
        cout << r + 1 << "  ";

        for (int c = 0; c < colSize; c++) {
            bool selected = (r == r1 && c == c1) || (r == r2 && c == c2);

            if (opened[r][c] || selected) {
                cout << board[r][c] << "   ";
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

    print_board(r1, c1, r2, c2);

    if (board[r1][c1] == board[r2][c2] || board[r1][c1] == '@' || board[r2][c2] == '@') {
        open_card(r1, c1);
        open_card(r2, c2);
        cout << "match" << endl;
    }
    else {
        cout << "not match" << endl;
    }

    print_board();
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
    }

    return 0;
}
