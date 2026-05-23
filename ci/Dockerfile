# Build this container using the open-source podman tool.
#
# podman build -t gemini-pycontrol-agent .
#

FROM node:lts

# Prevent interactive prompts during installation
ENV DEBIAN_FRONTEND=noninteractive

# Install Python and basic utilities
RUN apt-get update && apt-get install -y \
    autopoint \
    build-essential \
    cmake \
    g++ \
    gettext \
    git \
    lcov \
    libcurl4-gnutls-dev \
    libexif-dev \
    libgd-dev \
    libgettextpo-dev \
    libgtest-dev \
    libltdl-dev \
    libpopt-dev \
    libprotobuf-dev \
    libusb-1.0-0-dev \
    libwoff1 \
    libxml2-dev \
    make \
    ninja-build \
    nodejs \
    npm \
    pkg-config \
    protobuf-compiler \
    python3 \
    python3-pip \
    python3-venv \
    && rm -rf /var/lib/apt/lists/*

# Force pip to install meson globally in the container
RUN pip3 install --break-system-packages meson

# Install Playwright browsers AND their system dependencies
RUN npx playwright install --with-deps

# 3. Pre-install the Gemini CLI globally
RUN npm install -g @google/gemini-cli

# Set the working directory
WORKDIR /workspace

# Set the TERM variable to fix the "blinking" issue you saw earlier
ENV TERM=xterm-256color

# Default command to run when the container starts
CMD ["gemini"]

