#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

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

struct stat stats;
struct bmpHeader statsBmpHeader;
char buf[BUFSIZE];

void usage(char *name)
{
    printf("Usage: %s \n", name);
}

char* userPermission(mode_t userPerm) {
    static char buf[4];

    buf[0] = (userPerm & S_IRUSR) ? 'R' : '-';
    buf[1] = (userPerm & S_IWUSR) ? 'W' : '-';
    buf[2] = (userPerm & S_IXUSR) ? 'X' : '-';
    buf[3] = '\0';
    
    return buf;
}

char* groupPermission(mode_t groupPerm) {
    static char buf[4];

    buf[0] = (groupPerm & S_IRGRP) ? 'R' : '-';
    buf[1] = (groupPerm & S_IWGRP) ? 'W' : '-';
    buf[2] = (groupPerm & S_IXGRP) ? 'X' : '-';
    buf[3] = '\0';
    
    return buf;
}

char* otherPermission(mode_t otherPerm) {
    static char buf[4];

    buf[0] = (otherPerm & S_IROTH) ? 'R' : '-';
    buf[1] = (otherPerm & S_IWOTH) ? 'W' : '-';
    buf[2] = (otherPerm & S_IXOTH) ? 'X' : '-';
    buf[3] = '\0';
    
    return buf;
}

void writeStats(char *filename) {
    int fd;

    if ((fd = open("statistica.txt", O_APPEND | O_WRONLY | O_CREAT, S_IRWXU)) < 0) {
        printf("Error opening output file\n");
        exit(5);
    }

    char output[BUFSIZE*2];
    int outputSize = sprintf(output, "nume fisier: %s\ninaltime: %d\nlungime: %d\ndimensiune: %ld\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %s\ncontorul de legaturi: %ld\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n", filename, statsBmpHeader.height, statsBmpHeader.width, stats.st_size , stats.st_uid, buf, stats.st_nlink, userPermission(stats.st_mode & S_IRWXU), groupPermission(stats.st_mode & S_IRWXG), otherPermission(stats.st_mode & S_IRWXO));

    if(write(fd, output, outputSize) < 0)
    {
        printf("Error writing to file\n");
        exit(6);
    }
}

int main(int argc, char *argv[])
{
    int f1;
    int n;

    if(argc != 2)
    {
        usage(argv[0]);
        exit(1);
    }

    if (stat(argv[1], &stats) < 0) 
    {
        printf("Error getting stats from input file\n");
        exit(2);
    }
    strftime(buf, sizeof(buf), "%D", gmtime(&stats.st_mtim.tv_sec));

    if((f1 = open(argv[1], O_RDONLY)) < 0)
    {
        printf("Error opening input file\n");
        exit(3);
    }

    if (read(f1, &statsBmpHeader, sizeof(bmpHeader)) < 0)
    {
        printf("Error reading input file\n");
        exit(4);
    }

    writeStats(argv[1]);

    close(f1);
    return 0;
}
