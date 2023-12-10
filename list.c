
#include "list.h"

void createEmptyList(tList* L){
    *L=NULL;
}
bool isEmptyList(tList L){
    return (L==NULL);
}
tPosL first(tList L){
    return L;
}
tPosL last(tList L){
    tPosL i;
    for(i=L;i->next!=NULL;i=i->next);
    return i;
}
tPosL next(tPosL p, tList L){
    return p->next;
}
tPosL previous(tPosL p, tList L){
    tPosL i;
    if(p==L){
        return NULL;
    }else{
        for(i=L;i->next!=p;i=i->next);
        return i;
    }
}

bool insertItem(char *tr[], tList* L){
    tPosL n, p;
    n=malloc(sizeof(struct tNode));
    if(n==NULL){
        return false;
    }else{
        strcpy(n->command, tr[0]);
        for(int i = 1;tr[i]!=NULL;i++){
            strcat(n->command, " ");
            strcat(n->command, tr[i]);
        }
        n->next=NULL;
        if(*L==NULL){
            *L=n;
        }else{
            for(p=*L;p->next!=NULL;p=p->next);
            p->next=n;
        }
    }
    return true;
}


void deleteLast(tList* L){
    if(L==NULL){
        return;
    }else if(next(*L,*L)==NULL){
       free(*L);
       *L=NULL;
    } else{
        tPosL p=*L;
        while(p->next->next!=NULL)
            p=p->next;
        free(p->next);
        p->next=NULL;
    }
}


char* getItem(tPosL p, tList L){
    return p->command;
}

