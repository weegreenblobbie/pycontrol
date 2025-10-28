include ../config.mk

release: .venv.stamp .playwright.stamp
all: release

.venv.stamp: requirements.txt
	@echo "$(BOLD)$(BLUE)Building$(RESET) webapp/venv"
	$(SILENT)rm -rf ./venv && \
	python3 -m venv venv $(DEVNULL) && \
	. venv/bin/activate $(DEVNULL) && \
	    python -m pip install --upgrade pip $(DEVNULL) && \
		pip install -r requirements.txt $(DEVNULL) && \
		touch .venv.stamp

.playwright.stamp: .venv.stamp
	@echo "$(BOLD)$(BLUE)Installing Playwright$(RESET)"
	$(SILENT) . venv/bin/activate $(DEVNULL) && \
	playwright install && \
	playwright install-deps && \
	touch .playwright.stamp

.PHONY: clean real-clean
clean:
	@echo "$(BOLD)Cleaning$(RESET) $(shell pwd)"
	$(SILENT)rm -rf ./venv .venv.stamp .playwright.stamp

real-clean: clean

test: .venv.stamp .playwright.stamp
	cd ..; webapp/venv/bin/pytest --tb=short webapp

# :mode=Makefile:
