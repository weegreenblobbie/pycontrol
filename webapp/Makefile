.PHONY: all
all: venv

venv: requirements.txt
	rm -rf ./venv
	python3 -m venv venv
	. venv/bin/activate; \
	    python -m pip install --upgrade pip; \
		pip install -r requirements.txt

clean:
	@echo cleaning

real-clean:
	rm -rf ./venv

# :mode=Makefile: