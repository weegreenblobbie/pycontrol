include ../config.mk

all: venv

venv: requirements.txt
	@echo "$(BOLD)$(BLUE)Building$(RESET) $@"
	$(SILENT)rm -rf ./venv && \
	python3 -m venv venv $(DEVNULL) && \
	. venv/bin/activate $(DEVNULL) && \
	    python -m pip install --upgrade pip $(DEVNULL) && \
		pip install -r requirements.txt $(DEVNULL)

.PHONY: clean real-clen
clean:
	@echo "$(BOLD)Cleaning$(RESET) $(shell pwd)"

real-clean:
	@echo "$(BOLD)Real Cleaning$(RESET) $(shell pwd)"
	$(SILENT)rm -rf ./venv

test: venv
	@PYTHONPATH=`pwd` venv/bin/pytest

# :mode=Makefile: