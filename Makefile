CC	= 	gcc
CFLAGS 	=	-Wall -Werror -Wextra -std=gnu99

SRC =		swap.c
OBJ =		swap.o
EXE = 		swap


## Top level target is executable.
$(EXE):	$(OBJ)
		$(CC) $(CFLAGS) -o $(EXE) $(OBJ) -lm


## Clean: Remove object files and core dump files.
clean:
		/bin/rm $(OBJ) 


## Clobber: Performs Clean and removes executable file.

clobber: clean
		/bin/rm $(EXE) 

## Dependencies
swap.o:		swap.h
