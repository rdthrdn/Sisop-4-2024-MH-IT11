
#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <syslog.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

char dirpath[2048];

void downloadUnZIP();
void logFunc(const char *status, const char *type, const char *information);
void decodeBase64(const char *input, char *output), decodeROT13(const char *input, char *output), 
decodeHex(const char *input, char *output), decodeReverse(const char *input, char *output);

static int sysop_getattr(const char *path, struct stat *stbuf)
{
  char logEvent[2048];
  char fpath[2048];
  sprintf(fpath, "%s%s", dirpath, path);
  int res;

  res = lstat(fpath, stbuf);
  if (res == -1) 
  {
    strcpy(logEvent, "Failed to get information of file ");
    strcat(logEvent, fpath);
    logFunc("FAILED", "getattr", logEvent);

    return -errno;
  }

  strcpy(logEvent, "Successfully got information of file ");
  strcat(logEvent, fpath);
  logFunc("SUCCESS", "getattr", logEvent);

  return 0;
}

static int sysop_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
  char logEvent[2048];
  char fpath[2048];
  sprintf(fpath, "%s%s", dirpath, path);

  DIR *dp;
  struct dirent *de;

  dp = opendir(fpath);
  if (dp == NULL) 
  {
    strcpy(logEvent, "Failed to open directory ");
    strcat(logEvent, fpath);
    logFunc("FAILED", "readdir", logEvent);
    return -errno;
  }

  while ((de = readdir(dp)) != NULL) {
    struct stat st;
    memset(&st, 0, sizeof(st));
    st.st_ino = de->d_ino;
    st.st_mode = de->d_type << 12;
    if (filler(buf, de->d_name, &st, 0)) break;
  }

  strcpy(logEvent, "Successfully opened directory ");
  strcat(logEvent, fpath);
  logFunc("SUCCESS", "readdir", logEvent);

  closedir(dp);
  return 0;
}

static int sysop_open(const char *path, struct fuse_file_info *fi) 
{
  char logEvent[2048];
  char fpath[2048];
  snprintf(fpath, sizeof(fpath), "%s%s", dirpath, path);

  if (strstr(path, "rahasia-berkas") != NULL) 
  {
    char *stored_password = "sisopmudah";

    char user_password[100];
    char passPath[2048];
    strcpy(passPath, dirpath);
    strcat(passPath, "/../password.txt");
    
    FILE *user_password_file = fopen(passPath, "r");
    if (user_password_file == NULL) 
    {
      strcpy(logEvent, "Failed to open user password file for file ");
      strcat(logEvent, fpath);
      logFunc("FAILED", "open", logEvent);
      return -EACCES;
    }
    fscanf(user_password_file, "%99s", user_password);
    fclose(user_password_file);

    if (strcmp(user_password, stored_password) != 0) 
    {
      strcpy(logEvent, "Access denied to file ");
      strcat(logEvent, fpath);
      logFunc("FAILED", "accessDenied", logEvent);
      return -EACCES;
    } 
    else 
    {
      strcpy(logEvent, "Access granted to file ");
      strcat(logEvent, fpath);
      logFunc("SUCCESS", "accessGranted", logEvent);
    }
  }

  int res = open(fpath, fi->flags);
  if (res == -1) 
  {
    strcpy(logEvent, "Failed to open file ");
    strcat(logEvent, fpath);
    logFunc("FAILED", "open", logEvent);
    return -errno;
  }

  close(res);
  return 0;
}

static int sysop_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) 
{
  char fpath[2048];
  sprintf(fpath, "%s%s", dirpath, path);

  FILE *file = fopen(fpath, "rb");
  if (!file) return -errno;

  fseek(file, 0, SEEK_END);
  size_t fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *content = (char *)malloc(fileSize + 1);
  fread(content, 1, fileSize, file);
  content[fileSize] = '\0';
  fclose(file);

  char decodedContent[fileSize + 1];
  if (strstr(path, "base64") != NULL) decodeBase64(content, decodedContent);
  else if (strstr(path, "rot13") != NULL) decodeROT13(content, decodedContent);
  else if (strstr(path, "hex") != NULL) decodeHex(content, decodedContent);
  else if (strstr(path, "rev") != NULL) decodeReverse(content, decodedContent);
  else strcpy(decodedContent, content);

  free(content);
  size_t len = strlen(decodedContent);

  if (offset < len) 
  {
    if (offset + size > len) size = len - offset;
    memcpy(buf, decodedContent + offset, size);
  }
  else size = 0;

  return size;
}

static struct fuse_operations sysop_oper = {
  .getattr    = sysop_getattr,
  .readdir    = sysop_readdir,
  .open       = sysop_open,
  .read       = sysop_read,
};

int main(int argc, char *argv[])
{
  getcwd(dirpath, sizeof(dirpath));
  strcat(dirpath, "/sensitif");
  downloadUnZIP();
  umask(0);
  return fuse_main(argc, argv, &sysop_oper, NULL);
}

void downloadUnZIP()
{
  char zip[2048];
  strcpy(zip, dirpath);
  strcat(zip, ".zip");
  char cmd[2048];
  strcpy(cmd, "rm ");
  strcat(cmd, zip);
  strcat(cmd, " && rm -rf ");
  strcat(cmd, dirpath);
  char cmd1[2048];
  strcpy(cmd1, "wget --content-disposition --no-check-certificate \"https://drive.google.com/uc?export=download&id=1t1CXcJgesUYj2i7KrHKdwXd79neKWF1u\" -P ");
  strcat(cmd1, dirpath);
  strcat(cmd1, "/../");
  char cmd2[2048];
  strcpy(cmd2, "unzip ");
  strcat(cmd2, zip);
  system(cmd);
  system(cmd1);
  system(cmd2);

  logFunc("SUCCESS", "downloadUnzip", "Download and unzip file successfully");
}

void decodeBase64(const char *input, char *output)
{
  BIO *bio, *b64;
  int decodeLen = strlen(input);
  char *buffer = (char *)malloc(decodeLen + 1);

  b64 = BIO_new(BIO_f_base64());
  bio = BIO_new_mem_buf(input, -1);
  bio = BIO_push(b64, bio);

  BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
  int len = BIO_read(bio, buffer, decodeLen);
  buffer[len] = '\0';

  BIO_free_all(bio);

  strcpy(output, buffer);
  free(buffer);
}

void decodeROT13(const char *input, char *output)
{
  for (int i = 0; i < strlen(input); i++) 
  {
    if (isalpha(input[i])) 
    {
      if ((input[i] >= 'a' && input[i] <= 'm') || (input[i] >= 'A' && input[i] <= 'M')) output[i] = input[i] + 13;
      else output[i] = input[i] - 13;
    } 
    
    else output[i] = input[i];
  }
  output[strlen(input)] = '\0';
}

void decodeHex(const char *input, char *output) 
{
  int len = strlen(input);
  for (int i = 0; i < len; i += 2) sscanf(input + i, "%2hhx", &output[i / 2]);
  output[len / 2] = '\0';
}

void decodeReverse(const char *input, char *output)
{
  int len = strlen(input);
  for (int i = 0; i < len; i++) output[i] = input[len - i - 1];
  output[len] = '\0';
}

void logFunc(const char *status, const char *type, const char *information)
{
  char log[2048];
  strcpy(log, dirpath);
  strcat(log, "/../logs-fuse.log");

  time_t runtime;
  struct tm *digitime;
  runtime = time(NULL);
  digitime = localtime(&runtime);

  FILE *fp = fopen(log, "a");
  if (fp == NULL) return;
  fprintf(fp, "[%s]::%02d/%02d/%04d-%02d:%02d:%02d::[%s]::[%s]\n", status, digitime->tm_year + 1900, digitime->tm_mon + 1, digitime->tm_mday, digitime->tm_hour, digitime->tm_min, digitime->tm_sec, type, information);
  fclose(fp);
}
