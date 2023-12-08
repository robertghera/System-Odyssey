#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>
#include <sys/wait.h>
#include <string.h>
#include <regex.h>
#include <errno.h>

#define BUFSIZE 1024
#define MAX_PROCESS 16

#pragma pack(1)
typedef struct bmpHeader {
    char signature[2];
    int fileSize;
    int reserved;
    int dataOffset;
    int size;
    int width;
    int height;
    char planes[2];
    char bitCount[2];
    int compression;
    int xPixelPerM;
    int yPixelPerM;
    int colorsUsed;
    int colorsImportant;
} bmpHeader;
#pragma pack()

void usage(char *name) {
    printf("Usage: %s <entry_dir> <output_dir> <c>\n", name);
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

int openStatFile(char *filePath) {
    static int fd;
    char fileToOpen[BUFSIZE] = "data_out/";
    strcat(fileToOpen, filePath);
    strcat(fileToOpen, "_statistica.txt");

    if ((fd = open(fileToOpen, O_WRONLY | O_CREAT, S_IRWXU)) < 0) {
        perror("Error opening output file\n");
        exit(1);
    }

    return fd;
}

int processDirectoryStats(struct dirent *dirent, struct stat stats) {
    int fd = openStatFile(dirent->d_name);

    char output[BUFSIZE * 2];
    char buf[BUFSIZE];

    strftime(buf, sizeof(buf), "%d.%m.%Y", gmtime(&stats.st_mtime));
    int outputSize = sprintf(output,
                             "nume director: %s\nidentificatorul utilizatorului: %d\n%s\n",
                             dirent->d_name, stats.st_uid, writePermission(stats));

    if (write(fd, output, outputSize) < 0) {
        perror("Error writing to file\n");
        exit(1);
    }

    return 5;
}

int processImageStats(struct dirent *dirent, struct stat stats, char *filePath) {
    int fd = openStatFile(dirent->d_name), outputSize;
    char output[BUFSIZE * 2];
    char buf[BUFSIZE];

    strftime(buf, sizeof(buf), "%d.%m.%Y", gmtime(&stats.st_mtime));

    int f1;
    struct bmpHeader statsBmpHeader;

    if ((f1 = open(filePath, O_RDONLY)) < 0) {
        perror("Error opening input file\n");
        exit(1);
    }

    if (read(f1, &statsBmpHeader, sizeof(bmpHeader)) < 0) {
        perror("Error reading input file\n");
        exit(1);
    }

    outputSize = sprintf(output,
                         "nume fisier: %s\ninaltime: %d\nlungime: %d\ndimensiune: %ld\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %s\ncontorul de legaturi: %ld\n%s\n",
                         dirent->d_name, statsBmpHeader.height, statsBmpHeader.width, stats.st_size, stats.st_uid,
                         buf,
                         stats.st_nlink, writePermission(stats));

    if (close(f1) < 0) {
        perror("Error closing file\n");
        exit(1);
    }

    if (write(fd, output, outputSize) < 0) {
        perror("Error writing to file\n");
        exit(1);
    }

    return 10;
}

void processImageConversion(char *filePath) {
    int f1, bytesRead;
    char colors[3];
    struct bmpHeader statsBmpHeader;

    if ((f1 = open(filePath, O_RDWR)) < 0) {
        perror("Error opening input file\n");
        exit(1);
    }

    if (read(f1, &statsBmpHeader, sizeof(bmpHeader)) < 0) {
        perror("Error reading input file\n");
        exit(1);
    }

    lseek(f1, statsBmpHeader.dataOffset, SEEK_SET);

    while ((bytesRead = read(f1, colors, sizeof(colors))) > 0) {
        char grayscale = 0.299 * colors[0] + 0.587 * colors[1] + 0.114 * colors[2];
        memset(colors, grayscale, sizeof(colors));
        lseek(f1, -bytesRead, SEEK_CUR);
        write(f1, colors, sizeof(colors));
    }


    if (close(f1) < 0) {
        perror("Error closing file\n");
        exit(1);
    }
}

int processLinkFileStats(struct dirent *dirent, struct stat stats, struct stat statsFile) {
    int fd = openStatFile(dirent->d_name), outputSize;
    char output[BUFSIZE * 2];

    outputSize = sprintf(output,
                         "nume legatura: %s\ndimensiune: %ld\ndimensiune fisier: %ld\n%s\n",
                         dirent->d_name, stats.st_size, statsFile.st_size,writePermission(stats));

    if (write(fd, output, outputSize) < 0) {
        perror("Error writing to file\n");
        exit(1);
    }

    return 6;
}

int processFileStats(struct dirent *dirent, struct stat stats) {
    int fd = openStatFile(dirent->d_name), outputSize;
    char output[BUFSIZE * 2];
    char buf[BUFSIZE];

    strftime(buf, sizeof(buf), "%d.%m.%Y", gmtime(&stats.st_mtime));

    outputSize = sprintf(output,
                         "nume fisier: %s\ndimensiune: %ld\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %s\ncontorul de legaturi: %ld\n%s\n",
                         dirent->d_name, stats.st_size, stats.st_uid,
                         buf,
                         stats.st_nlink, writePermission(stats));

    if (write(fd, output, outputSize) < 0) {
        perror("Error writing to file\n");
        exit(1);
    }

    return 8;
}

void processDirectory(char *directoryPath, char *character) {
    DIR *dir = NULL;
    struct dirent *dirent = NULL;

    if ((dir = opendir(directoryPath)) == NULL) {
        perror("Error opening directory\n");
        exit(1);
    }

    pid_t pid[MAX_PROCESS];
    int totalProcessesRunning = 0;

    regex_t regexBMP;
    if (regcomp(&regexBMP, ".bmp$", 0) != 0) {
        perror("Regex could not compile\n");
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

            if (lstat(filePath, &stats) < 0) {
                perror("Error getting stats from input file\n");
                exit(1);
            }

            int isValidRegex = REG_NOMATCH;
            if (S_ISREG(stats.st_mode)) {
                isValidRegex = regexec(&regexBMP, dirent->d_name, 0, NULL, 0);
            }

            if (S_ISREG(stats.st_mode) && isValidRegex == 0) {
                // Is bmp file
                if (totalProcessesRunning < MAX_PROCESS && (pid[totalProcessesRunning] = fork()) < 0) {
                    perror("Error creating a new process\n");
                    exit(1);
                }
                totalProcessesRunning++;

                if (pid[totalProcessesRunning - 1] == 0) {
                    int exitProcess = processImageStats(dirent, stats, filePath);
                    exit(exitProcess);
                }

                if (totalProcessesRunning < MAX_PROCESS && (pid[totalProcessesRunning] = fork()) < 0) {
                    perror("Error creating a new process\n");
                    exit(1);
                }
                totalProcessesRunning++;

                if (pid[totalProcessesRunning - 1] == 0) {
                    processImageConversion(filePath);
                    exit(0);
                }
            } else if (S_ISREG(stats.st_mode)) {
                if (totalProcessesRunning < MAX_PROCESS && (pid[totalProcessesRunning] = fork()) < 0) {
                    perror("Error creating a new process\n");
                    exit(1);
                }
                totalProcessesRunning++;

                if (pid[totalProcessesRunning - 1] == 0) {
                    int exitProcess = 0;
                    exitProcess = processFileStats(dirent, stats);
                    exit(exitProcess);
                }

                if (totalProcessesRunning < MAX_PROCESS && (pid[totalProcessesRunning] = fork()) < 0) {
                    perror("Error creating a new process\n");
                    exit(1);
                }
                totalProcessesRunning++;

                if (pid[totalProcessesRunning - 1] == 0) {
                    //execlp("bash", "bash", "./script", character, (char *)NULL);

                    // perror("Error executin bash script\n");
                    // exit(1);
                    exit(0);
                }
            } else {
                if (totalProcessesRunning < MAX_PROCESS && (pid[totalProcessesRunning] = fork()) < 0) {
                    perror("Error creating a new process\n");
                    exit(1);
                }
                totalProcessesRunning++;

                if (pid[totalProcessesRunning - 1] == 0) {
                    int exitProcess = 0;
                    if (S_ISDIR(stats.st_mode)) {
                        exitProcess = processDirectoryStats(dirent, stats);
                    } else if (S_ISLNK(stats.st_mode)) {
                        struct stat statsFile;

                        if (stat(filePath, &stats) < 0) {
                            perror("Error getting stats from input file\n");
                            exit(1);
                        }
                        exitProcess = processLinkFileStats(dirent, stats, statsFile);
                    }

                    exit(exitProcess);
                }
            }
        }
    }

    pid_t waitProcessID;
    int processStatus;

    int fStats;
    if ((fStats = open("statistica.txt", O_APPEND | O_WRONLY | O_CREAT, S_IRWXU)) < 0) {
        perror("Error opening file for stats\n");
        exit(1);
    }

    for (int i = 0; i < totalProcessesRunning; i++) {
        waitProcessID = wait(&processStatus);
        if (WIFEXITED(processStatus)) {
            int exitStatus = WEXITSTATUS(processStatus);
            printf("S-a încheiat procesul cu pid-ul %d și codul %d\n", waitProcessID, exitStatus);

            if (exitStatus > 0) {
                char output[BUFSIZE];
                int outputSize = sprintf(output, "%d\n", exitStatus);

                if (write(fStats, output, outputSize) < 0) {
                    perror("Error writing to file\n");
                    exit(1);
                }
            }
        } else {
            printf("The child process %d finished abnormally", waitProcessID);
        }
    }

    if (closedir(dir) < 0) {
        perror("Error closing dir\n");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        usage(argv[0]);
        exit(1);
    }

    processDirectory(argv[1], argv[3]);

    return 0;
}
