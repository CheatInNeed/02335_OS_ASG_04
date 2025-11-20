CC = gcc

CCWARNINGS = -W -Wall -Wno-unused-parameter -Wno-unused-variable \
		-Wno-unused-function
CCOPTS     = -g -O0 

CFLAGS = $(CCWARNINGS) $(CCOPTS)

LIB_SOURCES = aq_tsafe.c
LIB_OBJECTS = $(LIB_SOURCES:.c=.o)
LIB         = aq
LIB_DIR     = mylib
LIB_NAME     = lib$(LIB).a

DEMO_FILE   ?= pool_demo.c
DEMO_SOURCES = $(DEMO_FILE) pool.c task.c 
DEMO_OBJECTS = $(DEMO_SOURCES:.c=.o)

SEARCH_FILE   ?= search.c
SEARCH_SOURCES = $(SEARCH_FILE) pool.c task.c 
SEARCH_OBJECTS = $(SEARCH_SOURCES:.c=.o)

DEMO_EXECUTABLE = demo
SEARCH_EXECUTABLE = search

EXECUTABLES = $(DEMO_EXECUTABLE) $(SEARCH_EXECUTABLE)

.PHONY:  all lib clean clean-all

all: lib demo search

lib: $(LIB_DIR)/$(LIB_NAME)

%.o: %.c 
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB_DIR)/$(LIB_NAME): $(LIB_OBJECTS)
	mkdir -p $(LIB_DIR)
	ar -rcs $@ $^

$(DEMO_EXECUTABLE): lib $(DEMO_OBJECTS)
	$(CC) $(CFLAGS) $(DEMO_OBJECTS) -lpthread -L$(LIB_DIR) -l$(LIB) -o $@ 

$(SEARCH_EXECUTABLE): lib $(SEARCH_OBJECTS)
	$(CC) $(CFLAGS) $(SEARCH_OBJECTS) -lpthread -L$(LIB_DIR) -l$(LIB) -o $@ 

clean:
	rm -rf *.o *~ 

clean-all: clean
	rm -rf $(LIB_DIR) $(EXECUTABLES)

