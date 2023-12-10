//
// Created by tomas on 30/11/21.
//
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


typedef struct tNodeP * tPosLP;
typedef tPosLP tListP;

typedef  struct dataP{
    pid_t pid;
    char user[1024];
    char executing_process[1024];
    char tim[20];
    char stat[1024];
    int signal_return;
}dataP;

struct tNodeP{
    struct dataP data;
    tPosLP next;
};

void createEmptyListP(tListP*);
bool isEmptyListP(tListP);
tPosLP firstP(tListP);
tPosLP lastP(tListP);
tPosLP nextP(tPosLP, tListP);
tPosLP previousP(tPosLP, tListP);
bool insertItemP(pid_t, char **, tListP*);
void deleteAtPositionP(tPosLP, tListP*);
