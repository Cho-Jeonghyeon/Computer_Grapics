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

void CopyMatrix(int source[4][4], int target[4][4]) {
	for (int rows = 0; rows < SIZE; rows++) {
		for (int cols = 0; cols < SIZE; cols++) {
			target[rows][cols] = source[rows][cols];
		}
	}
}

void AddMatrix(int matrix1[4][4], int matrix2[4][4], int result[4][4]) {
	for (int rows = 0; rows < SIZE; rows++) {
		for (int cols = 0; cols < SIZE; cols++) {
			result[rows][cols] = matrix1[rows][cols] + matrix2[rows][cols];
		}
	}
}

void SubtractMatrix(int matrix1[4][4], int matrix2[4][4], int result[4][4]) {
	for (int rows = 0; rows < SIZE; rows++) {
		for (int cols = 0; cols < SIZE; cols++) {
			result[rows][cols] = matrix1[rows][cols] - matrix2[rows][cols];
		}
	}
}

void MultiplyMatrix(int matrix1[4][4], int matrix2[4][4], int result[4][4]) {
	for (int rows = 0; rows < SIZE; rows++) {
		for (int cols = 0; cols < SIZE; cols++) {
			result[rows][cols] = 0;
			for (int index = 0; index < SIZE; index++) {
				result[rows][cols] += matrix1[rows][index] * matrix2[index][cols];
			}
		}
	}
}

void TransposeMatrix(int matrix[4][4], int result[4][4]) {
	for (int rows = 0; rows < SIZE; rows++) {
		for (int cols = 0; cols < SIZE; cols++) {
			result[cols][rows] = matrix[rows][cols];
		}
	}
}

void SubtractRowMin(int matrix[4][4]) {
	for (int rows = 0; rows < SIZE; rows++) {
		int minValue = matrix[rows][0];

		for (int cols = 1; cols < SIZE; cols++) {
			if (matrix[rows][cols] < minValue) {
				minValue = matrix[rows][cols];
			}
		}

		for (int cols = 0; cols < SIZE; cols++) {
			matrix[rows][cols] -= minValue;
		}
	}
}

void AddColumnMax(int matrix[4][4]) {
	for (int cols = 0; cols < SIZE; cols++) {
		int maxValue = matrix[0][cols];

		for (int rows = 1; rows < SIZE; rows++) {
			if (matrix[rows][cols] > maxValue) {
				maxValue = matrix[rows][cols];
			}
		}

		for (int rows = 0; rows < SIZE; rows++) {
			matrix[rows][cols] += maxValue;
		}
	}
}

void AddOneModulo(int matrix[4][4]) {
	for (int rows = 0; rows < SIZE; rows++) {
		for (int cols = 0; cols < SIZE; cols++) {
			matrix[rows][cols] = (matrix[rows][cols] + 1) % 10;
		}
	}
}

void SubtractOneModulo(int matrix[4][4]) {
	for (int rows = 0; rows < SIZE; rows++) {
		for (int cols = 0; cols < SIZE; cols++) {
			matrix[rows][cols] = (matrix[rows][cols] + 9) % 10;
		}
	}
}

void UpdateCurrentMatrix(int baseMatrix1[4][4], int baseMatrix2[4][4], int matrix1[4][4], int matrix2[4][4], bool rowMinMode, bool colMaxMode) {
	CopyMatrix(baseMatrix1, matrix1);
	CopyMatrix(baseMatrix2, matrix2);

	if (rowMinMode) {
		SubtractRowMin(matrix1);
		SubtractRowMin(matrix2);
	}

	if (colMaxMode) {
		AddColumnMax(matrix1);
		AddColumnMax(matrix2);
	}
}

int Determinant(int matrix[4][4]) {
	int det = 0;

	for (int col = 0; col < SIZE; col++) {
		int minor[3][3];

		for (int row = 1; row < SIZE; row++) {
			int minorCol = 0;
			for (int currentCol = 0; currentCol < SIZE; currentCol++) {
				if (currentCol == col) {
					continue;
				}
				minor[row - 1][minorCol] = matrix[row][currentCol];
				minorCol++;
			}
		}

		int minorDet =
			minor[0][0] * (minor[1][1] * minor[2][2] - minor[1][2] * minor[2][1])
			- minor[0][1] * (minor[1][0] * minor[2][2] - minor[1][2] * minor[2][0])
			+ minor[0][2] * (minor[1][0] * minor[2][1] - minor[1][1] * minor[2][0]);

		if (col % 2 == 0) {
			det += matrix[0][col] * minorDet;
		}
		else {
			det -= matrix[0][col] * minorDet;
		}
	}

	return det;
}

int main() {
	
	int baseMatrix1[SIZE][SIZE];
	int baseMatrix2[SIZE][SIZE];
	int matrix1[SIZE][SIZE];
	int matrix2[SIZE][SIZE];
	int result[SIZE][SIZE];
	char command;
	bool rowMinMode = false;
	bool colMaxMode = false;

	srand(static_cast<unsigned int>(time(nullptr)));

	SetRandomMatrix(baseMatrix1);
	SetRandomMatrix(baseMatrix2);
	UpdateCurrentMatrix(baseMatrix1, baseMatrix2, matrix1, matrix2, rowMinMode, colMaxMode);
	PrintMatrix("Matrix1", matrix1);
	PrintMatrix("Matrix2", matrix2);
	while (true) {
		
		std::cout << "m:*, a:+, d:-, r:det(), t:^t, e:min-, f:max+, +:+1, -:-1, s:reset, q:quit" << std::endl;
		std::cin >> command;

		if (command == 's') {
			rowMinMode = false;
			colMaxMode = false;
			SetRandomMatrix(baseMatrix1);
			SetRandomMatrix(baseMatrix2);
			UpdateCurrentMatrix(baseMatrix1, baseMatrix2, matrix1, matrix2, rowMinMode, colMaxMode);
			PrintMatrix("Matrix1", matrix1);
			PrintMatrix("Matrix2", matrix2);
		}
		else if (command == 'm') {
			MultiplyMatrix(matrix1, matrix2, result);
			PrintMatrix("Matrix1 * Matrix2", result);
		}
		else if (command == 'a') {
			AddMatrix(matrix1, matrix2, result);
			PrintMatrix("Matrix1 + Matrix2", result);
		}
		else if (command == 'd') {
			SubtractMatrix(matrix1, matrix2, result);
			PrintMatrix("Matrix1 - Matrix2", result);
		}
		else if (command == 'r') {
			std::cout << "det(Matrix1): " << Determinant(matrix1) << std::endl;
			std::cout << "det(Matrix2): " << Determinant(matrix2) << std::endl;
			std::cout << std::endl;
		}
		else if (command == 't') {
			TransposeMatrix(matrix1, result);
			PrintMatrix("Matrix1^t", result);
			std::cout << "det(Matrix1^t): " << Determinant(result) << std::endl;
			std::cout << std::endl;
			TransposeMatrix(matrix2, result);
			PrintMatrix("Matrix2^t", result);
			std::cout << "det(Matrix2^t): " << Determinant(result) << std::endl;
			std::cout << std::endl;
		}
		else if (command == 'e') {
			rowMinMode = !rowMinMode;
			UpdateCurrentMatrix(baseMatrix1, baseMatrix2, matrix1, matrix2, rowMinMode, colMaxMode);
			PrintMatrix("Matrix1", matrix1);
			PrintMatrix("Matrix2", matrix2);
		}
		else if (command == 'f') {
			colMaxMode = !colMaxMode;
			UpdateCurrentMatrix(baseMatrix1, baseMatrix2, matrix1, matrix2, rowMinMode, colMaxMode);
			PrintMatrix("Matrix1", matrix1);
			PrintMatrix("Matrix2", matrix2);
		}
		else if (command == '+') {
			AddOneModulo(baseMatrix1);
			AddOneModulo(baseMatrix2);
			UpdateCurrentMatrix(baseMatrix1, baseMatrix2, matrix1, matrix2, rowMinMode, colMaxMode);
			PrintMatrix("Matrix1", matrix1);
			PrintMatrix("Matrix2", matrix2);
		}
		else if (command == '-') {
			SubtractOneModulo(baseMatrix1);
			SubtractOneModulo(baseMatrix2);
			UpdateCurrentMatrix(baseMatrix1, baseMatrix2, matrix1, matrix2, rowMinMode, colMaxMode);
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
