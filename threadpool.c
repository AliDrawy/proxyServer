#include <stdio.h>
#include "threadpool.h"
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <netdb.h>





threadpool* create_threadpool(int num_threads_in_pool){
    threadpool * thread_pool= (threadpool*) malloc(sizeof(threadpool));
    if(thread_pool==NULL){
        perror("malloc failed\n");
        exit(1);
    }
    thread_pool->dont_accept=0;
    thread_pool->num_threads=num_threads_in_pool;
    thread_pool->qhead=NULL;
    thread_pool->qtail=NULL;
    thread_pool->qsize=0;
    thread_pool->shutdown=0;
    thread_pool->threads=(pthread_t*)malloc(sizeof(pthread_t)*num_threads_in_pool);
    if(thread_pool->threads==NULL){
        perror("malloc failed\n");
        exit(1);
    }
    pthread_mutex_init(&thread_pool->qlock, NULL);
    pthread_cond_init(&thread_pool->q_empty, NULL);
    pthread_cond_init(&thread_pool->q_not_empty, NULL);
    for (int i = 0; i <num_threads_in_pool ; ++i) {
        int rc;
        rc=pthread_create(&thread_pool->threads[i],NULL, do_work,thread_pool);
        if(rc){
            printf("ERROR\n");
            exit(-1);
        }
    }


    return thread_pool ;
}
void dispatch(threadpool* from_me, dispatch_fn dispatch_to_here, void *arg){
    pthread_mutex_lock(&from_me->qlock);
    if(from_me->dont_accept==1){
        return ;
    }
    work_t * new_work=(work_t*) malloc(sizeof(work_t));
    new_work->routine=dispatch_to_here;
    new_work->arg=arg;

    if(from_me->qsize==0){
        from_me->qhead=new_work;
        from_me->qtail=new_work;
        from_me->qtail->next=NULL;
    } else{
        from_me->qtail->next=new_work;
        from_me->qtail=new_work;
    }
    from_me->qsize++;
    pthread_cond_signal(&from_me->q_not_empty);
    pthread_mutex_unlock(&from_me->qlock);

}

void* do_work(void* p){
    threadpool * thread_pool=(threadpool*)p;
    work_t * temp;

    while (1){

        pthread_mutex_lock(&thread_pool->qlock);
        if(thread_pool->shutdown==1){
            pthread_mutex_unlock(&thread_pool->qlock);
            return NULL;
        }
       if(thread_pool->shutdown==0) {

           if (thread_pool->qsize==0) {
//               pthread_mutex_unlock(&(thread_pool->qlock));
               pthread_cond_wait(&thread_pool->q_not_empty, &thread_pool->qlock);
           }
           if(thread_pool->shutdown==1){
               pthread_mutex_unlock(&thread_pool->qlock);
               return NULL;
           }
           if(thread_pool->qsize!=0){

               thread_pool->qsize--;
               temp=thread_pool->qhead;
               thread_pool->qhead=thread_pool->qhead->next;

               if(thread_pool->qsize==0 && thread_pool->dont_accept==1){
                 //  pthread_mutex_unlock(&thread_pool->qlock);
                   pthread_cond_signal(&thread_pool->q_empty);
               }

               pthread_mutex_unlock(&(thread_pool->qlock));

               if(temp!=NULL) {
                   temp->routine(temp->arg);
                   free(temp);
               }
           }

       }


    }

}

void destroy_threadpool(threadpool* destroyme){
    pthread_mutex_lock(&destroyme->qlock);
    destroyme->dont_accept=1;
    if (destroyme->qsize>0){
        pthread_cond_wait(&destroyme->q_empty,&destroyme->qlock);
    }
    void * to_return;

    destroyme->shutdown=1;
    pthread_mutex_unlock(&destroyme->qlock);
    pthread_cond_broadcast(&(destroyme->q_not_empty));
    for (int i = 0; i < destroyme->num_threads; ++i) {
        pthread_join(destroyme->threads[i],&to_return);
    }

    free(destroyme->threads);
    pthread_mutex_destroy(&(destroyme->qlock));
    pthread_cond_destroy(&(destroyme->q_not_empty));
    pthread_cond_destroy(&(destroyme->q_empty));
    free(destroyme);
}
int f(void * num){
    int k = *((int* ) num);
    for (int i = 0; i < 3; ++i) {
        printf("%d \n",k);
        sleep(1);
    }
    return 0;
}
//int main() {
//threadpool * test;
//  test=  create_threadpool(3);
//  int val[5];
//    for (int i = 0; i < 5; ++i) {
//        val[i]=i;
//        dispatch(test,f,&(val[i]));
//    }
//    destroy_threadpool(test);
//
//    return 0;
//}