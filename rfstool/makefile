
all: rfstool.o reiserfs.o physicaldrive.o gtools.o makefile pdriveposix.o pdrivefile.o
	gcc -o rfstool reiserfs.o rfstool.o physicaldrive.o gtools.o pdriveposix.o pdrivefile.o

rfstool.o : rfstool.cpp
	gcc -O1 -c rfstool.cpp

reiserfs.o : reiserfs.cpp reiserfs.h
	gcc -O1 -c reiserfs.cpp

pdriveposix.o : pdriveposix.cpp pdriveposix.h
	gcc -O1 -c pdriveposix.cpp

pdrivefile.o : pdrivefile.cpp pdrivefile.h
	gcc -O1 -c pdrivefile.cpp

physicaldrive.o : physicaldrive.cpp physicaldrive.h
	gcc -O1 -c physicaldrive.cpp

gtools.o : gtools.cpp gtools.h
	gcc -O1 -c gtools.cpp

clean :
	rm rfstool reiserfs.o pdriveposix.o physicaldrive.o gtools.o pdrivefile.o