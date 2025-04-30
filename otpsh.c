#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

#define CONFIG_PATH_MAX 512
#define SECRET_MAX_LEN 128

int base32_secret_decode(const char *base32, unsigned char *out, size_t out_size) {
    int len = strlen(base32);
    int out_index = 0;
    int buffer = 0;
    int buffer_length = 0;

    for (int i = 0; i < len; i++) {
        char c = toupper((unsigned char)base32[i]);
        if (c == '=') break;

        int value = -1;
        if (c >= 'A' && c <= 'Z') value = c - 'A';
        else if (c >= '2' && c <= '7') value = c - '2' + 26;
        else return -1;

        buffer = (buffer << 5) | value;
        buffer_length += 5;

        if (buffer_length >= 8) {
            if ((size_t)out_index >= out_size) return -1; // overflow protezione
            out[out_index++] = (buffer >> (buffer_length - 8)) & 0xFF;
            buffer_length -= 8;
        }
    }
    return out_index;
}

void generate_otp(const unsigned char *secret, size_t secret_len, uint32_t timestamp, uint32_t *otp) {
    uint32_t time_counter = timestamp / 30;
    unsigned char time_counter_bytes[8] = {0};

    for (int i = 7; i >= 0; i--) {
        time_counter_bytes[i] = (unsigned char)(time_counter & 0xFF);
        time_counter >>= 8;
    }

    unsigned char hmac_result[EVP_MAX_MD_SIZE];
    unsigned int hmac_len = 0;

    if (!HMAC(EVP_sha1(), secret, secret_len, time_counter_bytes, sizeof(time_counter_bytes), hmac_result, &hmac_len) || hmac_len < 20) {
        fprintf(stderr, "Errore HMAC\n");
        exit(EXIT_FAILURE);
    }

    int offset = hmac_result[19] & 0xF;
    uint32_t code = ((hmac_result[offset] & 0x7F) << 24) |
                    ((hmac_result[offset + 1] & 0xFF) << 16) |
                    ((hmac_result[offset + 2] & 0xFF) << 8) |
                    (hmac_result[offset + 3] & 0xFF);
    *otp = code % 1000000;
}

int read_config(const char *filename, char *secret, size_t secret_size, char *command, size_t command_size) {
    if (!filename || !secret || !command) return -1;

    const char *home_dir = getenv("HOME");
    if (!home_dir) {
        perror("HOME non disponibile");
        return -1;
    }
    if (strlen(home_dir) + strlen(filename) + 2 > CONFIG_PATH_MAX) {
        fprintf(stderr, "Path troppo lungo\n");
        return -1;
    }

    char config_path[CONFIG_PATH_MAX];
    snprintf(config_path, sizeof(config_path), "%s/%s", home_dir, filename);

    FILE *file = fopen(config_path, "r");
    if (!file) {
        perror("Errore apertura config");
        return -1;
    }

    if (!fgets(secret, secret_size, file) || !fgets(command, command_size, file)) {
        fclose(file);
        fprintf(stderr, "Errore lettura config\n");
        return -1;
    }

    secret[strcspn(secret, "\r\n")] = 0;
    command[strcspn(command, "\r\n")] = 0;

    fclose(file);
    return 0;
}
