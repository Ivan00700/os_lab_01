// child.c
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUF_SIZE 1024

int main() {
    // буфер для чтения
    char buf[BUF_SIZE];
    ssize_t n;
    // собираем одну логическую строку посимвольно
    char line[BUF_SIZE];
    int idx = 0;

    // читаем из STDIN который нам прислал родитель
    while ((n = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        // обрабатываем символы поотдельности
        for (ssize_t i = 0; i < n; i++) {
            char c = buf[i];
            // идем посимвольно и проверяем конец ли строки или нет
            if (c == '\n' || c == '\r') { // конец строки
                if (idx > 0) {
                    line[idx] = '\0';
                    // проверяем на задание
                    char last = line[idx - 1];
                    if (last == '.' || last == ';') {
                        // если верно -> записываем в STDOUT
                        write(STDOUT_FILENO, line, idx);
                        write(STDOUT_FILENO, "\n", 1);
                    } else {
                        // если неверно, то отправляем в канал ошибки который мы создали в родителе
                        const char *errpref = "Invalid string (must end with '.' or ';'): ";
                        write(3, errpref, strlen(errpref));
                        write(3, line, idx);
                        write(3, "\n", 1);
                    }
                }
                idx = 0;
            } else {
                if (idx < BUF_SIZE - 1) {
                    line[idx++] = c;
                }
            }
        }
    }

    // если есть строка без \n обработаем ее так же как выше
    if (idx > 0) {
        line[idx] = '\0';
        char last = line[idx - 1];
        if (last == '.' || last == ';') {
            write(STDOUT_FILENO, line, idx);
            write(STDOUT_FILENO, "\n", 1);
        } else {
            const char *errpref = "Invalid string (must end with '.' or ';'): ";
            write(3, errpref, strlen(errpref));
            write(3, line, idx);
            write(3, "\n", 1);
        }
    }

    return 0;
}
