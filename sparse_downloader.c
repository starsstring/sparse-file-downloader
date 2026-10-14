#define _POSIX_C_SOURCE 200809L
#define _FILE_OFFSET_BITS 64
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* 文件内容使用文件 IO；printf/snprintf 仅负责终端显示及文本格式化。 */
#define MIN_SIZE (128 * 1024)
#define MAX_CHUNK (1024 * 1024)
static volatile sig_atomic_t stopped;

struct progress {
    intmax_t total, next;
    uintmax_t src_dev, src_ino, dst_dev, dst_ino;
    intmax_t mtime, mtime_ns, ctime, ctime_ns;
};

static void fail(const char *what) { perror(what); exit(EXIT_FAILURE); }
static void reject(const char *why) {
    fprintf(stderr, "Error: %s\n", why); exit(EXIT_FAILURE);
}
static void interrupt_handler(int sig) { (void)sig; stopped = 1; }

static intmax_t number(const char *s, intmax_t low, intmax_t high) {
    char *end;
    intmax_t n;
    if (!s[0] || s[0] < '0' || s[0] > '9') reject("expected unsigned decimal bytes");
    errno = 0;
    n = strtoimax(s, &end, 10);
    if (errno || *end || n < low || n > high) reject("numeric argument out of range");
    return n;
}

static char *sidecar(const char *path) {
    size_t n = strlen(path);
    char *s = malloc(n + sizeof ".resume");
    if (!s) fail("malloc");
    memcpy(s, path, n);
    memcpy(s + n, ".resume", sizeof ".resume");
    return s;
}

static void seek_to(int fd, intmax_t offset) {
    if (lseek(fd, (off_t)offset, SEEK_SET) == (off_t)-1) fail("lseek");
}
static void write_all(int fd, const void *data, size_t len) {
    const unsigned char *p = data;
    while (len) {
        ssize_t n = write(fd, p, len);
        if (n < 0) { if (errno == EINTR) continue; fail("write"); }
        if (!n) reject("write returned zero");
        p += n; len -= (size_t)n;
    }
}
static void read_exact(int fd, void *data, size_t len) {
    unsigned char *p = data;
    while (len) {
        ssize_t n = read(fd, p, len);
        if (n < 0) { if (errno == EINTR) continue; fail("read"); }
        if (!n) reject("unexpected end of file");
        p += n; len -= (size_t)n;
    }
}
static void sync_fd(int fd) {
    while (fsync(fd) < 0) { if (errno != EINTR) fail("fsync"); }
}
static void close_fd(int fd) { if (close(fd) < 0) fail("close"); }
static struct stat info(int fd) {
    struct stat st;
    if (fstat(fd, &st) < 0) fail("fstat");
    if (!S_ISREG(st.st_mode)) reject("only regular files are supported");
    return st;
}
static void lock_file(int fd, short type) {
    struct flock lk;
    memset(&lk, 0, sizeof lk);
    lk.l_type = type; lk.l_whence = SEEK_SET;
    if (fcntl(fd, F_SETLK, &lk) < 0) fail("file busy / fcntl lock");
}

/* 返回 0 表示无记录，1 表示成功；破损记录必须拒绝，不能猜测进度。 */
static int load_progress(const char *path, struct progress *p) {
    char buf[513], *token, *saveptr = NULL, *end;
    uintmax_t values[6];
    intmax_t times[4];
    size_t used = 0;
    int fd = open(path, O_RDONLY | O_NOFOLLOW);
    if (fd < 0) { if (errno == ENOENT) return 0; fail("open resume"); }
    while (used < sizeof buf - 1) {
        ssize_t n = read(fd, buf + used, sizeof buf - 1 - used);
        if (n < 0) { if (errno == EINTR) continue; fail("read resume"); }
        if (!n) break;
        used += (size_t)n;
    }
    close_fd(fd);
    if (used == sizeof buf - 1 || memchr(buf, '\0', used)) reject("invalid resume record length");
    buf[used] = '\0';
    token = strtok_r(buf, " \t\r\n", &saveptr);
    if (!token || strcmp(token, "SPARSE_V1")) reject("invalid resume record version");
    /* 不用 scanf 直接解析整数：溢出必须通过 errno 明确拒绝。 */
    for (int i = 0; i < 10; ++i) {
        token = strtok_r(NULL, " \t\r\n", &saveptr);
        if (!token || !token[0]) reject("incomplete resume record");
        errno = 0;
        if (i < 6) {
            if (token[0] < '0' || token[0] > '9') reject("invalid unsigned resume field");
            values[i] = strtoumax(token, &end, 10);
        } else {
            if ((token[0] < '0' || token[0] > '9') && token[0] != '-')
                reject("invalid signed resume field");
            times[i - 6] = strtoimax(token, &end, 10);
        }
        if (errno || *end || end == token) reject("invalid or overflowing resume field");
    }
    if (strtok_r(NULL, " \t\r\n", &saveptr) || values[0] > INT64_MAX || values[1] > INT64_MAX)
        reject("invalid resume record fields");
    p->total = (intmax_t)values[0]; p->next = (intmax_t)values[1];
    p->src_dev = values[2]; p->src_ino = values[3];
    p->dst_dev = values[4]; p->dst_ino = values[5];
    p->mtime = times[0]; p->mtime_ns = times[1]; p->ctime = times[2]; p->ctime_ns = times[3];
    if (
        p->total < MIN_SIZE || p->next < 0 || p->next > p->total ||
        p->mtime_ns < 0 || p->mtime_ns >= 1000000000 ||
        p->ctime_ns < 0 || p->ctime_ns >= 1000000000)
        reject("invalid resume record; preserve files and investigate");
    return 1;
}

static void sync_parent(const char *path) {
    char *dir = strdup(path);
    char *slash;
    int fd;
    if (!dir) fail("strdup");
    slash = strrchr(dir, '/');
    if (!slash) strcpy(dir, ".");
    else if (slash == dir) slash[1] = '\0';
    else *slash = '\0';
    fd = open(dir, O_RDONLY | O_DIRECTORY);
    if (fd < 0) fail("open parent directory");
    sync_fd(fd); close_fd(fd); free(dir);
}

/* 临时文件 + rename 避免 Ctrl+C/进程退出留下半条记录。 */
static void save_progress(const char *path, const struct progress *p) {
    char buf[512];
    size_t n = strlen(path);
    char *tmp = malloc(n + sizeof ".tmp.XXXXXX");
    int fd, count;
    if (!tmp) fail("malloc");
    memcpy(tmp, path, n);
    memcpy(tmp + n, ".tmp.XXXXXX", sizeof ".tmp.XXXXXX");
    fd = mkstemp(tmp);
    if (fd < 0) fail("mkstemp resume");
    count = snprintf(buf, sizeof buf,
                     "SPARSE_V1 %jd %jd %ju %ju %ju %ju %jd %jd %jd %jd\n",
                     p->total, p->next, p->src_dev, p->src_ino,
                     p->dst_dev, p->dst_ino, p->mtime, p->mtime_ns,
                     p->ctime, p->ctime_ns);
    if (count < 0 || (size_t)count >= sizeof buf) reject("resume record too large");
    write_all(fd, buf, (size_t)count); sync_fd(fd); close_fd(fd);
    if (rename(tmp, path) < 0) fail("rename resume");
    sync_parent(path); free(tmp);
}

static void check_target(const struct progress *p, const struct stat *st) {
    if (p->total != (intmax_t)st->st_size ||
        p->dst_dev != (uintmax_t)st->st_dev || p->dst_ino != (uintmax_t)st->st_ino)
        reject("target replaced or resized; resume refused");
}
static void check_source(const struct progress *p, const struct stat *st) {
    if (p->total != (intmax_t)st->st_size ||
        p->src_dev != (uintmax_t)st->st_dev || p->src_ino != (uintmax_t)st->st_ino ||
        p->mtime != (intmax_t)st->st_mtim.tv_sec || p->mtime_ns != st->st_mtim.tv_nsec ||
        p->ctime != (intmax_t)st->st_ctim.tv_sec || p->ctime_ns != st->st_ctim.tv_nsec)
        reject("source changed; resume refused");
}

static void create_file(const char *path, intmax_t size) {
    char *meta = sidecar(path);
    struct stat st;
    int fd;
    if (lstat(meta, &st) == 0) reject("resume record already exists");
    if (errno != ENOENT) fail("lstat resume");
    fd = open(path, O_RDWR | O_CREAT | O_EXCL | O_NOFOLLOW, 0644);
    if (fd < 0) fail("create (existing file will not be overwritten)");
    /* lseek 自身不改变大小；末端写入一个字节才产生中间的空洞。 */
    seek_to(fd, size - 1); write_all(fd, "\0", 1);
    sync_fd(fd); close_fd(fd); sync_parent(path);
    printf("Created: %s, logical_bytes=%jd\n", path, size);
    free(meta);
}

static void modify_file(const char *path, intmax_t offset, const char *text) {
    char *meta = sidecar(path);
    struct stat st, ms;
    size_t len = strlen(text);
    int fd = open(path, O_RDWR | O_NOFOLLOW);
    if (fd < 0) fail("open write target");
    lock_file(fd, F_WRLCK); st = info(fd);
    if (lstat(meta, &ms) == 0) reject("download record exists; manual write would invalidate resume");
    if (errno != ENOENT) fail("lstat resume");
    if (offset > (intmax_t)st.st_size ||
        (uintmax_t)len > (uintmax_t)((intmax_t)st.st_size - offset))
        reject("write exceeds logical file size");
    seek_to(fd, offset); write_all(fd, text, len);
    sync_fd(fd); close_fd(fd); free(meta);
    printf("Written: offset=%jd bytes=%zu\n", offset, len);
}

static void status_file(const char *path) {
    char *meta = sidecar(path);
    struct progress p;
    struct stat st;
    int fd = open(path, O_RDONLY | O_NOFOLLOW);
    intmax_t allocated;
    double ratio;
    if (fd < 0) fail("open status target");
    lock_file(fd, F_RDLCK); st = info(fd);
    allocated = (intmax_t)st.st_blocks * 512;
    ratio = st.st_size ? 100.0 * (1.0 - (double)allocated / (double)st.st_size) : 0.0;
    if (ratio < 0.0) ratio = 0.0;
    printf("File: %s\nlogical_bytes=%jd\nallocated_512b_blocks=%jd\n"
           "allocated_bytes=%jd\nhole_ratio_estimate=%.2f%%\n",
           path, (intmax_t)st.st_size, (intmax_t)st.st_blocks, allocated, ratio);
    if (load_progress(meta, &p)) {
        check_target(&p, &st);
        printf("completed_offset=%jd\nprogress=%.2f%%\nstate=%s\n",
               p.next, 100.0 * (double)p.next / (double)p.total,
               p.next == p.total ? "complete" : "paused");
    } else puts("state=no_download_record");
    close_fd(fd); free(meta);
}

/* 每次续传校验已完成前缀，防止外部修改目标后错误地跳过数据。 */
static void check_prefix(int src, int dst, intmax_t length) {
    unsigned char a[8192], b[8192];
    seek_to(src, 0); seek_to(dst, 0);
    while (length && !stopped) {
        size_t n = length > (intmax_t)sizeof a ? sizeof a : (size_t)length;
        read_exact(src, a, n); read_exact(dst, b, n);
        if (memcmp(a, b, n)) reject("completed target data changed; resume refused");
        length -= (intmax_t)n;
    }
}

static void require_empty_data(int fd, intmax_t size) {
    unsigned char buf[8192];
    seek_to(fd, 0);
    while (size) {
        size_t n = size > (intmax_t)sizeof buf ? sizeof buf : (size_t)size;
        read_exact(fd, buf, n);
        for (size_t i = 0; i < n; ++i)
            if (buf[i]) reject("existing target contains data without a resume record; use a new target");
        size -= (intmax_t)n;
    }
}

static int download_file(const char *source, const char *target, size_t chunk, long delay_ms) {
    char *meta = sidecar(target);
    struct progress p;
    struct stat ss, ds;
    struct sigaction sa;
    unsigned char *buf;
    int saved, src, dst;
    memset(&sa, 0, sizeof sa); sa.sa_handler = interrupt_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) < 0 || sigaction(SIGTERM, &sa, NULL) < 0) fail("sigaction");
    src = open(source, O_RDONLY | O_NOFOLLOW);
    if (src < 0) fail("open source");
    ss = info(src);
    if (ss.st_size < MIN_SIZE) reject("source must be at least 131072 bytes");
    saved = load_progress(meta, &p);
    /* 有续传记录时绝不自动重建丢失的目标。没有记录则创建或使用同尺寸文件。 */
    dst = open(target, O_RDWR | O_NOFOLLOW | (saved ? 0 : O_CREAT), 0644);
    if (dst < 0) fail("open download target");
    lock_file(dst, F_WRLCK); ds = info(dst);
    if (ss.st_dev == ds.st_dev && ss.st_ino == ds.st_ino)
        reject("source and target refer to the same file");
    /* 必须持锁后重读，防止锁前读到另一进程尚未更新的旧进度。 */
    saved = load_progress(meta, &p);
    if (saved) {
        check_source(&p, &ss); check_target(&p, &ds);
        check_prefix(src, dst, p.next);
    } else {
        if (ds.st_size && ds.st_size != ss.st_size) reject("existing target has a different size");
        require_empty_data(dst, (intmax_t)ds.st_size);
        if (ftruncate(dst, ss.st_size) < 0) fail("ftruncate target");
        sync_fd(dst); sync_parent(target);
        memset(&p, 0, sizeof p);
        p.total = (intmax_t)ss.st_size;
        p.src_dev = (uintmax_t)ss.st_dev; p.src_ino = (uintmax_t)ss.st_ino;
        p.dst_dev = (uintmax_t)ds.st_dev; p.dst_ino = (uintmax_t)ds.st_ino;
        p.mtime = (intmax_t)ss.st_mtim.tv_sec; p.mtime_ns = ss.st_mtim.tv_nsec;
        p.ctime = (intmax_t)ss.st_ctim.tv_sec; p.ctime_ns = ss.st_ctim.tv_nsec;
        save_progress(meta, &p);
    }
    printf("%s at offset=%jd / %jd\n", saved ? "Resume" : "Start", p.next, p.total);
    fflush(stdout);
    buf = malloc(chunk);
    if (!buf) fail("malloc chunk");
    seek_to(src, p.next); seek_to(dst, p.next);
    while (p.next < p.total && !stopped) {
        size_t n = (uintmax_t)(p.total - p.next) < chunk ? (size_t)(p.total - p.next) : chunk;
        struct stat now = info(src);
        check_source(&p, &now);
        read_exact(src, buf, n); write_all(dst, buf, n);
        /* 数据持久化在前，记录完成偏移在后；重复一块比跳过一块安全。 */
        sync_fd(dst);
        now = info(src); check_source(&p, &now);
        p.next += (intmax_t)n; save_progress(meta, &p);
        printf("completed_offset=%jd progress=%.2f%%\n", p.next,
               100.0 * (double)p.next / (double)p.total); fflush(stdout);
        if (delay_ms && p.next < p.total && !stopped) {
            struct timespec t = {delay_ms / 1000, (delay_ms % 1000) * 1000000L};
            while (nanosleep(&t, &t) < 0) { if (errno != EINTR) fail("nanosleep"); if (stopped) break; }
        }
    }
    free(buf); close_fd(dst); close_fd(src); free(meta);
    if (p.next == p.total) { puts("Download complete."); return 0; }
    printf("Paused safely at offset=%jd; run the same command to resume.\n", p.next);
    return 130;
}

static void usage(const char *name) {
    fprintf(stderr,
            "Usage (sizes and offsets are decimal bytes):\n"
            "  %s create FILE SIZE          (SIZE >= 131072)\n"
            "  %s write FILE OFFSET TEXT\n"
            "  %s status FILE\n"
            "  %s download SOURCE TARGET [CHUNK_BYTES] [DELAY_MS]\n"
            "  default chunk=4096, delay=0; download simulates transfer from a local source.\n",
            name, name, name, name);
}

int main(int argc, char **argv) {
    if (argc == 4 && !strcmp(argv[1], "create"))
        create_file(argv[2], number(argv[3], MIN_SIZE, INT64_MAX));
    else if (argc == 5 && !strcmp(argv[1], "write"))
        modify_file(argv[2], number(argv[3], 0, INT64_MAX), argv[4]);
    else if (argc == 3 && !strcmp(argv[1], "status")) status_file(argv[2]);
    else if (argc >= 4 && argc <= 6 && !strcmp(argv[1], "download"))
        return download_file(argv[2], argv[3],
                             argc >= 5 ? (size_t)number(argv[4], 1, MAX_CHUNK) : 4096,
                             argc >= 6 ? (long)number(argv[5], 0, 60000) : 0);
    else { usage(argv[0]); return EXIT_FAILURE; }
    return EXIT_SUCCESS;
}
