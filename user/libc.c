/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "libc.h"



void reverse(char *s, int length) {
  int c, i, j;

  for (i = 0, j = length - 1; i < j; i++, j--) {
     c = s[i];
     s[i] = s[j];
     s[j] = c;
  }
}

void printNumber(int x){
  if(x){
    int i = -1;
    char *str;
    while(x){
      str[++i] = '0' + (x % 10);
      
      x /= 10;
    }
    reverse(str, i+1);
    write(STDOUT_FILENO, str, i+1);
  }
  else write(STDOUT_FILENO, "0", 1);
  
}

void printString(char *s, int length){
  write(STDOUT_FILENO, s, length);
}

int  atoi( char* x        ) {
  char* p = x; bool s = false; int r = 0;

  if     ( *p == '-' ) {
    s =  true; p++;
  }
  else if( *p == '+' ) {
    s = false; p++;
  }

  for( int i = 0; *p != '\x00'; i++, p++ ) {
    r = s ? ( r * 10 ) - ( *p - '0' ) :
            ( r * 10 ) + ( *p - '0' ) ;
  }

  return r;
}

void itoa( char* r, int x ) {
  char* p = r; int t, n;

  if( x < 0 ) {
     p++; t = -x; n = t;
  }
  else {
          t = +x; n = t;
  }

  do {
     p++;                    n /= 10;
  } while( n );

    *p-- = '\x00';

  do {
    *p-- = '0' + ( t % 10 ); t /= 10;
  } while( t );

  if( x < 0 ) {
    *p-- = '-';
  }

  return;
}

int completeReadPipe ( int fd ){
  int pipeValue = readPipe( fd , 1 );
  while (pipeValue == NOTWRITTEN) {
    pipeValue = readPipe( fd, 1 );
  }
  writePipe(fd, NOTWRITTEN);
  return pipeValue;
}

void completeWritePipe( int fd, int data ){
  int readValue = readPipe( fd, 0 );
  while (readValue != NOTWRITTEN) {
    readValue = readPipe( fd, 0 );
    writePipe(fd, readValue);
  }
  writePipe(fd, data);
}


void yield() {
  asm volatile( "svc %0     \n" // make system call SYS_YIELD
              :
              : "I" (SYS_YIELD)
              : );

  return;
}

int write( int fd, const void* x, size_t n ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 = fd
                "mov r1, %3 \n" // assign r1 =  x
                "mov r2, %4 \n" // assign r2 =  n
                "svc %1     \n" // make system call SYS_WRITE
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r)
              : "I" (SYS_WRITE), "r" (fd), "r" (x), "r" (n)
              : "r0", "r1", "r2" );

  return r;
}

int  read( int fd,       void* x, size_t n ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 = fd
                "mov r1, %3 \n" // assign r1 =  x
                "mov r2, %4 \n" // assign r2 =  n
                "svc %1     \n" // make system call SYS_READ
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r)
              : "I" (SYS_READ),  "r" (fd), "r" (x), "r" (n)
              : "r0", "r1", "r2" );

  return r;
}

int  fork() {
  int r;

  asm volatile( "svc %1     \n" // make system call SYS_FORK
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r)
              : "I" (SYS_FORK)
              : "r0" );

  return r;
}

void exit( int x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 =  x
                "svc %0     \n" // make system call SYS_EXIT
              :
              : "I" (SYS_EXIT), "r" (x)
              : "r0" );

  return;
}

void exec( const void* x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 = x
                "svc %0     \n" // make system call SYS_EXEC
              :
              : "I" (SYS_EXEC), "r" (x)
              : "r0" );

  return;
}

int  kill( int pid, int x ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 =  pid
                "mov r1, %3 \n" // assign r1 =    x
                "svc %1     \n" // make system call SYS_KILL
                "mov %0, r0 \n" // assign r0 =    r
              : "=r" (r)
              : "I" (SYS_KILL), "r" (pid), "r" (x)
              : "r0", "r1" );

  return r;
}

void nice( int pid, int x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 =  pid
                "mov r1, %2 \n" // assign r1 =    x
                "svc %0     \n" // make system call SYS_NICE
              :
              : "I" (SYS_NICE), "r" (pid), "r" (x)
              : "r0", "r1" );

  return;
}

int createPipe( int fileDes[2] ){
  int r;
  asm volatile( "mov r0, %2 \n" // assign r0 =  fileDes[]
                "svc %1     \n" // make system call SYS_CREATEPIPE
                "mov %0, r0 \n" // assign r0 =  r
              : "=r" (r)
              : "I" (SYS_CREATEPIPE), "r" (fileDes)
              : "r0");
  return r;
}

int readPipe(int fd, int erase){
  int r;
  asm volatile(   "mov r0, %2 \n" // assign r0 =  fd
                  "mov r1, %3 \n" // assign r1 =  erase
                  "svc %1     \n" // make system call SYS_READPIPE
                  "mov %0, r0 \n" // assign r0 =  r
                : "=r" (r)
                : "I" (SYS_READPIPE), "r" (fd), "r" (erase)
                : "r0", "r1" );

  return r;
}

void writePipe(int fd, int data){
  asm volatile(   "mov r0, %1 \n" // assign r0 =  fd
                  "mov r1, %2 \n" // assign r0 =  data
                  "svc %0     \n" // make system call SYS_WRITEPIPE
                :
                : "I" (SYS_WRITEPIPE), "r" (fd), "r" (data)
                : "r0", "r1" );

}