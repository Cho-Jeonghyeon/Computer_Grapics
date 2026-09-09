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

bool add_rear(Deque& q, Point item) {
    if (is_full(q)) {
        error("list is full");
        return false;
    }

    q.data[q.rear] = item;
    q.used[q.rear] = true;
    q.rear = (q.rear + 1) % SIZE;
    q.count++;
    return true;
}

bool delete_rear(Deque& q, Point& item) {
    if (::is_empty(q)) {
        error("list is empty");
        return false;
    }

    q.rear = (q.rear - 1 + SIZE) % SIZE;
    item = q.data[q.rear];
    q.used[q.rear] = false;
    q.count--;

    return true;
}

bool add_front(Deque& q, Point item) {
    if (is_full(q)) {
        error("list is full");
        return false;
    }

    for (int i = SIZE - 1; i > 0; i--) {
        q.data[i] = q.data[i - 1];
        q.used[i] = q.used[i - 1];
    }

    q.data[0] = item;
    q.used[0] = true;
    q.count++;
    q.front = 0;
    q.rear = q.count % SIZE;
    return true;
}

bool delete_front(Deque& q, Point& item) {
    if (::is_empty(q)) {
        error("list is empty");
        return false;
    }

    item = q.data[q.front];
    q.used[q.front] = false;
    q.front = (q.front + 1) % SIZE;
    q.count--;

    return true;
}

int main() {
    Deque list;
    init_deque(list);

    char command;

    deque_print(list);

    while (true) {
        cin >> command;

        if (command == '+') {
            Point point;
            cin >> point.x >> point.y >> point.z;
            add_rear(list, point);
            deque_print(list);
        }
        else if (command == '-') {
            Point removed;
            delete_rear(list, removed);
            deque_print(list);
        }
        else if (command == 'e') {
            Point point;
            cin >> point.x >> point.y >> point.z;
            add_front(list, point);
            deque_print(list);
        }
        else if (command == 'd') {
            Point removed;
            delete_front(list, removed);
            deque_print(list);
        }
        else if (command == 'q') {
            break;
        }
    }

    return 0;
}
