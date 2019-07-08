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

 /*
  * More info. via http://www.malinov.com/Home/sergeys-projects/spi-flash-programmer
  * 2016.12.22. Treeyan, make it compatible with w25xx flash chip.
  * 2016.12.30. 25FUxxx need command 0x7C to erase chip.
  * 2017.09.10. I2C eeprom 24cxx been supports.
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



#include <stdio.h>

#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include <unistd.h>
#include <stdlib.h>     // make compiler happy.
#include <sys/stat.h>
#include <sys/ioctl.h>  // for struct winsize

#include "parport_driver.h"
#include "sst25vf016b.h"

typedef unsigned char   uchar;
typedef int             bool;
typedef unsigned short  ushort;

enum { false, true };

void usage( void )
{
    fprintf( stderr, "spi_programmer operate ...\n"
             "operate: [id], get chip device id.\n"
             "         [read] [size] [filename]. read spi flash to file, size is K or M.\n"
             "                  eg. \"spi_programmer read 4M save.bin\", read 4MB data of spi flash and write to file save.bin.\n"
             "         [write] [filename]. buring file to flash.\n"
             "         [erase] erase spi flash chip\n"
             "         [24cxx] [d] [size]. dump size of bytes to terminal.\n"
             "         [24cxx] [r] [size] [filename]. read i2c eeprom to file, size is byte\n"
             "         [24cxx] [w] [filename]. write file to i2c eeprom\n" );
}

#define SCL     SCK     // clock line.
#define SDA     SI      // i2c data in
#define SDAI    SO      // i2c data out.
#define A0A1    CE      // 24xx chip address line.

void p24start( void )
{
    SDA( 0 );
    SCL( 0 );
}

void p24stop( void )
{
    //
    // Here SDA|SCL state is unkonwn, however set to low for stop.
    //
    SDA( 0 );
    SCL( 0 );

    //
    // SDA high after SCL that mean is stop.
    //
    SCL( 1 );
    SDA( 1 );
}

void p24init( void )
{
    p24stop();  // Set i2c idle.
    A0A1( 0 );    // A1 | A0 set to low, ensure device address.
}

uchar p24recv( bool withACK )
{
    uchar val;
    int   i;

    for ( val = 0, i = 0; i < 8; i++ ) {

        val <<= 1;
        SCL( 1 );
        val |= ( SDAI() & 1 );
        SCL( 0 );
    }

    if ( withACK ) {

        SDA( 0 ); // low for ACK.
        SCL( 1 );
        SCL( 0 );
        SDA( 1 );
    }

    return ( val );
}

uchar p24send( uchar val )
{
    int     i;
    uchar   ack;

    for ( i = 0; i < 8; i++ ) {

        SDA( ( val & 0x80 ) ? 1 : 0 );
        val <<= 1;

        SCL( 1 );
        usleep( 5 );
        SCL( 0 );
    }

    SDA( 1 ); // Set to high for read ack.
    SCL( 1 );
    ack = SDAI();
    SCL( 0 );

    return ( ack );
}

//
// read sequential bytes.
//
int p24Cxx_rseq( uchar* buf, int siz )
{
    p24start();
    p24send( 0xa0 );
    p24send( 0x00 );
    p24stop();

    p24start();
    p24send( 0xa1 );

    while ( siz-- ) {

        *buf++ = p24recv( siz );
    }

    p24stop();
    return 0;
}

int p24Cxx_wb( uchar val, ushort address )
{
    uchar cmd;
    uchar add;

    add = ( uchar) ( address );
    cmd = 0xa0 | ( uchar) ( ( address >> 7 ) & 0xe );
    p24start();
    p24send( cmd );
    p24send( add );

    //
    // write byte.
    //
    p24send( val );
    p24stop();

    //
    // for chip 'Self-timed write cycle' 5ms MAX.
    //
    usleep( 5000 );

    return 0;
}
//
// write sequential bytes.
//
int p24Cxx_wseq( uchar* buf, ushort siz )
{
    ushort i;

    for ( i = 0; i < siz; i++ ) {

        p24Cxx_wb( *buf++, i );
    }

    return 0;
}

void hexDump( uchar* buf, int siz )
{
    int i, j;

    printf( "\n-----------------------DUMP BIN--------------------------\n\n" );

    for ( i = 0; i < siz; i += 16 ) {

        printf( "%06d: ", i / 16 );

        for ( j = 0; j < 16 && i + j < siz; j++ ) {
            printf( "%02X ", buf[i + j] );
        }

        printf( "\n" );
    }

    printf( "\n" );
}

void printProgress( int cur, int fin )
{
    static int  wid = 0;
    int         neq, nsp;
    uchar* equ;

    if ( !wid ) {

        struct winsize ws;

        ioctl( 0, TIOCGWINSZ, &ws );
        wid = ws.ws_col - 25;
    }

    if ( 0 >= wid )
        return;

    equ = malloc( wid );
    if ( !equ )
        return;

    memset( equ, '=', wid );

    cur = cur * 100 / fin;
    neq = wid * cur / 100;
    nsp = wid - neq;

    printf( "\rProgress %3d%% : %.*s%c%*c", cur, neq, equ, '>', nsp, '|' );

    if ( 100 == cur )
        printf( "\n" );

    fflush( stdout );

    free( equ );
}

int main( int argc, char* argv[] )
{
    int fd;
    unsigned long address;
    unsigned char status_reg;
    unsigned char* buffer;

    if ( argc < 2 ) {
        fprintf( stderr, "Required argument missing\n" );
        usage();
        exit( 1 );
    }

    parport_init();

    if ( !strcmp( argv[1], "24cxx" ) ) {

        p24init();

        if ( 4 > argc ) {
            perror( "Bad parameters.\n" );
            exit( 1 );
        }

        if ( !strcmp( argv[2], "d" ) ) {

            int   align, siz;

            siz = strtol( argv[3], NULL, 10 );

            if ( 2048 < siz ) {
                fprintf( stderr, "Unsupport chip.\n" );
                exit( 1 );
            }
            else if ( 0 == siz ) {
                fprintf( stderr, "Nothing to do...hehe.\n" );
                exit( 1 );
            }

            align = ( siz + 15 ) / 16 * 16;

            buffer = malloc( align );
            if ( !buffer ) {

                perror( "Fail memory.\n" );
                exit( 1 );
            }

            p24Cxx_rseq( buffer, align );

            hexDump( buffer, siz );


        }
        else if ( !strcmp( argv[2], "r" ) ) {

            int     siz;

            if ( 5 > argc ) {
                fprintf( stderr, "Miss parameters.\n" );
                exit( 1 );
            }

            siz = strtol( argv[3], NULL, 10 );

            if ( 2048 < siz ) {
                fprintf( stderr, "Unsupport chip.\n" );
                exit( 1 );
            }

            buffer = malloc( siz );
            if ( !buffer ) {

                perror( "Fail memory.\n" );
                exit( 1 );
            }

            fd = creat( argv[4], 0666 );
            if ( -1 == fd ) {

                perror( "Fail create file.\n" );
                exit( 1 );
            }

            p24Cxx_rseq( buffer, siz );

            if ( write( fd, buffer, siz ) != siz ) {

                perror( "Fail write file.\n" );
                exit( 1 );
            }

            close( fd );
            printf( "Done.\n" );

        }
        else if ( !strcmp( argv[2], "w" ) ) {

            int             n, nums;
            struct stat     st;

            if ( ( fd = open( argv[3], O_RDONLY ) ) == -1 ) {
                perror( "Unable to open file" );
                exit( 1 );
            }

            if ( fstat( fd, &st ) != 0 ) {
                perror( "Unable to get file size." );
                exit( 1 );
            }

            nums = ( st.st_size + 255 ) & ~255;

            if ( 2048 < nums ) {
                fprintf( stderr, "Unsupport chip now. 24c16A later.\n" );
                exit( 1 );
            }

            if ( ( buffer = malloc( nums ) ) == NULL ) {
                perror( "Unable to allocate memory" );
                exit( 1 );
            }

            memset( buffer, 0xff, nums );

            if ( read( fd, buffer, nums ) != /*nums*/ st.st_size ) {
                perror( "Error reading file" );
                exit( 1 );
            }

            n = st.st_size;

            while ( n ) {
                p24Cxx_wseq( buffer, ( 16 < n ) ? 16 : n );
                n -= 16;
                printProgress( st.st_size - n, st.st_size );
            }

        }
        else {

            fprintf( stderr, "Unknown command.\n" );
            exit( 1 );
        }

        return 0;
    }

    init();

    if ( !strcmp( argv[1], "id" ) ) {

        ulong id = Jedec_ID_Read();

        printf( "Device ID: 0x%lx\n", id );

    }
    else if ( !strcmp( argv[1], "status" ) ) {
        printf( "Status Register: 0x%x\n", Read_Status_Register() );
    }
    else if ( !strcmp( argv[1], "read" ) ) {
        int     nums;
        char* cap;

        if ( argc < 4 ) {
            fprintf( stderr, "Parameters missing\n" );
            exit( 1 );
        }

        nums = strtol( argv[2], &cap, 10 );
        if ( 0 >= nums ) {
            fprintf( stderr, "Read size error.\n" );
            exit( 5 );
        }

        switch ( *cap ) {
            case 'K':
            case 'k':
                nums *= 1024;
                break;
            case 'M':
            case 'm':
                nums *= 1024 * 1024;
                break;
            default:
                fprintf( stderr, "Wrong size, it can be 'K' or 'M'.\n" );
                exit( 6 );
        }

        if ( ( fd = creat( argv[3], 0666 ) ) == -1 ) {
            perror( "Unable to create file" );
            exit( 2 );
        }
        if ( ( buffer = malloc( nums ) ) == NULL ) {
            perror( "Unable to allocate memory" );
            exit( 3 );
        }
        HighSpeed_Read_Cont( 0, nums, buffer );
        if ( write( fd, buffer, nums ) != nums ) {
            perror( "Error writing file" );
            exit( 2 );
        }
        close( fd );
    }
    else if ( !strcmp( argv[1], "write" ) ) {

        int         nums;
        struct stat st;

        if ( argc < 3 ) {
            fprintf( stderr, "Filename missing\n" );
            exit( 1 );
        }
        if ( ( fd = open( argv[2], O_RDONLY ) ) == -1 ) {
            perror( "Unable to open file" );
            exit( 2 );
        }

        if ( fstat( fd, &st ) != 0 ) {
            perror( "Unable to get file size." );
            exit( 4 );
        }

        nums = ( st.st_size + 255 ) & ~255;

        if ( ( buffer = malloc( nums ) ) == NULL ) {
            perror( "Unable to allocate memory" );
            exit( 3 );
        }

        memset( buffer, 0xff, nums );

        if ( read( fd, buffer, nums ) != /*nums*/ st.st_size ) {
            perror( "Error reading file" );
            exit( 2 );
        }

        /* enable write */
        status_reg = Read_Status_Register();
        status_reg &= 0x43;	/* clear BPL and BPx bits */
        EWSR();
        WRSR( status_reg );
        WREN();
        EBSY();
        for ( address = 0; address < nums/*0x200000*/; address += 256 ) {
            Page_Program( address, buffer + address );
            //			if (address % 0x4000 == 0)
            //				printf ("Write: %ld Kbytes\n", address / 0x400);
            printProgress( address + 256, nums );
            WRDI();
            WREN();
        }
        WRDI();
        DBSY();
    }
    else if ( !strcmp( argv[1], "erase" ) ) {
        /* enable write */
        status_reg = Read_Status_Register();
        status_reg &= 0x43;	/* clear BPL and BPx bits */
        EWSR();
        WRSR( status_reg );
        WREN();
        /* erase chip */
        Chip_Erase( 0x60 );
        Wait_Busy();

        sleep( 1 );

        // 25FUxxx need this.
        WREN();
        /* erase chip */
        Chip_Erase( 0xC7 );
        Wait_Busy();
    }
    else {

        fprintf( stderr, "Wrong parameters.\n" );
    }

    return 0;
}
