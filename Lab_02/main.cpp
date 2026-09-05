//#include <iostream>
//#include <string>
//#include <fstream>
//
//using namespace std;
//
//const int SIZE = 10;
//
//int main() {
//	string line[SIZE];
//	string filename;
//
//	cout << "file name ? : ";
//	cin >> filename;
//
//	ifstream file(filename);
//
//	if (!file) {
//		cout << "Failed to open the file." << endl;
//		return 1;
//	}
//	for (int i = 0;i < SIZE; i++) {
//		getline(file, line[i]);
//	}
//	file.close();
//
//	for (int i = 0;i < SIZE;i++) {
//		cout << line[i] << endl;
//	}
//	return 0;
//}
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cctype>

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
		else if (command == 'q') {
			break;
		}
    }
    return 0;
}
