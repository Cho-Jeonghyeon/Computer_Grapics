#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>

using namespace std;

const int MAX_SIZE = 6;

char board[MAX_SIZE][MAX_SIZE];
bool opened[MAX_SIZE][MAX_SIZE];
int rowSize;
int colSize;
int totalCard;

void print_board() {
    cout << "   ";

    for (int c = 0; c < colSize; c++) {
        cout << static_cast<char>('a' + c) << "   ";
    }
    cout << endl;

    for (int r = 0; r < rowSize; r++) {
        cout << r + 1 << "  ";

        for (int c = 0; c < colSize; c++) {
            if (opened[r][c]) {
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
        string input;
        cin >> input;

        if (input == "q") {
            break;
        }
    }

    return 0;
}
