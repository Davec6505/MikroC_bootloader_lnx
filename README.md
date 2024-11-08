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


//////////////////////////////////////////////////////////////////////////////      			           HEX FFILE LOADING				                        //
////////////////////////////////////////////////////////////////////////////
Open the hex file, search for record type 04 with address 0000 and data 1D00, this is 
the 32bit start address and indicates where hex data after this line will be flashed 
to.

//////////////////////////////////////////////////////////////////////////////                            HEX FILE                                    //
////////////////////////////////////////////////////////////////////////////

HEX record types are shown below

00 = Data record                                                              
01 = End of file record                                                       
02 = Extended segment address record                                          
03 = Start segment address record                                             
04 = Extended linear address record                                           
05 = Start linear address record 

////////////////////////////////////////////////////////////////////////////
// First line of a hex file
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


////////////////////////////////////////////////////////////////////////////
// Format of Intel hex files

Intel hex files consist of a number of lines, each of the form

	:nnaaaattddddd...dcc

with

	:	start of record
	nn	length of record
	aaaa	memory address to load
	tt	type: 00 data, 01 end-of-file
	dd..d	data
	cc	checksum

All numbers are in ASCII hex. The checksum makes sure that the sum
of all bytes is zero.

For example,

	:03000000021A20C1

says 3 bytes, address 0000, bytes 02, 1A, 20.

For example,

	:011A5000D2C3

says 1 byte, address 1A50 (hex), byte D2. We have 01 + 1A + 50 + 00 + D2 + C3 = 00.

For example,

	:00000001FF

is the end-of-file record.
Firmware download for Carry USB chips
Comparing the Intel hex file with what UsbSnoop sees:

	:100E5800C203C200C202C201120A9ED2E843D820CD

	10 00 58 0e 00 c2 03 c2 00 c2 02 c2 01 12 0a 9e
	d2 e8 43 d8 20 cd

we see that the number and the address are both sent as little endian shorts.

	:040EA8008080D6224E
	:10054600907FE9E070030206E114700302075D2460

	04 00 a8 0e 00 80 80 d6 22 4e 10 05 46 00 90 7f
	e9 e0 70 03 02 06

	10 00 46 05 00 90 7f e9 e0 70 03 02 06 e1 14 70
	03 02 07 5d 24 60

short records are padded with (random?) stuff, namely the start of the following record. 

////////////////////////////////////////////////////////////////////////////
//Last line of a hex file is always
:00000001FF	
