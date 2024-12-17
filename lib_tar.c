#include "lib_tar.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

/**
 * Checks whether the archive is valid.
 *
 * Each non-null header of a valid archive has:
 *  - a magic value of "ustar" and a null,
 *  - a version value of "00" and no null,
 *  - a correct checksum
 *
 * @param tar_fd A file descriptor pointing to the start of a file supposed to contain a tar archive.
 *
 * @return a zero or positive value if the archive is valid, representing the number of non-null headers in the archive,
 *         -1 if the archive contains a header with an invalid magic value,
 *         -2 if the archive contains a header with an invalid version value,
 *         -3 if the archive contains a header with an invalid checksum value
 */
int check_archive(int tar_fd) {
    tar_header_t buffer;
    unsigned long int header_chksum, calculated_chksum;
    ssize_t bytes_read;

    // Initialize the position to the top of the file
    if (lseek(tar_fd, 0, SEEK_SET) == -1) {
        perror("An error occurred with lseek");
        return -4;
    }

    while ((bytes_read = read(tar_fd, &buffer, TAR_HEADER_SIZE)) > 0 && buffer.name[0] != '\0') {

        // Invalid magic value
        if (strncmp(buffer.magic, TMAGIC, TMAGLEN) != 0){
            return -1;
        }
        // Invalid version value
        if (strncmp(buffer.version, TVERSION, TVERSLEN) != 0){
            return -2;
        }
        // - Checksum section - 
        header_chksum = TAR_INT(buffer.chksum);
        memset(buffer.chksum, ' ', 8);              // Neutralises the chksum field as required by TAR standards
        calculated_chksum = 0;

        uint8_t *current_byte = (uint8_t *)&buffer;
        for (size_t i = 0; i < TAR_HEADER_SIZE; i++) {
            calculated_chksum += current_byte[i];
        }

        if (header_chksum != calculated_chksum){
            return -3;
        }
    }

    if (bytes_read < 0) {
        perror("Error reading from file descriptor");
        return -4;
    }
    return 0;
}


/**
 * Checks whether an entry exists in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive,
 *         any other value otherwise.
 */
int exists(int tar_fd, char *path) {
    struct posix_header header; 
    ssize_t read_bytes;

    while ((read_bytes = read(tar_fd, &header, sizeof(struct posix_header))) == sizeof(struct posix_header)) {
        // fin d'archive?
        if (header.name[0] == '\0') {
            break;
        }

        if (strcmp(header.name, path) == 0) {
            return 1; // L'entrée existe.
        }

        // Calcule la taille réelle des données du fichier (en octets).
        unsigned long file_size = strtoul(header.size, NULL, 8);
        
        // Avance dans l'archive en sautant les blocs de données correspondants.
        off_t offset = ((file_size + 511) / 512) * 512; // Taille alignée sur 512 octets.
        if (lseek(tar_fd, offset, SEEK_CUR) == (off_t)-1) {
            return 0; // Erreur si impossible de se déplacer dans le fichier.
        }
    }

    // Retourne 0 si le chemin n'a pas été trouvé.
    return 0;
}

/**
 * Checks whether an entry exists in the archive and is a directory.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a directory,
 *         any other value otherwise.
 */
int is_dir(int tar_fd, char *path) {
    struct posix_header header; 
    ssize_t read_bytes;

    
    while ((read_bytes = read(tar_fd, &header, sizeof(struct posix_header))) == sizeof(struct posix_header)) {
        if (header.name[0] == '\0') {
            break;
        }

        if (strcmp(header.name, path) == 0) {
            // Vérifie si l'entrée est un répertoire.
            if (header.typeflag == '5') { // '5' indique un répertoire dans les en-têtes POSIX.
                return 1; 
            } else {
                return 0; 
            }
        }

        unsigned long file_size = strtoul(header.size, NULL, 8);

        off_t offset = ((file_size + 511) / 512) * 512; 
        if (lseek(tar_fd, offset, SEEK_CUR) == (off_t)-1) {
            return 0; 
        }
    }

    return 0;
}

/**
 * Checks whether an entry exists in the archive and is a file.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a file,
 *         any other value otherwise.
 */

int is_file(int tar_fd, char *path) {
    struct posix_header header; 
    ssize_t read_bytes;

    // Positionne le pointeur au début
    if (lseek(tar_fd, 0, SEEK_SET) == (off_t)-1) {
        return 0; 
    }

    // va jusqu'à la fin de l'archive
    while ((read_bytes = read(tar_fd, &header, sizeof(struct posix_header))) == sizeof(struct posix_header)) {
        // fin de l'archive? => sortie de la boucle
        if (header.name[0] == '\0') {
            break;
        }

        
        if (strcmp(header.name, path) == 0) {
            if (header.typeflag == '0' || header.typeflag == '\0') { 
                // '0' ou '\0' pour un fichier régulier dans les en-têtes POSIX.
                return 1; // L'entrée est donc un fichier régulier.
            } else {
                return 0; 
            }
        }

        unsigned long file_size = strtoul(header.size, NULL, 8);

        off_t offset = ((file_size + 511) / 512) * 512; 
        if (lseek(tar_fd, offset, SEEK_CUR) == (off_t)-1) {
            return 0; 
        }
    }
    return 0;
}

/**
 * Checks whether an entry exists in the archive and is a symlink.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 * @return zero if no entry at the given path exists in the archive or the entry is not symlink,
 *         any other value otherwise.
 */

int is_symlink(int tar_fd, char *path) {
    struct posix_header header; 
    ssize_t read_bytes;

    if (lseek(tar_fd, 0, SEEK_SET) == (off_t)-1) {
        return 0; 
    }

    while ((read_bytes = read(tar_fd, &header, sizeof(struct posix_header))) == sizeof(struct posix_header)) {
        if (header.name[0] == '\0') {
            break;
        }
        if (strcmp(header.name, path) == 0) {
            // lien symbolique ? (champs typeflag des en-têtes POSIX)
            if (header.typeflag == '2') { 
                // '2' indique un lien symbolique selon la spécification POSIX.
                return 1; 
            } else {
                return 0; 
            }
        }

        unsigned long file_size = strtoul(header.size, NULL, 8);

        off_t offset = ((file_size + 511) / 512) * 512; 
        if (lseek(tar_fd, offset, SEEK_CUR) == (off_t)-1) {
            return 0; 
        }
    }
    return 0;
}



/**
 * Lists the entries at a given path in the archive.
 * list() does not recurse into the directories listed at the given path.
 *
 * Example:
 *  dir/          list(..., "dir/", ...) lists "dir/a", "dir/b", "dir/c/" and "dir/e/"
 *   ├── a
 *   ├── b
 *   ├── c/
 *   │   └── d
 *   └── e/
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive. If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param entries An array of char arrays, each one is long enough to contain a tar entry path.
 * @param no_entries An in-out argument.
 *                   The caller set it to the number of entries in `entries`.
 *                   The callee set it to the number of entries listed.
 *
 * @return zero if no directory at the given path exists in the archive,
 *         any other value otherwise.
 */
int list(int tar_fd, char *path, char **entries, size_t *no_entries) {
    return 0;
}

/**
 * Reads a file at a given path in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive to read from.  If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param offset An offset in the file from which to start reading from, zero indicates the start of the file.
 * @param dest A destination buffer to read the given file into.
 * @param len An in-out argument.
 *            The caller set it to the size of dest.
 *            The callee set it to the number of bytes written to dest.
 *
 * @return -1 if no entry at the given path exists in the archive or the entry is not a file,
 *         -2 if the offset is outside the file total length,
 *         zero if the file was read in its entirety into the destination buffer,
 *         a positive value if the file was partially read, representing the remaining bytes left to be read to reach
 *         the end of the file.
 *
 */
ssize_t read_file(int tar_fd, char *path, size_t offset, uint8_t *dest, size_t *len) {
    return 0;
}