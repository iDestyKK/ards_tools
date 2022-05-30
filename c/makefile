CC = gcc
CFLAGS = -g

# Directories
BIN = bin
OBJ = obj
LIB = lib
SRC = src

all: $(BIN)/game_analyser

# -----------------------------------------------------------------------------
# Executables                                                              {{{1
# -----------------------------------------------------------------------------

$(BIN)/game_analyser: $(OBJ)/game_analyser.o $(OBJ)/cnds.a
	$(CC) $(CFLAGS) -o $@ $^

# -----------------------------------------------------------------------------
# Object Files (Applications)                                              {{{1
# -----------------------------------------------------------------------------

$(OBJ)/game_analyser.o: $(SRC)/game_analyser.c
	$(CC) $(CFLAGS) -o $@ -c $<

# -----------------------------------------------------------------------------
# Libraries                                                                {{{1
# -----------------------------------------------------------------------------

# CNDS
$(OBJ)/cnds.a: $(OBJ)/cn_map.o $(OBJ)/cn_cmp.o $(OBJ)/cn_vec.o
	ar cr $@ $^

# -----------------------------------------------------------------------------
# Object Files (Libraries)                                                 {{{1
# -----------------------------------------------------------------------------

# CNDS/CN_Map
$(OBJ)/cn_map.o: $(LIB)/CN_Map/cn_map.c $(LIB)/CN_Map/cn_map.h
	$(CC) $(CFLAGS) -o $@ -c $<

# CNDS/CN_Map/CN_Cmp
$(OBJ)/cn_cmp.o: $(LIB)/CN_Map/cn_cmp.c $(LIB)/CN_Map/cn_cmp.h
	$(CC) $(CFLAGS) -o $@ -c $<

# CNDS/CN_Vec
$(OBJ)/cn_vec.o: $(LIB)/CN_Vec/cn_vec.c $(LIB)/CN_Vec/cn_vec.h
	$(CC) $(CFLAGS) -o $@ -c $<

# -----------------------------------------------------------------------------
# Extras                                                                   {{{1
# -----------------------------------------------------------------------------

clean:
	$(RM) $(BIN)/* $(OBJ)/*
