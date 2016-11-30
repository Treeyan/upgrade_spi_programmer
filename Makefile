#    Makefile - Written by Sergey Kiselev
# 
#    This file is part of spi_programmer.
# 
#    spi_programmer is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
# 
#    spi_programmer is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
# 
#    You should have received a copy of the GNU General Public License
#    along with spi_programmer.  If not, see <http://www.gnu.org/licenses/>.

all:		spi_programmer
spi_programmer:	main.c parport_driver.c sst25vf016b.c
	gcc -g -O0 -o spi_programmer main.c parport_driver.c sst25vf016b.c
