#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/statvfs.h>
#include <stdint.h>

// Compute gcd and lcm
static size_t gcd_size(size_t a, size_t b) {
    while (b) {
        size_t t = b;
        b = a % b;
        a = t;
    }
    return a;
}
static size_t lcm_size(size_t a, size_t b) {
    if (a == 0 || b == 0) return 0;
    return (a / gcd_size(a, b)) * b;
}

// Get combined buffer size aligned to page and fs block
size_t get_buffer_size(const char *path) {
    long page = sysconf(_SC_PAGESIZE);
    if (page < 1) page = 4096;

    struct statvfs sv;
    size_t fs_bsize;
    if (statvfs(path, &sv) == 0 && sv.f_bsize > 0) {
        fs_bsize = (size_t)sv.f_bsize;
    } else {
        fs_bsize = page;
    }
    // compute lcm so buffer is multiple of both
    size_t buf = lcm_size((size_t)page, fs_bsize);
    // guard: not zero
    if (buf == 0) buf = page;
    return buf;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *file = argv[1];

    // open file
    int fd = open(file, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    // determine buffer size
    size_t bufsize = get_buffer_size(file);
    // allocate page-aligned buffer
    void *buf;
    int res = posix_memalign(&buf, sysconf(_SC_PAGESIZE), bufsize);
    if (res != 0) {
        fprintf(stderr, "posix_memalign failed: %s\n", strerror(res));
        close(fd);
        return EXIT_FAILURE;
    }

    // read and write loop
    ssize_t nr;
    while ((nr = read(fd, buf, bufsize)) > 0) {
        ssize_t nw = 0;
        char *p = buf;
        while (nw < nr) {
            ssize_t w = write(STDOUT_FILENO, p + nw, nr - nw);
            if (w < 0) {
                perror("write");
                free(buf);
                close(fd);
                return EXIT_FAILURE;
            }
            nw += w;
        }
    }
    if (nr < 0) {
        perror("read");
    }

    free(buf);
    close(fd);
    return (nr < 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}
