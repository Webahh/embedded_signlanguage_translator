MAIN = main
OUTDIR = out

LATEX = pdflatex
LATEXFLAGS = -interaction=nonstopmode -halt-on-error --shell-escape -output-directory=$(OUTDIR)

BIBER = biber

.PHONY: all quick clean watch

all: $(OUTDIR)/$(MAIN).pdf

$(OUTDIR)/$(MAIN).pdf: $(MAIN).tex meta.tex styling/custom.sty
	@mkdir -p $(OUTDIR)
	$(LATEX) $(LATEXFLAGS) $(MAIN)
	$(BIBER) $(OUTDIR)/$(MAIN)
	$(LATEX) $(LATEXFLAGS) $(MAIN)
	$(LATEX) $(LATEXFLAGS) $(MAIN)
	@echo "Build complete: $(OUTDIR)/$(MAIN).pdf"
	@okular --unique $(OUTDIR)/$(MAIN).pdf &>/dev/null &

clean:
	rm -rf $(OUTDIR)

watch:
	latexmk -pdf -pvc -output-directory=$(OUTDIR) $(MAIN)
