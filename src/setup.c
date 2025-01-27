#include "setup.h"

#define CREDENTIALS_FILEPATH "~/.cache/discrub/credentials"

static int mkdir_p(char* path) {
  path = strdup(path);
  char* p = path;
  while ((p = strchr(p + 1, '/'))) {
    *p = '\0';
    if (mkdir(path, 0755) == -1 && errno != EEXIST) {
      perror("mkdir");
      return 1;
    }
    *p = '/';
  }
  free(path);
  return 0;
}

static char* read_file(const char* filepath) {
  FILE* file = fopen(filepath, "rb");
  if (file == NULL) {
    return NULL;
  }
  fseek(file, 0, SEEK_END);
  long filesize = ftell(file);
  rewind(file);
  char* buffer = malloc(filesize + 1);
  if (buffer == NULL) {
    fprintf(stderr, "read_file malloc failed on buffer\n");
    fclose(file);
    return NULL;
  }
  if (fread(buffer, filesize, 1, file) != 1) {
    fprintf(stderr, "read_file fread failed\n");
    fclose(file);
    return NULL;
  }
  fclose(file);
  return buffer;
}

static char* get_password() {
  size_t i = 0;
  char* password = malloc(1);
  int ch;
  if (password == NULL) {
    return NULL;
  }
#ifdef _WIN32
  while ((ch = _getch()) != '\r') {
    if (ch == 8 && index > 0) {
      printf("\b \b");
      i--;
    } else if (ch != 8) {
      password[i++] = ch;
      printf("*");
      password = realloc(password, i + 1);
      if (password == NULL)
        return NULL;
    }
  }
#else
  struct termios oldt, newt;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  while ((ch = getchar()) != '\n') {
    password[i++] = ch;
    password = realloc(password, i + 1);
    if (password == NULL) {
      return NULL;
    }
  }
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
  password[i] = '\0';
  printf("\n");
  return password;
}

int load_cache(char* filepath, char** uid, char** token) {
  if (filepath[0] == '~') {
    char* home = getenv("HOME");
    if (home == NULL) {
      home = getenv("USERPROFILE");
    }
    if (home == NULL) {
      return 1;
    }
    size_t home_len = strlen(home);
    size_t filepath_suffix_len = strlen(filepath + 1);
    size_t expanded_path_len = home_len + filepath_suffix_len + 1;
    char* expanded_path = malloc(expanded_path_len);
    if (expanded_path == NULL) {
      return 1;
    }
    strcpy(expanded_path, home);
    strcat(expanded_path, filepath + 1);
    filepath = expanded_path;
  }
  char* credentials = read_file(filepath);
  if (filepath[0] == '~') {
    free(filepath);
  }
  if (credentials == NULL) {
    return 1;
  }
  *token = strtok(credentials, " ");
  *uid = strtok(NULL, " ");
  if (*token == NULL || *uid == NULL) {
    free(credentials);
    return 1;
  }
  return 0;
}

int save_cache(char* filepath, const char* uid, const char* token) {
  if (filepath[0] == '~') {
    char* home = getenv("HOME");
    if (home == NULL) {
      home = getenv("USERPROFILE");
    }
    if (home == NULL) {
      return 1;
    }
    size_t home_len = strlen(home);
    size_t filepath_suffix_len = strlen(filepath + 1);
    size_t expanded_path_len = home_len + filepath_suffix_len + 1;
    char* expanded_path = malloc(expanded_path_len);
    if (expanded_path == NULL) {
      return 1;
    }
    strcpy(expanded_path, home);
    strcat(expanded_path, filepath + 1);
    filepath = expanded_path;
  }
  if (mkdir_p(filepath)) {
    return 1;
  }
  FILE* file = fopen(filepath, "w");
  if (file == NULL) {
    return 1;
  }
  fprintf(file, "%s %s", token, uid);
  fclose(file);
  if (filepath[0] == '~') {
    free(filepath);
  }
  return 0;
}

int prompt_credentials(char** email, char** password) {
  printf("Discord Authentication Required\n");
  printf("Email:");
  static char email_buffer[321];
  if (scanf("%320s", email_buffer) != 1) {
    fprintf(stderr, "Failed to get email from stdin\n");
    return 1;
  }
  char c;
  while ((c = getchar()) != '\n' && c != EOF)
    ;
  printf("Password:");
  *password = get_password();
  if (*password == NULL) {
    fprintf(stderr, "Failed to get password from stdin\n");
    return 1;
  }
  *email = email_buffer;
  return 0;
}
