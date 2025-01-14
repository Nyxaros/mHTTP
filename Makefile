CC = gcc
CFLAGS = -Wall -DDEBUG -g -c

SRCS = ./src/mhttp.c
OBJS = $(SRCS:.c=.o)

LIB = mhttp.a

all: $(LIB)

$(LIB): $(OBJS)
	@ar rcs $@ $(OBJS)
	@echo "Static library $(LIB) created."

%.o: %.c
	@$(CC) $(CFLAGS) $< -o $@
	@echo "Compiled $< into $@"

clean:
	@rm -f $(OBJS) $(LIB)
	@echo "Cleaned up."

.PHONY: clean all

