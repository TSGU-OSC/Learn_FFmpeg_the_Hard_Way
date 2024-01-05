CC      = gcc
C_SRC   = ${wildcard *.c}
BIN = ${patsubst %.c, %, $(C_SRC)}
 
INCDIRS :=
LIBDIRS :=
 
INCDIRS += /home/richard/Downloads/SDL2-2.0.14/_install/include/SDL2
LIBDIRS += /home/richard/Downloads/SDL2-2.0.14/_install/lib
 
CFLAGS  = -g -Wall -I$(INCDIRS) -L$(LIBDIRS) -lSDL2 -lpthread -lm -ldl
 
all:$(BIN)
 
$(BIN):%:%.c
		$(CC) -o $@ $^ $(CFLAGS)
 
clean:
		$(RM) a.out $(BIN)
 
.PHONY: all clean