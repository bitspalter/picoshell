#ifndef __CSHELL_H__
#define __CSHELL_H__

#include "pico.h"
#include "pico/time.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "pico/multicore.h"

#include "hardware/watchdog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <exception>
#include <array>
#include <vector>

#include "sd_card.h"
#include "ff.h"

///////////////////////////////////////////////////
// [ const ]
///////////////////////////////////////////////////

// ascii codes //////////////////////

#define NUL 0x00 // 
#define SOH 0x01 // start of heading
#define STX 0x02 // start od text
#define ETX 0x03 // end of text
#define EOT 0x04 // end of transmission
#define ENQ 0x05 // enquiry
#define ACK 0x06 // acknowledge
#define BEL 0x07 // bell

///// MY OWN /////////////

#define PWD 0x08 // print working directory
#define CWD 0x09 // change working directory
#define LST 0x0A // list working directory
#define MKD 0x0B // make directory
#define MKF 0x0C // make file
#define RDF 0x0D // read file
#define WRF 0x0E // write file
#define SON 0x0F // start of name
#define LFD 0x10 // line feed
#define EON 0x11 // end of name
#define CLF 0x12 // clear file
#define HXS 0x13 // hex string
#define COM 0x14 // Commando

///// MY OWN /////////////

// ascii codes //////////////////////
#define NAK 0x15 // negative acknowledge
#define SYN 0x16 // synchronize
#define CAN 0x18 // cancel

#define ESC 0x1B // escape
#define MSG 0x1C // message
#define DEL 0x7F // delete

#define ENDSTDIN 0xFF

#define BUFFER_LENGTH 1024

#define TEST_NUM 10

#define MAX_PACKET_SIZE 10 * 1024


#define CPS_DATACHUNK 0x70

#define CPS_REBOOT   '1'
#define CPS_RB2USB   '2'
#define CPS_PICOID   '3'
#define CPS_CALLBACK '4'

///////////////////////////////////////////////////
// [ class ]
///////////////////////////////////////////////////

class C_Shell {

   public:

      C_Shell();
     ~C_Shell();

      void run(void (*fptr)(char*, uint16_t));

   private:

      void parse_command(char* pCommand, uint16_t cCommand);

      void com_lst();
      void com_pwd();
      void com_syn();
      void com_com(char* pCommand, uint16_t cCommand);
      void com_del(char* pCommand, uint16_t cCommand);
      void com_mkd(char* pCommand, uint16_t cCommand);
      void com_mkf(char* pCommand, uint16_t cCommand);
      void com_cwd(char* pCommand, uint16_t cCommand);
      void com_clf(char* pCommand, uint16_t cCommand);
      void com_wrf(char* pCommand, uint16_t cCommand);
      void com_rdf(char* pCommand, uint16_t cCommand);

      ////////////////

      static void com_loop();
      static void core1_entry();

      ////////////////

      bool bCallback = {false};
      void (*pCallback)(char* pCommand, uint16_t cCommand) = {nullptr};

      ////////////////

      std::array<char, MAX_PACKET_SIZE> pPacket;
};




#endif // __CSHELL_H__