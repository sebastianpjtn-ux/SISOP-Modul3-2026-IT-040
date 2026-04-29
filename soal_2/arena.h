#ifndef ARENA_H
#define ARENA_H
#include <sys/ipc.h>
#include <sys/msg.h>

#define SHM_KEY 1234
#define MSG_KEY 5678

typedef struct{
    int hp1, hp2;
    int active;
    char player1[50], player2[50];
    char logs[5][100];
    int log_index;
}Battle;

typedef struct{
        int status, choice;
        char username[50], password[50];
        Battle battle;
}SharedData;

typedef struct{
        long type;
        char text[100];
}Message;

#endif
