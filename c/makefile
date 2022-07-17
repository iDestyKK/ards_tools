CC = gcc
CFLAGS = -g --std=gnu89

# Directories
BIN = bin
OBJ = obj
LIB = lib
SRC = src

all: $(BIN)/game_analyser $(BIN)/get_gameid $(BIN)/ards_game_to_xml \
     $(BIN)/ards_game_ls $(BIN)/ards_mem_eval $(BIN)/ards_firm_checksum \
     $(BIN)/ards_firm_extract

# -----------------------------------------------------------------------------
# Executables                                                              {{{1
# -----------------------------------------------------------------------------

$(BIN)/game_analyser: $(OBJ)/game_analyser.o $(OBJ)/cn_vec.o
	$(CC) $(CFLAGS) -o $@ $^

$(BIN)/get_gameid: $(OBJ)/get_gameid.o $(OBJ)/ards_gameid.o
	$(CC) $(CFLAGS) -o $@ $^

$(BIN)/ards_game_to_xml: $(OBJ)/ards_game_to_xml.o $(OBJ)/cn_vec.o \
                         $(OBJ)/ards_io.o
	$(CC) $(CFLAGS) -o $@ $^

$(BIN)/ards_game_ls: $(OBJ)/ards_game_ls.o $(OBJ)/cn_vec.o $(OBJ)/cn_cmp.o \
                     $(OBJ)/cn_map.o $(OBJ)/ards_io.o
	$(CC) $(CFLAGS) -o $@ $^

$(BIN)/ards_mem_eval: $(OBJ)/ards_mem_eval.o
	$(CC) $(CFLAGS) -o $@ $^

$(BIN)/ards_firm_checksum: $(OBJ)/ards_firm_checksum.o $(OBJ)/ards_firmware.o
	$(CC) $(CFLAGS) -o $@ $^

$(BIN)/ards_firm_extract: $(OBJ)/ards_firm_extract.o $(OBJ)/ards_firmware.o
	$(CC) $(CFLAGS) -o $@ $^

# -----------------------------------------------------------------------------
# Object Files (Applications)                                              {{{1
# -----------------------------------------------------------------------------

$(OBJ)/game_analyser.o: $(SRC)/game_analyser.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ)/get_gameid.o: $(SRC)/get_gameid.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ)/ards_game_to_xml.o: $(SRC)/ards_game_to_xml.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ)/ards_game_ls.o: $(SRC)/ards_game_ls.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ)/ards_mem_eval.o: $(SRC)/ards_mem_eval.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ)/ards_firm_checksum.o: $(SRC)/ards_firm_checksum.c
	$(CC) $(CFLAGS) -o $@ -c $^

$(OBJ)/ards_firm_extract.o: $(SRC)/ards_firm_extract.c
	$(CC) $(CFLAGS) -o $@ -c $^

# -----------------------------------------------------------------------------
# Libraries                                                                {{{1
# -----------------------------------------------------------------------------

# ARDS Utils
#$(OBJ)/ards_util.a: $(OBJ)/ards_gameid.o $(OBJ)/ards_io.o
#	ar cr $@ $^

# CNDS
#$(OBJ)/cnds.a: $(OBJ)/cn_map.o $(OBJ)/cn_cmp.o $(OBJ)/cn_vec.o
#	ar cr $@ $^

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

# ARDS/gameid
$(OBJ)/ards_gameid.o: $(LIB)/ards_util/gameid.c $(LIB)/ards_util/gameid.h
	$(CC) $(CFLAGS) -o $@ -c $<

# ARDS/io
$(OBJ)/ards_io.o: $(LIB)/ards_util/io.c $(LIB)/ards_util/io.h
	$(CC) $(CFLAGS) -o $@ -c $<

# ARDS/firmware
$(OBJ)/ards_firmware.o: $(LIB)/ards_util/firmware.c $(LIB)/ards_util/firmware.h
	$(CC) $(CFLAGS) -o $@ -c $<

# -----------------------------------------------------------------------------
# Extras                                                                   {{{1
# -----------------------------------------------------------------------------

clean:
	$(RM) $(BIN)/* $(OBJ)/*
