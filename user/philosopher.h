#ifndef __PHILO_H
#define __PHULO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include "libc.h"


void pickUpLeftFork(int channel[2]);
void pickUpRightFork(int channel[2]);
void eat(int channel[2]);
void waiter();
void philosopher(int channel[2]);
void waitRandom();
#endif
