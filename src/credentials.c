#include "credentials.h"
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SALT_SIZE 32
#define IV_SIZE 12
#define TAG_SIZE 16
#define KEY_SIZE 32
#define MAX_EMAIL_LEN 320

static int derive_key(const char *password, const unsigned char *salt, unsigned char *key) {
  EVP_KDF *kdf;
  EVP_KDF_CTX *kctx;
  OSSL_PARAM params[3];
  int ret = 0;
  kdf = EVP_KDF_fetch(NULL, "PBKDF2", NULL);
  if (!kdf)
    return 0;
  kctx = EVP_KDF_CTX_new(kdf);
  EVP_KDF_free(kdf);
  if (!kctx)
    return 0;
  params[0] = OSSL_PARAM_construct_octet_string("pass", (void *)password, strlen(password));
  params[1] = OSSL_PARAM_construct_octet_string("salt", (void *)salt, SALT_SIZE);
  params[2] = OSSL_PARAM_construct_end();
  if (EVP_KDF_derive(kctx, key, KEY_SIZE, params) > 0) {
    ret = 1;
  }
  EVP_KDF_CTX_free(kctx);
  return ret;
}

static unsigned char *encrypt_credentials(const char *plaintext, const char *password,
                                          size_t *out_len) {
  unsigned char salt[SALT_SIZE];
  unsigned char iv[IV_SIZE];
  unsigned char key[KEY_SIZE];
  unsigned char tag[TAG_SIZE];
  EVP_CIPHER_CTX *ctx;
  int len;
  size_t plaintext_len = strlen(plaintext);
  if (RAND_bytes(salt, SALT_SIZE) != 1 || RAND_bytes(iv, IV_SIZE) != 1) {
    return NULL;
  }
  if (!derive_key(password, salt, key)) {
    return NULL;
  }
  size_t buffer_size = SALT_SIZE + IV_SIZE + plaintext_len + TAG_SIZE;
  unsigned char *output = malloc(buffer_size);
  if (!output) {
    OPENSSL_cleanse(key, KEY_SIZE);
    return NULL;
  }
  memcpy(output, salt, SALT_SIZE);
  memcpy(output + SALT_SIZE, iv, IV_SIZE);
  ctx = EVP_CIPHER_CTX_new();
  if (!ctx) {
    free(output);
    OPENSSL_cleanse(key, KEY_SIZE);
    return NULL;
  }
  if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, iv) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    free(output);
    OPENSSL_cleanse(key, KEY_SIZE);
    return NULL;
  }
  unsigned char *ciphertext_pos = output + SALT_SIZE + IV_SIZE;
  if (plaintext_len > INT_MAX ||
      EVP_EncryptUpdate(ctx, ciphertext_pos, &len, (unsigned char *)plaintext,
                        (int)plaintext_len) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    free(output);
    OPENSSL_cleanse(key, KEY_SIZE);
    return NULL;
  }
  size_t ciphertext_len = (size_t)len;
  if (EVP_EncryptFinal_ex(ctx, ciphertext_pos + len, &len) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    free(output);
    OPENSSL_cleanse(key, KEY_SIZE);
    return NULL;
  }
  ciphertext_len += (size_t)len;
  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tag) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    free(output);
    OPENSSL_cleanse(key, KEY_SIZE);
    return NULL;
  }
  memcpy(output + SALT_SIZE + IV_SIZE + ciphertext_len, tag, TAG_SIZE);
  EVP_CIPHER_CTX_free(ctx);
  OPENSSL_cleanse(key, KEY_SIZE);
  *out_len = SALT_SIZE + IV_SIZE + ciphertext_len + TAG_SIZE;
  return output;
}

static char *decrypt_credentials(const unsigned char *encrypted, size_t encrypted_len,
                                 const char *password) {
  if (encrypted_len < SALT_SIZE + IV_SIZE + TAG_SIZE) {
    return NULL;
  }
  unsigned char key[KEY_SIZE];
  const unsigned char *salt = encrypted;
  const unsigned char *iv = encrypted + SALT_SIZE;
  const unsigned char *ciphertext = encrypted + SALT_SIZE + IV_SIZE;
  size_t ciphertext_len = encrypted_len - SALT_SIZE - IV_SIZE - TAG_SIZE;
  const unsigned char *tag = encrypted + encrypted_len - TAG_SIZE;
  if (!derive_key(password, salt, key)) {
    return NULL;
  }
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  if (!ctx) {
    OPENSSL_cleanse(key, KEY_SIZE);
    return NULL;
  }
  if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, iv) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    OPENSSL_cleanse(key, KEY_SIZE);
    return NULL;
  }
  char *plaintext = malloc(ciphertext_len + 1);
  if (!plaintext) {
    EVP_CIPHER_CTX_free(ctx);
    OPENSSL_cleanse(key, KEY_SIZE);
    return NULL;
  }
  int len;
  if (ciphertext_len > INT_MAX || EVP_DecryptUpdate(ctx, (unsigned char *)plaintext, &len,
                                                    ciphertext, (int)ciphertext_len) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    free(plaintext);
    OPENSSL_cleanse(key, KEY_SIZE);
    return NULL;
  }
  size_t plaintext_len = (size_t)len;
  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE, (void *)tag) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    free(plaintext);
    OPENSSL_cleanse(key, KEY_SIZE);
    return NULL;
  }
  if (EVP_DecryptFinal_ex(ctx, (unsigned char *)plaintext + len, &len) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    OPENSSL_cleanse(plaintext, plaintext_len);
    free(plaintext);
    OPENSSL_cleanse(key, KEY_SIZE);
    return NULL;
  }
  plaintext_len += (size_t)len;
  plaintext[plaintext_len] = '\0';
  EVP_CIPHER_CTX_free(ctx);
  OPENSSL_cleanse(key, KEY_SIZE);
  return plaintext;
}

static int mkdir_p(const char *path) {
  if (!path || strlen(path) == 0) {
    return 1;
  }
#if defined(_WIN32)
  if (_access(path, 0) == 0) {
    return 0;
  }
#else
  if (access(path, 0) == 0) {
    return 0;
  }
#endif
  char *p = malloc(strlen(path) + 1);
  if (!p) {
    return 1;
  }
  strcpy(p, path);
  char *separator = strrchr(p, '/');
  if (!separator) {
    separator = strrchr(p, '\\');
  }
  if (separator && separator != p) {
    *separator = '\0';
    if (mkdir_p(p)) {
      free(p);
      return 1;
    }
  }
  free(p);
#if defined(_WIN32)
  if (_mkdir(path) != 0) {
    return 1;
  }
#else
  if (mkdir(path, 0700) != 0) {
    return 1;
  }
#endif
  return 0;
}

static char *get_config_dir(const char *appname) {
  const char *home = getenv("HOME");
#if defined(_WIN32)
  if (!home) {
    home = getenv("USERPROFILE");
  }
#endif
  if (!home) {
    return NULL;
  }
  size_t len = strlen(home) + strlen("/.config/") + strlen(appname) + 2;
  char *path = malloc(len);
  if (!path) {
    return NULL;
  }
  int n = snprintf(path, len, "%s/.config/%s", home, appname);
  if (n < 0 || (size_t)n >= len) {
    free(path);
    return NULL;
  }
  if (mkdir_p(path) != 0) {
    free(path);
    return NULL;
  }
  return path;
}

char *credentials_filepath(const char *appname) {
  char *dir = get_config_dir(appname);
  if (!dir)
    return NULL;
  int len = snprintf(NULL, 0, "%s/credentials.enc", dir);
  char *filepath = malloc((size_t)len + 1);
  if (!filepath) {
    free(dir);
    return NULL;
  }
  snprintf(filepath, len + 1, "%s/credentials.enc", dir);
  free(dir);
  return filepath;
}

int save_credentials(const char *filepath, const char *credentials, const char *password) {
  if (!filepath || !credentials || !password) {
    return 1;
  }
  size_t encrypted_len;
  unsigned char *encrypted = encrypt_credentials(credentials, password, &encrypted_len);
  if (!encrypted) {
    return 1;
  }
  FILE *f = fopen(filepath, "wb");
  if (!f) {
    OPENSSL_cleanse(encrypted, encrypted_len);
    free(encrypted);
    return 1;
  }
#ifndef _WIN32
  chmod(filepath, 0600);
#endif
  if (fwrite(encrypted, 1, encrypted_len, f) != encrypted_len) {
    OPENSSL_cleanse(encrypted, encrypted_len);
    free(encrypted);
    fclose(f);
    return 1;
  }
  OPENSSL_cleanse(encrypted, encrypted_len);
  free(encrypted);
  fclose(f);
  return 0;
}

char *load_credentials(const char *filepath, const char *password) {
  if (!filepath || !password) {
    return NULL;
  }
  FILE *f = fopen(filepath, "rb");
  if (!f) {
    return NULL;
  }
  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return NULL;
  }
  long size = ftell(f);
  if (size < 0 || (size_t)size > SIZE_MAX) {
    fclose(f);
    return NULL;
  }
  rewind(f);
  unsigned char *encrypted = malloc((size_t)size);
  if (!encrypted) {
    fclose(f);
    return NULL;
  }
  if (fread(encrypted, 1, (size_t)size, f) != (size_t)size) {
    OPENSSL_cleanse(encrypted, (size_t)size);
    free(encrypted);
    fclose(f);
    return NULL;
  }
  fclose(f);
  char *credentials = decrypt_credentials(encrypted, (size_t)size, password);
  OPENSSL_cleanse(encrypted, (size_t)size);
  free(encrypted);
  return credentials;
}

char *get_email(void) {
  static char email[MAX_EMAIL_LEN + 1];
  if (scanf("%320s", email) != 1) {
    return NULL;
  }
  return email;
}

char *get_password(void) {
  size_t capacity = 64;
  size_t length = 0;
  char *password = malloc(capacity);
  int ch;
  if (password == NULL)
    return NULL;
#ifdef _WIN32
  while ((ch = _getch()) != '\r') {
    if (ch == 8 && length > 0) {
      printf("\b \b");
      length--;
    } else if (ch != 8 && ch >= 32) {
      if (length >= capacity - 1) {
        capacity *= 2;
        char *temp = realloc(password, capacity);
        if (temp == NULL) {
          free(password);
          return NULL;
        }
        password = temp;
      }
      password[length++] = ch;
      printf("*");
    }
  }
#else
  struct termios oldt, newt;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= (tcflag_t)~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  while ((ch = getchar()) != '\n' && ch != EOF) {
    if (length >= capacity - 1) {
      capacity *= 2;
      char *temp = realloc(password, capacity);
      if (temp == NULL) {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        free(password);
        return NULL;
      }
      password = temp;
    }
    password[length++] = (char)ch;
  }
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
  password[length] = '\0';
  printf("\n");
  return password;
}

void flush_stdin(void) {
  int c;
  while ((c = getchar()) != '\n' && c != EOF)
    ;
}
