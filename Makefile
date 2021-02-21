NAME = Carcassonne

CC = gcc

PKGS = allegro-5 allegro_primitives-5 allegro_image-5 allegro_font-5 allegro_ttf-5
CFLAGS = -Wall -Wextra -Werror -pedantic `pkg-config $(PKGS) --cflags`
LDFLAGS = -lm `pkg-config $(PKGS) --libs`

SRC = ./src
OBJ = ./obj
BIN = ./bin
RELEASE = ./release

OBJ_FILES = $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(wildcard $(SRC)/*.c))
RES_FILES = $(patsubst $(SRC)/res/%, $(BIN)/res/%, $(wildcard $(SRC)/res/*))

debug: CFLAGS += -g
debug: main

prod: CFLAGS += -O3
prod: clean main

main: dirs res $(BIN)/$(NAME)
res: $(RES_FILES)

$(BIN)/$(NAME): $(OBJ_FILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c -o $@ $< $(LDFLAGS)

$(BIN)/res/%: $(SRC)/res/%
	cp $^ $@

dirs:
	@mkdir -p $(BIN) $(BIN)/res $(OBJ) $(RELEASE)

format:
	clang-format -i ./src/*.{c,h}

release: prod
	cd $(BIN); tar -czvf ../$(RELEASE)/carcassonne-linux-$(shell uname -m).tar.gz *

.PHONY: clean

clean:
	-rm -r $(OBJ) $(BIN) $(RELEASE)