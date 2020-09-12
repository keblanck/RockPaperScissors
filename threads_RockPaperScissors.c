/*
 * Kathleen Blanck
 * 10/20/17
 * trps.c - a rock, paper, scissors game that is mediated and played by a computer
 */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/file.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <pthread.h>


int go, throw, thrown, turn;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t t = PTHREAD_COND_INITIALIZER;
pthread_cond_t tt = PTHREAD_COND_INITIALIZER;
pthread_cond_t thing1 = PTHREAD_COND_INITIALIZER;


/*
 * play1() this function handles the first player's throws
 */
void *play1(void *arg) {
  struct timeval time;
  gettimeofday(&time, NULL);
  srand(time.tv_sec);
  
  //this makes sure the players wait for the ref's first signal to throw
  pthread_mutex_lock(&m);
  while (go == 0) {
    pthread_cond_wait(&t, &m);;
  }
  pthread_mutex_unlock(&m);

  while (go == 1) {
    pthread_mutex_lock(&m);
    while (turn == 2) {
      pthread_cond_wait(&t, &m);
    }
    throw = rand();
    throw %= 3;
    thrown = 1; //this is a shared variable that will let the ref know the first player went
    turn = 2; //this is a shared variable that will keep the first player from going again
    pthread_cond_signal(&thing1);
    pthread_mutex_unlock(&m);
  }
  //idk it just said it needed to return something
  int *garbage;
  pthread_exit(garbage);
}

/*
 * play2() this function does the same as player 1 but with the opposite of each variables
 */
void *play2(void *arg) {
  struct timeval time;
  gettimeofday(&time, NULL);
  srand(time.tv_usec);
  
  //keeps the player from going before it's time
  pthread_mutex_lock(&m);
  while (go == 0) {
    pthread_cond_wait(&tt, &m);;
  }
  pthread_mutex_unlock(&m);

  while (go == 1) {
    pthread_mutex_lock(&m);
    while (turn == 1) {
      pthread_cond_wait(&tt, &m);
    }
    throw = rand();
    throw %= 3;
    thrown = 0; // variable to let the ref know player 2 has thrown
    turn = 1; // variable to keep player 2 from throwing again
    pthread_cond_signal(&thing1);
    pthread_mutex_unlock(&m);
  }
  //yeah garbarge
  int *garbage;
  pthread_exit(garbage);
}



int main(int argc, char **argv) {
  pthread_t p, pp, r;
  int err, numR, ind;
  pthread_attr_t attr;
  int winMat[9] = {0,2,1,1,0,2,2,1,0}; // 0 = tie, 1 = player 1 wins, 2 = player 2 wins
  int ties = 0, w1 = 0, w2 = 0, rock = 0, paper = 0, scissors = 0; 
  const char * types[3] = {"Rock", "Paper", "Scissors"}; //strings match with winMat indices

  if (argc != 2) {
    printf("Must indicate a number of rounds to play\n");
    return 0;
  }

  numR = atoi(argv[1]); 
  thrown = 0;
  turn = 1;
  go = 0;

  err = pthread_create(&p, NULL, play1, NULL); 
  if (err != 0) {
    perror("pthread_create");
  }
  err = pthread_create(&pp, NULL, play2, NULL);
  if (err != 0) {
    perror("pthread_create");
  }

  printf("Beginning %d Rounds...\nFight!\n", numR);
  printf("Child 1 PID: %lu\n", p);
  printf("Child 2 PID: %lu\n", pp);
  fflush(stdout);
  
  go = 1;

  for (int i = 0; i < numR; i++) {
    printf("----------------\n");
    printf("Round %d:\n", i + 1);
   
    pthread_mutex_lock(&m);
    pthread_cond_signal(&t);
    while (thrown == 0) {
      pthread_cond_wait(&thing1, &m);
    }
    pthread_mutex_unlock(&m);

    ind = throw * 3;
    printf("Child 1 throws %s!\n", types[throw]);
    fflush(stdout);
    
    // can be uncommented to check that each option is being thrown equally
    /*
    if (throw == 0) {
      rock++;
    } else if (throw == 1) {
      paper++;
    } else {
      scissors++;
    }
    */

    pthread_mutex_lock(&m);
    pthread_cond_signal(&tt);
    while (thrown == 1) {
      pthread_cond_wait(&thing1, &m);
    }
    pthread_mutex_unlock(&m);

    ind += throw;
    printf("Child 2 throws %s!\n", types[throw]);
    fflush(stdout);
    
    /*
    if (throw == 0) {
      rock++;
    } else if (throw == 1) {
      paper++;
    } else {
      scissors++;
    }
    */

    if (winMat[ind] == 0) { 
      printf("Game is a tie!\n");
      ties++;
    } else if (winMat[ind] == 1) {
      printf("Child 1 Wins!\n");
      w1++;
    } else {
      printf("Child 2 Wins!\n");
      w2++;
    }
  } 
  // reports the number of times each option was thrown
     /*printf("\n\nRocks   : %d\n", rock);
     printf("Paper   : %d\n", paper);
     printf("Scissors: %d\n", scissors);
     */
  go = 0;
  pthread_cond_signal(&t);
  pthread_cond_signal(&tt);
  pthread_cond_signal(&thing1);
  //pthread_cond_signal(&thing2);
 
  printf("----------------\n");
  printf("----------------\n");
  printf("Results:\n");
  printf("Child 1: %d\n", w1);
  printf("Child 2: %d\n", w2);
  printf("Ties   : %d\n", ties);
  if (w1 > w2) {
    printf("Child 1 Wins!\n");
  } else if (w2 > w1) {
    printf("Child 2 Wins!\n");
  } else {
    printf("It's a tie!\n");
  }
 
 
 err = pthread_join(p, NULL);
  if (err != 0) {
    perror("pthread_join 1");
    exit(12);
  }
  pthread_join(pp, NULL);
  if (err != 0) {
    perror("pthread_join 2");
    exit(11);
  }

  return 0;
}



