# Sisop-4-2024-MH-IT11
# Langkah-langkah penyelesaian setiap soal
# Soal 1
Dikerjakan oleh Ricko Mianto Jaya Saputra (5027231031)

Kode yang disajikan adalah implementasi dari layanan pemrograman Filesystem in Userspace (FUSE) yang menggunakan bahasa c. Fungsi utama dari kode ini adalah untuk membalik isi dari suatu berkas atau file dan menambahkan watermark pada sebuah foto.

# Deskripsi Kode

Kode ini berfungsi untuk membalik isi dari suatu berkas yang memiliki prefix `test_` dan menambahkan watermark pada foto yang di pindahkan ke dalam folder yang memiliki awalan `/wm.`

```C
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
```

# Hasil Run 



# Kendala yang Dialami

Pada saat di run kode saya tidak menunjukkan tanda tanda adanya error namun foto yang dipindahkan ke dalam folder dengan prefix `wm.` masih tidak terwatermark dan isi dari berkas dengan prefix `test_` juga masih belum terbalik

# SOAL 2
Dikerjakan oleh Rafi' Afnaan Fathurrahman (5027231040)
Pengerjaan soal ini menggunakan 2 file, pastibisa.c dan password.txt, serta 1 folder mount
## pastibisa.c
Masih dengan Ini Karya Kita, sang CEO ingin melakukan tes keamanan pada folder sensitif Ini Karya Kita. Karena Teknologi Informasi merupakan departemen dengan salah satu fokus di Cyber Security, maka dia kembali meminta bantuan mahasiswa Teknologi Informasi angkatan 2023 untuk menguji dan mengatur keamanan pada folder sensitif tersebut. Untuk mendapatkan folder sensitif itu, mahasiswa IT 23 harus kembali mengunjungi website Ini Karya Kita pada www.inikaryakita.id/schedule . Silahkan isi semua formnya, tapi pada form subject isi dengan nama kelompok_SISOP24 , ex: IT01_SISOP24 . Lalu untuk form Masukkan Pesanmu, ketik “Mau Foldernya” . Tunggu hingga 1x24 jam, maka folder sensitif tersebut akan dikirimkan melalui email kalian. Apabila folder tidak dikirimkan ke email kalian, maka hubungi sang CEO untuk meminta bantuan. 
![image](https://github.com/rdthrdn/Sisop-4-2024-MH-IT11/assets/143690594/e9d4bdb4-228e-4ce8-938f-f2c1c960fb40)

## subsoal 1
Pada folder "pesan" Adfi ingin meningkatkan kemampuan sistemnya dalam mengelola berkas-berkas teks dengan menggunakan fuse.
```c
// ...
static int sysop_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) 
{
  // ...
  if (strstr(path, "base64") != NULL) decodeBase64(content, decodedContent);
  else if (strstr(path, "rot13") != NULL) decodeROT13(content, decodedContent);
  else if (strstr(path, "hex") != NULL) decodeHex(content, decodedContent);
  else if (strstr(path, "rev") != NULL) decodeReverse(content, decodedContent);
  else strcpy(decodedContent, content);

  // ...
}

static struct fuse_operations sysop_oper = {
  .getattr    = sysop_getattr,
  .readdir    = sysop_readdir,
  .open       = sysop_open,
  .read       = sysop_read,
};

int main(int argc, char *argv[])
{
  // ...

  umask(0);
  return fuse_main(argc, argv, &sysop_oper, NULL);
}

// ...
```
![image](https://github.com/rdthrdn/Sisop-4-2024-MH-IT11/assets/143690594/dcb0d54c-5f4d-4b7c-ae14-f6e433df6153)

### subsoal 1.1
Jika sebuah file memiliki prefix "base64," maka sistem akan langsung mendekode isi file tersebut dengan algoritma Base64.
```c
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
```
![image](https://github.com/rdthrdn/Sisop-4-2024-MH-IT11/assets/143690594/66a4321b-8d60-4a2b-8e7a-24dba519faf7)

### subsoal 1.2
Jika sebuah file memiliki prefix "rot13," maka isi file tersebut akan langsung di-decode dengan algoritma ROT13.
```c
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
```
![image](https://github.com/rdthrdn/Sisop-4-2024-MH-IT11/assets/143690594/8ccc07a7-a591-4add-aa09-646f3a1bc676)

### subsoal 1.3
Jika sebuah file memiliki prefix "hex," maka isi file tersebut akan langsung di-decode dari representasi heksadesimalnya.
```c
void decodeHex(const char *input, char *output) 
{
  int len = strlen(input);
  for (int i = 0; i < len; i += 2) sscanf(input + i, "%2hhx", &output[i / 2]);
  output[len / 2] = '\0';
}
```
![image](https://github.com/rdthrdn/Sisop-4-2024-MH-IT11/assets/143690594/d18548bb-c14b-4c40-89ec-3b9f543bb053)

### subsoal 1.4
Jika sebuah file memiliki prefix "rev," maka isi file tersebut akan langsung di-decode dengan cara membalikkan teksnya.
```c
void decodeReverse(const char *input, char *output)
{
  int len = strlen(input);
  for (int i = 0; i < len; i++) output[i] = input[len - i - 1];
  output[len] = '\0';
}
```
![image](https://github.com/rdthrdn/Sisop-4-2024-MH-IT11/assets/143690594/e2483966-a484-496b-888b-c15d4320d61d)

## subsoal 2
Pada folder “rahasia-berkas”, Adfi dan timnya memutuskan untuk menerapkan kebijakan khusus. Mereka ingin memastikan bahwa folder dengan prefix "rahasia" tidak dapat diakses tanpa izin khusus. 
- Jika seseorang ingin mengakses folder dan file pada “rahasia”, mereka harus memasukkan sebuah password terlebih dahulu (password bebas). 
```c
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
```
![image](https://github.com/rdthrdn/Sisop-4-2024-MH-IT11/assets/143690594/c976043a-92aa-4c97-bada-c090074ccbcc)

## subsoal 3
Setiap proses yang dilakukan akan tercatat pada logs-fuse.log dengan format :
[SUCCESS/FAILED]::dd/mm/yyyy-hh:mm:ss::[tag]::[information]
Ex:
[SUCCESS]::01/11/2023-10:43:43::[moveFile]::[File moved successfully]
```c
// ...
type funcName(type denominator, ...)
{
  //...

  // status = SUCCESS/FAILED
  // type = open, readdir, etc
  // information = successfully/failed to do something...
  logFunc(status, type, information);

  // ...
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
```
![image](https://github.com/rdthrdn/Sisop-4-2024-MH-IT11/assets/143690594/310d4e98-272b-44d8-95c2-a2ebd8fb431c)
# Soal 3
Dikerjakan oleh Raditya Hardian Santoso (5027231033)

Kode archeology.c adalah implementasi dari filesystem yang menggunakan FUSE (Filesystem in Userspace). Kode ini memungkinkan pengguna untuk melihat file yang terpecah menjadi beberapa bagian sebagai satu file utuh di direktori mount point. Kode ini menggabungkan pecahan-pecahan file dari direktori relics dan menyajikannya sebagai file utuh di direktori mount point yang dapat diakses dan dimanipulasi seperti filesystem biasa.

# Deskripsi Kode

### Header dan Deklarasi
```C
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
```
Bagian ini mendefinisikan versi FUSE yang digunakan dan menyertakan header yang diperlukan. Variabel relics_path menyimpan jalur ke direktori yang berisi pecahan file, dan chunk_size mendefinisikan ukuran pecahan file (10 KB).

### Fungsi relics_getattr
```C
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
```
Fungsi ini mendapatkan atribut file atau direktori. Jika path adalah root (/), maka itu adalah direktori. Jika bukan, ia mencari file pecahan pertama (.000) dan menghitung ukuran total file dengan menjumlahkan ukuran semua pecahan.

### Fungsi relics_readdir
```C
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
```
Fungsi ini membaca isi direktori. Jika path adalah root, ia mengisi buffer dengan daftar file yang ditemukan di direktori relics yang diakhiri dengan .000 (file pecahan pertama).

### Fungsi relics_open
```C
static int relics_open(const char *path, struct fuse_file_info *fi)
{
 return 0;
}

Fungsi ini hanya membuka file. Tidak ada operasi khusus yang dilakukan di sini.
Fungsi relics_read
c
Copy code
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
```
Fungsi ini membaca data dari file pecahan. Ia menghitung bagian file yang perlu dibaca berdasarkan offset dan ukuran chunk, lalu membaca data dari pecahan yang sesuai.

### Fungsi relics_write
```C
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
```
Fungsi ini menulis data ke file pecahan. Ia menghitung bagian file yang perlu ditulis berdasarkan offset dan ukuran chunk, lalu menulis data ke pecahan yang sesuai.

### Fungsi relics_create
```C
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
```
Fungsi ini membuat file baru dengan nama pecahan pertama (.000).

### Fungsi relics_unlink
```C
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
```
Fungsi ini menghapus file dengan menghapus semua pecahan file yang ada.

### Struktur fuse_operations
```C
static struct fuse_operations relics_oper = {
 .getattr = relics_getattr,
 .readdir = relics_readdir,
 .open = relics_open,
 .read = relics_read,
 .write = relics_write,
 .create = relics_create,
 .unlink = relics_unlink,
};
```
Struktur ini menghubungkan fungsi-fungsi yang telah didefinisikan dengan operasi FUSE.

### Fungsi main
```C
int main(int argc, char *argv[])
{
 return fuse_main(argc, argv, &relics_oper, NULL);
}
```
Fungsi ini memulai filesystem FUSE dengan operasi yang telah ditentukan.
# Langkah Pengerjaan
## 1. Instalasi FUSE:
Pastikan FUSE terinstal di sistem
```C
sudo apt install fuse
```
## 2. Kompilasi Kode
Kompilasi kode menggunakan GCC dan flag yang diperlukan untuk FUSE
```C
gcc archeology.c -o relics_fs `pkg-config fuse --cflags --libs`
```
## 3. Buat Direktori untuk Mount Point:

Buat direktori mount point untuk filesystem FUSE
```C
mkdir /home/kali/arsipsisop/sisopmodul4/testing
```
## 4. Jalankan Filesystem FUSE
```C
./relics_fs /home/kali/arsipsisop/sisopmodul4/testing
```
## 5. Verifikasi

Verifikasi bahwa file-file di direktori testing terlihat sebagai file utuh.
## 6.Salin File ke Direktori report:

Salin file dari testing ke report
```C
cp -r /home/kali/arsipsisop/sisopmodul4/testing/* /home/kali/arsipsisop/sisopmodul4/report/
```
## 7. Konfigurasi Samba:

Tambahkan konfigurasi Samba untuk direktori report di file /etc/samba/smb.conf:
```C
[report]
   path = /home/kali/arsipsisop/sisopmodul4/report
   available = yes
   valid users = [your_username]
   read only = no
   browsable = yes
   public = yes
   writable = yes
```
## 8. Restart layanan Samba:
```C
sudo systemctl restart smbd
sudo systemctl restart nmbd
```
## 9. Akses dari Windows:

Akses folder report dari Windows melalui Samba dengan alamat IP mesin Linux
```C
\\192.168.168.130\report
```


# Kendala yang Dialami
Tidak ada.
