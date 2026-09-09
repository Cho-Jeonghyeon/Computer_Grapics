#include <iostream>
#include <string>

using namespace std;

const int SIZE = 10;

struct Point {
    int x;
    int y;
    int z;
};

struct Deque {
    Point data[SIZE];
    bool used[SIZE];
    int front;
    int rear;
    int count;
};

void init_deque(Deque& q) {
    q.front = 0;
    q.rear = 0;
    q.count = 0;

    for (int i = 0; i < SIZE; i++) {
        q.used[i] = false;
    }
}

bool is_empty(const Deque& q) {
    return q.count == 0;
}

bool is_full(const Deque& q) {
    return q.count == SIZE;
}

void error(const string& message) {
    cout << message << endl;
}

void print_point(const Point& point) {
    cout << point.x << " " << point.y << " " << point.z;
}

void deque_print(const Deque& q) {
    for (int i = SIZE - 1; i >= 0; i--) {
        cout << "| " << i << " |";

        if (q.used[i]) {
            cout << " ";
            print_point(q.data[i]);
        }

        cout << endl;
    }
}

int main() {
    Deque list;
    init_deque(list);

    deque_print(list);

    return 0;
}
