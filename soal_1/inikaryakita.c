#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>

static const char *dir_path = "/home/rickomianto/portofolio";
static const char *watermark_cmd = "convert '%s' -gravity South -pointsize 80 -fill white -annotate +0+100 'inikaryakita.id' '%s'";
static const char *reverse_prefix = "test_";

static void reverse_file(const char *source, const char *dest) {
    FILE *file, *temp;
    char ch;
    long size;

    file = fopen(source, "r");
    if (file == NULL) {
        perror("Error opening source file");
        return;
    }

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    rewind(file);

    temp = fopen(dest, "w");
    if (temp == NULL) {
        perror("Error opening destination file");
        fclose(file);
        return;
    }

    while (size > 0) {
        fseek(file, --size, SEEK_SET);
        ch = fgetc(file);
        fputc(ch, temp);
    }

    fclose(file);
    fclose(temp);
}

static void add_watermark(const char *source, const char *dest) {
    char cmd[1024];
    sprintf(cmd, watermark_cmd, source, dest);
    int result = system(cmd);
    if (result != 0) {
        fprintf(stderr, "Error executing watermark command: %s\n", cmd);
    }
}

static int xmp_getattr(const char *path, struct stat *stbuf) {
    int res;
    char full_path[1000];
    sprintf(full_path, "%s%s", dir_path, path);
    res = lstat(full_path, stbuf);
    if (res == -1)
        return -errno;
    return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
    int res;
    char full_path[1000];
    sprintf(full_path, "%s%s", dir_path, path);

    res = chmod(full_path, mode);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    DIR *dp;
    struct dirent *de;

    (void) offset;
    (void) fi;

    char full_path[1000];
    sprintf(full_path, "%s%s", dir_path, path);

    dp = opendir(full_path);
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0))
            break;
    }

    closedir(dp);
    return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi) {
    int res;
    char full_path[1000];
    sprintf(full_path, "%s%s", dir_path, path);

    res = open(full_path, fi->flags);
    if (res == -1)
        return -errno;

    close(res);
    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int fd;
    int res;
    char full_path[1000];
    sprintf(full_path, "%s%s", dir_path, path);

    (void) fi;
    fd = open(full_path, O_RDONLY);
    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    close(fd);
    return res;
}

static int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int fd;
    int res;
    char full_path[1000];
    sprintf(full_path, "%s%s", dir_path, path);

    (void) fi;
    fd = open(full_path, O_WRONLY);
    if (fd == -1)
        return -errno;

    res = pwrite(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    close(fd);

    if (strncmp(path, "/wm.", 4) == 0) {
        char wm_path[1000];
        sprintf(wm_path, "%s%s", dir_path, path);
        add_watermark(full_path, wm_path);
    }

    if (strncmp(path, reverse_prefix, strlen(reverse_prefix)) == 0) {
        char rev_path[1000];
        sprintf(rev_path, "%s%s", dir_path, path);
        reverse_file(full_path, rev_path);
    }

    return res;
}

static int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    int res;
    char full_path[1000];
    sprintf(full_path, "%s%s", dir_path, path);

    res = creat(full_path, mode);
    if (res == -1)
        return -errno;

    fi->fh = res;
    return 0;
}

static int xmp_unlink(const char *path) {
    int res;
    char full_path[1000];
    sprintf(full_path, "%s%s", dir_path, path);

    res = unlink(full_path);
    if (res == -1)
        return -errno;
    return 0;
}

static int xmp_mkdir(const char *path, mode_t mode) {
    int res;
    char full_path[1000];
    sprintf(full_path, "%s%s", dir_path, path);

    res = mkdir(full_path, mode);
    if (res == -1)
        return -errno;
    return 0;
}

static int xmp_rmdir(const char *path) {
    int res;
    char full_path[1000];
    sprintf(full_path, "%s%s", dir_path, path);

    res = rmdir(full_path);
    if (res == -1)
        return -errno;
    return 0;
}

static int xmp_rename(const char *from, const char *to) {
    int res;
    char full_path_from[1000];
    char full_path_to[1000];
    sprintf(full_path_from, "%s%s", dir_path, from);
    sprintf(full_path_to, "%s%s", dir_path, to);

    res = rename(full_path_from, full_path_to);
    if (res == -1)
        return -errno;
    return 0;
}

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .chmod = xmp_chmod,
    .readdir = xmp_readdir,
    .open = xmp_open,
    .read = xmp_read,
    .write = xmp_write,
    .create = xmp_create,
    .unlink = xmp_unlink,
    .mkdir = xmp_mkdir,
    .rmdir = xmp_rmdir,
    .rename = xmp_rename,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &xmp_oper, NULL);
}

