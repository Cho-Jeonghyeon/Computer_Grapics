#include <iostream>
#include <cstdlib>
#include <ctime>

//using namespace std;

const int SIZE = 4;

void SetRandomMatrix(int matrix[4][4]) {
	for (int rows = 0; rows < SIZE; rows++) {
		for (int cols = 0; cols < SIZE; cols++) {
			matrix[rows][cols] = rand() % 9 + 1;
		}
	}
}

void PrintMatrix(const char* name, int matrix[4][4]) {
	std::cout << name << std::endl;
	for (int rows = 0; rows < SIZE; rows++) {
		std::cout << "| ";
		for (int cols = 0; cols < SIZE; cols++) {
			std::cout << matrix[rows][cols] << " ";
		}
		std::cout << "|" << std::endl;
	}
	std::cout << std::endl;
}

int main() {
	
	int matrix1[SIZE][SIZE];
	int matrix2[SIZE][SIZE];
	char command;

	srand(static_cast<unsigned int>(time(nullptr)));

	SetRandomMatrix(matrix1);
	SetRandomMatrix(matrix2);
	PrintMatrix("Matrix1", matrix1);
	PrintMatrix("Matrix2", matrix2);
	while (true) {
		
		std::cout << "m:*, a:+, d:-, r:det(), t:^t, e:min-, f:max+, +:+1, -:-1, s:reset, q:quit" << std::endl;
		std::cin >> command;

		if (command == 's') {
			SetRandomMatrix(matrix1);
			SetRandomMatrix(matrix2);
			PrintMatrix("Matrix1", matrix1);
			PrintMatrix("Matrix2", matrix2);
		}
		else if (command == 'q') {
			break;
		}
		else {
			std::cout << "try again" << std::endl;
		}
	}
	return 0;
}