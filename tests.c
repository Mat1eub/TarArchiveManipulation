#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lib_tar.h"

typedef struct {
    int code;
    const char *message;
} ErrorCode;

/**
 * You are free to use this file to write tests for your implementation
 */

void debug_dump(const uint8_t *bytes, size_t len) {
    for (int i = 0; i < len;) {
        printf("%04x:  ", (int) i);

        for (int j = 0; j < 16 && i + j < len; j++) {
            printf("%02x ", bytes[i + j]);
        }
        printf("\t");
        for (int j = 0; j < 16 && i < len; j++, i++) {
            printf("%c ", bytes[i]);
        }
        printf("\n");
    }
}

void check_archive_test(int fd)
{
    int ret = check_archive(fd);

    ErrorCode error_messages[] = {
    {0, "Valid archive with correct headers"},
    {-1, "Archive contains a header with an invalid magic value"},
    {-2, "Archive contains a header with an invalid version value"},
    {-3, "Archive contains a header with an invalid checksum value"}
    };

    for (size_t i = 0; i < sizeof(error_messages)/sizeof(error_messages[0]); i++)
    {
        if(error_messages[i].code == ret){
            printf("check_archive() returned: %d\nResult: %s\n", ret, error_messages[i].message);
            return;
        }
    }
    printf("check_archive() returned an unexpected value: %d\n", ret);
}


int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s tar_file\n", argv[0]);
        return -1;
    }

    int fd = open(argv[1] , O_RDONLY);
    if (fd == -1) {
        perror("open(tar_file)");
        return -1;
    }

    check_archive_test(fd);

    return 0;
}