#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cctype>
#include <windows.h>

using namespace std;

void PrintOrigin(const vector<string>& sentences) {
    for (int i = 0;i < sentences.size(); i++) {
        cout << sentences[i] << endl;
    }
}

void PrintChange(const vector<string>& sentences) {
    for (int i = 0;i < sentences.size(); i++) {
        for (int j = 0; j < sentences[i].length(); j++) {
			char ch = sentences[i][j];
            if (islower(ch)) {
                cout << static_cast<char>(toupper(ch));
            }
            else if (isupper(ch)) {
                cout << static_cast<char>(tolower(ch));
            }
            else {
				cout << ch;
            }
        }
        cout << endl;
    }
}

void Command_b(const vector<string>& sentences) {
    for (int i = 0;i < sentences.size(); i++) {
        bool word = false;
        int wordcount = 0;
        for (int j = 0;j < sentences[i].length();j++) {
            char ch = sentences[i][j];
            if (ch == ' ') {
                word = false;
            }
            else if (!word) {
                word = true;
                wordcount++;
            }
        }
        cout << sentences[i] << " | WordCount = " << wordcount << endl;
    }
}

void Command_c(const vector<string>& sentences) {
    int uppercount = 0;
    for (int i = 0;i < sentences.size(); i++) {
        bool word = false;

        for (int j = 0; j < sentences[i].length(); j++) {
            char ch = sentences[i][j];
            if (ch == ' ') {
                word = false;
            }
            else if (!word) {
                word = true;
                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
                if (isupper(ch)) {
                    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
                    uppercount++;
                }
            }
            cout << sentences[i][j];
        }
        cout << endl;
    }
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
    cout << "Uppercount = " << uppercount << endl;
}

void Command_d(const vector<string>& sentences) {
    for (int i = 0;i < sentences.size();i++) {
        for (int j = sentences[i].length()-1;j >= 0;j--) {
            cout << sentences[i][j];
        }
        cout << endl;
    }
}

void Command_e(const vector<string>& sentences) {
    for (int i = 0;i < sentences.size();i++) {
        for (int j = 0;j < sentences[i].length();j++) {
            char ch = sentences[i][j];
            if (ch == ' ') {
                ch = '*';
            }
            cout << ch;
        }
        cout << endl;
    }
}

void Command_f(const vector<string>& sentences) {
    for (int i = 0; i < sentences.size();i++) {
        string word = "";
        
        for (int j = 0; j < sentences[i].length();j++) {
            char ch = sentences[i][j];
            if (ch == ' ' || ch == '*') {
                for (int k = word.length() - 1; k >= 0; k--) {
                    cout << word[k];
                }
                word = "";
                cout << ch;
            }
            else {
                word += ch;
            }
        }
        for (int k = word.length() - 1; k >= 0; k--) {
            cout << word[k];
        }
        cout << endl;
    }
}

void Command_g(const vector<string>& sentences, const char change_before, const char change_after) {

    for (int i = 0; i < sentences.size(); i++) {
        for (int j = 0; j < sentences[i].length(); j++) {
            if (sentences[i][j] == change_before) {
                cout << change_after;
            } else {
                cout << sentences[i][j];
            }
        }
        cout << endl;
    }
}

void Command_h(const vector <string>& sentences) {
    for (int i = 0;i < sentences.size(); i++) {
        bool newline = false;
        for (int j = 0;j < sentences[i].length();j++) {
            if (newline && sentences[i][j] == ' ') {
                continue;
            }
            cout << sentences[i][j];

            if (isdigit(sentences[i][j])) {
                cout << endl;
				newline = true;
            }
            else {
				newline = false;
            }
        }
        cout << endl;
    }
}

void Command_i(const vector<string>& sentences, const string& find_word) {
    string lower_find_word = "";
    int find_count = 0;

    for (int i = 0; i < find_word.length(); i++) {
        lower_find_word += static_cast<char>(tolower(find_word[i])); //일단 소문자로
    }

    for (int i = 0; i < sentences.size(); i++) {
        string word = "";

        for (int j = 0; j <= sentences[i].length(); j++) {
            if (j == sentences[i].length() || sentences[i][j] == ' ') { //그 문장을 다 돌았거나 빈칸이면(단어)
                if (word != "") { // 그리고 또한 단어가 비어있지 않다면
                    string lower_word = "";

                    for (int k = 0; k < word.length(); k++) {
                        lower_word += static_cast<char>(tolower(word[k])); //그 단어를 소문자로 바꿈
                    }

                    if (lower_word == lower_find_word) {
                        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
                        cout << word;
                        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
                        find_count++;
                    }
                    else {
                        cout << word;
                    }

                    word = "";
                }

                if (j != sentences[i].length()) {
                    cout << sentences[i][j];
                }
            }
            else {
                word += sentences[i][j]; //단어가 완성될때까지 문자를 더함
            }
        }

        cout << endl;
    }

    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
    cout << "count = " << find_count << endl;
}

void Command_j(const vector<string>& sentences) {
    for (int i = 0; i < sentences.size(); i++) {
        if (i+1 == sentences.size()) {
            i = 0;
        }
        cout << sentences[i+1] << endl;
    }
}

int main() {
    string fileName;
    vector<string> sentences;
    string line;
	char command;
    bool changeMode = false;

    cout << "input data file name: ";
    cin >> fileName;

    ifstream file(fileName);

    if (!file) {
        cout << "file open failed" << endl;
        return 0;
    }

    while (getline(file, line)) {
        sentences.push_back(line);
    }

    for (int i = 0; i < sentences.size(); i++) {
        cout << sentences[i] << endl;
    }

    bool toggle_c = false;
    bool toggle_d = false;
    bool toggle_e = false;
    bool toggle_f = false;
	bool toggle_g = false;
    bool toggle_h = false;
    string find_word;
	char change_before = ' ';
	char change_after = ' ';

    while (true) {
		cin >> command;

        if (command == 'a') {
			changeMode = !changeMode;
            if (changeMode) {
				PrintChange(sentences);
            }
			else {
				PrintOrigin(sentences);
            }
        }
        else if (command == 'b') {
            Command_b(sentences);
        }
        else if (command == 'c') {
            toggle_c = !toggle_c;
            if (toggle_c) {
                Command_c(sentences);
            }
            else {
                PrintOrigin(sentences);
            }
            
        }
        else if (command == 'd') {
            toggle_d = !toggle_d;
            if (toggle_d) {
                Command_d(sentences);
            }
            else {
                PrintOrigin(sentences);
            }
        }
        else if (command == 'e') {
            toggle_e = !toggle_e;
            if (toggle_e) {
                Command_e(sentences);
            }
            else {
                PrintOrigin(sentences);
            }
        }
        else if (command == 'f') {
            toggle_f = !toggle_f;
            if (toggle_f) {
                Command_f(sentences);
            }
            else {
                PrintOrigin(sentences);
            }
        }
        else if (command == 'g') {
            toggle_g = !toggle_g;
            if (toggle_g) {
				cin >> change_before >> change_after;
				Command_g(sentences, change_before, change_after);
			}
            else {
                PrintOrigin(sentences);
                change_before = ' ';
                change_after = ' ';
            }
        }
        else if (command == 'h') {
            toggle_h = !toggle_h;
            if (toggle_h) {
                Command_h(sentences);
            }
            else {
				PrintOrigin(sentences);
            }
        }
        else if (command == 'i') {
            cin >> find_word;
            Command_i(sentences, find_word);
        }
        else if (command == 'j') {
			Command_j(sentences);
        }
		else if (command == 'q') {
			break;
		}
    }
    return 0;
}
