#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

int base32_secret_decode(const char *base32, unsigned char *out) {
    //const char base32_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    int len = strlen(base32);
    int out_index = 0;
    int buffer = 0;
    int buffer_length = 0;

    for (int i = 0; i < len; i++) {
        char c = base32[i];
        if (c == '=') break;

        int value = -1;
        if (c >= 'A' && c <= 'Z') {
            value = c - 'A';
        } else if (c >= '2' && c <= '7') {
            value = c - '2' + 26;
        }

        if (value != -1) {
            buffer = (buffer << 5) | value;
            buffer_length += 5;

            if (buffer_length >= 8) {
                out[out_index++] = (buffer >> (buffer_length - 8)) & 0xFF;
                buffer_length -= 8;
            }
        }
    }
    return out_index;
}

void generate_otp(const unsigned char *secret, size_t secret_len, uint32_t timestamp, uint32_t *otp) {
    uint32_t time_counter = timestamp / 30;
    unsigned char time_counter_bytes[8];
    for (int i = 7; i >= 0; i--) {
        time_counter_bytes[i] = (unsigned char)(time_counter & 0xFF);
        time_counter >>= 8;
    }

    unsigned char hmac_result[EVP_MAX_MD_SIZE];
    unsigned int hmac_len;

    HMAC(EVP_sha1(), secret, secret_len, time_counter_bytes, 8, hmac_result, &hmac_len);

    int offset = hmac_result[19] & 0xF;
    uint32_t code = (hmac_result[offset] & 0x7F) << 24 |
                    (hmac_result[offset + 1] & 0xFF) << 16 |
                    (hmac_result[offset + 2] & 0xFF) << 8 |
                    (hmac_result[offset + 3] & 0xFF);
    *otp = code % 1000000;
}

int read_config(const char *filename, char *secret, char *command) {
    const char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        perror("Impossibile ottenere la variabile d'ambiente HOME");
        return -1;
    }
    char config_path[512];
    snprintf(config_path, sizeof(config_path), "%s/%s", home_dir, filename);
    
    FILE *file = fopen(config_path, "r");
    if (!file) {
        perror("Error reading config");
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "secret=", 7) == 0) {
            strcpy(secret, line + 7);
            secret[strcspn(secret, "\n")] = '\0';
        }
        if (strncmp(line, "command=", 8) == 0) {
            strcpy(command, line + 8);
            command[strcspn(command, "\n")] = '\0';
        }
    }

    fclose(file);
    return 0;
}

int main() {
    char secret[64] = {0};
    char command[256] = {0};
    if (read_config(".otpsh", secret, command) != 0) {
        return -1;
    }

    unsigned char decoded_secret[64];
    int secret_len = base32_secret_decode(secret, decoded_secret);
    uint32_t timestamp = (uint32_t)time(NULL);
    //printf("Unixtime: %u\n", timestamp);

    uint32_t otp;
    generate_otp(decoded_secret, secret_len, timestamp, &otp);

    uint32_t user_otp;
    printf("Input OTP: ");
    scanf("%u", &user_otp);

    if (user_otp == otp) {
        //printf("Valid OTP!\n");

        int status = execlp(command, command, (char *)NULL);
        if (status == -1) {
            perror("Command not executed.");
        }
    } else {
        printf("Invalid OTP\n");
    }
    return 0;
}
