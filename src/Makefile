#------------------------------------------------------------------------------------------------------------------------
# Makefile - Makefile for FM22 central
#------------------------------------------------------------------------------------------------------------------------
# Copyright (c) 2022 Frank Meyer - frank(at)uclock.de
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#------------------------------------------------------------------------------------------------------------------------
CXXFLAGS = -g -Wall -Werror -Wextra

HTTP_OBJ = http.o http-loco.o http-addon.o http-switch.o http-railroad.o http-s88.o http-rcl.o http-pom.o http-pgm.o http-pommap.o http-pomout.o http-common.o
HTTP_INC = http.h http-loco.h http-addon.h http-switch.h http-railroad.h http-s88.h http-rcl.h http-pom.h http-pgm.h http-pommap.h http-pomout.h http-common.h

OBJ = $(HTTP_OBJ) millis.o msg.o userio.o serial.o func.o loco.o addon.o fileio.o switch.o railroad.o s88.o rcl.o event.o dcc.o pom.o stm32.o base.o gpio.o debug.o main.o
INC = $(HTTP_INC) millis.h msg.h userio.h serial.h func.h loco.h addon.h fileio.h switch.h railroad.h s88.h rcl.h event.h dcc.h pom.h stm32.h base.h gpio.h debug.h version.h

fm22: $(OBJ)
	c++ $(OBJ) -l bcm2835 -o fm22

clean:
	rm -f *.o fm22

http.o: http.cc $(INC)
http-loco.o: http-loco.cc $(INC)
http-addon.o: http-addon.cc $(INC)
http-switch.o: http-switch.cc $(INC)
http-railroad.o: http-railroad.cc $(INC)
http-s88.o: http-s88.cc $(INC)
http-rcl.o: http-rcl.cc $(INC)
http-pom.o: http-pom.cc $(INC)
http-pgm.o: http-pgm.cc $(INC)
http-pommap.o: http-pommap.cc $(INC)
http-pomout.o: http-pomout.cc $(INC)
http-common.o: http-common.cc $(INC)
msg.o: msg.cc $(INC)
millis.o: millis.cc $(INC)
userio.o: userio.cc $(INC)
serial.o: serial.cc $(INC)
fileio.o: fileio.cc $(INC)
func.o: func.cc $(INC)
loco.o: loco.cc $(INC)
addon.o: addon.cc $(INC)
switch.o: switch.cc $(INC)
railroad.o: railroad.cc $(INC)
s88.o: s88.cc $(INC)
rcl.o: rcl.cc $(INC)
event.o: event.cc $(INC)
dcc.o: dcc.cc $(INC)
pom.o: pom.cc $(INC)
stm32.o: stm32.cc $(INC)
gpio.o: gpio.cc $(INC)
base.o: base.cc $(INC)
debug.o: debug.cc $(INC)
main.o: main.cc $(INC)
