#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>

// 求最大公约数
static size_t gcd(size_t a, size_t b) {
    while (b) {
        size_t t = b;
        b = a % b;
        a = t;
    }
    return a;
}
// 求最小公倍数
static size_t lcm(size_t a, size_t b) {
    if (a == 0 || b == 0) return a + b;
    return a / gcd(a, b) * b;
}

// 实验确定的倍率
#define MULTIPLIER 16

// 获取页面大小与文件系统块大小的 LCM，并乘以 MULTIPLIER
static size_t io_blocksize_for_fd(int fd) {
    long page = sysconf(_SC_PAGESIZE);
    if (page < 0) {
        fprintf(stderr,
                "Warning: sysconf(_SC_PAGESIZE) failed (%s), defaulting to 4096\n",
                strerror(errno));
        page = 4096;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        return (size_t)page * MULTIPLIER;
    }

    size_t blksz = st.st_blksize;
    if (blksz == 0) {
        blksz = (size_t)page;
    }

    size_t base = lcm((size_t)page, blksz);
    return base * MULTIPLIER;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // 打开文件
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    // 计算缓冲区大小
    size_t bufsize = io_blocksize_for_fd(fd);

    // 分配对齐缓冲区，按系统页大小对齐
    void *buffer;
    long page = sysconf(_SC_PAGESIZE);
    if (page < 0) page = 4096;
    if (posix_memalign(&buffer, (size_t)page, bufsize) != 0) {
        perror("posix_memalign");
        close(fd);
        return EXIT_FAILURE;
    }

    // 循环读取并写入
    ssize_t nread;
    while ((nread = read(fd, buffer, bufsize)) > 0) {
        ssize_t written = 0;
        char *ptr = buffer;
        while (written < nread) {
            ssize_t w = write(STDOUT_FILENO, ptr + written, nread - written);
            if (w < 0) {
                perror("write");
                free(buffer);
                close(fd);
                return EXIT_FAILURE;
            }
            written += w;
        }
    }
    if (nread < 0) {
        perror("read");
        free(buffer);
        close(fd);
        return EXIT_FAILURE;
    }

    free(buffer);
    if (close(fd) < 0) {
        perror("close");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
