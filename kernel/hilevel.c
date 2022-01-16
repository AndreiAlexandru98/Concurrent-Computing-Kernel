/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"

pcb_t pcb[ 100 ]; 
pipe_t pipes[ 100 ];
int numberOfProcesses = 1; 
int executing = 0;    // The process that is executing
int highPriority = 0; // The process that should be executed
int currentPID = 0;
int offset = 0x00001000;
int consoleTime = 0;


void schedule( ctx_t* ctx ) {
    
    consoleTime = (consoleTime + 1) % 2; // Alternate between 0 and 1
    if(!consoleTime){
      for (int i = 1; i < numberOfProcesses; i++) {
        if(pcb[i].status != STATUS_TERMINATED)
          if(pcb[i].age > pcb[highPriority].age)
             highPriority = i;
      }    
    }
     else highPriority = 0;




    memcpy( &pcb[executing].ctx, ctx, sizeof( ctx_t ) );          // Preserve the executing process

    if(pcb[ executing ].status != STATUS_TERMINATED)
      pcb[ executing ].status = STATUS_READY;                     // Change executing process's status

    memcpy( ctx, &pcb[ highPriority ].ctx, sizeof( ctx_t ) );      // Update with the processs wth the highest priority
    pcb[ highPriority ].status = STATUS_EXECUTING;                 // Start the process
    executing = highPriority;                                      // Update the index

  return;
}

extern void main_console();
extern uint32_t tos_general;

void hilevel_handler_rst( ctx_t* ctx              ) {


    // Initialise pcb[0]
    memset( &pcb[ 0 ], 0, sizeof( pcb_t ) );
    pcb[ 0 ].pid      = 0;
    pcb[ 0 ].status   = STATUS_READY;
    pcb[ 0 ].ctx.cpsr = 0x50;
    pcb[ 0 ].ctx.pc   = ( uint32_t )( &main_console );
    pcb[ 0 ].ctx.sp   = ( uint32_t )( &(tos_general)  );
    pcb[ 0 ].tos   = ( uint32_t )( &(tos_general)  );
    pcb[ 0 ].basePriority = 0;
    pcb[ 0 ].age = 50;
    
    for(int i = 1; i < 10; i++){
      pcb[i].status = STATUS_TERMINATED;
    }



    // Start pcb[0]
    memcpy( ctx, &pcb[ 0 ].ctx, sizeof( ctx_t ) );
    pcb[ 0 ].status = STATUS_EXECUTING;
   

    // TIMER0->Timer1Load  = 0x00011010; // select period = 2^20 ticks ~= 1 sec
    TIMER0->Timer1Load  = 0x00020000; // select period = 2^20 ticks ~= 1 sec
    TIMER0->Timer1Ctrl  = 0x00000002; // select 32-bit   timer
    TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
    TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
    TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

    GICC0->PMR          = 0x000000F0; // unmask all            interrupts
    GICD0->ISENABLER1  |= 0x00000010; // enable timer          interrupt
    GICC0->CTLR         = 0x00000001; // enable GIC interface
    GICD0->CTLR         = 0x00000001; // enable GIC distributor

    int_enable_irq();

    // Initialise pipes
    for(int i = 0; i < 100; i++){
      pipes[i].isWritten = 0;
      pipes[i].isClosed  = 0;
      pipes[i].isFree    = 1;
      pipes[i].message   = NOTWRITTEN;
    }

    return;
}

void hilevel_handler_irq(ctx_t* ctx) {

  uint32_t id = GICC0->IAR;

  if( id == GIC_SOURCE_TIMER0 ) {
    TIMER0->Timer1IntClr = 0x01;
      
      for(int i=0; i< numberOfProcesses; i++)
          if(pcb[i].status == STATUS_EXECUTING || pcb[i].status == STATUS_TERMINATED)
          {
              pcb[i].age = pcb[i].basePriority;
          }
          else
              pcb[i].age++;
    schedule(ctx);
  }

  GICC0->EOIR = id;

  return;
}

void hilevel_handler_svc( ctx_t* ctx, uint32_t id ) {
  /* Based on the identified encoded as an immediate operand in the
   * instruction,
   *
   * - read  the arguments from preserved usr mode registers,
   * - perform whatever is appropriate for this system call,
   * - write any return value back to preserved usr mode registers.
   */

  switch( id ) {
    case 0x00 : { // 0x00 => yield()

      break;
    }

    case 0x01 : { // 0x01 => write( fd, x, n )
      int   fd = ( int   )( ctx->gpr[ 0 ] );
      char*  x = ( char* )( ctx->gpr[ 1 ] );
      int    n = ( int   )( ctx->gpr[ 2 ] );

      for( int i = 0; i < n; i++ ) {
        PL011_putc( UART0, *x++, true );
      }

      ctx->gpr[ 0 ] = n;
      break;
    }

    case 0x03: { // 0x03 => fork()

      int child = -1;
      // Check for terminated processes
      for(int i = 0; i < numberOfProcesses; i++)
        if(pcb[ i ].status == STATUS_TERMINATED){
          child = i;
          break;
        }

      //CASE: No terminated processes - > Need to create a new one
      if(child == -1) {
          numberOfProcesses++; 
          child = numberOfProcesses - 1;
      }
      
        
      currentPID++;
      // Initialise the new process
      memset(&pcb[child], 0, sizeof(pcb_t));
      pcb[ child ].pid      = currentPID;
      pcb[ child ].status   = STATUS_READY;

      // Copy parent's context
      memcpy(&(pcb[ child ].ctx), ctx, sizeof(ctx_t));

      // Assign 0 to child process
      pcb[ child ].ctx.gpr[ 0 ] = 0;

      //Update top of stack and stack pointer
      pcb[ child ].tos          =  (uint32_t) pcb[ executing ].tos + (uint32_t) ((child) * offset);
      pcb[ child ].ctx.sp       =  pcb[ child ].tos + (uint32_t) pcb[ executing ].tos - (uint32_t) ctx->sp;

      memcpy((void*) pcb[ child ].tos, (void*) pcb[ executing ].tos , offset);
      // Assign child pid to parent
      ctx->gpr[0]                             = currentPID;
        
      pcb[ child ].basePriority = 0;
      pcb[ child ].age = 100;

      break;
    }
          
    case 0x04: { //0x04 => exit()
      pcb[ executing ].status = STATUS_TERMINATED;
      schedule(ctx);

      break;
    }

    case 0x05: { //0x05 => exec()]
      if(pcb[ executing ].status != STATUS_TERMINATED){
        pcb[ executing ].status = STATUS_EXECUTING;
        ctx->pc = (uint32_t) ctx->gpr[0];
      }

      break;
    }

    case 0x06: { //0x06 => kill()
      uint32_t p = ctx->gpr[0];
      for(int i = 0; i < numberOfProcesses; i++)
        if(pcb[ i ].pid == p)
            pcb[ i ].status = STATUS_TERMINATED;
      
      break;
    }

    case 0x08: { //0x08 => createPipe()
      int *filedes = (int *)ctx->gpr[0];
      filedes[0] = -1;
      filedes[1] = -1;
      int done = 0;
      for(int i = 0; i < 100; i++){
        if(pipes[i].isFree){
          if(!done){
            filedes[0] = i;
            pipes[i].isFree = 0;
            done = 1;
          }
          else{
            filedes[1] = i;
            pipes[i].isFree = 0;
            break;
          }
        }
      }

      break;
    }

    case 0x09: { //0x09 => readPipe()
      int fd = ctx->gpr[0];
      int erase = ctx->gpr[1];
        if((!pipes[fd].isClosed) && pipes[fd].isWritten == 1){
            ctx->gpr[0] = pipes[fd].message;
            if(erase == 1) 
                pipes[fd].isWritten = 0;

        }
        else ctx->gpr[0] = NOTWRITTEN;

      break;
    }

    case 0x10: { //0x10 => writePipe()
      int fd = ctx->gpr[0];

      if(!pipes[fd].isClosed && !pipes[fd].isWritten){
          pipes[fd].message = ctx->gpr[1];
          if(pipes[fd].message == NOTWRITTEN) 
              pipes[fd].isWritten = 0;
          else 
              pipes[fd].isWritten = 1;
        }

      break;
    }

    default   : { // 0x?? => unknown/unsupported
      break;
    }
  }

  return;
}
