#include "C_Shell.hpp"

static C_Shell* gSp = nullptr; // is needed for calling non-static functions

/////////////////////////////////////////////////////////////////////
// [ C_Shell ]
/////////////////////////////////////////////////////////////////////
C_Shell::C_Shell(){
   gSp = this;
}

/////////////////////////////////////////////////////////////////////
// [ ~C_Shell ]
/////////////////////////////////////////////////////////////////////
C_Shell::~C_Shell(){

}

/////////////////////////////////////////////////////////////////////
// [ run ]
/////////////////////////////////////////////////////////////////////
void C_Shell::run(void (*fptr)(char*, uint16_t)){

   if(fptr != nullptr){
      pCallback = fptr;
      bCallback = true;
   }
   
   printf("start 2nd Core ...\n");

   multicore_launch_core1(&C_Shell::core1_entry);
   multicore_fifo_push_blocking((uintptr_t) &C_Shell::com_loop);
   multicore_fifo_push_blocking(10);
}

/////////////////////////////////////////////////////////////////////
// [ core1_entry ]
/////////////////////////////////////////////////////////////////////
void C_Shell::core1_entry(){

   while(1){
      // Function pointer is passed to us via the FIFO
      // We have one incoming int32_t as a parameter, and will provide an
      // int32_t return value by simply pushing it back on the FIFO
      // which also indicates the result is ready.

      // Get Function Pointer
      int32_t (*func)(int32_t) = (int32_t(*)(int32_t)) multicore_fifo_pop_blocking();

      int32_t p = multicore_fifo_pop_blocking(); // Function Parameter

      int32_t result = (*func)(p); // Execute the Function

      multicore_fifo_push_blocking(result); // Return Result to Core0
   }
}

/////////////////////////////////////////////////////////////////////
// [ com_loop ]
/////////////////////////////////////////////////////////////////////
void C_Shell::com_loop(){

   std::array<char, BUFFER_LENGTH> pBuffer;
   uint16_t cBuffer = 0;
   bool bData = false;
   char chr;

   while(true){

      chr = getchar_timeout_us(100);

      while(chr != ENDSTDIN && cBuffer < BUFFER_LENGTH - 1){

         if(chr == SOH){

            cBuffer = 0x00;
            pBuffer[cBuffer] = 0x00;

         }else
         if(chr == STX){

            bData = true;

         }else
         if(chr == ETX){

            bData = false;

            pBuffer[cBuffer] = 0x00;

            gSp->parse_command(pBuffer.data(), cBuffer);

            break;

         }else{

            if(bData) 
               pBuffer[cBuffer++] = chr;
         }

         chr = getchar_timeout_us(100);
      }
   } 
}

/////////////////////////////////////////////////////////////////////
// [ parse_command ]
/////////////////////////////////////////////////////////////////////
void C_Shell::parse_command(char* pCommand, uint16_t cCommand){

   if(!pCommand || !cCommand) return;

   switch(pCommand[0]){
      case SYN: com_syn(); break;                    /* printf("Syncronize\n"); */  
      case PWD: com_pwd(); break;                    /* printf("Print Working Directory\n"); */  
      case LST: com_lst(); break;                    /* printf("List Working Directory\n"); */   
      case CWD: com_cwd(pCommand, cCommand); break;  /* printf("Change Working Directory\n"); */ 
      case MKD: com_mkd(pCommand, cCommand); break;  /* printf("Make Directory\n"); */           
      case MKF: com_mkf(pCommand, cCommand); break;  /* printf("Make File\n"); */  
      case RDF: com_rdf(pCommand, cCommand); break;  /* printf("Read File\n"); */  
      case WRF: com_wrf(pCommand, cCommand); break;  /* printf("Write File\n"); */ 
      case CLF: com_clf(pCommand, cCommand); break;  /* printf("clear File\n"); */ 
      case DEL: com_del(pCommand, cCommand); break;  /* printf("delete File or Folder\n"); */ 
      case COM: com_com(pCommand, cCommand); break;  /* printf("Command\n"); */ 

      default: break; /* printf("Unknown Command: %d\n", (int)pCommand[0]);  */
   }
}

/////////////////////////////////////////////////////////////////////
// [ com_com ] // TEST
/////////////////////////////////////////////////////////////////////
void C_Shell::com_com(char* pCommand, uint16_t cCommand){

   int p = 0;

   pPacket[p++] = SOH;
   pPacket[p++] = STX;

   switch(pCommand[1]){
      case CPS_REBOOT: 

         watchdog_enable(1000, 1);

         while(1); 

         break;

      case CPS_RB2USB: 

         reset_usb_boot(0, 0); 

         break;     

      case CPS_PICOID: 

         pico_unique_board_id_t board_id;

         pico_get_unique_board_id(&board_id);

         p += sprintf(&pPacket[p], "Unique identifier:");

         for(int i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; ++i){
            p += sprintf(&pPacket[p], " %02x", board_id.id[i]);
         }

         break;

      case CPS_CALLBACK: 

         if(bCallback)
            pCallback(pCommand, cCommand);

         break;

      default: 
      
         p += sprintf(&pPacket[p], "OkDefault %d", (int)pCommand[1]); break;
   }

   pPacket[p++] = ETX;
   pPacket[p++] = 0;

   printf(pPacket.data());
}

/////////////////////////////////////////////////////////////////////
// [ com_syn ]
/////////////////////////////////////////////////////////////////////
void C_Shell::com_syn(){

   int p = 0;

   pPacket[p++] = SOH;
   pPacket[p++] = STX;
   pPacket[p++] = ACK;

   p += sprintf(&pPacket[p], "Welcome: Pic0shell Server 0.1");

   pPacket[p++] = ETX;
   pPacket[p++] = 0;

   printf(pPacket.data());
}

/////////////////////////////////////////////////////////////////////
// [ com_del ]
/////////////////////////////////////////////////////////////////////
void C_Shell::com_del(char* pCommand, uint16_t cCommand){

   int p = 0;

   pPacket[p++] = SOH;
   pPacket[p++] = STX;

   FRESULT fr = f_unlink(&pCommand[1]);

   if(FR_OK != fr) p += sprintf(&pPacket[p], "Error1:%d", fr);
   else            p += sprintf(&pPacket[p], "Ok");
   
   pPacket[p++] = ETX;
   pPacket[p++] = 0;

   printf(pPacket.data());
}

/////////////////////////////////////////////////////////////////////
// [ com_lst ] list working directory
/////////////////////////////////////////////////////////////////////
void C_Shell::com_lst(){

   int p = 0;

   pPacket[p++] = SOH;
   pPacket[p++] = STX;

   FRESULT fr;

   std::array<char, FF_LFN_BUF> cwdbuf;

   char const *p_dir = cwdbuf.data();

   fr = f_getcwd(cwdbuf.data(), FF_LFN_BUF);

   if(FR_OK != fr){
      p += sprintf(&pPacket[p], "Error1:%d", fr);
      pPacket[p++] = ETX;
      pPacket[p++] = 0;
      printf(pPacket.data());
      return;
   }

   p += sprintf(&pPacket[p], "%s", p_dir);

   pPacket[p++] = ETX;
   pPacket[p++] = 0;

   printf(pPacket.data());

   /////////////////////////////////////////////////////

   DIR dj;      
   FILINFO fno;

   memset(&dj,  0, sizeof(dj));
   memset(&fno, 0, sizeof(fno));   

   fr = f_findfirst(&dj, &fno, p_dir, "*");

   if(FR_OK != fr){
      p = 0;
      pPacket[p++] = SOH;
      pPacket[p++] = STX;
      p += sprintf(&pPacket[p], "Error2:%d", fr);
      pPacket[p++] = ETX;
      pPacket[p++] = 0;
      printf(pPacket.data());
      return;
   }

   while(fr == FR_OK && fno.fname[0]){ 

      const char *pcWritableFile = "writable file",
                 *pcReadOnlyFile = "read only file",
                 *pcDirectory    = "directory";

      const char *pcAttrib;

      if(fno.fattrib & AM_DIR) pcAttrib = pcDirectory;
      else 
      if(fno.fattrib & AM_RDO) pcAttrib = pcReadOnlyFile;
      else                     pcAttrib = pcWritableFile;
      
      p = 0;

      pPacket[p++] = SOH;
      pPacket[p++] = STX;

      if(fno.fattrib & AM_DIR){
         p += sprintf(&pPacket[p], "%s [%s]", fno.fname, pcAttrib);
      }else{
         p += sprintf(&pPacket[p], "%s [%s] [size=%llu]", fno.fname, pcAttrib, fno.fsize);
      }
      
      pPacket[p++] = ETX;
      pPacket[p++] = 0;

      printf(pPacket.data());

      sleep_ms(10);

      fr = f_findnext(&dj, &fno);
   }

   f_closedir(&dj);
}

/////////////////////////////////////////////////////////////////////
// [ com_pwd ] print working directory
/////////////////////////////////////////////////////////////////////
void C_Shell::com_pwd(){

   int p = 0;

   pPacket[p++] = SOH;
   pPacket[p++] = STX;

   std::array<char, FF_LFN_BUF> cwdbuf;

   FRESULT fr = f_getcwd(cwdbuf.data(), FF_LFN_BUF);

   if(FR_OK != fr){
      p += sprintf(&pPacket[p], "Error1:%d", fr);
      pPacket[p++] = ETX;
      pPacket[p++] = 0;
      printf(pPacket.data());
      return;
   }

   p += sprintf(&pPacket[p], "%s", cwdbuf);

   pPacket[p++] = ETX;
   pPacket[p++] = 0;

   printf(pPacket.data());
}

/////////////////////////////////////////////////////////////////////
// [ com_mkd ] make directory
/////////////////////////////////////////////////////////////////////
void C_Shell::com_mkd(char* pCommand, uint16_t cCommand){

   int p = 0;

   pPacket[p++] = SOH;
   pPacket[p++] = STX;

   FRESULT fr = f_mkdir(&pCommand[1]);

   if(FR_OK != fr) p += sprintf(&pPacket[p], "Error1:%d", fr);
   else            p += sprintf(&pPacket[p], "Ok");
   
   pPacket[p++] = ETX;
   pPacket[p++] = 0;

   printf(pPacket.data());
}

/////////////////////////////////////////////////////////////////////
// [ com_mkf ] make file
/////////////////////////////////////////////////////////////////////
void C_Shell::com_mkf(char* pCommand, uint16_t cCommand){

   int p = 0;

   pPacket[p++] = SOH;
   pPacket[p++] = STX;

   FIL fil;

   FRESULT fr = f_open(&fil, &pCommand[1], FA_CREATE_NEW);

   if(FR_OK != fr){
      p += sprintf(&pPacket[p], "Error1:%d", fr);
   }else{
      p += sprintf(&pPacket[p], "Ok");

      fr = f_close(&fil);
   }

   pPacket[p++] = ETX;
   pPacket[p++] = 0;

   printf(pPacket.data());
}

/////////////////////////////////////////////////////////////////////
// [ com_cwd ] change working directory
/////////////////////////////////////////////////////////////////////
void C_Shell::com_cwd(char* pCommand, uint16_t cCommand){

   int p = 0;

   pPacket[p++] = SOH;
   pPacket[p++] = STX;

   FRESULT fr = f_chdir(&pCommand[1]);

   if(FR_OK != fr) p += sprintf(&pPacket[p], "Error1:%d", fr);
   else            p += sprintf(&pPacket[p], "Ok");
   
   pPacket[p++] = ETX;
   pPacket[p++] = 0;

   printf(pPacket.data());
}

/////////////////////////////////////////////////////////////////////
// [ com_clf ] clear file
/////////////////////////////////////////////////////////////////////
void C_Shell::com_clf(char* pCommand, uint16_t cCommand){

   int p = 0;

   pPacket[p++] = SOH;
   pPacket[p++] = STX;

   FIL fil;
   UINT rBuffer = 0;

   FRESULT fr = f_open(&fil, &pCommand[1], FA_WRITE);

   if(FR_OK != fr){
      p += sprintf(&pPacket[p], "Error1: %d", fr);
      pPacket[p++] = ETX;
      pPacket[p++] = 0;
      printf(pPacket.data());
      return;
   }

   fr = f_lseek(&fil, 0);

   if(FR_OK != fr){
      p += sprintf(&pPacket[p], "Error2: %d", fr);
      pPacket[p++] = ETX;
      pPacket[p++] = 0;
      printf(pPacket.data());
      f_close(&fil);
      return;
   }

   fr = f_truncate(&fil);

   if(FR_OK != fr){
      p += sprintf(&pPacket[p], "Error3: %d", fr);
      pPacket[p++] = ETX;
      pPacket[p++] = 0;
      printf(pPacket.data());
      f_close(&fil);
      return;
   }

   f_close(&fil);

   p += sprintf(&pPacket[p], "Ok");
   pPacket[p++] = ETX;
   pPacket[p++] = 0;

   printf(pPacket.data());
}

/////////////////////////////////////////////////////////////////////
// [ com_wrf ] write file
/////////////////////////////////////////////////////////////////////
void C_Shell::com_wrf(char* pCommand, uint16_t cCommand){

   int p = 0;

   pPacket[p++] = SOH;
   pPacket[p++] = STX;

   FRESULT fr;
   FIL fil;
   UINT rBuffer = 0;

   std::string sFile;
   std::string sData;

   bool bReady = false;

   ////////////////////////
   // Test
   if(pCommand[1] == HXS){

      std::string hex  = &pCommand[2];
      std::string part = hex.substr(0, 2);

      hex = &pCommand[4];

      std::string ascii = "";
      char ch;

      int start = 0, stop = 0, end = 0;

      for(int i = 0; i < hex.length(); i += 2){

         std::string part2 = hex.substr(i, 2);

         ch = stoul(part2, nullptr, 16);

         if(int(ch) == SON){
            ascii += '@';
            start = i;
         }else
         if(int(ch) == EON){
            ascii += '+'; //'\0';
            stop = i;
         }else{
            ascii += ch;
         }

         end++;
      }

      if(start != stop){

         sFile = ascii.substr(start + 1, stop / 2 - 1);
         sData = ascii.substr(stop / 2 + 1, end);

         bReady = true;
      }
   }

   //////////////////////////////////////////////////////////////

   if(!bReady){
      p += sprintf(&pPacket[p], "Error1:%d", fr);
      pPacket[p++] = ETX;
      pPacket[p++] = 0;
      printf(pPacket.data());
      return;
   }

   fr = f_open(&fil, sFile.c_str(), FA_OPEN_APPEND | FA_WRITE);
   
   if(FR_OK != fr){
      p += sprintf(&pPacket[p], "Error3:%d", fr);
      pPacket[p++] = ETX;
      pPacket[p++] = 0;
      printf(pPacket.data());
      return;
   }

   fr = f_write(&fil, sData.c_str(), sData.length(), &rBuffer);

   if(FR_OK != fr){
      p += sprintf(&pPacket[p], "Error4:%d", fr);
      pPacket[p++] = ETX;
      pPacket[p++] = 0;
      printf(pPacket.data());
      f_close(&fil);
      return;
   }

   fr = f_close(&fil);
   
   ///////////////////////////////

   sprintf(&pPacket[p], "Ok");
   p += 2;
   pPacket[p++] = ETX;
   pPacket[p++] = 0;

   printf(pPacket.data());
}

/////////////////////////////////////////////////////////////////////
// [ com_rdf ] read file
/////////////////////////////////////////////////////////////////////
void C_Shell::com_rdf(char* pCommand, uint16_t cCommand){

   int p = 0;
   
   pPacket[p++] = SOH;
   pPacket[p++] = STX;

   FIL fil;
   UINT rBuffer = 0;

   FRESULT fr = f_open(&fil, &pCommand[1], FA_READ);

   if(FR_OK != fr){
      p += sprintf(&pPacket[p], "Error1:%d", fr);
      pPacket[p++] = ETX;
      pPacket[p++] = 0;
      printf(pPacket.data());
      return;
   }

   //////////////////

   FILINFO fno;

   fr = f_stat(&pCommand[1], &fno);

   //////////////////

   std::vector<char> pData(fno.fsize);

   fr = f_read(&fil, pData.data(), fno.fsize, &rBuffer);

   if(FR_OK != fr){
      p += sprintf(&pPacket[p], "Error2:%d", fr);
      pPacket[p++] = ETX;
      pPacket[p++] = 0;
      printf(pPacket.data());
   }

   fr = f_close(&fil);

   ///////////////////////////////////////////////

   FSIZE_t cFile  = fno.fsize;

   pPacket[p++] = SOH;
   pPacket[p++] = STX;

   std::vector<char> pHexData(fno.fsize * 2);

   for(FSIZE_t nFile = 0; nFile < cFile; nFile++){
      sprintf(&pHexData[nFile * 2], "%02x", pData[nFile]);
   }

   ///////////////////////////////////////////////

   FSIZE_t cSFile = cFile * 2;

   FSIZE_t cParts = cSFile / CPS_DATACHUNK;
   FSIZE_t cRest  = cSFile % CPS_DATACHUNK;

   FSIZE_t nHexData = 0;

   for(FSIZE_t n = 0; n < cParts; n++){
      p = 0;

      pPacket[p++] = SOH;
      pPacket[p++] = STX;
      pPacket[p++] = HXS;
      
      for(FSIZE_t i = 0; i < 0x70; i++, nHexData++){
         pPacket[p++] = pHexData[nHexData];
      }

      pPacket[p++] = ETX;
      pPacket[p++] = 0;

      printf(pPacket.data());

      sleep_ms(10);
   }

   if(cRest){
      p = 0;

      pPacket[p++] = SOH;
      pPacket[p++] = STX;
      pPacket[p++] = HXS;

      for(FSIZE_t i = 0; i < cRest; i++, nHexData++){
         pPacket[p++] = pHexData[nHexData];
      }

      pPacket[p++] = ETX;
      pPacket[p++] = 0;

      printf(pPacket.data());

      sleep_ms(10);
   }
}

