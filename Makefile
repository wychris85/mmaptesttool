
BYTE  = _MAP_BYTE_
WORD  = _MAP_WORD_
DWORD = _MAP_DWORD_


all : mmapttool_b.o mmapttool_w.o mmapttool_dw.o

mmapttool_b.o : mmapttool.c
	gcc mmapttool.c -D$(BYTE) -o mmapttool_b.o

mmapttool_w.o : mmapttool.c
	gcc mmapttool.c -D$(WORD) -o mmapttool_w.o

mmapttool_dw.o : mmapttool.c
	gcc mmapttool.c -D$(DWORD) -o mmapttool_dw.o

clean :
	rm -rf *.o
