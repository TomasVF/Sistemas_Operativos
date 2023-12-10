/*
Tomás Villalba Ferreiro: tomas.villalba.ferreiro@udc.es
Brais Gómez Espiñeira: brais.gomez2@udc.es
*/


#include <stdio.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <sys/shm.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/resource.h>

#include "list.c"
#include "listMemory.c"
#include "listProcess.c"

#define MAX_LINE 1024
#define LEERCOMPLETO ((ssize_t)-1)


/******************************SENALES ******************************************/
struct SEN{
    char *nombre;
    int senal;
};
static struct SEN sigstrnum[]={
        {"HUP", SIGHUP},
        {"INT", SIGINT},
        {"QUIT", SIGQUIT},
        {"ILL", SIGILL},
        {"TRAP", SIGTRAP},
        {"ABRT", SIGABRT},
        {"IOT", SIGIOT},
        {"BUS", SIGBUS},
        {"FPE", SIGFPE},
        {"KILL", SIGKILL},
        {"USR1", SIGUSR1},
        {"SEGV", SIGSEGV},
        {"USR2", SIGUSR2},
        {"PIPE", SIGPIPE},
        {"ALRM", SIGALRM},
        {"TERM", SIGTERM},
        {"CHLD", SIGCHLD},
        {"CONT", SIGCONT},
        {"STOP", SIGSTOP},
        {"TSTP", SIGTSTP},
        {"TTIN", SIGTTIN},
        {"TTOU", SIGTTOU},
        {"URG", SIGURG},
        {"XCPU", SIGXCPU},
        {"XFSZ", SIGXFSZ},
        {"VTALRM", SIGVTALRM},
        {"PROF", SIGPROF},
        {"WINCH", SIGWINCH},
        {"IO", SIGIO},
        {"SYS", SIGSYS},
/*senales que no hay en todas partes*/
#ifdef SIGPOLL
        {"POLL", SIGPOLL},
#endif
#ifdef SIGPWR
        {"PWR", SIGPWR},
#endif
#ifdef SIGEMT
        {"EMT", SIGEMT},
#endif
#ifdef SIGINFO
        {"INFO", SIGINFO},
#endif
#ifdef SIGSTKFLT
        {"STKFLT", SIGSTKFLT},
#endif
#ifdef SIGCLD
        {"CLD", SIGCLD},
#endif
#ifdef SIGLOST
        {"LOST", SIGLOST},
#endif
#ifdef SIGCANCEL
        {"CANCEL", SIGCANCEL},
#endif
#ifdef SIGTHAW
        {"THAW", SIGTHAW},
#endif
#ifdef SIGFREEZE
        {"FREEZE", SIGFREEZE},
#endif
#ifdef SIGLWP
        {"LWP", SIGLWP},
#endif
#ifdef SIGWAITING
        {"WAITING", SIGWAITING},
#endif
        {NULL,-1},
}; /*fin array sigstrnum */

int Senal(char * sen) /*devuel el numero de senial a partir del nombre*/
{
    int i;
    for (i=0; sigstrnum[i].nombre!=NULL; i++)
        if (!strcmp(sen, sigstrnum[i].nombre))
            return sigstrnum[i].senal;
    return -1;
}

char *NombreSenal(int sen) /*devuelve el nombre senal a partir de la senal*/
{ /* para sitios donde no hay sig2str*/
    int i;
    for (i=0; sigstrnum[i].nombre!=NULL; i++)
        if (sen==sigstrnum[i].senal)
            return sigstrnum[i].nombre;
    return ("SIGUNKNOWN");
}


void cmd_ayuda(char**);
void cmd_comandoN(char**, tList, tListM, tListP, char **);
void recOption(char *,int, int, int, int, char);
int aux4 = 3;
int aux5 = 4;
int aux6 = 5;
int saved_stderr = 2;
char new_sterr[1024];

struct CMD{
    char *name;
    void (*func)(char **);
    char *help;

};

extern char ** environ;


char * Ejecutable (char *s)
{
    char path[4096];
    static char aux2[1024];
    struct stat st;
    char *p;
    if (s==NULL || (p=getenv("PATH"))==NULL)
        return s;
    if (s[0]=='/' || !strncmp (s,"./",2) || !strncmp (s,"../",3)){
        return s; /*is an absolute pathname*/
    }
    strncpy (path, p, 4096);
    for (p=strtok(path,":"); p!=NULL; p=strtok(NULL,":")){
        sprintf (aux2,"%s/%s",p,s);
        if (lstat(aux2,&st)!=-1)
            return aux2;
    }
    return s;
}
int OurExecvpe(char *file, char *const argv[], char *const envp[])
{
    return (execve(Ejecutable(file),argv, envp));
}


void cmd_priority(char ** tr){
    int id, priority;
    if(tr[0] == NULL){
        if((id = getpid()) == -1){
            perror("");
        }
    }else if(tr[1] == NULL){
        id = atoi(tr[0]);
    }else{
        if(setpriority(PRIO_PROCESS, atoi(tr[0]), atoi(tr[1])) == -1){
            perror("Cannot change priority");
            return;
        }
        printf("Priority of process %d was changed to %d\n", atoi(tr[0]), atoi(tr[1]));
        return;
    }
    if((priority = getpriority(PRIO_PROCESS, id)) == -1){
        perror("Cannot get priority");
    }
    printf("Priority of the process %d is %d\n", id, priority);
}


void cmd_rederr(char * tr[]){
    int d;
    if(tr[0] == NULL){
        if(saved_stderr == 2){
            printf("stderr in original file configuration\n");
        }else{
            printf("stderr in file %s\n", new_sterr);
        }

    }else if(!strcmp(tr[0], "-reset")){
        if(dup2(saved_stderr, STDERR_FILENO) == -1){
            perror("");
            return;
        }
        saved_stderr = 2;
    }else{
        if((d = open(tr[0], O_RDWR|O_CREAT, 0777)) == -1){
            perror("");
            return;
        }
        if(saved_stderr == 2){
            if((saved_stderr = dup(STDERR_FILENO)) == -1){
                perror("");
                return;
            }
        }
        if(dup2(d, STDERR_FILENO) == -1){
            perror("");
            return;
        }
        close(d);
        strcpy(new_sterr, tr[0]);
    }
}


void MuestraEntorno (char **entorno, char * nombre_entorno){
    int i=0;
    while (entorno[i]!=NULL) {
        printf ("%p->%s[%d]=(%p) %s\n", &entorno[i],nombre_entorno, i,entorno[i],entorno[i]);
        i++;
    }
}


void cmd_entorno(char * tr[], char * env[]){
    if(tr[0] == NULL){
        MuestraEntorno(env, "env");
    }else if(!strcmp(tr[0], "-environ")){
        MuestraEntorno(environ, "environ");
    }else if(!strcmp(tr[0], "-addr")){
        printf("environ:  %p (almacenado en %p)\nmain env: %p (almacenado en %p)\n", environ, &environ, env, &env);
    }else{
        printf("%s is not a valid option for the command entorno", tr[0]);
    }
}


void find_entorno(char * variable, char ** entorno, char * nombre_entorno){
    char name_value[1024] = "";
    strcpy(name_value, variable);
    strcat(name_value, "=");
    strcat(name_value, getenv(variable));
    for(int i = 0;entorno[i] != NULL;i++){
        if(!strcmp(entorno[i], name_value)){
            printf("Con %s %s(%p) @%p\n", nombre_entorno, entorno[i], entorno[i], &entorno[i]);
            return;
        }
    }
}


void cmd_mostrarvar(char * tr[], char * env[]){
    if(tr[0] == NULL){
        MuestraEntorno(env, "env");
    }else{
        if(getenv(tr[0]) == NULL){
            printf("That variable doesn't exist");
            return;
        }
        find_entorno(tr[0], env, "env main");
        find_entorno(tr[0], environ, "environ");
        printf("Con getenv %s(%p)\n", getenv(tr[0]), getenv(tr[0]));
    }
}


void change_entorno_variable(char * name_value, char ** entorno, char * name_value_old){
    for(int i = 0;entorno[i] != NULL;i++){
        if(!strcmp(entorno[i], name_value_old)){
            entorno[i] = name_value;
            return;
        }
    }
}


void cmd_cambiarvar(char * tr[], char * env[]){
    if(tr[0] == NULL || tr[1] == NULL || tr[2] == NULL){
        return;
    }
    char value[1024] = "", name[1024] = "", value_name[2048] = "", name_value_old[2048] = "";
    strcpy(value, tr[2]);
    strcpy(name, tr[1]);
    strcat(value_name, name);
    strcat(value_name, "=");
    strcat(value_name, value);
    if(getenv(name) == NULL){
        printf("That variable doesn't exist");
        return;
    }
    strcpy(name_value_old, name);
    strcat(name_value_old, "=");
    strcat(name_value_old, getenv(name));
    if(!strcmp(tr[0], "-a")){
        change_entorno_variable(value_name, env, name_value_old);
    }
    if(!strcmp(tr[0], "-e")){
        change_entorno_variable(value_name, environ, name_value_old);
    }
    if(!strcmp(tr[0], "-p")){
        putenv(value_name);
    }
}


char * NombreUsuario (uid_t uid)
{
    struct passwd *p;
    if ((p=getpwuid(uid))==NULL)
        return (" ??????");
    return p->pw_name;
}


void MostrarUidsProceso (void){
    uid_t real=getuid(), efec=geteuid();
    printf ("Credencial real: %d, (%s)\n", real, NombreUsuario (real));
    printf ("Credencial efectiva: %d, (%s)\n", efec, NombreUsuario (efec));
}


uid_t UidUsuario (char * nombre)
{
    struct passwd *p;
    if ((p=getpwnam (nombre))==NULL)
        return (uid_t) -1;
    return p->pw_uid;
}


void CambiarUidLogin (char * login)
{
    uid_t uid;
    if ((uid=UidUsuario(login))==(uid_t) -1){
        printf("loin no valido: %s\n", login);
        return;
    }
    if (setuid(uid)==-1)
        printf ("Imposible cambiar credencial: %s\n", strerror(errno));
}


void CambiarUidUid (uid_t uid){
    if (setuid(uid)==-1)
        printf ("Imposible cambiar credencial: %s\n", strerror(errno));
}


void cmd_uid(char * tr[]){
    if(tr[0] == NULL){
        MostrarUidsProceso();
    }else{
        if(!strcmp(tr[0], "-get")){
            MostrarUidsProceso();
        }
        if(!strcmp(tr[0], "-set")){
            if(tr[1] == NULL){
                MostrarUidsProceso();
            }else{
                if(!strcmp(tr[1], "-l")){
                    if(tr[2] == NULL){
                        MostrarUidsProceso();
                        return;
                    }
                    CambiarUidLogin(tr[2]);
                }else{
                    CambiarUidUid((uid_t)atoi(tr[1]));
                }
            }
        }
    }
}


void cmd_fork(char *tr[]){
/* fork a child process */
    pid_t pid = fork();
    if (pid < 0) { /* error occurred */
        fprintf(stderr, "Fork Failed");

    }else if (pid == 0) { /* child process */
    }else { /* parent process */
        /* parent will wait for the child to complete */
        waitpid(pid,NULL,0);
    }
}


int BuscarVariable (char * var, char *e[])
{
    int pos=0;
    char aux[1024];
    if(var == NULL){
        return (-1);
    }
    strcpy (aux,var);
    strcat (aux,"=");
    while (e[pos]!=NULL)
        if (!strncmp(e[pos],aux,strlen(aux)))
            return (pos);
        else
            pos++;
    errno=ENOENT; /*no hay tal variable*/
    return(-1);
}


int crear_env(char * env_var[], char * new_env[], char * old_env[]){
    int i = 0, j = 0;
    if(env_var[0] == NULL){
        return 0;
    }
    if(!strcmp(env_var[0], "NULLENV")){
        new_env[0] = NULL;
        return 1;
    }
    while((j = BuscarVariable(env_var[i], old_env)) != -1){
        new_env[i] = old_env[j];
        i++;
    }
    new_env[i] = NULL;
    return i;
}


void cmd_ejec(char *tr[]){
    char * env[1024];
    int i;
    i = crear_env(tr, env, environ);
    tr = tr + i;
    if(tr[0]==NULL)return;
    else {
        if(i == 0){
            if(execvp(tr[0], tr) == -1)perror("");
        } else{
            if (OurExecvpe(tr[0], tr, env) == -1)perror("");
        }
    }
}


void cmd_ejecpri(char *tr[]){
    char * env[1024];
    if(tr[0] == NULL){
        return;
    }
    int i, priority = atoi(tr[0]);
    tr = tr+1;
    i = crear_env(tr, env, environ);
    tr = tr + i;
    if(tr[0]==NULL){
        return;
    }else {
        if (setpriority(PRIO_PROCESS, getpid(), priority) == -1) {
            perror("");
            return;
        }
        if (tr[1] == NULL)return;
        else {
            if(i == 0){
                if(execvp(tr[0], tr) == -1)perror("");
            } else{
                if (OurExecvpe(tr[0], tr, env) == -1)perror("");
            }
        }
    }
}


void cmd_fg(char *tr[]){
    char * env[1024];
    int pid;
    int i;
    i = crear_env(tr, env, environ);
    tr = tr + i;
    if(tr[0]==NULL){
        perror("");
        return;
    }
    if ((pid = fork()) == 0){
        if(i == 0){
            if(execvp(tr[0], tr) == -1)perror("");
        } else{
            if (OurExecvpe(tr[0], tr, env) == -1)perror("");
        }
    }else{
        waitpid(pid, NULL, 0);
    }
}


void cmd_fgpri(char *tr[]){
    int pid;
    char * env[1024];
    if(tr[0] == NULL){
        return;
    }
    int i, priority = atoi(tr[0]);
    tr = tr+1;
    i = crear_env(tr, env, environ);
    tr = tr + i;
    if(tr[0]==NULL)perror("");
    else {
        if (setpriority(PRIO_PROCESS, getpid(), priority) == -1) {
            perror("");
            return;
        }
        if ((pid = fork()) == 0){
            if(i == 0){
                if(execvp(tr[0], tr) == -1)perror("");
            } else{
                if (OurExecvpe(tr[0], tr, env) == -1)perror("");
            }
        }else{
            waitpid(pid, NULL, 0);
        }
    }
}


void cmd_back(char * tr[], tListP * Lp){
    int pid;
    char * env[1024];
    int i;
    i = crear_env(tr, env, environ);
    tr = tr + i;
    if((pid = fork()) == 0){
        if(i == 0){
            if(execvp(tr[0], tr) == -1)perror("");
        } else{
            if (OurExecvpe(tr[0], tr, env) == -1)perror("");
        }
        perror("Cannot execute");
        exit(255);
    }
    insertItemP(pid, tr, Lp);
}


void cmd_backpri(char * tr[], tListP * Lp){
    int pid;
    char * env[1024];
    if(tr[0] == NULL){
        return;
    }
    int i, priority = atoi(tr[0]);
    i = crear_env(tr, env, environ);
    tr = tr + i;
    if(tr[1] != NULL){
        pid = fork();
        if(pid > 0){
            if(setpriority(PRIO_PROCESS, pid, priority) == -1){
                perror("Cannot change priority");
                return;
            }
        }
        if(pid == 0){
            if(i == 0){
                if(execvp(tr[0], tr) == -1)perror("");
            } else{
                if (OurExecvpe(tr[0], tr, env) == -1)perror("");
            }
            perror("Cannot execute");
            exit(255);
        }else{
            insertItemP(pid, tr+1, Lp);
        }
    }
}


void cmd_ejecas(char *tr[]) {
    char * env[1024];
    if(tr[0] == NULL){
        return;
    }
    int i, uid = atoi(tr[0]);
    tr = tr+1;
    i = crear_env(tr, env, environ);
    tr = tr + i;
    if (setuid(uid)==-1)perror("");
    else {
        if (tr[1] == NULL) perror("");
        else {
            if(i == 0){
                if(execvp(tr[0], tr) == -1)perror("");
            } else{
                if (OurExecvpe(tr[0], tr, env) == -1)perror("");
            }
        }
    }
}



void cmd_fgas(char *tr[]) {
    int pid;
    char * env[1024];
    if(tr[0] == NULL){
        return;
    }
    int i, uid = atoi(tr[0]);
    tr = tr+1;
    i = crear_env(tr, env, environ);
    tr = tr + i;
    if(tr[0] == NULL) return;
    else {
        if (tr[1] == NULL) return;
        else {
            if (setuid(uid)==-1){
                perror("");
                return;
            }
            if ((pid = fork()) == 0){
                if(i == 0){
                    if(execvp(tr[0], tr) == -1)perror("");
                } else{
                    if (OurExecvpe(tr[0], tr, env) == -1)perror("");
                }
            }else{
                waitpid(pid, NULL, 0);
            }
        }
    }
}


void cmd_bgas(char *tr[], tListP * Lp) {
    int pid;
    char * env[1024];
    if(tr[0] == NULL){
        return;
    }
    int i, uid = atoi(tr[0]);
    tr = tr+1;
    i = crear_env(tr, env, environ);
    tr = tr + i;
    if (setuid(uid)==-1){
        perror("");
        return;
    }
    if((pid = fork()) == 0){
        if(i == 0){
            if(execvp(tr[0], tr) == -1)perror("");
        } else{
            if (OurExecvpe(tr[0], tr, env) == -1)perror("");
        }
        perror("Cannot execute");
        exit(255);
    }
    if(pid < 0){
        perror("");
        return;
    }
    insertItemP(pid, tr+1, Lp);
}


void actualizar_process_list(tListP * Lp){
    int valor;
    tPosLP p = firstP(*Lp);
    while(p != NULL){
        if(!strcmp(p->data.stat, "RUNNING") || !strcmp(p->data.stat, "STOPPED")){
            if (waitpid (p->data.pid,&valor, WNOHANG |WUNTRACED |WCONTINUED)==p->data.pid){
                if(WIFEXITED(valor)){
                    strcpy(p->data.stat, "TERMINATED NORMALLY");
                    p->data.signal_return = WEXITSTATUS(valor);
                }else if(WIFSIGNALED(valor)){
                    strcpy(p->data.stat, "TERMINATED BY SIGNAL");
                    p->data.signal_return = WTERMSIG(valor);
                }else if(WIFSTOPPED(valor)){
                    strcpy(p->data.stat, "STOPPED");
                    p->data.signal_return = WSTOPSIG(valor);
                }else if (WIFCONTINUED(valor)){
                    strcpy(p->data.stat, "RUNNING");
                }
            }
        }
        p = nextP(p, *Lp);
    }
}


tPosLP find_process_pid(pid_t pid, tListP Lp){
    tPosLP p = firstP(Lp);
    while(p != NULL){
        if(p->data.pid == pid){
            return p;
        }
        p = nextP(p, Lp);
    }
    return p;
}


void listjob(tPosLP p, tListP * Lp){
    actualizar_process_list(Lp);
    printf("%d       %s p=%d %s %s ", p->data.pid, p->data.user, getpriority(PRIO_PROCESS, p->data.pid), p->data.tim, p->data.stat);
    if(!strcmp(p->data.stat, "TERMINATED NORMALLY") || !strcmp(p->data.stat, "RUNNING")){
        printf("(%d) ", p->data.signal_return);
    }else{
        printf("(%s) ", NombreSenal(p->data.signal_return));
    }
    printf("%s", p->data.executing_process);
    printf("\n");
}


void cmd_listjobs(char * tr[], tListP * Lp){
    tPosLP p = firstP(*Lp);
    while(p != NULL){
        listjob(p, Lp);
        p = nextP(p, *Lp);
    }
}


void cmd_job(char * tr[], tListP * Lp){
    tPosLP p;
    int valor;
    if(tr[0] == NULL){
        cmd_listjobs(tr, Lp);
    }else if(!strcmp(tr[0], "-fg")){
        if(tr[1] == NULL){
            cmd_listjobs(tr, Lp);
        }else{
            if((p = find_process_pid(atoi(tr[1]), *Lp)) == NULL){
                cmd_listjobs(tr, Lp);
            }else{
                if (waitpid (p->data.pid,&valor, 0)==p->data.pid){
                    if(WIFEXITED(valor)){
                        strcpy(p->data.stat, "TERMINATED NORMALLY");
                        p->data.signal_return = WEXITSTATUS(valor);
                    }else if(WIFSIGNALED(valor)){
                        strcpy(p->data.stat, "TERMINATED BY SIGNAL");
                        p->data.signal_return = WTERMSIG(valor);
                    }else if(WIFSTOPPED(valor)){
                        strcpy(p->data.stat, "STOPPED");
                        p->data.signal_return = WSTOPSIG(valor);
                    }else if (WIFCONTINUED(valor)){
                        strcpy(p->data.stat, "RUNNING");
                    }
                    listjob(p, Lp);
                    deleteAtPositionP(p, Lp);
                }else{
                    perror("");
                }
            }
        }
    }else{
        if((p = find_process_pid(atoi(tr[0]), *Lp)) == NULL){
            cmd_listjobs(tr, Lp);
        }else{
            listjob(p, Lp);
        }
    }
}


tPosLP find_process_state(int terminated_normally, int terminated_signal, tListP Lp){
    tPosLP p = firstP(Lp);
    while(p != NULL){
        if(terminated_normally && !strcmp(p->data.stat, "TERMINATED NORMALLY")){
            return p;
        }
        if(terminated_signal && !strcmp(p->data.stat, "TERMINATED BY SIGNAL")){
            return p;
        }
        p = nextP(p, Lp);
    }
    return p;
}


void remove_terminated(tListP * Lp, int signal, int normal){
    while(find_process_state(normal, signal, *Lp) != NULL){
        tPosLP p =find_process_state(normal, signal, *Lp);
        deleteAtPositionP(p,Lp);
    }
}


void vaciar_listaP(tListP * Lp){
    while(!isEmptyListP(*Lp)){
        tPosLP p =lastP(*Lp);
        deleteAtPositionP(p,Lp);
    }
}


void cmd_borrarjobs(char * tr[], tListP * Lp){
    if(tr[0] == NULL){
        return;
    }
    if(!strcmp(tr[0], "-term")){
        remove_terminated(Lp, 0, 1);
    }
    if(!strcmp(tr[0], "-sig")){
        remove_terminated(Lp, 1, 0);
    }
    if(!strcmp(tr[0], "-all")){
        remove_terminated(Lp, 1, 1);
    }
    if(!strcmp(tr[0], "-clear")){
        vaciar_listaP(Lp);
    }
}


void executeNonPredefinedCommand(char * tr[], tListP * Lp){
    int back = 0;
    for(int i = 0;tr[i] != NULL;i++){
        if(!strcmp(tr[i], "&")){
            back = 1;
            tr[i] = NULL;
            break;
        }
    }
    if(back){
        cmd_back(tr, Lp);
    }else{
        cmd_fg(tr);
    }
}


void listAllMemory(tListM Lm, int mp, int s, int mll){
    printf("******Lista de bloques asignados shared para el proceso %d\n", getpid());
    tPosLM p= firstM(Lm);
    if(isEmptyListM(Lm)) return;
    for(;p!= lastM(Lm);p= nextM(p, Lm)){
        if((!strcmp(p->data.type_of_allocation_or_name, "shared") && s) || (!strcmp(p->data.type_of_allocation_or_name, "malloc") && mll) || (strcmp(p->data.type_of_allocation_or_name, "shared")!=0 && strcmp(p->data.type_of_allocation_or_name, "malloc")!=0 && mp)){
            printf("   %p%7zu %s %s", p->data.address, p->data.size, p->data.tim, p->data.type_of_allocation_or_name);
        }
        if(!strcmp(p->data.type_of_allocation_or_name, "shared") && s){
            printf(" (key %d)",p->data.fileDescriptor_key);
        }
        if(strcmp(p->data.type_of_allocation_or_name, "shared")!=0 && strcmp(p->data.type_of_allocation_or_name, "malloc")!=0 && mp){
            printf(" (descriptor %d)",p->data.fileDescriptor_key);
        }
        printf("\n");
    }
    if((!strcmp(p->data.type_of_allocation_or_name, "shared") && s) || (!strcmp(p->data.type_of_allocation_or_name, "malloc") && mll) || (strcmp(p->data.type_of_allocation_or_name, "shared")!=0 && strcmp(p->data.type_of_allocation_or_name, "malloc")!=0 && mp)){
        printf("   %p%7zu %s %s", p->data.address, p->data.size, p->data.tim, p->data.type_of_allocation_or_name);
    }
    if(!strcmp(p->data.type_of_allocation_or_name, "shared") && s){
        printf(" (key %d)",p->data.fileDescriptor_key);
    }
    if(strcmp(p->data.type_of_allocation_or_name, "shared")!=0 && strcmp(p->data.type_of_allocation_or_name, "malloc")!=0 && mp){
        printf(" (descriptor %d)",p->data.fileDescriptor_key);
    }
    printf("\n");
}


void * MmapFichero (char * fichero, int protection,tListM *Lm){
    int df, map=MAP_PRIVATE,modo=O_RDONLY;
    struct stat s;
    void *p;
    if (protection&PROT_WRITE) modo=O_RDWR;
    if (stat(fichero,&s)==-1 || (df=open(fichero, modo))==-1)
        return NULL;
    if ((p=mmap (NULL,s.st_size, protection,map,df,0))==MAP_FAILED)
        return NULL;
    insertItemMmap(p,s.st_size,fichero,df,Lm);
    return p;
}

void Mmap (char *arg[],tListM *Lm) /*arg[0] is the file name and arg[1] is the permissions*/{
    char *perm;
    void *p;
    int protection=0;
    if (arg[0]==NULL)
    {
        listAllMemory(*Lm, 1, 0, 0);
        return;}
    if ((perm=arg[1])!=NULL && strlen(perm)<4) {
        if (strchr(perm,'r')!=NULL) protection|=PROT_READ;
        if (strchr(perm,'w')!=NULL) protection|=PROT_WRITE;
        if (strchr(perm,'x')!=NULL) protection|=PROT_EXEC;
    }
    if ((p=MmapFichero(arg[0],protection,Lm))==NULL)
        perror ("Imposible mapear fichero");
    else
        printf ("fichero %s mapeado en %p\n", arg[0], p);
}


void mmapFree(char * tr, tListM *Lm){
    if (access(tr, F_OK) != 0) {
        perror("");
    } else {
        tPosLM p = findItemMap(tr, *Lm);
        if (p == NULL) {
            listAllMemory(*Lm, 1, 0, 0);
        } else {
            dataM d = getItemM(p, *Lm);
            deleteAtPositionM(p, Lm);
            munmap(d.address, d.size);

        }
    }
}


void cmd_mmap(char *tr[],tListM *Lm){
    if(tr[0]==NULL){
        listAllMemory(*Lm, 1, 0, 0);
    }else{
        if(!strcmp(tr[0],"-free")){
            if (tr[1] == NULL) {
                listAllMemory(*Lm, 1, 0, 0);
            }else {
                mmapFree(tr[1], Lm);
            }
        }
        else{
            if(access(tr[0],F_OK)!=0){
                perror("");
            }else{
                Mmap(tr,Lm);
            }
        }
    }
}

void mallocCreate(char * tr, tListM *Lm){
    size_t tam= atol(tr);
    void *memadr=malloc(tam);
    if(insertItemMalloc(memadr,tam,Lm))printf("Allocated %zu at %p\n",tam,memadr);
}

void mallocFree(char * tr, tListM *Lm){
    size_t tam = (size_t)atol(tr);
    tPosLM p= findItemMalloc(tam,*Lm);
    if(p==NULL)perror("");

    else{printf("Dealocated %lu at %p\n",tam,p->data.address);
        dataM d= getItemM(p,*Lm);
        free(d.address);
        deleteAtPositionM(p,Lm);


    }
}

void cmd_malloc(char *tr[],tListM *Lm){
    if(tr[0]==NULL){
        listAllMemory(*Lm, 0, 0, 1);
    }else if(!strcmp(tr[0],"-free")){
        if(tr[1]==NULL){
            listAllMemory(*Lm, 0, 0, 1);
        }else {
            mallocFree(tr[1], Lm);
        }
    }else{
        mallocCreate(tr[0], Lm);
    }
}


void * ObtenerMemoriaShmget (key_t clave, size_t tam, tListM *Lm)
{ /*Obtienen un puntero a una zaona de memoria compartida*/
/*si tam >0 intenta crearla y si tam==0 asume que existe*/
    void * p;
    int aux,id,flags=0777;
    struct shmid_ds s;
    if (tam) /*si tam no es 0 la crea en modo exclusivo esta funcion vale para shared y shared -create*/
        flags=flags | IPC_CREAT | IPC_EXCL;
/*si tam es 0 intenta acceder a una ya creada*/
    if (clave==IPC_PRIVATE) /*no nos vale*/
    {errno=EINVAL; return NULL;}
    if ((id=shmget(clave, tam, flags))==-1)
        return (NULL);
    if ((p=shmat(id,NULL,0))==(void*) -1){
        aux=errno; /*si se ha creado y no se puede mapear*/
        if (tam) /*se borra */
            shmctl(id,IPC_RMID,NULL);
        errno=aux;
        return (NULL);
    }
    shmctl (id,IPC_STAT,&s);
    char tim[18];
    strftime(tim, 18, "%Y/%m/%d-%H:%M", localtime(&s.shm_ctime));
    insertItemShared(p, s.shm_segsz, tim, clave, Lm);
    return (p);
}


void SharedCreate (char *arg[], tListM *Lm) /*arg[0] is the key and arg[1] is the size*/
{
    key_t k;
    size_t tam=0;
    void *p;
    if (arg[0]==NULL || arg[1]==NULL){
        listAllMemory(*Lm, 0, 1, 0);
        return;
    }
    k=(key_t) atoi(arg[0]);
    if (arg[1]!=NULL)
        tam=(size_t) atoll(arg[1]);
    if ((p=ObtenerMemoriaShmget(k,tam, Lm))==NULL)
        perror ("Imposible obtener memoria shmget\n");
    else
        printf ("Memoria de shmget de clave %d asignada en %p\n",k,p);
}


void SharedDelkey (char *args[]) /*arg[0] points to a str containing the key*/
{
    key_t clave;
    int id;
    char *key=args[0];
    if (key==NULL || (clave=(key_t) strtoul(key,NULL,10))==IPC_PRIVATE){
        printf (" shared -delkey clave_valida\n");
        return;
    }
    if ((id=shmget(clave,0,0666))==-1){
        perror ("shmget: imposible obtener memoria compartida\n");
        return;
    }
    if (shmctl(id,IPC_RMID,NULL)==-1)
        perror ("shmctl: imposible eliminar memoria compartida\n");
}

void SharedFree(char * tr, tListM *Lm){
    if(tr == NULL){
        listAllMemory(*Lm, 0, 1, 0);
    }else{
        tPosLM p;
        p = findItemShared(atoi(tr), *Lm);
        if(p == NULL){
            return;
        }
        if(shmdt(p->data.address)==-1){
            perror("");
        }
        printf("Shared memory block at %p (key %d) has been dealocated\n", p->data.address, p->data.fileDescriptor_key);
        deleteAtPositionM(p, Lm);
    }
}


void cmd_shared(char *tr[], tListM * Lm){
    if(tr[0] == NULL){
        listAllMemory(*Lm, 0, 1, 0);
    }else if(!strcmp(tr[0], "-create")){
        SharedCreate(tr+1, Lm);
    }else if(!strcmp(tr[0], "-delkey")){
        SharedDelkey(tr+1);
    }else if(!strcmp(tr[0], "-free")){
        SharedFree(tr[1], Lm);
    }else{
        ObtenerMemoriaShmget((key_t) atoi(tr[0]), 0, Lm);
    }
}


void cmd_dealloc(char *tr[], tListM * Lm){
    char key[20] = "";
    char size[100] = "";
    tPosLM p = firstM(*Lm);
    if(tr[0]==NULL){
        listAllMemory(*Lm, 1, 1, 1);
    }else if(!strcmp(tr[0], "-malloc")){
        mallocFree(tr[1], Lm);
    }else if(!strcmp(tr[0], "-shared")){
        SharedFree(tr[1], Lm);
    }else if(!strcmp(tr[0], "-mmap")){
        mmapFree(tr[1], Lm);
    }else{
        if(tr[0] == NULL){
            listAllMemory(*Lm, 1, 1, 1);
            return;
        }
        if(isEmptyListM(*Lm)){
            return;
        }
        for(;p!= lastM(*Lm);p= nextM(p, *Lm)){
            if(p->data.address == (void *)strtoull(tr[0], NULL, 16)){
                if(!strcmp(p->data.type_of_allocation_or_name, "shared")){
                    sprintf(key, "%d", p->data.fileDescriptor_key);
                    SharedFree(key, Lm);
                }else if(!strcmp(p->data.type_of_allocation_or_name, "malloc")){
                    sprintf(size, "%zu", p->data.size);
                    mallocFree(size, Lm);
                }else{
                    mmapFree(p->data.type_of_allocation_or_name, Lm);
                }
                return;
            }
        }
        if(p->data.address == (void *)strtoull(tr[0], NULL, 16)){
            if(!strcmp(p->data.type_of_allocation_or_name, "shared")){
                sprintf(key, "%d", p->data.fileDescriptor_key);
                SharedFree(key, Lm);
            }else if(!strcmp(p->data.type_of_allocation_or_name, "malloc")){
                sprintf(size, "%zu", p->data.size);
                mallocFree(size, Lm);
            }else{
                mmapFree(p->data.type_of_allocation_or_name, Lm);
            }
            return;
        }
        listAllMemory(*Lm, 1, 1, 1);
    }
}


void dopmap (void) /*no arguments necessary*/
{ pid_t pid; /*ejecuta el comando externo pmap para */
    char elpid[32]; /*pasandole el pid del proceso actual */
    char *argv[3]={"pmap",elpid,NULL};
    sprintf (elpid,"%d", (int) getpid());
    if ((pid=fork())==-1){
        perror ("Imposible crear proceso");
        return;
    }
    if (pid==0){
        if (execvp(argv[0],argv)==-1)
            perror("cannot execute pmap");
        exit(1);
    }
    waitpid (pid,NULL,0);
}


void cmd_memoria(char * tr[], tListM Lm){
    int block=0, var=0, func=0, pmap = 0;
    static int aux1 = 0;
    static int aux2 = 1;
    static int aux3 = 2;
    for(int i = 0; tr[i]!=NULL;i++){
        if(!strcmp(tr[0], "-all")){
            block = 1;
            var = 1;
            func = 1;
            break;
        }
        if(!strcmp(tr[0], "-blocks")){
            block = 1;
        }
        if(!strcmp(tr[0], "-vars")){
            var = 1;
        }
        if(!strcmp(tr[0], "-funcs")){
            func = 1;
        }
        if(!strcmp(tr[0], "-pmap")){
            pmap = 1;
        }
    }
    if(var){
        printf("Variables locales:%18p,%18p,%18p\nVariables globales:%18p,%18p,%18p\nVariables estaticas:%18p,%18p,%18p\n", &block, &var, &func, &aux4, &aux5, &aux6, &aux1, &aux2, &aux3);
    }
    if(func){
        printf("Funciones programa:%18p,%18p,%18p\nFunciones libreria:%18p,%18p,%18p\n", &cmd_dealloc, &listAllMemory, &cmd_shared, &atoi, &shmget, &open);
    }
    if(pmap){
        dopmap();
    }
    if(block){
        listAllMemory(Lm, 1, 1, 1);
    }

}


void cmd_recursiva (char * tr[]){
    if(tr[0]==NULL){
        return;
    }else{
        int n = atoi(tr[0]);
        char automatico[1024];
        static char estatico[1024];
        printf ("parametro n:%d en %p\n",n,&n);
        printf ("array estatico en:%p \n",estatico);
        printf ("array automatico en %p\n",automatico);
        n--;
        if (n>0)
            cmd_recursiva(tr);
    }
}


void cmd_volcarmem(char * tr[]){
    int cont = 25;
    if(tr[1]!=NULL){
        cont = atoi(tr[1]);
    }
    void * i = (void *)strtoull(tr[0], NULL, 16);
    void * x = i;
    for(int a = 0;a<cont;a=a+25){
        for(int j = 0 ; j < 25 && j < cont; j++){
            if((unsigned)*(unsigned char *)i<127 && (unsigned)*(unsigned char *)i>31){
                printf("%c  ", *(unsigned char *)i);
            }else{
                printf("   ");
            }
            i = i + 1;
        }
        printf("\n");
        for(int j = 0 ; j < 25 && j < cont; j++){
            printf("%02x ", (unsigned )*(unsigned char *)x);
            x = x + 1;
        }
        printf("\n");
    }
}


void cmd_llenarmem(char * tr[]){
    int cont = 128;
    int  byte = 65;
    if(tr[1]!=NULL){
        cont = atoi(tr[1]);
    }
    if(tr[2]!=NULL && strcmp(tr[2], "")!=0){
        byte = atoi(tr[2]);
    }
    void * p = (void *)strtoull(tr[0], NULL, 16);
    memset(p, byte, cont);
    printf("Llenando %d bytes de memoria con el byte %c(%d) a partir de la direccion %p\n", cont, (char)byte, byte, p);
}


ssize_t LeerFichero (char *fich, void *p, ssize_t n)
{ /* le n bytes del fichero fich en p */
    ssize_t nleidos,tam=n; /*si n==-1 lee el fichero completo*/
    int df, aux;
    struct stat s;
    if (stat (fich,&s)==-1 || (df=open(fich,O_RDONLY))==-1)
        return ((ssize_t)-1);
    if (n==LEERCOMPLETO)
        tam=(ssize_t) s.st_size;
    if ((nleidos=read(df,p, tam))==-1){
        aux=errno;
        close(df);
        errno=aux;
        return ((ssize_t)-1);
    }
    close (df);
    return (nleidos);
}


void writeFich(char *fich, void *p, size_t size, int o){
    int df;
    if ((df=open(fich,O_CREAT|O_WRONLY|O_APPEND, 0777))==-1)
        perror("");
    if(o){
        close(df);
        remove(fich);
        if ((df=open(fich,O_CREAT|O_WRONLY, 0777))==-1)
            perror("");
    }
    if (write(df,p, size)==-1){
        perror("");
    }
    close (df);
}


void cmd_es(char * tr[]){
    int o = 0;
    if(!strcmp(tr[0], "read")){
        ssize_t i = (ssize_t)-1;
        if(tr[3]!=NULL){
            i = strtol(tr[3], NULL, 10);
        }
        void * p = (void *)strtoull(tr[2], NULL, 16);
        if(LeerFichero(tr[1], p, i) == -1){
            perror("\n");
        }
    }
    if(!strcmp(tr[0], "write")){
        if(!strcmp(tr[1], "-o")){
            o = 1;
            tr = tr + 1;
        }
        void * p = (void *)strtoull(tr[2], NULL, 16);
        size_t i = strtol(tr[3], NULL, 10);
        writeFich(tr[1], p, i, o);
    }
}


void cmd_carpeta(char *tr[]){
    char dir[MAX_LINE];
    if(tr[0] == NULL)
        printf("%s\n", getcwd(dir, MAX_LINE));
    else if(chdir(tr[0])==-1)
        perror("Cannot change directory");
}


void cmd_crear (char * tr[]){
    if(tr[0] == NULL){
        cmd_carpeta(tr);
    }else if(!strcmp(tr[0], "-f")){
        if(tr[1] == NULL){
            cmd_carpeta(tr);
            return;
        }
        if(creat(tr[1], 0755) == -1){
            fprintf(stderr, "cannot create %s: ", tr[1]);
            perror("");
        }
    }else{
        if(mkdir(tr[0], 0755) == -1){
            fprintf(stderr, "cannot create %s: ", tr[0]);
            perror("");
        }
    }
}

void borrar_file (char * tr){
    if (remove(tr) == -1){
        fprintf(stderr, "cannot delete %s: ", tr);
        perror("");
    }
}


void borrar_dir(char* tr){
    if (rmdir(tr) == -1){
        fprintf(stderr, "cannot delete %s: ", tr);
        perror("");
    }
}

void borrarDir(char* dir){
    struct stat buff;
    DIR *p;
    struct dirent *d;
    char path[1024]="";
    if((p=opendir(dir))==NULL){
        fprintf(stderr, "couldn't open %s: ", dir);
        perror("");
        return;
    }
    while((d=readdir(p))!=NULL){
        if(!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
            continue;
        strcpy(path, dir);
        strcat(path, "/");
        strcat(path, d->d_name);
        if(lstat(path, &buff) == -1){
            fprintf(stderr, "couldn't open %s: ", d->d_name);
            perror("");
            continue;
        }
        if(S_ISDIR(buff.st_mode)){
            borrarDir(path);
        }else{
            borrar_file(path);
        }
    }
    closedir(p);
    borrar_dir(dir);
}


void cmd_borrar (char * tr[]){
    if(tr[0] == NULL){
        cmd_carpeta(tr);
    }else{
        for(int i = 0; tr[i] != NULL; i++){
            struct stat buff;
            if(lstat(tr[i], &buff) == -1){
                fprintf(stderr, "cannot find %s: ", tr[i]);
                perror("");
                continue;
            }
            if(S_ISDIR(buff.st_mode)){
                borrar_dir(tr[i]);
            }else{
                borrar_file(tr[i]);
            }
        }
    }
}

void cmd_borrarrec (char * tr[]){
    struct stat buff;
    if(tr[0] == NULL){
        cmd_carpeta(tr);
    }else{
        for(int i = 0;tr[i] != NULL;i++){
            if(lstat(tr[i], &buff) == -1){
                fprintf(stderr, "couldn't open %s: ", tr[i]);
                perror("");
                continue;
            }
            if(S_ISDIR(buff.st_mode)){
                borrarDir(tr[i]);
            }else{
                borrar_file(tr[i]);
            }
        }
    }
}


char LetraTF (mode_t m){
    switch (m&S_IFMT) { /*and bit a bit con los bits de formato,0170000 */
        case S_IFSOCK: return 's'; /*socket */
        case S_IFLNK: return 'l'; /*symbolic link*/
        case S_IFREG: return '-'; /* fichero normal*/
        case S_IFBLK: return 'b'; /*block device*/
        case S_IFDIR: return 'd'; /*directorio */
        case S_IFCHR: return 'c'; /*char device*/
        case S_IFIFO: return 'p'; /*pipe*/
        default: return '?'; /*desconocido, no deberia aparecer*/
    }
}


char * ConvierteModo (mode_t m, char *permisos){
    strcpy (permisos,"---------- ");
    permisos[0]=LetraTF(m);
    if (m&S_IRUSR) permisos[1]='r'; /*propietario*/
    if (m&S_IWUSR) permisos[2]='w';
    if (m&S_IXUSR) permisos[3]='x';
    if (m&S_IRGRP) permisos[4]='r'; /*grupo*/
    if (m&S_IWGRP) permisos[5]='w';
    if (m&S_IXGRP) permisos[6]='x';
    if (m&S_IROTH) permisos[7]='r'; /*resto*/
    if (m&S_IWOTH) permisos[8]='w';
    if (m&S_IXOTH) permisos[9]='x';
    if (m&S_ISUID) permisos[3]='s'; /*setuid, setgid y stickybit*/
    if (m&S_ISGID) permisos[6]='s';
    if (m&S_ISVTX) permisos[9]='t';
    return permisos;
}

void listElement(char * tr, int lng, int lnk, int acc, char * element){
    char realPath[1024]="", permisos[11]="", time[18]="";
    struct stat buff;
    if(lstat(tr, &buff) == -1){
        fprintf(stderr, "couldn't open %s: ", tr);
        perror("");
        return;
    }
    if(lng==1){
        if(acc == 1){
            strftime(time, 18, "%Y/%m/%d-%H:%M", localtime(&buff.st_atime));
            printf("%s", time);
        }else{
            strftime(time, 18, "%Y/%m/%d-%H:%M", localtime(&buff.st_mtime));
            printf("%s", time);
        }
        struct passwd *pw = getpwuid(buff.st_uid);
        struct group  *gr = getgrgid(buff.st_gid);
        printf("   %ld   (%ld)   ",buff.st_nlink, buff.st_ino);
        if(pw != NULL){
            printf("%s   ",pw->pw_name);
        }else{
            printf("%d   ", buff.st_uid);
        }
        if(gr != NULL){
            printf("%s   ",gr->gr_name);
        }else{
            printf("%d   ", buff.st_gid);
        }
        printf("%s", ConvierteModo(buff.st_mode, permisos));
    }
    printf("%6lld    %s", (long long)buff.st_size, element);
    if(lng == 1 && lnk == 1 && S_ISLNK(buff.st_mode)){
        if(readlink(tr, realPath, sizeof(realPath))==-1){
            fprintf(stderr, "error when reading %s: ", tr);
            perror("");
        }
        printf(" -> %s  ", realPath);
    }
    printf("\n");
}


void cmd_listfich(char * tr[]){
    int lng=0, lnk=0, acc=0, i=0;
    if(tr[0] == NULL){
        cmd_carpeta(tr);
        return;
    }
    for(;i<3;i++){
        if(!strcmp(tr[i], "-link")){
            lnk=1;
            if(tr[i+1] == NULL){
                cmd_carpeta(tr);
                return;
            }
            continue;
        }
        if(!strcmp(tr[i], "-acc")){
            acc=1;
            if(tr[i+1] == NULL){
                cmd_carpeta(tr);
                return;
            }
            continue;
        }
        if(!strcmp(tr[i], "-long")){
            lng = 1;
            if(tr[i+1] == NULL){
                cmd_carpeta(tr);
                return;
            }
            continue;
        }
        break;
    }
    for(;tr[i] != NULL;i++){
        listElement(tr[i], lng, lnk, acc, tr[i]);
    }
}


void printContent(char *dir,int ocultos, int lng, int lnk, int acc){
    DIR *p;
    struct dirent *d;
    struct stat buff;
    char path[1024]="";
    if((p=opendir(dir))==NULL){
        fprintf(stderr, "couldn't open %s: ", dir);
        perror("");
        return;
    }
    printf("************%s\n", dir);
    while((d=readdir(p))!=NULL){
        if((!ocultos && d->d_name[0]=='.'))
            continue;
        strcpy(path, dir);
        strcat(path, "/");
        strcat(path, d->d_name);
        if(lstat(path, &buff) == -1){
            fprintf(stderr, "couldn't open %s: ", path);
            perror("");
            continue;
        }
        listElement(path, lng, lnk, acc, d->d_name);
    }
    closedir(p);
}


void ListarDirectorio(char *dir,int ocultos, int lng, int lnk, int acc, char rec){

    if(rec=='b'){
        recOption( dir, ocultos, lng, lnk, acc, rec);
    }

    printContent(dir, ocultos, lng, lnk, acc);

    if(rec=='a'){
        recOption( dir, ocultos, lng, lnk, acc, rec);
    }
}


void recOption(char *dir,int ocultos, int lng, int lnk, int acc, char rec){
    char path[1024]="";
    DIR *p;
    struct dirent *d;
    struct stat buff;

    if((p=opendir(dir))==NULL){
        fprintf(stderr, "couldn't open %s: ", dir);
        perror("");
        return;
    }
    while((d=readdir(p))!=NULL){
        if((!ocultos && d->d_name[0]=='.') || !strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
            continue;
        strcpy(path, dir);
        strcat(path, "/");
        strcat(path, d->d_name);
        if(lstat(path, &buff) == -1){
            fprintf(stderr, "couldn't open %s: ", path);
            perror("");
            continue;
        }
        if(S_ISDIR(buff.st_mode)){
            ListarDirectorio(path, ocultos, lng, lnk, acc, rec);
        }
    }
    if(closedir(p)==-1){
        fprintf(stderr, "couldn't close %s: ", dir);
        perror("");
    }
}


void cmd_listdir(char *tr[]){
    if(tr[0]==NULL){
        cmd_carpeta(tr);
        return;
    }
    char rec=' ';
    int ocultos=0, lng=0, lnk=0, acc=0;
    int i = 0;
    for(;i<6;i++){
        if(!strcmp(tr[i], "-long")){
            lng = 1;
            if(tr[i+1] == NULL){
                cmd_carpeta(tr);
                return;
            }
            continue;
        }
        if(!strcmp(tr[i], "-link")){
            lnk=1;
            if(tr[i+1] == NULL){
                cmd_carpeta(tr);
                return;
            }
            continue;
        }
        if(!strcmp(tr[i], "-acc")){
            acc=1;
            if(tr[i+1] == NULL){
                cmd_carpeta(tr);
                return;
            }
            continue;
        }
        if(!strcmp(tr[i], "-reca")){
            rec='a';
            if(tr[i+1] == NULL){
                cmd_carpeta(tr);
                return;
            }
            continue;
        }
        if(!strcmp(tr[i], "-recb")){
            rec='b';
            if(tr[i+1] == NULL){
                cmd_carpeta(tr);
                return;
            }
            continue;
        }
        if(!strcmp(tr[i], "-hid")){
            ocultos=1;
            if(tr[i+1] == NULL){
                cmd_carpeta(tr);
                return;
            }
            continue;
        }
        break;
    }
    for(;tr[i] != NULL;i++){
        struct stat buff;
        if(lstat(tr[i], &buff) == -1){
            fprintf(stderr, "couldn't open %s: ", tr[i]);
            perror("");
            continue;
        }
        if(!S_ISDIR(buff.st_mode)){
            listElement(tr[i], lng, lnk, acc, tr[i]);
        }else{
            ListarDirectorio(tr[i], ocultos, lng, lnk, acc, rec);
        }
    }
}

void cmd_pid(char *tr[]){
    if(tr[0]==NULL){
        printf("Shell process pid: %d\n", getpid());
    }else if(!strcmp(tr[0], "-p"))
        printf("Shell parent process pid: %d\n", getpid());
    else
        printf("%s isn't an existing option\n", tr[0]);
}


void cmd_autores(char *tr[]){
    if(tr[0]==NULL){
        printf("Tomás Villalba Ferreiro: tomas.villalba.ferreiro@udc.es\nBrais Gómez Espiñeira: brais.gomez2@udc.es\n");
    }else if(!strcmp(tr[0], "-n")){
        printf("Tomás Villalba Ferreiro\nBrais Gómez Espiñeira\n");
    }else if(!strcmp(tr[0], "-l")){
        printf("tomas.villalba.ferreiro@udc.es\nbrais.gomez2@udc.es\n");
    }else
        printf("%s isn't an existing option\n", tr[0]);

}

void cmd_fecha(char *tr[]){
    time_t t = time(NULL);
    struct tm tim = *localtime(&t);
    if(tr[0]==NULL){
        printf("%02d/%d/%02d\n%02d:%02d:%02d\n", tim.tm_mday, tim.tm_mon + 1, tim.tm_year+ 1900, tim.tm_hour, tim.tm_min, tim.tm_sec);
    }else if(!strcmp(tr[0], "-d")){
        printf("%02d/%d/%02d\n", tim.tm_mday, tim.tm_mon + 1, tim.tm_year+ 1900);
    }else if(!strcmp(tr[0], "-h")){
        printf("%02d:%02d:%02d\n", tim.tm_hour, tim.tm_min, tim.tm_sec);
    }else
        printf("%s isn't an existing option\n", tr[0]);
}


void cmd_infosis(char* tr[]){
    struct utsname machineInformation;

    if (uname(&machineInformation) < 0)
        perror("Cannot get the information succesfully\n");
    else {
        printf("Sysname:  %s\n", machineInformation.sysname);
        printf("Nodename: %s\n", machineInformation.nodename);
        printf("Release:  %s\n", machineInformation.release);
        printf("Version:  %s\n", machineInformation.version);
        printf("Machine:  %s\n", machineInformation.machine);
    }
}


void vaciarLista(tList * L){
    while(!isEmptyList(*L)){
        deleteLast(L);
    }
}

void vaciarListaM(tListM * Lm){
    while(!isEmptyListM(*Lm)){
        tPosLM p =lastM(*Lm);
        deleteAtPositionM(p,Lm);
    }
}

void cmd_hist(char* tr[], tList *L){
    if(!isEmptyList(*L)){
        if(tr[0]==NULL){
            int i=1;
            tPosL p=first(*L);
            for(;next(p, *L)!=NULL;p=next(p, *L)){
                printf("%d-> %s\n", i, getItem(p, *L));
                i++;
            }
            printf("%d-> %s\n", i, getItem(p, *L));
        }else if(!strcmp(tr[0], "-c")){
            vaciarLista(L);
            printf("Hist cleared\n");
        }else if(atoi(tr[0])*(-1)>0){
            int j= atoi(tr[0])*(-1);
            tPosL p=first(*L);
            int i=1;
            for(;i<=j;p=next(p, *L)){
                printf("%d-> %s\n", i, getItem(p,*L));
                i++;
            }
        }else{
            printf("there is no %s option for this command\n", tr[0]);
        }

    }else{
        printf("The history is empty\n");
    }
}


void cmd_fin(char* tr[]){
    exit(0);
}

struct CMD C[]={
        {"fin",cmd_fin, "Ends the shell\n"},
        {"bye",cmd_fin, "Ends the shell\n"},
        {"salir",cmd_fin, "Ends the shell\n"},
        {"carpeta",cmd_carpeta, "Prints the current working directory\n\t[direct] changes the current working directory of the shell to direct\n"},
        {"autores",cmd_autores, "Prints the name and login of the program authors\n\t[-l] pritns only the logins\n\t[-n] prints only the names\n"},
        {"pid",cmd_pid, "Prints the pid of the process executing the shell\n\t[-p] prints the pid of the shell's parent process\n"},
        {"fecha",cmd_fecha, "Prints the current date and time\n\t[-d] prints only the date\n\t[-h] prints only the time\n"},
        {"infosis",cmd_infosis, "Prints information of the machine\n"},
        {"ayuda",cmd_ayuda, "List of avaliable commands\n\t[comand] gives help about the ussage of the given command\n"},
        {"crear", cmd_crear, "Prints the current working directory\n\t[name]Creates a directory named name in the file system\n[-f][name] creates an empty file named name\n"},
        {"borrar", cmd_borrar, "Prints the current working directory\n\t[name1 name2...]Deletes the files and empty directories name name1, name2...\n"},
        {"borrarrec", cmd_borrarrec, "Prints the current working directory\n\t[name1 name2...]Deletes the files and directories name name1, name2...\n"},
        {"listfich", cmd_listfich, "Prints the current working directory\n\t[name1, name2...]Prints the size and name of the files name1, name2...\n\t[-long][name1, name2...]Prints more complex information about the files\n\t[-long][-link][name1, name2...]if the file is a symbolic link the name of the file it points to is also printed\n\t[-long][-acc][name1, name2...]Instead of the date of last access instead of last modification time\n"},
        {"listdir", cmd_listdir, "Prints the current working directory\n\t[name1, name2...]Lists the contents of directories with names name1, name2... If any of them is not a directory, info on it will be printed exactly as with command listfich\n\t[-long] [-link] [-acc] Have exactly the same meaning as in listfich, but affect all files inside directories\n\t[-hid][name1, name2...] hidden files and/or directories will also get listed\n\t[-reca] when listing a directory’s contents, subdirectories will be listed recursively after all the files in the directory\n\t[-recb] the same as -reca but files are printed first\n"},
        {"volcarmem", cmd_volcarmem, "volcarmem addr [cont]\n\tShows the contents of cont bytes starting at memory address addr. If cont is not specified, it shows 25 bytes. For each byte it prints its hex value and its associated char\n"},
        {"llenarmem", cmd_llenarmem, "llenarmem addr [cont] [byte]\n\tFills cont bytes of memory starting at address addr with the value ’byte’. If ’byte’ is not specified, the value of 65 is assumed, and if cont is not specified, we’ll use a default value of 128. \n"},
        {"recursiva", cmd_recursiva, "recursiva n\n\tCalls a recursive function passing the integer n n as its parameter.\n"},
        {"e-s", cmd_es, "e-s read fich addr cont\n\tReads (using ONE read system call) cont bytes from file fich into memory address addr. If cont is not specified ALL of fich is read onto memory address addr.\ne-s write [-o] fich addr cont\n\tWrites cont bytes from memory address addr into file fich. If file fich does not exist it gets created; if it already exists it is not overwritten unless “-o” is specified\n"},
        {"priority",cmd_priority, "priority [pid] [valor] 	Muestra o cambia la prioridad del proceso pid a valor\n"},
        {"rederr",cmd_rederr, "rederr [-reset] fich 	Redirects the standard error to fich\n"},
        {"fork",cmd_fork, "fork  calls fork to create a process\n"},
        {"uid",cmd_uid, "uid [-get|-set] [-l] [id] Shows or change (if possible) the credential of the process that executes the shell\n"},
        {"ejec",cmd_ejec, "ejec prog args....  executes, without creating a process, prog with arguments\n"},
        {"ejecpri",cmd_ejecpri, "ejecpri prio prog args....  executes, without creating a process, prog with arguments with the priority changed to prio\n"},
        {"fg",cmd_fg, "fg prog args...  creates a process tha executes in foreground with arguments\n"},
        {"fgpri",cmd_fgpri, "fg prog args...  creates a process tha executes in foreground with arguments with the priority changed to prio\n"},
        {"ejecas",cmd_ejecas, "ejecas user prog args..  executes without creating a process and with the user user, prog with arguments\n"},
        {"fgas",cmd_fgas, "fgas us prog args...  creates a process that executes in foreground, and as user us, prog with arguments\n"},
        {NULL,NULL},
};


void cmd_ayuda(char* tr[]){
    if(tr[0]==NULL){
        printf("hist\n");
        printf("comando\n");
        for(int i=0;C[i].name!=NULL;i++){
            printf("%s\n", C[i].name);
        }
        return;
    }else
        for(int i=0;C[i].name!=NULL;i++){
            if(!strcmp(tr[0], C[i].name)){
                printf("%s", C[i].help);
                return;
            }else if(!strcmp(tr[0], "hist")){
                printf("Shows the historic of commands executed by this shell\n\t[-c] empties the historic of commands\n\t[-N] prints the first N commands");
                return;
            }else if(!strcmp(tr[0], "comando")){
                printf("[N] Repits command number N\n");
                return;
            }else if(!strcmp(tr[0], "malloc")){
                printf("malloc [-free] [tam]\n\t The shell allocates tam bytes using malloc and shows the memory address returned by malloc\n");
                return;
            }else if(!strcmp(tr[0], "dealloc")){
                printf("dealloc [-malloc|-shared|-mmap]\n\tdeallocates one of the memory blocks allocated with the command malloc, mmap or shared and removes it from the list. If no arguments are given, it prints a list of the allocated memory blocks\n");
                return;
            }else if(!strcmp(tr[0], "shared")){
                printf("shared [-free|-create|-delkey ] cl [tam]\n\tGets shared memory of key cl, maps it in the proccess address space and shows the memory address where the shared memory has been mapped\n");
                return;
            }else if(!strcmp(tr[0], "mmap")){
                printf("mmap [-free] fich [perm]\n\tMaps in memory the file fich and shows the memory address where the file has been mapped. perm represents the mapping permissions If fich is not specified, the command will show the list of addresses allocated with the mmap command\n");
                return;
            }else if(!strcmp(tr[0], "mamoria")){
                printf("memoria [-blocks] [-vars] [-funcs] [-all] [-pmap]\n\tShows addresses inside the process memory space. If no arguments are given, is equivalent to -all\n");
                return;
            }else if(!strcmp(tr[0], "entorno")){
                printf("entorno [-environ|-addr]  shows the entorno of the process\n");
                return;
            }else if(!strcmp(tr[0], "mostrarvar")){
                printf("mostrarvar  shows the value and the directions of an entorno variable\n");
                return;
            }else if(!strcmp(tr[0], "cambiarvar")){
                printf("cambiarvar [-a|-e|-p] var valor  changes the value of an entorno variable\n");
                return;
            }else if(!strcmp(tr[0], "listjobs")){
                printf("listjobs  lists the processes in background\n");
                return;
            }else if(!strcmp(tr[0], "back")){
                printf("back prog args...  cretaes a process that executes in background prog with arguments\n");
                return;
            }else if(!strcmp(tr[0], "backpri")){
                printf("backpri prio prog args...  cretaes a process that executes in background prog with arguments and priority changed to prio\n");
                return;
            }else if(!strcmp(tr[0], "job")){
                printf("job [-fg] pid  shows the information of the process pid. -fg takes it to foreground\n");
                return;
            }else if(!strcmp(tr[0], "borrarjobs")){
                printf("borrarjobs [-term][-sig]  deletes the processes terminated normally or by signal inthe process list\n");
                return;
            }else if(!strcmp(tr[0], "bgas")){
                printf("bgas us prog args... creates a process that executes in background, and as user us, prog with arguments\n");
                return;
            }
        }
    printf("%s Command not found\n", tr[0]);
}


int TrocearCadena(char * cadena, char * trozos[]){
    int i=1;
    if ((trozos[0]=strtok(cadena," \n\t"))==NULL)
        return 0;
    while ((trozos[i]=strtok(NULL," \n\t"))!=NULL)
        i++;
    return i;
}

void ProcesarEntrada(char* tr[], tList *L, tListM * Lm, tListP * Lp, char * env[]){
    int i;
    if (tr[0]==NULL){
        return;
    }else if(!strcmp(tr[0], "hist")){
        cmd_hist(tr+1, L);
        return;
    }else if(!strcmp(tr[0], "comando")){
        cmd_comandoN(tr+1, *L, *Lm, *Lp, env);
        return;
    }else if(!strcmp(tr[0], "shared")){
        if(!insertItem(tr, L)){
            perror("There isn't enough space to save the command in the history");
        }
        cmd_shared(tr+1, Lm);
        return;
    }else if(!strcmp(tr[0], "dealloc")){
        if(!insertItem(tr, L)){
            perror("There isn't enough space to save the command in the history");
        }
        cmd_dealloc(tr+1, Lm);
        return;
    }else if(!strcmp(tr[0], "memoria")){
        if(!insertItem(tr, L)){
            perror("There isn't enough space to save the command in the history");
        }
        cmd_memoria(tr+1, *Lm);
        return;
    }else if(!strcmp(tr[0], "malloc")){
        if(!insertItem(tr, L)){
            perror("There isn't enough space to save the command in the history");
        }
        cmd_malloc(tr+1, Lm);
        return;
    }else if(!strcmp(tr[0], "mmap")){
        if(!insertItem(tr, L)){
            perror("There isn't enough space to save the command in the history");
        }
        cmd_mmap(tr+1, Lm);
        return;
    }else if(!strcmp(tr[0], "entorno")){
        if(!insertItem(tr, L)){
            perror("There isn't enough space to save the command in the history");
        }
        cmd_entorno(tr+1, env);
        return;
    }else if(!strcmp(tr[0], "mostrarvar")){
        if(!insertItem(tr, L)){
            perror("There isn't enough space to save the command in the history");
        }
        cmd_mostrarvar(tr+1, env);
        return;
    }else if(!strcmp(tr[0], "cambiarvar")){
        if(!insertItem(tr, L)){
            perror("There isn't enough space to save the command in the history");
        }
        cmd_cambiarvar(tr+1, env);
        return;
    }else if(!strcmp(tr[0], "listjobs")){
        if(!insertItem(tr, L)){
            perror("There isn't enough space to save the command in the history");
        }
        cmd_listjobs(tr+1, Lp);
        return;
    }else if(!strcmp(tr[0], "back")){
        if(!insertItem(tr, L)){
            perror("There isn't enough space to save the command in the history");
        }
        cmd_back(tr+1, Lp);
        return;
    }else if(!strcmp(tr[0], "backpri")){
        if(!insertItem(tr, L)){
            perror("There isn't enough space to save the command in the history");
        }
        cmd_backpri(tr+1, Lp);
        return;
    }else if(!strcmp(tr[0], "job")){
        if(!insertItem(tr, L)){
            perror("There isn't enough space to save the command in the history");
        }
        cmd_job(tr+1, Lp);
        return;
    }else if(!strcmp(tr[0], "borrarjobs")){
        if(!insertItem(tr, L)){
            perror("There isn't enough space to save the command in the history");
        }
        cmd_borrarjobs(tr+1, Lp);
        return;
    }else if(!strcmp(tr[0], "bgas")){
        if(!insertItem(tr, L)){
            perror("There isn't enough space to save the command in the history");
        }
        cmd_bgas(tr+1, Lp);
        return;
    }else{
        for(i=0;C[i].name!=NULL;i++){
            if(!strcmp(tr[0], C[i].name)){
                if(!insertItem(tr, L)){
                    perror("There isn't enough space to save the command in the history");
                }
                if(!strcmp(tr[0], "fin") || !strcmp(tr[0], "bye") || !strcmp(tr[0], "salir")){
                    vaciarLista(L);
                    vaciar_listaP(Lp);
                    vaciarListaM(Lm);
                }
                (*C[i].func)(tr+1);
                return;
            }
        }
        executeNonPredefinedCommand(tr, Lp);
        return;
    }
}


void cmd_comandoN(char* tr[], tList L, tListM Lm, tListP Lp, char * env[]){
    if(isEmptyList(L)){
        printf("The hist is empty\n");
        return;
    }
    if(tr[0] == NULL){
        return;
    }
    int a=1;
    char c[MAX_LINE];
    tPosL t=first(L);
    for(tPosL p = first(L);next(p, L)!=NULL;p=next(p, L)){
        a++;
    }
    if(atoi(tr[0]) > 0 && atoi(tr[0]) <= a){
        for(int i = 0;i < atoi(tr[0])-1;i++){
            t=next(t, L);
        }
        strcpy(c, getItem(t, L));
        TrocearCadena(c, tr);
        ProcesarEntrada(tr, &L, &Lm, &Lp, env);
    }else{
        printf("There is no command in that position in the hist\n");
    }
}


int main(int argc, char *argv[], char * env[]){
    char line[MAX_LINE];
    char* tr[MAX_LINE/2];
    tList L;
    createEmptyList(&L);
    tListM Lm;
    createEmptyListM(&Lm);
    tListP Lp;
    createEmptyListP(&Lp);
    while(1){
        printf("*)");
        fgets(line, MAX_LINE, stdin);
        TrocearCadena(line, tr);
        ProcesarEntrada(tr, &L, &Lm, &Lp, env);
    }
}
