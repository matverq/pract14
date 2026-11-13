#include <iostream>
#include <Windows.h>

using namespace std;

#define MAX_LOGS 1000
#define MAX_TEXT 128

struct LogRecord {
    DWORD threadId;
    int priority;
    DWORD tick;
    char message[MAX_TEXT];
};

struct LogBuffer {
    LogRecord records[MAX_LOGS];
    LONG index;
};

LogBuffer logBuffer;

struct ThreadParams {
    int priorityLevel;
    const char* threadName;
};

DWORD WINAPI LoggerThread(LPVOID lpParam) {
    ThreadParams* p = (ThreadParams*)lpParam;
    SetThreadPriority(GetCurrentThread(), p->priorityLevel);

    DWORD start = GetTickCount();

    while (GetTickCount() - start < 3000) {
        LONG idx = logBuffer.index;

        if (idx < MAX_LOGS) {
            LogRecord& r = logBuffer.records[idx];
            r.threadId = GetCurrentThreadId();
            r.priority = p->priorityLevel;
            r.tick = GetTickCount();
            sprintf_s(r.message, "Запись от %s", p->threadName);

            Sleep(1);

            logBuffer.index++;
        }
        Sleep(10 + rand() % 91);
    }
    return 0;
}

DWORD WINAPI ObserverThread(LPVOID lpParam) {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
    DWORD start = GetTickCount();

    while (GetTickCount() - start < 3500) {
        LONG ind = logBuffer.index;

        if (ind > 0) {
            int s = (ind >= 5) ? (ind - 5) : 0;
            cout << "\nЧитаем " << (ind - s) << " записей\n";
            for (int i = s; i < ind; ++i) {
                LogRecord& r = logBuffer.records[i];
                cout << "Время: " << r.tick << " | ID: " << r.threadId
                    << " | Приоритет: " << r.priority << " | " << r.message << "\n";
            }
        }

        Sleep(100);
    }
    return 0;
}

int main() {
    setlocale(LC_ALL, "Russian");
    srand((unsigned int)time(NULL));

    if (!SetProcessAffinityMask(GetCurrentProcess(), 1)) {
        cout << "Ошибка привязки к ядру: " << GetLastError() << "\n";
        return 1;
    }

    ZeroMemory(&logBuffer, sizeof(LogBuffer));

    cout << "Старт без критических секций\n";

    ThreadParams p1 = { THREAD_PRIORITY_HIGHEST, "T1" };
    ThreadParams p2 = { THREAD_PRIORITY_NORMAL, "T2" };
    ThreadParams p3 = { THREAD_PRIORITY_BELOW_NORMAL, "T3" };

    HANDLE h[4] = { NULL };
    h[0] = CreateThread(NULL, 0, LoggerThread, &p1, 0, NULL);
    h[1] = CreateThread(NULL, 0, LoggerThread, &p2, 0, NULL);
    h[2] = CreateThread(NULL, 0, LoggerThread, &p3, 0, NULL);
    h[3] = CreateThread(NULL, 0, ObserverThread, NULL, 0, NULL);

    bool potok = true;
    for (int i = 0; i < 4; ++i) {
        if (h[i] == NULL) {
            cout << "Ошибка создания потока " << i << "\n";
            potok = false;
        }
    }

    if (potok) {
        WaitForMultipleObjects(4, h, TRUE, INFINITE);
        cout << "\nВсего записей: " << logBuffer.index << "\n";
    }

    for (int i = 0; i < 4; ++i) {
        if (h[i] != NULL) CloseHandle(h[i]);
    }
}