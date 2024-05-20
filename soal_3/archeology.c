#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *relics_path = "/home/kali/arsipsisop/sisopmodul4/relics";
static const int chunk_size = 10240; // 10 KB

static int relics_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s%s.000", relics_path, path);

        res = stat(fullpath, stbuf);
        if (res == -1) return -errno;

        if (S_ISREG(stbuf->st_mode)) {
            stbuf->st_mode = S_IFREG | 0644;
            stbuf->st_size = 0;
            int part = 0;
            char partpath[1024];
            while (1) {
                snprintf(partpath, sizeof(partpath), "%s%s.%03d", relics_path, path, part);
                struct stat part_stat;
                if (stat(partpath, &part_stat) == -1) break;
                stbuf->st_size += part_stat.st_size;
                part++;
            }
        }
    }
    return res;
}

static int relics_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;

    if (strcmp(path, "/") != 0) return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    DIR *dp;
    struct dirent *de;

    dp = opendir(relics_path);
    if (dp == NULL) return -errno;

    while ((de = readdir(dp)) != NULL) {
        if (strstr(de->d_name, ".000") != NULL) {
            char name[256];
            strncpy(name, de->d_name, strlen(de->d_name) - 4);
            name[strlen(de->d_name) - 4] = '\0';
            filler(buf, name, NULL, 0);
        }
    }

    closedir(dp);
    return 0;
}

static int relics_open(const char *path, struct fuse_file_info *fi)
{
    return 0;
}

static int relics_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    (void) fi;
    char partpath[1024];
    size_t total_read = 0;
    int part = offset / chunk_size;
    size_t part_offset = offset % chunk_size;

    while (size > 0) {
        snprintf(partpath, sizeof(partpath), "%s%s.%03d", relics_path, path, part);
        FILE *file = fopen(partpath, "r");
        if (!file) break;

        fseek(file, part_offset, SEEK_SET);
        size_t read_size = fread(buf, 1, size, file);
        fclose(file);

        if (read_size <= 0) break;

        buf += read_size;
        size -= read_size;
        total_read += read_size;
        part++;
        part_offset = 0;
    }

    return total_read > 0 ? total_read : -errno;
}

static int relics_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    (void) fi;
    char partpath[1024];
    size_t total_written = 0;
    int part = offset / chunk_size;
    size_t part_offset = offset % chunk_size;

    while (size > 0) {
        snprintf(partpath, sizeof(partpath), "%s%s.%03d", relics_path, path, part);
        FILE *file = fopen(partpath, "r+");
        if (!file) {
            file = fopen(partpath, "w");
            if (!file) return -errno;
        }

        fseek(file, part_offset, SEEK_SET);
        size_t write_size = fwrite(buf, 1, size, file);
        fclose(file);

        if (write_size <= 0) break;

        buf += write_size;
        size -= write_size;
        total_written += write_size;
        part++;
        part_offset = 0;
    }

    return total_written > 0 ? total_written : -errno;
}

static int relics_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    (void) fi;
    char partpath[1024];
    snprintf(partpath, sizeof(partpath), "%s%s.000", relics_path, path);

    FILE *file = fopen(partpath, "w");
    if (!file) return -errno;
    fclose(file);

    return 0;
}

static int relics_unlink(const char *path)
{
    char partpath[1024];
    int part = 0;
    while (1) {
        snprintf(partpath, sizeof(partpath), "%s%s.%03d", relics_path, path, part);
        if (remove(partpath) != 0) break;
        part++;
    }

    return 0;
}

static struct fuse_operations relics_oper = {
    .getattr   = relics_getattr,
    .readdir   = relics_readdir,
    .open      = relics_open,
    .read      = relics_read,
    .write     = relics_write,
    .create    = relics_create,
    .unlink    = relics_unlink,
};

int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &relics_oper, NULL);
}

