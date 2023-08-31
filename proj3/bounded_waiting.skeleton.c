 #include <stdio.h>
 #include <stdbool.h>
 #include <unistd.h>
 #include <pthread.h>
 #include <stdatomic.h>

 #define N 8             /* 스레드 개수 */
 #define RUNTIME 100000  /* 출력량을 제한하기 위한 실행시간 (마이크로초) */

 /*
  * ANSI 컬러 코드: 출력을 쉽게 구분하기 위해서 사용한다.
  * 순서대로 BLK, RED, GRN, YEL, BLU, MAG, CYN, WHT, RESET을 의미한다.
  */
 char *color[N+1] = {"\e[0;30m","\e[0;31m","\e[0;32m","\e[0;33m","\e[0;34m","\e[0;35m","\e[0;36m","\e[0;37m","\e[0m"};

 /*
  * waiting[i]는 스레드 i가 임계구역에 들어가기 위해 기다리고 있음을 나타낸다.
  * alive 값이 false가 될 때까지 스레드 내의 루프가 무한히 반복된다.
  */
 bool waiting[N];
 bool alive = true;

 atomic_flag lock = ATOMIC_FLAG_INIT;

 /*
  * N 개의 스레드가 임계구역에 배타적으로 들어가기 위해 스핀락을 사용하여 동기화한다.
  */
 void *worker(void *arg)
 {
     int i = *(int *)arg;
     
     while (alive) {
         while (atomic_flag_test_and_set(&lock)) {
             // 스핀락을 획득할 때까지 기다린다.
         }
         
         /*
          * 임계구역: 알파벳 문자를 한 줄에 40개씩 10줄 출력한다.
          */
         for (int k = 0; k < 400; ++k) {
             printf("%s%c%s", color[i], 'A'+i, color[N]);
             if ((k+1) % 40 == 0)
                 printf("\n");
         }
         
         atomic_flag_clear(&lock);
         
         /*
          * 임계구역이 성공적으로 종료되었다.
          */
     }
     pthread_exit(NULL);
 }

 int main(void)
 {
     pthread_t tid[N];
     int i, id[N];

     /*
      * N 개의 자식 스레드를 생성한다.
      */
     for (i = 0; i < N; ++i) {
         id[i] = i;
         pthread_create(tid+i, NULL, worker, id+i);
     }
     /*
      * 스레드가 출력하는 동안 RUNTIME 마이크로초 쉰다.
      * 이 시간으로 스레드의 출력량을 조절한다.
      */
     usleep(RUNTIME);
     /*
      * 스레드가 자연스럽게 무한 루프를 빠져나올 수 있게 한다.
      */
     alive = false;
    /*
     * 자식 스레드가 종료될 때까지 기다린다.
     */
    for (i = 0; i < N; ++i)
        pthread_join(tid[i], NULL);
    /*
     * 메인함수를 종료한다.
     */
    return 0;
}
