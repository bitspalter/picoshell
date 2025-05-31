
#include <queue> 
#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "hardware/gpio.h"

#include "sd_card.h"

FATFS fs;

#include "C_Shell.hpp"

C_Shell CShell;

uint init_sdcard();

void Callback(char* pCommand, uint16_t cCommand);


/////////////////////////////////////////////////////////////////////
// [ init_sdcard ]
/////////////////////////////////////////////////////////////////////
uint init_sdcard(){

   if(!sd_init_driver()){
      printf("ERROR: Could not initialize SD Driver\r\n");
      return(0);
   }

   FRESULT fr;

   fr = f_mount(&fs, "0:", 1);

   if(fr != FR_OK){
      printf("ERROR: Could not mount filesystem (%d)\r\n", fr);
      return(0);
   }

   return(1);  
}

/////////////////////////////////////////////////////////////////////
// [ main ]
/////////////////////////////////////////////////////////////////////
int main(){

   sleep_ms(100);

   stdio_init_all();

   sleep_ms(100);

   printf("init_sdcard ...\n");
   init_sdcard();

   //////////////////////////////////////////////////////////
   // init 2nd Core and Command Loop

   CShell.run(Callback);

   sleep_ms(1000);

   //////////////////////////////////////////////////////////

   while(true){
      // hang
      sleep_ms(100);
   }

   return(0);
}

/////////////////////////////////////////////////////////////////////
// [ Callback ]
/////////////////////////////////////////////////////////////////////
void Callback(char* pCommand, uint16_t cCommand){
   //printf("\n TEST TEST %s\n", pCommand);
}


