//
// Created by tomas on 30/11/21.
//
#include "listProcess.h"


void createEmptyListP(tListP* L){
    *L=NULL;
}
bool isEmptyListP(tListP L){
    return (L==NULL);
}
tPosLP firstP(tListP L){
    return L;
}
tPosLP lastP(tListP L){
    tPosLP i;
    for(i=L;i->next!=NULL;i=i->next);
    return i;
}
tPosLP nextP(tPosLP p, tListP L){
    return p->next;
}
tPosLP previousP(tPosLP p, tListP L){
    tPosLP i;
    if(p==L){
        return NULL;
    }else{
        for(i=L;i->next!=p;i=i->next);
        return i;
    }
}
bool insertItemP(pid_t  pid, char * tr[], tListP* L){
    tPosLP n, p;
    struct passwd *pa;
    int i = 1;
    uid_t u = getuid();
    n=malloc(sizeof(struct tNodeP));
    if(n==NULL){
        return false;
    }else{
        dataP d;
        d.pid = pid;
        char ti[20];
        time_t t = time(NULL);
        strftime(ti, 18, "%Y/%m/%d-%H:%M", localtime(&t));
        strcpy(d.tim, ti);
        if ((pa=getpwuid(u))==NULL){
            sprintf(d.user, "%d", (int)u);
        }else{
            strcpy(d.user, pa->pw_name);
        }
        strcpy(d.stat, "RUNNING");
        d.signal_return = 000;
        if(tr[0] == NULL){
            strcpy(d.executing_process, "NULL");
        }else{
            strcpy(d.executing_process, tr[0]);
            for(;tr[i] != NULL;i++){
                strcat(d.executing_process, " ");
                strcat(d.executing_process, tr[i]);
            }
        }
        n->data = d;
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
void deleteAtPositionP(tPosLP p, tListP* L){
    tPosLP i;
    if(p==*L){
        *L=p->next;
    }else if(p->next==NULL){
        for(i=*L;i->next!=p; i=i->next);
        i->next=NULL;
    }else{
        i=p->next;
        p->data=i->data;
        p->next=i->next;
        p=i;
    }
    free(p);
}
