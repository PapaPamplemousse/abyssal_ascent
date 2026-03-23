TARGET = abyssal_ascent

CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99

# Détection de l'OS pour appliquer les bons flags de Raylib
ifeq ($(OS),Windows_NT)
    LDFLAGS = -lraylib -lopengl32 -lgdi32 -lwinmm
else
    LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
endif

INCLUDES = $(patsubst %,-I%,$(wildcard */inc))
SRCS = $(wildcard */src/*.c)
OBJS = $(SRCS:.c=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "--- Édition des liens ---"
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	@echo "Compilation de $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@echo "Nettoyage..."
	rm -f $(OBJS) $(TARGET)

run: all
	@echo "--- Lancement ---"
	./$(TARGET)