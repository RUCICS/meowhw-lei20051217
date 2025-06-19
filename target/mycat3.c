#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <stdint.h>  // for uintptr_t

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

// 分配页面对齐的内存
char* align_alloc(size_t size) {
    // 获取系统页大小
    size_t page_size = io_blocksize();
    
    // 计算需要分配的总大小（大小 + 对齐空间）
    size_t total_size = size + page_size - 1;
    
    // 分配原始内存
    void* raw_ptr = malloc(total_size);
    if (raw_ptr == NULL) {
        return NULL;
    }
    
    // 计算对齐后的地址
    uintptr_t raw_addr = (uintptr_t)raw_ptr;
    uintptr_t aligned_addr = (raw_addr + page_size - 1) & ~(page_size - 1);
    
    return (char*)aligned_addr;
}

// 释放页面对齐的内存
void align_free(void* ptr) {
    if (ptr == NULL) return;
    
    // 获取原始分配地址（存储在指针前面的元数据中）
    void** meta_ptr = (void**)((uintptr_t)ptr - sizeof(void*));
    void* raw_ptr = *meta_ptr;
    
    free(raw_ptr);
}

int main(int argc, char *argv[]) {
    // 检查参数数量
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 获取系统内存页大小
    size_t buffer_size = io_blocksize();
    
    // 分配页面对齐的缓冲区
    char *buffer = align_alloc(buffer_size);
    if (buffer == NULL) {
        perror("align_alloc failed");
        exit(EXIT_FAILURE);
    }

    // 打开文件
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        align_free(buffer);
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
                align_free(buffer);
                exit(EXIT_FAILURE);
            }
            
            bytes_written += result;
        }
    }

    // 处理读取错误
    if (bytes_read == -1) {
        perror("read failed");
        close(fd);
        align_free(buffer);
        exit(EXIT_FAILURE);
    }

    // 关闭文件和释放资源
    if (close(fd) == -1) {
        perror("close failed");
        align_free(buffer);
        exit(EXIT_FAILURE);
    }
    
    align_free(buffer);

    return EXIT_SUCCESS;
}