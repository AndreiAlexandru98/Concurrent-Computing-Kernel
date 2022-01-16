#include "philosopher.h"

#define NP 16       //Number of philosophers
int fildes[NP][2];


void main_philosopher() {
  printString("\n", 1);
  for(int i = 0; i < NP; i++){
    createPipe(fildes[i]);
    switch(fork()){
      case -1:{
        break;
      }
      case 0:{
        philosopher(fildes[i]);
      }
      default:{
        //CASE: No more philosophers
        if(i == NP - 1) waiter();
        break;
      }
    }
  }
}

void think() {
  for(int i = 0; i < 1000; i++);
}

void pickUpLeftFork(int channel[2]){
  completeWritePipe(channel[1], REQUESTLEFTFORK);
  printString("Philosopher ", 12);
  printNumber((channel[0] / 2));  // Print the philospher's number
  printString(" is waiting for the left fork.", 30);
  printString("\n", 1);
  //Check if fork is available
  if (completeReadPipe(channel[0]) == AVAILABLEFORK) {
    printString("Philosopher ", 12);
    printNumber((channel[0] / 2)); // Print the philospher's number
    printString(" has picked up the left fork.", 29);
    printString("\n", 1);
  }
}

void pickUpRightFork(int channel[2]){
  completeWritePipe(channel[1], REQUESTRIGHTFORK);
  printString("Philosopher ", 12);
  printNumber((channel[0] / 2));  // Print the philospher's number
  printString(" is waiting for the right fork.", 31);
  printString("\n", 1);
  //Check if fork is available
  if (completeReadPipe(channel[0]) == AVAILABLEFORK) {
    printString("Philosopher ", 12);
    printNumber((channel[0] / 2)); // Print the philospher's number
    printString(" has picked up the right fork.", 30);
    printString("\n", 1);
  }
}

void eat(int channel[2]){
  printString("Philosopher ", 12);
  printNumber((channel[0] / 2)); // Print philosopher's number
  printString(" has finished eating.", 21);
  printString("\n", 1);
  completeWritePipe(channel[1], EAT);
}

void waiter(){
  int reservedForks[NP];
  // Set all forks down
  for(int i = 0; i < NP; i++){
    reservedForks[i] = DOWN;
  }
  while(1){

    for(int i = 0; i < NP; i++){
      int request = readPipe(fildes[i][1], 0);
      //RULE: Only philosophers with an even number can pick up a left fork at first
      if(request == REQUESTLEFTFORK){
        if(reservedForks[i] == DOWN && reservedForks[(i+1) % NP] == DOWN){
          request = readPipe(fildes[i][1], 1);  // Erase pipe
          completeWritePipe(fildes[i][0], AVAILABLEFORK);
          printString("The waiter gave the left fork to philosopher ", 45);
          printNumber(i);  // Print philosopher's number
          printString("\n",1);
          reservedForks[i] = i;
          reservedForks[(i + 1 + NP) % NP] = i;
        }
      }

      if(request == REQUESTRIGHTFORK){
        if(reservedForks[(i+1) % NP] == i){
          request = readPipe(fildes[i][1], 1);  // Erase pipe
          printString("The waiter gave the right fork to philosopher ", 46);
          printNumber(i);  // Print philosopher's number
          printString("\n", 1);
          completeWritePipe(fildes[i][0], AVAILABLEFORK);
        }
      }

      if(request == EAT){
        request = readPipe(fildes[i][1], 1); // Erase pipe
        if(reservedForks[i] == i)
            reservedForks[i] = DOWN;
        if(reservedForks[(i + 1 + NP) % NP] == i)
          reservedForks[(i + 1 + NP) % NP] = DOWN;
      }


    }
  }
  exit(EXIT_SUCCESS);
}

void philosopher(int channel[2]){
  while(1){
    think();
    pickUpLeftFork(channel);
    pickUpRightFork(channel);
    eat(channel);
    exit(EXIT_SUCCESS);
  }
  exit(EXIT_SUCCESS);
}
