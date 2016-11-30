/*
 *   main.c - Written by Sergey Kiselev
 *
 *   This file is part of spi_programmer.
 *
 *   spi_programmer is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   spi_programmer is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with spi_programmer.  If not, see <http://www.gnu.org/licenses/>.
 */

//#include <stdio.h>
//#include <errno.h>
//#include <string.h>
//#include <fcntl.h>
//#include "parport_driver.h"
//#include "sst25vf016b.h"
//
//int main (int argc, char *argv[])
//{
//	int fd;
//	unsigned long address;
//	unsigned char status_reg;
//	unsigned char *buffer;
//	if (argc < 2) {
//		fprintf (stderr, "Required argument missing\n");
//		exit (1);
//	}
//	parport_init ();
//	init ();
//	if (!strcmp (argv[1], "id")) {
//		printf ("Device ID: 0x%x\n", Jedec_ID_Read());
//	} else if (!strcmp (argv[1], "status")) {
//		printf ("Status Register: 0x%x\n", Read_Status_Register());
//	} else if (!strcmp (argv[1], "read")) {
//		if (argc < 3) {
//			fprintf (stderr, "Filename missing\n");
//			exit (1);
//		}
//		if ((fd = creat (argv[2], 0666)) == -1) {
//			perror ("Unable to create file");
//			exit (2);
//		}
//		if ((buffer = malloc (0x200000)) == NULL) {
//			perror ("Unable to allocate memory");
//			exit (3);
//		}
//		HighSpeed_Read_Cont (0, 0x200000, buffer);
//		if (write (fd, buffer, 0x200000) != 0x200000) {
//			perror ("Error writing file");
//			exit (2);
//		}
//		close (fd);
//	} else if (!strcmp (argv[1], "write")) {
//		if (argc < 3) {
//			fprintf (stderr, "Filename missing\n");
//			exit (1);
//		}
//		if ((fd = open (argv[2], O_RDONLY)) == -1) {
//			perror ("Unable to open file");
//			exit (2);
//		}
//		if ((buffer = malloc (0x200000)) == NULL) {
//			perror ("Unable to allocate memory");
//			exit (3);
//		}
//		if (read (fd, buffer, 0x200000) != 0x200000) {
//			perror ("Error reading file");
//			exit (2);
//		}
//		/* enable write */
//		status_reg = Read_Status_Register ();
//		status_reg &= 0x43;	/* clear BPL and BPx bits */
//		EWSR ();
//		WRSR (status_reg);
//		WREN ();
//		EBSY ();
//		for (address = 0; address < 0x200000; address += 2) {
//			if (address == 0)
//				Auto_Add_IncA (address, buffer[address], buffer[address+1]);
//			else
//				Auto_Add_IncB (buffer[address], buffer[address+1]);
//			Poll_SO ();
//			if (address % 0x4000 == 0)
//				printf ("Write: %d Kbytes\n", address / 0x400);
//		}
//		WRDI ();
//		DBSY ();
//	} else if (!strcmp (argv[1], "erase")) {
//		/* enable write */
//		status_reg = Read_Status_Register ();
//		status_reg &= 0x43;	/* clear BPL and BPx bits */
//		EWSR ();
//		WRSR (status_reg);
//		WREN ();
//		/* erase chip */
//		Chip_Erase ();
//		sleep (1);
//	}
//	return 0;
//}
//


/*
 * More info. via http://www.malinov.com/Home/sergeys-projects/spi-flash-programmer
 * 2016.12.22. Treeyan, make it compatible with w25xx flash chip.
*/ 


#include <stdio.h>

#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include <unistd.h>
#include <stdlib.h>     // make compiler happy.
#include <sys/stat.h>

#include "parport_driver.h"
#include "sst25vf016b.h" 

void usage( void )
{
    fprintf( stderr, "spi_programmer operate ...\n"
            "operate: [id], get chip device id.\n"
            "         [read] [size] [filename]. read flash to file, size is K or M.\n"
            "         eg. \"spi_programmer read 4M save.bin\", read 4MB flash data to file save.bin.\n"
            "         [write] [filename]. buring file to flash.\n"
            "         [erase]. erase flash chip\n" );
}

int main (int argc, char *argv[])
{
	int fd;
	unsigned long address;
	unsigned char status_reg;
	unsigned char *buffer;
	if (argc < 2) {
		fprintf (stderr, "Required argument missing\n");
        usage();
		exit (1);
	}

	parport_init ();
	init ();

	if (!strcmp (argv[1], "id")) {
		printf ("Device ID: 0x%lx\n", Jedec_ID_Read());
	} else if (!strcmp (argv[1], "status")) {
		printf ("Status Register: 0x%x\n", Read_Status_Register());
	} else if (!strcmp (argv[1], "read")) {
        int     nums;
        char *  cap;

		if (argc < 4) {
			fprintf (stderr, "Parameters missing\n");
			exit (1);
		}

        nums = strtol( argv[2], &cap, 10 );
        if( 0 >= nums ) {
            fprintf( stderr, "Read size error.\n" );
            exit( 5 );
        }

        switch( *cap ) {
            case 'K':
            case 'k':
                nums *= 1024;
                break;
            case 'M':
            case 'm':
                nums *= 1024*1024;
                break;
            default:
                fprintf( stderr, "Wrong size, it can be 'K' or 'M'.\n" );
                exit( 6 );
        }

		if ((fd = creat (argv[3], 0666)) == -1) {
			perror ("Unable to create file");
			exit (2);
		}
		if ((buffer = malloc ( nums )) == NULL) {
			perror ("Unable to allocate memory");
			exit (3);
        }
		HighSpeed_Read_Cont (0, nums , buffer);
		if (write (fd, buffer, nums ) != nums ) {
			perror ("Error writing file");
			exit (2);
		}
		close (fd);
	} else if (!strcmp (argv[1], "write")) {

        int         nums;
        struct stat st;

		if (argc < 3) {
			fprintf (stderr, "Filename missing\n");
			exit (1);
		}
		if ((fd = open (argv[2], O_RDONLY)) == -1) {
			perror ("Unable to open file");
			exit (2);
		}

        if( fstat( fd, &st ) != 0 ) {
            perror( "Unable to get file size." );
            exit( 4 );
        }

        nums = ( st.st_size + 255 ) & ~255;        

		if ((buffer = malloc ( nums )) == NULL) {
			perror ("Unable to allocate memory");
			exit (3);
		}
        
        memset( buffer, 0xff, nums );

		if (read (fd, buffer, nums ) != nums ) {
            perror ("Error reading file");
			exit (2);
		}

		/* enable write */
		status_reg = Read_Status_Register ();
		status_reg &= 0x43;	/* clear BPL and BPx bits */
		EWSR ();
		WRSR (status_reg);
		WREN ();
		EBSY ();
		for (address = 0; address < nums/*0x200000*/; address += 256) {
            Page_Program( address, buffer+address );
			if (address % 0x4000 == 0)
				printf ("Write: %ld Kbytes\n", address / 0x400);
            WRDI();
            WREN();
		}
		WRDI ();
		DBSY ();
	} else if (!strcmp (argv[1], "erase")) {
		/* enable write */
		status_reg = Read_Status_Register ();
		status_reg &= 0x43;	/* clear BPL and BPx bits */
		EWSR ();
		WRSR (status_reg);
		WREN ();
		/* erase chip */
		Chip_Erase ();
		sleep (1);
	}
	return 0;
}
