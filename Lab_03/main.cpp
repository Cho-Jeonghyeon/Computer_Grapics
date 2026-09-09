#include <iostream>
#include <cmath>
#include <iomanip>
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

double distance_origin(const Point& point) {
    return sqrt(point.x * point.x + point.y * point.y + point.z * point.z);
}

double distance_points(const Point& p1, const Point& p2) {
    int x = p1.x - p2.x;
    int y = p1.y - p2.y;
    int z = p1.z - p2.z;

    return sqrt(x * x + y * y + z * z);
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

void print_distance_list(const Deque& q) {
    Point sortData[SIZE];
    double sortDistance[SIZE];
    int sortCount = 0;

    for (int i = 0; i < SIZE; i++) {
        if (q.used[i]) {
            sortData[sortCount] = q.data[i];
            sortDistance[sortCount] = distance_origin(q.data[i]);
            sortCount++;
        }
    }

    for (int i = 0; i < sortCount - 1; i++) {
        for (int j = i + 1; j < sortCount; j++) {
            if (sortDistance[i] > sortDistance[j]) {
                Point tempPoint = sortData[i];
                sortData[i] = sortData[j];
                sortData[j] = tempPoint;

                double tempDistance = sortDistance[i];
                sortDistance[i] = sortDistance[j];
                sortDistance[j] = tempDistance;
            }
        }
    }

    for (int i = SIZE - 1; i >= 0; i--) {
        cout << "| " << i << " |";

        if (i < sortCount) {
            cout << " ";
            print_point(sortData[i]);
            cout << " distance = " << fixed << setprecision(2) << sortDistance[i];
        }

        cout << endl;
    }
}

void print_near_far(const Deque& q) {
    if (q.count < 2) {
        cout << "need more points" << endl;
        return;
    }

    Point nearPoint1;
    Point nearPoint2;
    Point farPoint1;
    Point farPoint2;
    double nearDistance = 0;
    double farDistance = 0;
    bool firstCheck = true;

    for (int i = 0; i < SIZE; i++) {
        if (!q.used[i]) {
            continue;
        }

        for (int j = i + 1; j < SIZE; j++) {
            if (!q.used[j]) {
                continue;
            }

            double distance = distance_points(q.data[i], q.data[j]);

            if (firstCheck) {
                nearPoint1 = q.data[i];
                nearPoint2 = q.data[j];
                farPoint1 = q.data[i];
                farPoint2 = q.data[j];
                nearDistance = distance;
                farDistance = distance;
                firstCheck = false;
            }
            else if (distance < nearDistance) {
                nearPoint1 = q.data[i];
                nearPoint2 = q.data[j];
                nearDistance = distance;
            }
            else if (distance > farDistance) {
                farPoint1 = q.data[i];
                farPoint2 = q.data[j];
                farDistance = distance;
            }
        }
    }

    cout << "near: ";
    print_point(nearPoint1);
    cout << " / ";
    print_point(nearPoint2);
    cout << " distance = " << fixed << setprecision(2) << nearDistance << endl;

    cout << "far: ";
    print_point(farPoint1);
    cout << " / ";
    print_point(farPoint2);
    cout << " distance = " << fixed << setprecision(2) << farDistance << endl;
}

bool add_rear(Deque& q, Point item) {
    if (is_full(q)) {
        error("list is full");
        return false;
    }

    while (q.used[q.rear]) {
        q.rear = (q.rear + 1) % SIZE;
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

void move_down(Deque& q) {
    if (::is_empty(q)) {
        return;
    }

    Point tempData[SIZE];
    bool tempUsed[SIZE];

    for (int i = 0; i < SIZE; i++) {
        tempUsed[i] = false;
    }

    for (int i = 0; i < SIZE; i++) {
        if (q.used[i]) {
            int moveIndex = (i - 1 + SIZE) % SIZE;
            tempData[moveIndex] = q.data[i];
            tempUsed[moveIndex] = true;
        }
    }

    for (int i = 0; i < SIZE; i++) {
        q.data[i] = tempData[i];
        q.used[i] = tempUsed[i];
    }

    q.front = (q.front - 1 + SIZE) % SIZE;
    q.rear = (q.rear - 1 + SIZE) % SIZE;
}

int main() {
    Deque list;
    init_deque(list);

    char command;
    bool fMode = false;

    deque_print(list);

    while (true) {
        cin >> command;

        if (command == '+') {
            Point point;
            cin >> point.x >> point.y >> point.z;
            add_rear(list, point);
            fMode = false;
            deque_print(list);
        }
        else if (command == '-') {
            Point removed;
            delete_rear(list, removed);
            fMode = false;
            deque_print(list);
        }
        else if (command == 'e') {
            Point point;
            cin >> point.x >> point.y >> point.z;
            add_front(list, point);
            fMode = false;
            deque_print(list);
        }
        else if (command == 'd') {
            Point removed;
            delete_front(list, removed);
            fMode = false;
            deque_print(list);
        }
        else if (command == 'a') {
            cout << "count = " << list.count << endl;
        }
        else if (command == 'b') {
            move_down(list);
            fMode = false;
            deque_print(list);
        }
        else if (command == 'c') {
            init_deque(list);
            fMode = false;
            deque_print(list);
        }
        else if (command == 'f') {
            fMode = !fMode;

            if (fMode) {
                print_distance_list(list);
            }
            else {
                deque_print(list);
            }
        }
        else if (command == 'g') {
            print_near_far(list);
        }
        else if (command == 'q') {
            break;
        }
    }

    return 0;
}
