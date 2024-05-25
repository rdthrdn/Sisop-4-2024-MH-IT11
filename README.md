# Sisop-4-2024-MH-IT11
# Langkah-langkah penyelesaian setiap soal
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
