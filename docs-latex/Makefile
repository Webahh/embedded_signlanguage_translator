MAIN = main
OUTDIR = out

LATEX = pdflatex
LATEXFLAGS = -interaction=nonstopmode -halt-on-error --shell-escape -output-directory=$(OUTDIR)

BIBER = biber
MAKEGLOSSARIES = makeglossaries

.PHONY: all quick clean watch

all: $(OUTDIR)/$(MAIN).pdf

$(OUTDIR)/$(MAIN).pdf: $(MAIN).tex meta.tex styling/custom.sty
	@mkdir -p $(OUTDIR)
	$(LATEX) $(LATEXFLAGS) $(MAIN)
	$(BIBER) $(OUTDIR)/$(MAIN)
	cd $(OUTDIR) && $(MAKEGLOSSARIES) $(MAIN)
	$(LATEX) $(LATEXFLAGS) $(MAIN)
	@echo "Build complete: $(OUTDIR)/$(MAIN).pdf"
	@xdg-open $(OUTDIR)/$(MAIN).pdf

clean:
	rm -rf $(OUTDIR)

watch:
	latexmk -pdf -pvc -output-directory=$(OUTDIR) $(MAIN)
