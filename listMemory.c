#include "listMemory.h"


void createEmptyListM(tListM* L){
    *L=NULL;
}
bool isEmptyListM(tListM L){
    return (L==NULL);
}
tPosLM firstM(tListM L){
    return L;
}
tPosLM lastM(tListM L){
    tPosLM i;
    for(i=L;i->next!=NULL;i=i->next);
    return i;
}
tPosLM nextM(tPosLM p, tListM L){
    return p->next;
}
tPosLM previousM(tPosLM p, tListM L){
    tPosLM i;
    if(p==L){
        return NULL;
    }else{
        for(i=L;i->next!=p;i=i->next);
        return i;
    }
}
tPosLM findItemMalloc(size_t size,tListM L) {
    tPosLM p;
    for(p=firstM(L);p!=NULL;p=p->next){
        if( size==p->data.size && strcmp(p->data.type_of_allocation_or_name,"malloc")==0){
            return p;
        }
    }
    return NULL;

}
tPosLM findItemMap(char file[], tListM L){
    tPosLM p;
    for(p=firstM(L);(p!=NULL)&&(strcmp(p->data.type_of_allocation_or_name,file)!=0);p=p->next);
    if((p!=NULL)&&strcmp(p->data.type_of_allocation_or_name,file)==0){
        return p;
    }
    else{
        return NULL;
    }
}
tPosLM findItemShared(int key, tListM Lm){
    tPosLM p = firstM(Lm);
    if(isEmptyListM(Lm)) return NULL;
    for(;p!= lastM(Lm);p= nextM(p, Lm)){
        if(!strcmp(p->data.type_of_allocation_or_name, "shared")){
            if(p->data.fileDescriptor_key == key){
                return p;
            }
        }
    }
    if(!strcmp(p->data.type_of_allocation_or_name, "shared")){
        if(p->data.fileDescriptor_key == key){
            return p;
        }
    }
    return NULL;
}
bool insertItemMalloc(void * address, size_t size, tListM* L){
    tPosLM n, p;
    n=malloc(sizeof(struct tNodeM));
    if(n==NULL){
        return false;
    }else{
        dataM d;
        d.address = address;
        d.size = size;
        char ti[20];
        time_t t = time(NULL);
        strftime(ti, 18, "%Y/%m/%d-%H:%M", localtime(&t));
        strcpy(d.tim, ti);
        strcpy(d.type_of_allocation_or_name, "malloc");
        d.fileDescriptor_key = 0;
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
bool insertItemMmap(void * address, size_t size, char* name, int descriptor,  tListM* L){
    tPosLM n, p;
    n=malloc(sizeof(struct tNodeM));
    if(n==NULL){
        return false;
    }else{
        dataM d;
        d.address = address;
        d.size = size;
        char ti[20];
        time_t t = time(NULL);
        strftime(ti, 18, "%Y/%m/%d-%H:%M", localtime(&t));
        strcpy(d.tim, ti);
        d.fileDescriptor_key = descriptor;
        strcpy(d.type_of_allocation_or_name, name);
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
bool insertItemShared(void * address, size_t size, char * tim, int key, tListM* L){
    tPosLM n, p;
    n=malloc(sizeof(struct tNodeM));
    if(n==NULL){
        return false;
    }else{
        dataM d;
        d.address = address;
        d.size = size;
        strcpy(d.tim, tim);
        d.fileDescriptor_key = key;
        strcpy(d.type_of_allocation_or_name, "shared");
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


void deleteAtPositionM(tPosLM p, tListM* L){
    tPosLM i;
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

dataM getItemM(tPosLM p, tListM L){
    return p->data;
}
