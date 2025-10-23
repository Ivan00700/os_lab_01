// parent.c
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define BUF_SIZE 1024 // хренова туча байт чтобы читать когда будем вводить строки

int main() {
    char filename[256];

    const char *prompt = "Enter output filename:\n";

    // write() 1) дескриптор на запись 2) указатель на место в памяти откуда брать то что писать 3) количество байт для записи
    write(STDOUT_FILENO, prompt, strlen(prompt));

    // read() 1) дескриптор на чтение 2) указатель на место в памяти куда считать 3) максимальное количество байт
    ssize_t n = read(STDIN_FILENO, filename, sizeof(filename));

    if (n <= 0) {
        const char *err = "No filename provided\n";
        write(STDERR_FILENO, err, strlen(err));
        return 1;
    }
    if (filename[n-1] == '\n') filename[n-1] = '\0'; // убираем endl если пользователь не умеет читать

    int pipe_parent_to_child[2]; // канал через который родитель пишет а сын читает
    int pipe_child_to_parent[2]; // канал через который сын отправляет родакам о траблах

    // pipe() создает два файловых дескриптера: [0] - для чтения и [1] - для записи
    if (pipe(pipe_parent_to_child) == -1 || pipe(pipe_child_to_parent) == -1) {
        perror("pipe"); // если каналы упали => завершаем с ошибкой
        return 1;
    }

    // рождаем дочерний процесс
    pid_t pid = fork(); // PID = (Procces ID)
    if (pid == -1) {
        perror("fork failed");
        return 1;
    }
    // ㄟ(≧◇≦)ㄏ
    if (pid == 0) {
        // CHILD
        // закрываем концы
        close(pipe_parent_to_child[1]); // child reads from pipe_parent_to_child[0]
        close(pipe_child_to_parent[0]); // child writes to pipe_child_to_parent[1]

        // перенаправление стандартного ввода STDIN: ребёнок читает то что родитель записал в pipe_parent_to_child[1]
        if (dup2(pipe_parent_to_child[0], STDIN_FILENO) == -1) { perror("dup2 stdin"); _exit(1); }
        close(pipe_parent_to_child[0]);

        // специальный дескриптор для отправки ошибок от ребенка к родителю
        // 3 - потому что 0 1 2 заняты стандартными дескрипторами
        if (dup2(pipe_child_to_parent[1], 3) == -1) { perror("dup2 fd3"); _exit(1); }
        close(pipe_child_to_parent[1]);

        // тут идет перенаправление стандартного вывода в файл filename
        // open() открывается файл для записи (O_WRONLY), создается если не существует (O_CREAT), очищается при открытии (O_TRUNC)
        int out_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644); // 0644 = владелец: чтение+запись
        if (out_fd == -1) { perror("open output"); _exit(1); }
        if (dup2(out_fd, STDOUT_FILENO) == -1) { perror("dup2 stdout"); _exit(1); }
        close(out_fd);

        // заменяет образ текущего процесса (после fork) на исполняемый файл ./child.
        // важно: NULL служит ограничителем переменного списка аргументов
        execlp("./child", "./child", NULL);

        // если вернуло что-то, то ошибку
        perror("execlp");
        _exit(1);
    } else {
        // PARENT
        // закрываем концы которые не используются
        close(pipe_parent_to_child[0]); // родитель пишет в pipe_parent_to_child[1]
        close(pipe_child_to_parent[1]); // парэнт читает из pipe_child_to_parent[0]

        // читаем строки из STDIN и направляем их в ребенка через pipe_parent_to_child[1]
        char buf[BUF_SIZE]; // буфер для чтения
        ssize_t m;
        const char *input_prompt = "Enter a string (Ctrl+D to end):\n";

        while (1) {
            write(STDOUT_FILENO, input_prompt, strlen(input_prompt));
            m = read(STDIN_FILENO, buf, sizeof(buf));
            if (m < 0) {
                if (errno == EINTR) continue;
                perror("read stdin");
                break;
            }
            if (m == 0) break; // EOF (Ctrl+D)
            // write everything read to child
            ssize_t written = 0;
            while (written < m) {
                ssize_t w = write(pipe_parent_to_child[1], buf + written, m - written);
                if (w == -1) {
                    if (errno == EINTR) continue;
                    perror("write to pipe");
                    break;
                }
                written += w;
            }
        } // ಥ_ಥ (˘･_･˘)

        // close write end to signal EOF to child
        close(pipe_parent_to_child[1]);

        // читаем ошибки ребенка и выводим их в стандартный STDOUT
        while ((m = read(pipe_child_to_parent[0], buf, sizeof(buf))) > 0) {
            ssize_t written = 0;
            while (written < m) {
                ssize_t w = write(STDOUT_FILENO, buf + written, m - written);
                if (w == -1) {
                    if (errno == EINTR) continue;
                    perror("write stdout");
                    break;
                }
                written += w;
            }
        }
        if (m == -1) perror("read from child pipe");

        close(pipe_child_to_parent[0]);

        waitpid(pid, NULL, 0);
    }

    return 0;
}
