/********************************************************************/
/* Copyright Silicon Storage Technology, Inc. (SST), 1994-2005	    */
/* Example "C" language Driver of SST25VF016B Serial Flash	    */
/* Conrado Canio, Silicon Storage Technology, Inc.                  */
/*                                                                  */
/* Revision 1.0, August 1st, 2005			  	    */   
/*                                                                  */
/*								    */
/********************************************************************/

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
void Chip_Erase(unsigned char cmd);
void Sector_Erase(unsigned long Dst);
void Block_Erase_32K(unsigned long Dst);
void Block_Erase_64K(unsigned long Dst);
void Wait_Busy();
void Wait_Busy_AAI();
void WREN_Check();
void WREN_AAI_Check();
void Page_Program(unsigned long Dst, unsigned char * buffer);

