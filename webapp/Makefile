include ../config.mk

release: venv
all: release

.venv.stamp: requirements.txt
	@echo "$(BOLD)$(BLUE)Building$(RESET) webapp/venv"
	$(SILENT)rm -rf ./venv && \
	python3 -m venv venv $(DEVNULL) && \
	. venv/bin/activate $(DEVNULL) && \
	    python -m pip install --upgrade pip $(DEVNULL) && \
		pip install -r requirements.txt $(DEVNULL) && \
		touch .venv.stamp

venv: .venv.stamp

.playwright.stamp: venv
	@echo "$(BOLD)$(BLUE)Installing Playwright$(RESET)"
	$(SILENT) . venv/bin/activate $(DEVNULL) && \
	playwright install && \
	playwright install-deps && \
	touch .playwright.stamp

playwright: .playwright.stamp

.PHONY: clean real-clean coverage
clean:
	@echo "$(BOLD)Cleaning$(RESET) $(shell pwd)"
	$(SILENT)rm -rf ./venv .venv.stamp .playwright.stamp htmlcov .coverage

real-clean: clean

test: venv playwright
	sudo echo "Warming up password in case we need to install playwright"
	cd ..; webapp/venv/bin/pytest --tb=short webapp

coverage: venv playwright
	sudo echo "Warming up password in case we need to install playwright"
	cd ..; webapp/venv/bin/pytest --tb=short \
		--cov=webapp \
		--cov-report=html:webapp/htmlcov \
		webapp

# :mode=Makefile:
