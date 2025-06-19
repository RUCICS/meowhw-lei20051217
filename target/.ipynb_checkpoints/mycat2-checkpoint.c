#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

// 获取系统内存页大小
size_t io_blocksize(void) {
    long page_size = sysconf(_SC_PAGESIZE);
    
    if (page_size == -1) {
        // 如果获取失败，使用默认值4096并打印警告
        fprintf(stderr, "Warning: Failed to get system page size (%s). Using default 4096.\n", strerror(errno));
        return 4096;
    }
    
    return (size_t)page_size;
}

int main(int argc, char *argv[]) {
    // 检查参数数量
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 获取系统内存页大小
    size_t buffer_size = io_blocksize();
    
    // 动态分配缓冲区
    char *buffer = malloc(buffer_size);
    if (buffer == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    // 打开文件
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        free(buffer);
        exit(EXIT_FAILURE);
    }

    // 读取文件并写入标准输出
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, buffer_size)) > 0) {
        ssize_t bytes_written = 0;
        
        // 确保所有数据都被写入（处理部分写入情况）
        while (bytes_written < bytes_read) {
            ssize_t result = write(STDOUT_FILENO, buffer + bytes_written, bytes_read - bytes_written);
            
            if (result == -1) {
                perror("write failed");
                close(fd);
                free(buffer);
                exit(EXIT_FAILURE);
            }
            
            bytes_written += result;
        }
    }

    // 处理读取错误
    if (bytes_read == -1) {
        perror("read failed");
        close(fd);
        free(buffer);
        exit(EXIT_FAILURE);
    }

    // 关闭文件和释放资源
    if (close(fd) == -1) {
        perror("close failed");
        free(buffer);
        exit(EXIT_FAILURE);
    }
    
    free(buffer);

    return EXIT_SUCCESS;
}