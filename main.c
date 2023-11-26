#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>

#define BUFSIZE 1024

#pragma pack(1)
typedef struct bmpHeader {
    char signature[2];
    int fileSize;
    int reserved;
    int dataOffset;
    int size;
    int width;
    int height;
} bmpHeader;
#pragma pack()

//struct stat stats;
//struct bmpHeader statsBmpHeader;
//char buf[BUFSIZE];

void usage(char *name) {
    printf("Usage: %s \n", name);
}

char *userPermission(mode_t userPerm) {
    static char buf[4];

    buf[0] = (userPerm & S_IRUSR) ? 'R' : '-';
    buf[1] = (userPerm & S_IWUSR) ? 'W' : '-';
    buf[2] = (userPerm & S_IXUSR) ? 'X' : '-';
    buf[3] = '\0';

    return buf;
}

char *groupPermission(mode_t groupPerm) {
    static char buf[4];

    buf[0] = (groupPerm & S_IRGRP) ? 'R' : '-';
    buf[1] = (groupPerm & S_IWGRP) ? 'W' : '-';
    buf[2] = (groupPerm & S_IXGRP) ? 'X' : '-';
    buf[3] = '\0';

    return buf;
}

char *otherPermission(mode_t otherPerm) {
    static char buf[4];

    buf[0] = (otherPerm & S_IROTH) ? 'R' : '-';
    buf[1] = (otherPerm & S_IWOTH) ? 'W' : '-';
    buf[2] = (otherPerm & S_IXOTH) ? 'X' : '-';
    buf[3] = '\0';

    return buf;
}

char *writePermission(struct stat stats) {
    static char buf[BUFSIZE];

    sprintf(buf, "drepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n",
            userPermission(stats.st_mode & S_IRWXU),
            groupPermission(stats.st_mode & S_IRWXG), otherPermission(stats.st_mode & S_IRWXO));

    return buf;
}

int openStatFile() {
    static int fd;

    if ((fd = open("statistica.txt", O_APPEND | O_WRONLY | O_CREAT, S_IRWXU)) < 0) {
        printf("Error opening output file\n");
        exit(5);
    }

    return fd;
}

void processDirectoryStats(struct dirent *dirent, struct stat stats) {
    int fd = openStatFile();

    char output[BUFSIZE * 2];
    char buf[BUFSIZE];

    strftime(buf, sizeof(buf), "%d.%m.%Y", gmtime(&stats.st_mtime));
    int outputSize = sprintf(output,
                             "nume director: %s\nidentificatorul utilizatorului: %d\n%s\n",
                             dirent->d_name, stats.st_uid, writePermission(stats));

    printf("%s", output);
    if (write(fd, output, outputSize) < 0) {
        printf("Error writing to file\n");
        exit(6);
    }
}

void processFileStats(struct dirent *dirent, struct stat stats, char *filePath) {
    int fd = openStatFile(), outputSize;
    char output[BUFSIZE * 2];
    char buf[BUFSIZE];

    strftime(buf, sizeof(buf), "%d.%m.%Y", gmtime(&stats.st_mtime));

    if (strcmp(&dirent->d_name[dirent->d_namlen - 4], ".bmp") == 0) {
        int f1;
        struct bmpHeader statsBmpHeader;

        if ((f1 = open(filePath, O_RDONLY)) < 0) {
            printf("Error opening input file\n");
            exit(3);
        }

        if (read(f1, &statsBmpHeader, sizeof(bmpHeader)) < 0) {
            printf("Error reading input file\n");
            exit(4);
        }

        outputSize = sprintf(output,
                                 "nume fisier: %s\ninaltime: %d\nlungime: %d\ndimensiune: %ld\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %s\ncontorul de legaturi: %d\n%s\n",
                                 dirent->d_name, statsBmpHeader.height, statsBmpHeader.width, stats.st_size, stats.st_uid,
                                 buf,
                                 stats.st_nlink, writePermission(stats));

        if(close(f1) < 0) {
            printf("Error closing file\n");
            exit(8);
        }
    } else {
        outputSize = sprintf(output,
                                 "nume fisier: %s\ndimensiune: %ld\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %s\ncontorul de legaturi: %d\n%s\n",
                                 dirent->d_name, stats.st_size, stats.st_uid,
                                 buf,
                                 stats.st_nlink, writePermission(stats));
    }

    printf("%s", output);
    if (write(fd, output, outputSize) < 0) {
        printf("Error writing to file\n");
        exit(6);
    }
}

void processDirectory(char *directoryPath) {
    DIR *dir = NULL;
    struct dirent *dirent = NULL;

    if ((dir = opendir(directoryPath)) == NULL) {
        printf("Error opening directory\n");
        exit(1);
    }

    while ((dirent = readdir(dir)) != NULL) {
        if (strcmp(dirent->d_name, ".") != 0 &&
            strcmp(dirent->d_name, "..") != 0) { // treat current directory and parent directory
            struct stat stats;
            char filePath[BUFSIZE];
            strcpy(filePath, directoryPath);
            strcat(filePath, "/");
            strcat(filePath, dirent->d_name);

            if (stat(filePath, &stats) < 0) {
                printf("Error getting stats from input file\n");
                exit(2);
            }

            if (S_ISDIR(stats.st_mode)) {
                processDirectoryStats(dirent, stats);
            } else if (S_ISREG(stats.st_mode)) {
                processFileStats(dirent, stats, filePath);
            }
        }
    }

    if(closedir(dir) < 0) {
        printf("Error closing dir\n");
        exit(8);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        usage(argv[0]);
        exit(1);
    }

    processDirectory(argv[1]);

    return 0;
}
