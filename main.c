#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

int *place, iterA = 0, iterB = 0, howA = 0, howB = 0, *queueA, *queueB, N=10;
int debug=0, condition = 0;
int empty=0;
void *guardian();
void *bridge(void *number);
void *bridge_cond(void *number);
void go_car(int car);
void add_queue(int car);
void show(int car, int side);


pthread_mutex_t blockQueues = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t blockBridge = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t trytoride = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t printer = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t ask = PTHREAD_COND_INITIALIZER;


int main(int argc, char *argv[]) {
    if(argc == 4){
        N = atoi(argv[1]);
        if(strcmp(argv[2],"-debug")==0 || strcmp(argv[3],"-debug")==0){
            debug=1;
        }
        if(strcmp(argv[2],"-condition")==0 || strcmp(argv[3],"-condition")==0){
            condition=1;
        }
    }
    if (argc==3){
        N = atoi(argv[1]);
        if(strcmp(argv[2],"-debug")==0){
            debug=1;
        }else if(strcmp(argv[2],"-condition")==0){
            condition=1;
        }

    }
    if (argc==2){
        N = atoi(argv[1]);
    }
    if(argc>4 || argc<2){
        printf("błędne parametry\n");
        exit(0);
    }

    queueA= malloc((N)* sizeof(int));
    queueB= malloc((N)* sizeof(int));
    place= malloc((N)* sizeof(int));

    for(int i =0; i<N; i++){
        queueB[i]=-1;
        queueA[i]=-1;
        place[i]=0;
    }

    srand(time(NULL));
    for(int i=0; i<N;i++) {
        int temp = rand() % 2;
        if (temp) {
            iterA++;
            place[i] = temp;
        }else {
            iterB++;
        }
    }

    int iret[N];
    pthread_t th[N];
    if(condition){
        int guard;
        pthread_t th_guard;
        guard = pthread_create(&th_guard, NULL, guardian, NULL);
        if (guard) {
            fprintf(stderr, "Error - pthread_create() return code: %d\n", guard);
            exit(EXIT_FAILURE);
        }
        for (long i = 0; i < N; i++) {
            iret[i] = pthread_create(&th[i], NULL, bridge_cond, (void *) i);
            if (iret[i]) {
                fprintf(stderr, "Error - pthread_create() return code: %d\n", iret[i]);
                exit(EXIT_FAILURE);
            }
        }
    }else {
        for (long i = 0; i < N; i++) {
            iret[i] = pthread_create(&th[i], NULL, bridge, (void *) i);
            if (iret[i]) {
                fprintf(stderr, "Error - pthread_create() return code: %d\n", iret[i]);
                exit(EXIT_FAILURE);
            }
        }
    }
    for (long i = 0; i < N; i++) {
        pthread_join(th[i], NULL);
    }

    free(queueA);
    free(queueB);
    free(place);
    return 0;
}
void *guardian (){
    while(1){
        pthread_mutex_lock(&blockBridge);
        if(empty!=0){
            pthread_cond_wait(&cond,&blockBridge);
        }
        pthread_cond_broadcast(&ask);
        pthread_mutex_unlock(&blockBridge);
    }
}

void *bridge( void *number ) {
    int car = (int) number;

    while (1) {
        if(place[car] == 1 || place[car] == 0){
            pthread_mutex_lock(&blockQueues);
            add_queue(car);
            if(debug == 1)
                show(-1, 2);
            pthread_mutex_unlock(&blockQueues);
        }else {
            if ((!empty && queueA[0] == car) || (!empty && queueB[0] == car)) {
                pthread_mutex_lock(&blockBridge);
                empty = 1;
                go_car(car);
                pthread_mutex_unlock(&blockBridge);
                empty = 0;
            }
        }
        sleep(1);
    }

}

void *bridge_cond(void *number){
    int car = (int) number;

    while (1) {
        if(place[car] == 1 || place[car] == 0){
            pthread_mutex_lock(&blockQueues);
            add_queue(car);
            if(debug == 1)
                show(-1, 2);
            pthread_mutex_unlock(&blockQueues);
        }else {
            if ((!empty && queueA[0] == car) || (!empty && queueB[0] == car)) {
                pthread_mutex_lock(&blockBridge);
                pthread_cond_wait(&ask,&blockBridge);
                empty = 1;
                go_car(car);
                pthread_cond_signal(&cond);
                pthread_mutex_unlock(&blockBridge);
                empty = 0;
            }
        }
        sleep(1);
    }
}

void go_car(int car){
    if(place[car] == 3) {
        show(car, 0);
        for (int i = 1; i < howA; i++) {
            queueA[i - 1] = queueA[i];
        }
        queueA[howA - 1] = -1;
        howA--;
        place[car] = 0;
        iterB++;
    }else if(place[car] == 2){
        show(car, 1);
        for (int i = 1; i < howB; i++) {
            queueB[i - 1] = queueB[i];
        }
        queueB[howB - 1] = -1;
        howB--;
        place[car] = 1;
        iterA++;
    }
}

void add_queue(int car){
    if(place[car]) {
        place[car] = 3;
        iterA--;
        queueA[howA] = car;
        howA++;
    }else {
        place[car] = 2;
        iterB--;
        queueB[howB] = car;
        howB++;
    }
}

void show(int car, int side) {
    pthread_mutex_lock(&printer);
    if (debug == 0 && side == 0)
        printf("A:%d %d>>%d>>%d %d:B\n", iterA, howA, car, howB, iterB);
    else if(debug == 0 && side == 1){
        printf("A:%d %d<<%d<<%d %d:B\n", iterA, howA, car, howB, iterB);
    }else{
        printf("A: ");
        for (int i = 0; i < N; i++) {
            if (place[i] == 1)
                printf("%d|", i);
        }
        printf("   ");
        for (int i = howA-1; i >= 0; i--) {
            printf("%d|", queueA[i]);
        }
        if(side == 0){
            printf(">>%d>>", car);
        }else if(side == 1){
            printf("<<%d<<", car);
        }else if(side == 2 && car == -1){
            printf(">>  <<");
        }
        for (int i = 0; i < howB; i++) {
            printf("%d|", queueB[i]);
        }
        printf("   ");
        for (int i = 0; i < N; i++) {
            if (place[i] == 0)
                printf("%d|", i);
        }
        printf(" :B\n");
    }
    pthread_mutex_unlock(&printer);
}