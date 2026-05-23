include ../config.mk

all:

clean:

.PHONY: os-img run-ansible

os-img:
	sudo echo "Warming up password" && \
	sudo rm -f pi-os-pycontrol-*img && \
	podman run --rm --privileged \
	    -v ${PWD}:/build \
	    -w /build \
	    docker.io/mkaczanowski/packer-builder-arm:latest init build.pkr.hcl && \
	sudo podman run --name pi-builder --rm --privileged \
	    -v /dev:/dev \
	    -v ${PWD}:/build \
	    -w /build \
	    docker.io/mkaczanowski/packer-builder-arm:latest build build.pkr.hcl && \
	sudo chown $(USER):$(USER) pi-os-pycontrol-*img && \
	ls pi-os-pycontrol-*img >/dev/null 2>&1 && \
	echo "SUCCESS"

flash-os-img:
	sudo echo "Warming up password" && \
	@image=$$(ls pi-os-pycontrol-*.img 2>/dev/null | sort -r | head -n 1); \
	if [ -z "$$image" ]; then \
		echo "❌ ERROR: No image file found!  Run make os-img first!"; \
		exit 1; \
	fi; \
	read -p "Enter target device (e.g., /dev/sdb): " dev; \
	lsblk $$dev ; \
	echo "======================================================"; \
	echo "🚨 WARNING: ALL DATA ON $$dev WILL BE DESTROYED! 🚨"; \
	echo "======================================================"; \
	read -p "Type 'YES' to confirm and continue: " confirm; \
	if [ "$$confirm" = "YES" ]; then \
	    sudo umount $${dev}* 2>/dev/null || true; \
		sudo dd if=$$image of=$$dev bs=4M status=progress conv=fsync; \
		sync; \
		echo "✅ Flash complete and synced. Safe to unplug!"; \
	else \
		echo "Flash operation aborted."; \
	fi

ansible/.venv.stamp: ansible/requirements.txt
	@echo "$(BOLD)$(BLUE)Building$(RESET) ansible/venv"
	cd ansible && \
	$(SILENT)rm -rf ./venv && \
	python3 -m venv venv $(DEVNULL) && \
	. venv/bin/activate $(DEVNULL) && \
	    python -m pip install --upgrade pip $(DEVNULL) && \
		pip install -r requirements.txt $(DEVNULL) && \
		touch .venv.stamp


run-ansible: ansible/.venv.stamp
	@echo "$(BOLD)$(RED)Runnding Ansible$(RESET)"
	cd ansible && \
	. venv/bin/activate && \
	ansible-playbook -i inventory.ini playbook.yaml --limit pycontrol
