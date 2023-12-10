#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct tNode * tPosL;
typedef tPosL tList;

struct tNode{
    char  command[1024];
    tPosL next;
};

void createEmptyList(tList*);
bool isEmptyList(tList);
tPosL first(tList);
tPosL last(tList);
tPosL next(tPosL, tList);
tPosL previous(tPosL, tList);
bool insertItem(char**, tList*);
void deleteLast(tList *);
char* getItem(tPosL, tList);
