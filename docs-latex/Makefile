MAIN = main
OUTDIR = out

LATEX = pdflatex
LATEXFLAGS = -interaction=nonstopmode -halt-on-error --shell-escape -output-directory=$(OUTDIR)

BIBER = biber
MAKEGLOSSARIES = makeglossaries

.PHONY: all quick clean watch

all: $(OUTDIR)/$(MAIN).pdf

$(OUTDIR)/$(MAIN).pdf: $(MAIN).tex meta.tex
	@mkdir -p $(OUTDIR)
	$(LATEX) $(LATEXFLAGS) $(MAIN)
	$(BIBER) $(OUTDIR)/$(MAIN)
	$(MAKEGLOSSARIES) $(OUTDIR)/$(MAIN)
	$(LATEX) $(LATEXFLAGS) $(MAIN)
	$(LATEX) $(LATEXFLAGS) $(MAIN)
	@echo "Build complete: $(OUTDIR)/$(MAIN).pdf"

quick:
	@mkdir -p $(OUTDIR)
	$(LATEX) $(LATEXFLAGS) $(MAIN)

clean:
	rm -rf $(OUTDIR)

watch:
	latexmk -pdf -pvc -output-directory=$(OUTDIR) $(MAIN)
