# MikroC_bootloader_lnx
A work in progress - MikroC pic32 bootloader written with C for linux Fedora dist.

/* Protocol Description as form MikroC pro bootloader code.

  UHB protocol is a typical master-slave communication protocol, where
  master (PC) sends commands and slave (bootloader equiped device) executes
  them and aknowledges execution.

  * Command format.
    
    <STX[0]><CMD_CODE[0]><ADDRESS[0..3]><COUNT[0..1]> <DATA[0..COUNT-1]>
    |-- 1 --|---- 1 -----|------ 4 -----|----- 2 ----|------ COUNT -----|

    STX      - Command start delimiter (for future upgrades).
               Length: 1 byte. Mandatory.
    CMD_CODE - Command index (TCmd).
               Length: 1 byte. Mandatory.
    ADDRESS  - Address field. Flash start address for
               CMD_CODE command operation.
               Length: 4 bytes. Optional (command specific).
    COUNT    - Count field. Amount of data/blocks for
               CMD_CODE command operation.
               Length: 2 bytes. Optional (command specific).
    DATA     - Data array.
               Length: COUNT bytes. Optional (command specific).

  Some commands do not utilize all of these fields.
  See 'Command Table' below for details on specific command's format.

  * Command Table.
   --------------------------+---------------------------------------------------
  |       Description        |                      Format                       |
  |--------------------------+---------------------------------------------------|
  | Synchronize with PC tool |                  <STX><cmdSYNC>                   |
  |--------------------------+---------------------------------------------------|
  | Send bootloader info     |                  <STX><cmdINFO>                   |
  |--------------------------+---------------------------------------------------|
  | Go to bootloader mode    |                  <STX><cmdBOOT>                   |
  |--------------------------+---------------------------------------------------|
  | Restart MCU              |                  <STX><cmdREBOOT>                 |
  |--------------------------+---------------------------------------------------|
  | Write to MCU flash       | <STX><cmdWRITE><START_ADDR><DATA_LEN><DATA_ARRAY> |
  |--------------------------+---------------------------------------------------|
  | Erase MCU flash.         |  <STX><cmdERASE><START_ADDR><ERASE_BLOCK_COUNT>   |
   ------------------------------------------------------------------------------ 
   
  * Acknowledge format.
   
    <STX[0]><CMD_CODE[0]>
    |-- 1 --|---- 1 -----|
   
    STX      - Response start delimiter (for future upgrades).
               Length: 1 byte. Mandatory.
    CMD_CODE - Index of command (TCmd) we want to acknowledge.
               Length: 1 byte. Mandatory.

  See 'Acknowledgement Table' below for details on specific command's 
  acknowledgement process.
  
  * Acknowledgement Table.
   --------------------------+---------------------------------------------------
  |       Description        |                   Acknowledgement                 |
  |--------------------------+---------------------------------------------------|
  | Synchronize with PC tool |                  upon reception                   |
  |--------------------------+---------------------------------------------------|
  | Send bootloader info     |          no acknowledge, just send info           |
  |--------------------------+---------------------------------------------------|
  | Go to bootloader mode    |                  upon reception                   |
  |--------------------------+---------------------------------------------------|
  | Restart MCU              |                  no acknowledge                   |
  |--------------------------+---------------------------------------------------|
  | Write to MCU flash       | upon each write of internal buffer data to flash  |
  |--------------------------+---------------------------------------------------|
  | Erase MCU flash.         |                  upon execution                   |
   ------------------------------------------------------------------------------*/


This code is implimented as best as I can understand the above and by following Wireshark dumps whislt using the windows 
version of their app.

using usb interrupt transfers

Step 1:   send         [STX][cmdSYNC]...
          recieve      [STX][cmdSYNC]...
step 2:   send         [STX][cmdINFO]...
          recieve      [size][field type][field value][field type][field value]....        
step 3:   send         [STX][cmdBOOT]...
          recieve      [STX][cmdBOOT]...
step 4:   send         [STX][cmdSYNC]...
          recieve      [STX][cmdSYNC]...
step 5:   send         [STX][cmdERASE][START ADDRESS]...
          recieve      [STX][cmdERASE]...
step 6:   send         [STX][cmdWRITE][START ADDRESS][COUNT]...
          recieve      [STX][cmdWRITE]...
step 7:  send          [HEX]... 64 bytes at a time for the entire file, pad 0xff at the end of file. 
                                a packet will be sent back after the count has been reached.
          recieve      [STX][cmdWRITE]... after all packets have been sent, 


Will update as progress is made.


///////////////////////////////////////////////////////////////////////////////////////
//                                  HEX FILE FORMAT                                  //
///////////////////////////////////////////////////////////////////////////////////////

HEX record types are shown below

00 = Data record                                                              
01 = End of file record                                                       
02 = Extended segment address record                                          
03 = Start segment address record                                             
04 = Extended linear address record                                           
05 = Start linear address record 

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

//First line of a hex file
:
02
0000
04
1FC0
1B


//Line of a hex file
:101100003C0C400B000000700000007000000070FC

:											//Start of record
10											//Record length
1100										//Load address
00											//Record type
3C0C400B000000700000007000000070			//Actual data
FC											//checksum		



//Last line of a hex file is always
:00000001FF	
