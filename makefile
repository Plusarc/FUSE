CC      = gcc 
CFLAGS	=  -Wall -D_FILE_OFFSET_BITS=64 -g
TARGET1 = u_fs
TARGET2 = init
OBJS1	= u_fs.o helper.o
OBJS2   = init.o
INC	=  -pthread -lfuse -lrt -ldl

all:init u_fs
init:$(OBJS2)
	$(CC)   $(OBJS2) -o $(TARGET2)
u_fs:$(OBJS1)
	$(CC)   $(CFLAGS) $(OBJS1) -o $(TARGET1) $(INC)
u_fs.o:u_fs.h 
helper.o:helper.h
init.o:u_fs.h

.PHONY : all
clean :
	rm  -f $(TARGET1) $(TARGET2) $(OBJS1) $(OBJS2)









