#include "credentials.h"

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
    int len = snprintf(NULL, 0, "%s/credentials.txt", dir);
    char *filepath = malloc((size_t)len + 1);
    if (!filepath) {
        free(dir);
        return NULL;
    }
    snprintf(filepath, len + 1, "%s/credentials.txt", dir);
    free(dir);
    return filepath;
}

int save_credentials(const char *filepath, const char *credentials) {
    if (!filepath || !credentials) {
        return 1;
    }
    FILE *f = fopen(filepath, "w");
    if (!f) {
        return 1;
    }
    if (fputs(credentials, f) == EOF) {
        fclose(f);
        return 1;
    }
    fclose(f);
    return 0;
}

char *load_credentials(const char *filepath) {
    if (!filepath) {
        return NULL;
    }
    FILE *f = fopen(filepath, "r");
    if (!f) {
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return NULL;
    }
    rewind(f);
    char *credentials = malloc((size_t)size + 1);
    if (!credentials) {
        fclose(f);
        return NULL;
    }
    if (fread(credentials, 1, (size_t)size, f) != (size_t)size) {
        free(credentials);
        fclose(f);
        return NULL;
    }
    credentials[size] = '\0';
    fclose(f);
    return credentials;
}
