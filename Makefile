TARGET = abyssal_ascent

CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99
WIN_CC = x86_64-w64-mingw32-gcc
WIN_CFLAGS = -Wall -Wextra -std=c99 -Iraylib_win/include -Iclicker/inc -Icombat/inc -Icore/inc -Iaudio/inc -Idungeon/inc -Iscenes/inc -Iui/inc -Iutils/inc
WIN_LDFLAGS = -Lraylib_win/lib -lraylib -lopengl32 -lgdi32 -lwinmm -static -mwindows
WIN_SRC = clicker/src/*.c combat/src/*.c core/src/*.c audio/src/*.c dungeon/src/*.c scenes/src/*.c ui/src/*.c utils/src/*.c

WIN_OUT_DIR = lib-win/Abyssal_Ascent_Game
WIN_OUT = $(WIN_OUT_DIR)/AbyssalAscent.exe

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
	rm -rf $(WIN_OUT_DIR)

run: all
	@echo "--- Lancement ---"
	./$(TARGET)

# ========================================================
# --- COMPILATION WINDOWS (CROSS-COMPILING) ---
# ========================================================



windows:
	@echo "--- Creation du dossier lib-win... ---"
	mkdir -p $(WIN_OUT_DIR)
	@echo "--- Compilation de l'executable Windows... ---"
	$(WIN_CC) $(WIN_SRC) -o $(WIN_OUT) $(WIN_CFLAGS) $(WIN_LDFLAGS)
	@echo "--- Copie des assets (Images, Json, Polices)... ---"
	cp -r assets $(WIN_OUT_DIR)/
	@echo "--- Nettoyage de la sauvegarde (Nouvelle partie)... ---"
	rm -f $(WIN_OUT_DIR)/save.json
	@echo "========================================================"
	@echo " SUCCES ! Le jeu Windows est pret dans :"
	@echo " -> lib-win/Abyssal_Ascent_Game/"
	@echo "========================================================"