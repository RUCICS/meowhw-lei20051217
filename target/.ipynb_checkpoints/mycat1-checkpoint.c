// mycat1.c
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    // 检查参数数量
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 打开文件
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        exit(EXIT_FAILURE);
    }

    // 逐个字符读写
    char buffer[1];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, 1)) > 0) {
        if (write(STDOUT_FILENO, buffer, 1) == -1) {
            perror("write failed");
            close(fd);
            exit(EXIT_FAILURE);
        }
    }

    // 处理读取错误
    if (bytes_read == -1) {
        perror("read failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // 关闭文件
    if (close(fd) == -1) {
        perror("close failed");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}