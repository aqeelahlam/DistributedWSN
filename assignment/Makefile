CC		= mpicc
CFLAGS	= -Wall
LDFLAGS	= -lm
OBJFILES = simulator.c slave.c basestation.c
TARGET	= a.out

all:	$(TARGET)

$(TARGET):$(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)

exe:
	mpirun -oversubscribe -np $(p) ./a.out $(row) $(col)
clean:
	rm a.out log.txt
