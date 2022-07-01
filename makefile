CC=gcc
CFLAGS=-c -Wall -Iinclude
header=include/common.h
SOURCES=$(wildcard src/*.c)
SRCDIR=src
OBJECT=$(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(SOURCES:.c=.o))
OBJDIR=obj
serverDep=$(OBJDIR)/http.o $(OBJDIR)/helper.o 
OUTPUTDIR=bin
SERVER=http
CLIENT=client
all: $(SERVER) $(CLIENT)

$(SERVER): $(OBJECT)
	$(CC) -o $(OUTPUTDIR)/$@ obj/helper.o obj/http.o 

$(CLIENT): $(OBJECT)
	$(CC) -g -o $(OUTPUTDIR)/$@ obj/helper.o obj/client.o -lpthread

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -g $(CFLAGS) -c $< -o $@

.PHONY: clean 
clean:
	rm -f $(OBJDIR)/*.o $(OUTPUTDIR)/$(SERVER) $(OUTPUTDIR)/$(CLIENT)
