# ============================================================================
# Makefile -- INF3421 : Langage Formel & Compilation - UY1
# Boite a outils interactive des automates finis et expressions regulieres
# ============================================================================

CC      = gcc
CSTD    = -std=c11
WARN    = -Wall -Wextra
OPT     = -O2
CFLAGS  = $(CSTD) $(WARN) $(OPT) -Iinclude
LDFLAGS = -lm

SRC_DIR = src
OBJ_DIR = obj
BIN     = tp_inf3421

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))

RESULT_DIRS = results/01_equations results/02_afn_afd results/03_regex_extraction \
              results/04_afdc results/05_etats results/06_emondage \
              results/07_conversions results/08_thompson results/09_minimisation \
              results/10_glushkov results/11_canonique results/12_clotures \
              results/13_mots results/14_divers

.PHONY: all run clean dirs rebuild

all: dirs $(BIN)

$(BIN): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo ""
	@echo "Compilation terminee avec succes -> ./$(BIN)"
	@echo "Lancez 'make run' pour demarrer le programme interactif."

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

dirs:
	@mkdir -p $(RESULT_DIRS)

run: all
	./$(BIN)

rebuild: clean all

clean:
	rm -rf $(OBJ_DIR) $(BIN)
	@echo "Fichiers objets et executable supprimes (les images de results/ sont conservees)."

distclean: clean
	rm -f results/*/*.pgm
	@echo "Toutes les images generees ont ete supprimees."
