/********************************************************************/
/* Copyright Silicon Storage Technology, Inc. (SST), 1994-2005	    */
/* Example "C" language Driver of SST25VF016B Serial Flash	    */
/* Conrado Canio, Silicon Storage Technology, Inc.                  */
/*                                                                  */
/* Revision 1.0, August 1st, 2005			  	    */   
/*                                                                  */
/*								    */
/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "parport_driver.h"

/* Function Prototypes */

void init();
void Send_Byte(unsigned char out);
unsigned char Get_Byte();
void Poll_SO();
void CE_High();
void CE_Low();
unsigned char Read_Status_Register();
void EWSR();
void WRSR(unsigned char byte);
void WREN();
void WRDI();
void EBSY();
void DBSY();
unsigned char Read_ID(unsigned char ID_addr);
unsigned long Jedec_ID_Read(); 
unsigned char Read(unsigned long Dst);
void Read_Cont(unsigned long Dst, unsigned long no_bytes);
unsigned char HighSpeed_Read(unsigned long Dst); 
void HighSpeed_Read_Cont(unsigned long Dst, unsigned long no_bytes, unsigned char *buffer);
void Byte_Program(unsigned long Dst, unsigned char byte);
void Auto_Add_IncA(unsigned long Dst, unsigned char byte1, unsigned char byte2);
void Auto_Add_IncB(unsigned char byte1, unsigned char byte2);
void Auto_Add_IncA_EBSY(unsigned long Dst, unsigned char byte1, unsigned char byte2);
void Auto_Add_IncB_EBSY(unsigned char byte1, unsigned char byte2);
void Chip_Erase();
void Sector_Erase(unsigned long Dst);
void Block_Erase_32K(unsigned long Dst);
void Block_Erase_64K(unsigned long Dst);
void Wait_Busy();
void Wait_Busy_AAI();
void WREN_Check();
void WREN_AAI_Check();

unsigned char upper_128[128];		/* global array to store read data */
					/* to upper RAM area from 80H - FFH */

/************************************************************************/
/* PROCEDURE: init							*/
/*									*/
/* This procedure initializes the SCK to low. Must be called prior to 	*/
/* setting up mode 0.							*/
/*									*/
/* Input:								*/
/*		None							*/
/*									*/
/* Output:								*/
/*		SCK							*/
/************************************************************************/
void init()
{
	SCK (0);	/* set clock to low initial state */
	CE (1);		/* unselect the chip */
}

/************************************************************************/
/* PROCEDURE: Send_Byte							*/
/*									*/
/* This procedure outputs a byte shifting out 1-bit per clock rising	*/
/* edge on the the SI pin(LSB 1st).					*/
/*									*/
/* Input:								*/
/*		out							*/
/*									*/
/* Output:								*/
/*		SI							*/
/************************************************************************/
void Send_Byte(unsigned char out)
{
	
	unsigned char i = 0;
	for (i = 0; i < 8; i++)
	{
		
		if ((out & 0x80) == 0x80)	/* check if MSB is high */
			SI (1);
		else
			SI (0);			/* if not, set to low */
		SCK (1);			/* toggle clock high */
		out = (out << 1);		/* shift 1 place for next bit */
		SCK (0);			/* toggle clock low */
	}
}

/************************************************************************/
/* PROCEDURE: Get_Byte							*/
/*									*/
/* This procedure inputs a byte shifting in 1-bit per clock falling	*/
/* edge on the SO pin(LSB 1st).						*/
/*									*/
/* Input:								*/
/*		SO							*/
/*									*/
/* Output:								*/
/*		None							*/
/************************************************************************/
unsigned char Get_Byte()
{
	unsigned char i = 0, in = 0, temp = 0;
	for (i = 0; i < 8; i++)
	{
		in = (in << 1);		/* shift 1 place to the left or shift in 0 */
		temp = SO ();		/* save input */
		SCK (1);		    /* toggle clock high */
	    if (temp == 1)			/* check to see if bit is high */
		    in = in | 0x01;		/* if high, make bit high */

		SCK (0);		/* toggle clock low */
	}

	return in;
}

/************************************************************************/
/* PROCEDURE: Poll_SO							*/
/*									*/
/* This procedure polls for the SO line during AAI programming  	*/
/* waiting for SO to transition to 1 which will indicate AAI programming*/
/* is completed								*/
/*									*/
/* Input:								*/
/*		SO							*/
/*									*/
/* Output:								*/
/*		None							*/
/************************************************************************/
void Poll_SO()
{
	unsigned char temp = 0;
	CE_Low();
   	while (temp == 0x00)	/* waste time until not busy */
		temp = SO ();
	CE_High();
}

/************************************************************************/
/* PROCEDURE: CE_High							*/
/*									*/
/* This procedure set CE = High.					*/
/*									*/
/* Input:								*/
/*		None							*/
/*									*/
/* Output:								*/
/*		CE							*/
/*									*/
/************************************************************************/
void CE_High() 
{
	CE (1);				/* set CE high */
}

/************************************************************************/
/* PROCEDURE: CE_Low							*/
/*									*/
/* This procedure drives the CE of the device to low.  			*/
/*									*/
/* Input:								*/
/*		None							*/
/*									*/
/* Output:								*/
/*		CE							*/
/*									*/
/************************************************************************/
void CE_Low() 
{	
	CE (0);				/* clear CE low */
}

/************************************************************************/
/* PROCEDURE: Read_Status_Register					*/
/*									*/
/* This procedure read the status register and returns the byte.	*/
/*									*/
/* Input:								*/
/*		None							*/
/*									*/
/* Returns:								*/
/*		byte							*/
/************************************************************************/
unsigned char Read_Status_Register()
{
	unsigned char byte = 0;
	CE_Low();			/* enable device */
	Send_Byte(0x05);		/* send RDSR command */
	byte = Get_Byte();		/* receive byte */
	CE_High();			/* disable device */
	return byte;
}

/************************************************************************/
/* PROCEDURE: EWSR							*/
/*									*/
/* This procedure Enables Write Status Register.  			*/
/*									*/
/* Input:								*/
/*		None							*/
/*									*/
/* Returns:								*/
/*		Nothing							*/
/************************************************************************/
void EWSR()
{
	CE_Low();			/* enable device */
	Send_Byte(0x50);		/* enable writing to the status register */
	CE_High();			/* disable device */
}

/************************************************************************/
/* PROCEDURE: WRSR							*/
/*									*/
/* This procedure writes a byte to the Status Register.			*/
/*									*/
/* Input:								*/
/*		byte							*/
/*									*/
/* Returns:								*/
/*		Nothing							*/
/************************************************************************/
void WRSR(unsigned char byte)
{
	CE_Low();			/* enable device */
	Send_Byte(0x01);		/* select write to status register */
	Send_Byte(byte);		/* data that will change the status of BPx 
					   or BPL (only bits 2,3,4,5,7 can be written) */
	CE_High();			/* disable the device */
}

/************************************************************************/
/* PROCEDURE: WREN							*/
/*									*/
/* This procedure enables the Write Enable Latch.  It can also be used 	*/
/* to Enables Write Status Register.					*/
/*									*/
/* Input:								*/
/*		None							*/
/*									*/
/* Returns:								*/
/*		Nothing							*/
/************************************************************************/
void WREN()
{
	CE_Low();			/* enable device */
	Send_Byte(0x06);		/* send WREN command */
	CE_High();			/* disable device */
}

/************************************************************************/
/* PROCEDURE: WRDI							*/
/*									*/
/* This procedure disables the Write Enable Latch.			*/
/*									*/
/* Input:								*/
/*		None							*/
/*									*/
/* Returns:								*/
/*		Nothing							*/
/************************************************************************/
void WRDI()
{
	CE_Low();			/* enable device */
	Send_Byte(0x04);		/* send WRDI command */
	CE_High();			/* disable device */
}

/************************************************************************/
/* PROCEDURE: EBSY							*/
/*									*/
/* This procedure enable SO to output RY/BY# status during AAI 		*/
/* programming.								*/
/*									*/
/* Input:								*/
/*		None							*/
/*									*/
/* Returns:								*/
/*		Nothing							*/
/************************************************************************/
void EBSY()
{
	CE_Low();			/* enable device */
	Send_Byte(0x70);		/* send EBSY command */
	CE_High();			/* disable device */
}

/************************************************************************/
/* PROCEDURE: DBSY							*/
/*									*/
/* This procedure disable SO as output RY/BY# status signal during AAI	*/
/* programming.								*/
/*									*/
/* Input:								*/
/*		None							*/
/*									*/
/* Returns:								*/
/*		Nothing							*/
/************************************************************************/
void DBSY()
{
	CE_Low();			/* enable device */
	Send_Byte(0x80);		/* send DBSY command */
	CE_High();			/* disable device */
}

/************************************************************************/
/* PROCEDURE: Read_ID							*/
/*									*/
/* This procedure Reads the manufacturer's ID and device ID.  It will 	*/
/* use 90h or ABh as the command to read the ID (90h in this sample).   */
/* It is up to the user to give the last byte ID_addr to determine      */
/* whether the device outputs manufacturer's ID first, or device ID 	*/
/* first.  Please see the product datasheet for details.  Returns ID in */
/* variable byte.							*/
/*									*/
/* Input:								*/
/*		ID_addr							*/
/*									*/
/* Returns:								*/
/*		byte:	ID1(Manufacture's ID = BFh or Device ID = 80h)	*/
/*									*/
/************************************************************************/
unsigned char Read_ID(unsigned char ID_addr)
{
	unsigned char byte;
	CE_Low();			/* enable device */
	Send_Byte(0x90);		/* send read ID command (90h or ABh) */
   	Send_Byte(0x00);		/* send address */
	Send_Byte(0x00);		/* send address */
	Send_Byte(ID_addr);		/* send address - either 00H or 01H */
	byte = Get_Byte();		/* receive byte */
	CE_High();			/* disable device */
	return byte;
}

/************************************************************************/
/* PROCEDURE: Jedec_ID_Read						*/
/*									*/
/* This procedure Reads the manufacturer's ID (BFh), memory type (25h)  */
/* and device ID (41h).  It will use 9Fh as the JEDEC ID command.    	*/
/* Please see the product datasheet for details.  			*/
/*									*/
/* Input:								*/
/*		None							*/
/*									*/
/* Returns:								*/
/*		IDs_Read:ID1(Manufacture's ID = BFh, Memory Type (25h), */
/*		 and Device ID (80h)					*/
/*									*/
/************************************************************************/
unsigned long Jedec_ID_Read() 
{
	unsigned long temp;
	
	temp = 0;

	CE_Low();			 /* enable device */
	Send_Byte(0x9F);		 /* send JEDEC ID command (9Fh) */
   	temp = (temp | Get_Byte()) << 8; /* receive byte */
	temp = (temp | Get_Byte()) << 8;	
	temp = (temp | Get_Byte()); 	 /* temp value = 0xBF2541 */
	CE_High();			 /* disable device */

	return temp;
}

/************************************************************************/
/* PROCEDURE:	Read							*/
/*									*/		
/* This procedure reads one address of the device.  It will return the 	*/
/* byte read in variable byte.						*/
/*									*/
/*									*/
/*									*/
/* Input:								*/
/*		Dst:	Destination Address 000000H - 1FFFFFH		*/
/*      								*/
/*									*/
/* Returns:								*/
/*		byte							*/
/*									*/
/************************************************************************/
unsigned char Read(unsigned long Dst) 
{
	unsigned char byte = 0;	

	CE_Low();			/* enable device */
	Send_Byte(0x03); 		/* read command */
	Send_Byte(((Dst & 0xFFFFFF) >> 16));	/* send 3 address bytes */
	Send_Byte(((Dst & 0xFFFF) >> 8));
	Send_Byte(Dst & 0xFF);
	byte = Get_Byte();
	CE_High();			/* disable device */
	return byte;			/* return one byte read */
}

/************************************************************************/
/* PROCEDURE:	Read_Cont						*/
/*									*/		
/* This procedure reads multiple addresses of the device and stores	*/
/* data into 128 byte buffer. Maximum byte that can be read is 128 bytes*/
/*									*/
/* Input:								*/
/*		Dst:		Destination Address 000000H - 1FFFFFH	*/
/*      	no_bytes	Number of bytes to read	(max = 128)	*/
/*									*/
/* Returns:								*/
/*		Nothing							*/
/*									*/
/************************************************************************/
void Read_Cont(unsigned long Dst, unsigned long no_bytes)
{
	unsigned long i = 0;
	CE_Low();				/* enable device */
	Send_Byte(0x03); 			/* read command */
	Send_Byte(((Dst & 0xFFFFFF) >> 16)); 	/* send 3 address bytes */
	Send_Byte(((Dst & 0xFFFF) >> 8));
	Send_Byte(Dst & 0xFF);
	for (i = 0; i < no_bytes; i++)		/* read until no_bytes is reached */
	{
		upper_128[i] = Get_Byte();	/* receive byte and store at address 80H - FFH */
	}
	CE_High();				/* disable device */

}

/************************************************************************/
/* PROCEDURE:	HighSpeed_Read						*/
/*									*/		
/* This procedure reads one address of the device.  It will return the 	*/
/* byte read in variable byte.						*/
/*									*/
/*									*/
/*									*/
/* Input:								*/
/*		Dst:	Destination Address 000000H - 1FFFFFH		*/
/*      								*/
/*									*/
/* Returns:								*/
/*		byte							*/
/*									*/
/************************************************************************/
unsigned char HighSpeed_Read(unsigned long Dst) 
{
	unsigned char byte = 0;	

	CE_Low();			/* enable device */
	Send_Byte(0x0B); 		/* read command */
	Send_Byte(((Dst & 0xFFFFFF) >> 16));	/* send 3 address bytes */
	Send_Byte(((Dst & 0xFFFF) >> 8));
	Send_Byte(Dst & 0xFF);
	Send_Byte(0xFF);		/*dummy byte*/
	byte = Get_Byte();
	CE_High();			/* disable device */
	return byte;			/* return one byte read */
}

/************************************************************************/
/* PROCEDURE:	HighSpeed_Read_Cont					*/
/*									*/		
/* This procedure reads multiple addresses of the device and stores	*/
/* data into 128 byte buffer. Maximum byte that can be read is 128 bytes*/
/*									*/
/* Input:								*/
/*		Dst:		Destination Address 000000H - 1FFFFFH	*/
/*      	no_bytes	Number of bytes to read	(max = 128)	*/
/*									*/
/* Returns:								*/
/*		Nothing							*/
/*									*/
/************************************************************************/
void HighSpeed_Read_Cont(unsigned long Dst, unsigned long no_bytes, unsigned char *buffer)
{
	unsigned long i = 0;
	CE_Low();				/* enable device */
	Send_Byte(0x0B); 			/* read command */
	Send_Byte(((Dst & 0xFFFFFF) >> 16)); 	/* send 3 address bytes */
	Send_Byte(((Dst & 0xFFFF) >> 8));
	Send_Byte(Dst & 0xFF);
	Send_Byte(0xFF);			/*dummy byte*/
	for (i = 0; i < no_bytes; i++)		/* read until no_bytes is reached */
	{
		if (i % 0x4000 == 0) {
			printf ("Read: %ld KBytes\n", i / 0x400);
		}
		buffer[i] = Get_Byte();		/* receive byte and store in buffer */
	}
	CE_High();				/* disable device */
}

/************************************************************************/
/* PROCEDURE:	Byte_Program						*/
/*									*/
/* This procedure programs one address of the device.			*/
/* Assumption:  Address being programmed is already erased and is NOT	*/
/*		block protected.					*/
/*									*/
/*									*/
/*									*/
/* Input:								*/
/*		Dst:		Destination Address 000000H - 1FFFFFH	*/
/*		byte:		byte to be programmed			*/
/*      								*/
/*									*/
/* Returns:								*/
/*		Nothing							*/
/*									*/
/************************************************************************/
void Byte_Program(unsigned long Dst, unsigned char byte)
{
	CE_Low();				/* enable device */
	Send_Byte(0x02); 			/* send Byte Program command */
	Send_Byte(((Dst & 0xFFFFFF) >> 16));	/* send 3 address bytes */
	Send_Byte(((Dst & 0xFFFF) >> 8));
	Send_Byte(Dst & 0xFF);
	Send_Byte(byte);			/* send byte to be programmed */
	CE_High();				/* disable device */
}

//
// by treeyan
//
void Page_Program(unsigned long Dst, unsigned char * buffer)
{
    int i;

	CE_Low();				/* enable device */
	Send_Byte(0x02); 			/* send Byte Program command */
	Send_Byte(((Dst & 0xFFFFFF) >> 16));	/* send 3 address bytes */
	Send_Byte(((Dst & 0xFFFF) >> 8));
	Send_Byte(Dst & 0xFF);

    for( i = 0; i < 256; i++ ) {
    	Send_Byte(buffer[i]);			/* send byte to be programmed */
    }

	CE_High();				/* disable device */
    Wait_Busy();
}


/************************************************************************/
/* PROCEDURE:	Auto_Add_IncA						*/
/*									*/
/* This procedure programs consecutive addresses of 2 bytes of data into*/
/* the device:  1st data byte will be programmed into the initial 	*/
/* address [A23-A1] and with A0 = 0.  The 2nd data byte will be be 	*/
/* programmed into initial address [A23-A1] and with A0  = 1.  This  	*/
/* is used to to start the AAI process.  It should be followed by 	*/
/* Auto_Add_IncB.							*/
/* Assumption:  Address being programmed is already erased and is NOT	*/
/*		block protected.					*/
/*									*/
/*									*/
/* Note: Only RDSR command can be executed once in AAI mode with SO  	*/
/* 	 disable to output RY/BY# status.  Use WRDI to exit AAI mode 	*/
/*	 unless AAI is programming the last address or last address of  */
/* 	 unprotected block, which automatically exits AAI mode.		*/
/*									*/
/* Input:								*/
/*		Dst:		Destination Address 000000H - 1FFFFFH	*/
/*		byte1:		1st byte to be programmed		*/
/*      	byte1:		2nd byte to be programmed		*/
/*									*/
/* Returns:								*/
/*		Nothing							*/
/*									*/
/************************************************************************/
void Auto_Add_IncA(unsigned long Dst, unsigned char byte1, unsigned char byte2)
{
	CE_Low();				/* enable device */
	Send_Byte(0xAD);			/* send AAI command */
	Send_Byte(((Dst & 0xFFFFFF) >> 16)); 	/* send 3 address bytes */
	Send_Byte(((Dst & 0xFFFF) >> 8));
	Send_Byte(Dst & 0xFF);
	Send_Byte(byte1);			/* send 1st byte to be programmed */	
	Send_Byte(byte2);			/* send 2nd byte to be programmed */
	CE_High();				/* disable device */
}

/************************************************************************/
/* PROCEDURE:	Auto_Add_IncB						*/
/*									*/
/* This procedure programs consecutive addresses of 2 bytes of data into*/
/* the device:  1st data byte will be programmed into the initial 	*/
/* address [A23-A1] and with A0 = 0.  The 2nd data byte will be be 	*/
/* programmed into initial address [A23-A1] and with A0  = 1.    This  	*/
/* is used after Auto_Address_IncA.					*/
/* Assumption:  Address being programmed is already erased and is NOT	*/
/*		block protected.					*/
/*									*/
/* Note: Only WRDI and AAI command can be executed once in AAI mode 	*/
/*	 with SO enabled as RY/BY# status.  When the device is busy 	*/
/*	 asserting CE# will output the status of RY/BY# on SO.  Use WRDI*/
/* 	 to exit AAI mode unless AAI is programming the last address or */
/*	 last address of unprotected block, which automatically exits 	*/
/*	 AAI mode.							*/
/*									*/
/* Input:								*/
/*									*/
/*		byte1:		1st byte to be programmed		*/
/*		byte2:		2nd byte to be programmed		*/
/*      								*/
/*									*/
/* Returns:								*/
/*		Nothing							*/
/*									*/
/************************************************************************/
void Auto_Add_IncB(unsigned char byte1, unsigned char byte2)
{
	CE_Low();				/* enable device */
	Send_Byte(0xAD);			/* send AAI command */
	Send_Byte(byte1);			/* send 1st byte to be programmed */
	Send_Byte(byte2);			/* send 2nd byte to be programmed */
	CE_High();				/* disable device */
}

/************************************************************************/
/* PROCEDURE:	Auto_Add_IncA_EBSY					*/
/*									*/
/* This procedure is the same as procedure Auto_Add_IncA except that it */
/* uses EBSY and Poll_SO functions to check for RY/BY. It programs 	*/
/* consecutive addresses of the device.  The 1st data byte will be 	*/
/* programmed into the initial address [A23-A1] and with A0 = 0.  The 	*/
/* 2nd data byte will be programmed into initial address [A23-A1] and 	*/
/* with A0  = 1.  This is used to to start the AAI process.  It should  */
/* be followed by Auto_Add_IncB_EBSY.					*/
/* Assumption:  Address being programmed is already erased and is NOT	*/
/*		block protected.					*/
/*									*/
/*									*/
/* Note: Only WRDI and AAI command can be executed once in AAI mode 	*/
/*	 with SO enabled as RY/BY# status.  When the device is busy 	*/
/*	 asserting CE# will output the status of RY/BY# on SO.  Use WRDI*/
/* 	 to exit AAI mode unless AAI is programming the last address or */
/*	 last address of unprotected block, which automatically exits 	*/
/*	 AAI mode.							*/
/*									*/
/* Input:								*/
/*		Dst:		Destination Address 000000H - 1FFFFFH	*/
/*		byte1:		1st byte to be programmed		*/
/*      	byte1:		2nd byte to be programmed		*/
/*									*/
/* Returns:								*/
/*		Nothing							*/
/*									*/
/************************************************************************/
void Auto_Add_IncA_EBSY(unsigned long Dst, unsigned char byte1, unsigned char byte2)
{
	CE_Low();				/* enable device */
	Send_Byte(0xAD);			/* send AAI command */
	Send_Byte(((Dst & 0xFFFFFF) >> 16)); 	/* send 3 address bytes */
	Send_Byte(((Dst & 0xFFFF) >> 8));
	Send_Byte(Dst & 0xFF);
	Send_Byte(byte1);			/* send 1st byte to be programmed */	
	Send_Byte(byte2);			/* send 2nd byte to be programmed */
	CE_High();				/* disable device */
	
	Poll_SO();				/* polls RY/BY# using SO line */

}

/************************************************************************/
/* PROCEDURE:	Auto_Add_IncB_EBSY					*/
/*									*/
/* This procedure is the same as Auto_Add_IncB excpet that it uses 	*/
/* Poll_SO to poll for RY/BY#.  It demonstrate on how to use DBSY after	*/
/* AAI programmming is completed.  It programs consecutive addresses of */
/* the device.  The 1st data byte will be programmed into the initial   */
/* address [A23-A1] and with A0 = 0.  The 2nd data byte will be 	*/
/* programmed into initial address [A23-A1] and with A0  = 1.  This is 	*/
/* used after Auto_Address_IncA.					*/
/* Assumption:  Address being programmed is already erased and is NOT	*/
/*		block protected.					*/
/*									*/
/* Note: Only WRDI and AAI command can be executed once in AAI mode 	*/
/*	 with SO enabled as RY/BY# status.  When the device is busy 	*/
/*	 asserting CE# will output the status of RY/BY# on SO.  Use WRDI*/
/* 	 to exit AAI mode unless AAI is programming the last address or */
/*	 last address of unprotected block, which automatically exits 	*/
/*	 AAI mode.							*/
/*									*/
/* Input:								*/
/*									*/
/*		byte1:		1st byte to be programmed		*/
/*		byte2:		2nd byte to be programmed		*/
/*      								*/
/*									*/
/* Returns:								*/
/*		Nothing							*/
/*									*/
/************************************************************************/
void Auto_Add_IncB_EBSY(unsigned char byte1, unsigned char byte2)
{
	CE_Low();				/* enable device */
	Send_Byte(0xAD);			/* send AAI command */
	Send_Byte(byte1);			/* send 1st byte to be programmed */
	Send_Byte(byte2);			/* send 2nd byte to be programmed */
	CE_High();				/* disable device */

	Poll_SO();				/* polls RY/BY# using SO line */
}

/************************************************************************/
/* PROCEDURE: Chip_Erase						*/
/*									*/
/* This procedure erases the entire Chip.				*/
/*									*/
/* Input:								*/
/*		None							*/
/*									*/
/* Returns:								*/
/*		Nothing							*/
/************************************************************************/
void Chip_Erase()
{						
	CE_Low();				/* enable device */
	Send_Byte(0x60);			/* send Chip Erase command (60h or C7h) */
	CE_High();				/* disable device */
}

/************************************************************************/
/* PROCEDURE: Sector_Erase						*/
/*									*/
/* This procedure Sector Erases the Chip.				*/
/*									*/
/* Input:								*/
/*		Dst:		Destination Address 000000H - 1FFFFFH	*/
/*									*/
/* Returns:								*/
/*		Nothing							*/
/************************************************************************/
void Sector_Erase(unsigned long Dst)
{


	CE_Low();				/* enable device */
	Send_Byte(0x20);			/* send Sector Erase command */
	Send_Byte(((Dst & 0xFFFFFF) >> 16)); 	/* send 3 address bytes */
	Send_Byte(((Dst & 0xFFFF) >> 8));
	Send_Byte(Dst & 0xFF);
	CE_High();				/* disable device */
}

/************************************************************************/
/* PROCEDURE: Block_Erase_32K						*/
/*									*/
/* This procedure Block Erases 32 KByte of the Chip.			*/
/*									*/
/* Input:								*/
/*		Dst:		Destination Address 000000H - 1FFFFFH	*/
/*									*/
/* Returns:								*/
/*		Nothing							*/
/************************************************************************/
void Block_Erase_32K(unsigned long Dst)
{
	CE_Low();				/* enable device */
	Send_Byte(0x52);			/* send 32 KByte Block Erase command */
	Send_Byte(((Dst & 0xFFFFFF) >> 16)); 	/* send 3 address bytes */
	Send_Byte(((Dst & 0xFFFF) >> 8));
	Send_Byte(Dst & 0xFF);
	CE_High();				/* disable device */
}

/************************************************************************/
/* PROCEDURE: Block_Erase_64K						*/
/*									*/
/* This procedure Block Erases 64 KByte of the Chip.			*/
/*									*/
/* Input:								*/
/*		Dst:		Destination Address 000000H - 1FFFFFH	*/
/*									*/
/* Returns:								*/
/*		Nothing							*/
/************************************************************************/
void Block_Erase_64K(unsigned long Dst)
{
	CE_Low();				/* enable device */
	Send_Byte(0xD8);			/* send 64KByte Block Erase command */
	Send_Byte(((Dst & 0xFFFFFF) >> 16)); 	/* send 3 address bytes */
	Send_Byte(((Dst & 0xFFFF) >> 8));
	Send_Byte(Dst & 0xFF);
	CE_High();				/* disable device */
}

/************************************************************************/
/* PROCEDURE: Wait_Busy							*/
/*									*/
/* This procedure waits until device is no longer busy (can be used by	*/
/* Byte-Program, Sector-Erase, Block-Erase, Chip-Erase).		*/
/*									*/
/* Input:								*/
/*		None							*/
/*									*/
/* Returns:								*/
/*		Nothing							*/
/************************************************************************/
void Wait_Busy()
{
	while (Read_Status_Register() == 0x03)	/* waste time until not busy */
		Read_Status_Register();
}

/************************************************************************/
/* PROCEDURE: Wait_Busy_AAI						*/
/*									*/
/* This procedure waits until device is no longer busy for AAI mode.	*/
/*									*/
/* Input:								*/
/*		None							*/
/*									*/
/* Returns:								*/
/*		Nothing							*/
/************************************************************************/
void Wait_Busy_AAI()
{
	while (Read_Status_Register() == 0x43)	/* waste time until not busy */
		Read_Status_Register();
}

/************************************************************************/
/* PROCEDURE: WREN_Check						*/
/*									*/
/* This procedure checks to see if WEL bit set before program/erase.	*/
/*									*/
/* Input:								*/
/*		None							*/
/*									*/
/* Returns:								*/
/*		Nothing							*/
/************************************************************************/
void WREN_Check()
{
	unsigned char byte;
	byte = Read_Status_Register();	/* read the status register */
	if (byte != 0x02)		/* verify that WEL bit is set */
	{
		fprintf (stderr, "ERROR: WEL bit is not set\n");
	}
}

/************************************************************************/
/* PROCEDURE: WREN_AAI_Check						*/
/*									*/
/* This procedure checks for AAI and WEL bit once in AAI mode.		*/
/*									*/
/* Input:								*/
/*		None							*/
/*									*/
/* Returns:								*/
/*		Nothing							*/
/************************************************************************/
void WREN_AAI_Check()
{
	unsigned char byte;
	byte = Read_Status_Register();	/* read the status register */
	if (byte != 0x42)		/* verify that AAI and WEL bit is set */
	{
		fprintf (stderr, "ERROR: WEL bit is not set\n");
	}
}
