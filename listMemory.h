//
// Created by tomas on 10/11/21.
//
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


typedef struct tNodeM * tPosLM;
typedef tPosLM tListM;

typedef  struct dataM{
    void * address;
    size_t size;
    char type_of_allocation_or_name[1024];
    char tim[20];
    int fileDescriptor_key;
}dataM;

struct tNodeM{
    struct dataM data;
    tPosLM next;
};

void createEmptyListM(tListM*);
bool isEmptyListM(tListM);
tPosLM firstM(tListM);
tPosLM lastM(tListM);
tPosLM nextM(tPosLM, tListM);
tPosLM previousM(tPosLM, tListM);
tPosLM findItemMalloc(size_t ,tListM );
tPosLM findItemMap(char[] , tListM);
tPosLM findItemShared();
bool insertItemMalloc(void *, size_t, tListM*);
bool insertItemShared(void *, size_t, char*, int, tListM*);
bool insertItemMmap(void *, size_t, char*, int,  tListM*);
void deleteAtPositionM(tPosLM, tListM*);
dataM getItemM(tPosLM , tListM);