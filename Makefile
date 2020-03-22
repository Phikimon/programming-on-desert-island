SHELL = /bin/bash

AUX_DIR = aux
OUT_DIR = out
SRC_DIR = src
FIG_DIR = fig

PRESENTATION_NAMES = programming_on_desert_island_rus.pdf
PRESENTATIONS = $(addprefix $(OUT_DIR)/, $(PRESENTATION_NAMES))

PDFLATEX = pdflatex -synctex=1 -file-line-error -interaction=nonstopmode -shell-escape -output-directory=$(AUX_DIR) $< \
 | grep ".*:[0-9]*:.*"; exit $${PIPESTATUS[0]}

.PHONY: all
all: $(PRESENTATIONS)

$(PRESENTATIONS): $(OUT_DIR)/%.pdf : $(SRC_DIR)/%.tex
	echo "==>" $@
	mkdir -p $(AUX_DIR)
	$(call PDFLATEX)
	$(call PDFLATEX)
	mkdir -p $(OUT_DIR)
	mv $(AUX_DIR)/$(notdir $@) $(OUT_DIR)

.PHONY: clean
clean:
	@echo "==>" $@
	@rm -rf $(AUX_DIR)/*
	@rm -rf $(OUT_DIR)/*
