# https://github.com/mkaczanowski/packer-builder-arm
#
# HCL (HashiCorp Configuration Language)
#
variable "target_username" {
  type    = string
  default = "nhilton"
}

variable "ssh_public_key" {
  type    = string
  default = "ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABgQC68cYPmB0ZSEyVoKGqnDd8LqGWTKHbDNxxyXZYSgDJB1Ot3sJnjk6+bXYl+qIU+rXxCUe/aPcucWtbhy1YCGXYbs3tNkFp3uAv7R/zipDQHFnYDa/KdQzEvGlH5J8guQRvdtWpYFAXPdhxupg61/73XXJVZd7ytoge+4RuQOqCktTUSGFyhUF3PClDiJpVHhXnSIfy/457ff998kLlsdTRZMq+f4LtuUV7yOabMfBDOmkQnf9ou2d7RA48T6mpmgsaZp6fpUBWMueqTH3x3Lpa/1r4il+VDjvG2ji+tK/q+oPvxinx8reGv2/9eL5BWuxVI1YB1i1j4+JtsLlhViCPZTSMjCcU+ynJvKNT97rNNweBLpUxnpSKWBWDmi33GgGpuEEwaBsoYy0W/4g4rAP3DgIoX6bKN5+osgpfMXVNGyH1wlp2mGmuwFqr6ht4SDLQSwhDrDeiMdgVRbMg1Ji69x/SdFaA0kDnazqlDK+Bh5+9w5P/M+/dvIOjtSbWQm0= nhilton@quadegg"
}

locals {
  # Generates a string like: 2026-05-18_191700
  timestamp = formatdate("YYYY-MM-DD_hhmmss", timestamp())
}

source "arm" "pi_os" {
  file_urls             = ["https://downloads.raspberrypi.com/raspios_lite_arm64/images/raspios_lite_arm64-2023-12-11/2023-12-11-raspios-bookworm-arm64-lite.img.xz"]
  file_checksum         = "sha256:7b520cb2ce20d367468bbdf104677f502ff228a474c1070e304f57cbbd25f75e"
  file_target_extension = "xz"
  file_unarchive_cmd    = ["xz", "--decompress", "$ARCHIVE_PATH"]
  image_build_method    = "resize"
  image_path            = "pi-os-pycontrol-${local.timestamp}.img"
  image_size            = "8G"
  image_type            = "dos"

  # Boot Partition Definition
  image_partitions {
    name         = "boot"
    type         = "c"
    start_sector = "8192"
    filesystem   = "fat"
    size         = "512M"
    mountpoint   = "/boot/firmware"
  }

  # Root Partition Definition
  image_partitions {
    name         = "root"
    type         = "83"
    start_sector = "1056768"
    filesystem   = "ext4"
    size         = "0"
    mountpoint   = "/"
  }

  image_chroot_env             = ["PATH=/usr/local/bin:/usr/local/sbin:/usr/bin:/usr/sbin:/bin:/sbin"]
  qemu_binary_source_path      = "/usr/bin/qemu-aarch64-static"
  qemu_binary_destination_path = "/usr/bin/qemu-aarch64-static"
}

build {
  sources = ["source.arm.pi_os"]

  # Add your user account and ssh key.
  provisioner "shell" {
    environment_vars = [
      "NEW_USER=${var.target_username}",
      "SSH_KEY=${var.ssh_public_key}"
    ]
    inline = [
      "set -ex",

      # 0. Purge the dfault user.
      "apt-get purge -y userconf-pi piwiz",

      # 1. Create the system user with a home directory and bash shell
      "useradd -m -s /bin/bash $NEW_USER",

      # 2. Add the user to the 'sudo' group so they have root privileges
      "usermod -aG sudo,plugdev,video,dialout,gpio,i2c,spi $NEW_USER",

      # 3. Configure passwordless sudo for this user (standard for headless setups)
      "echo \"$NEW_USER ALL=(ALL) NOPASSWD:ALL\" > /etc/sudoers.d/$NEW_USER",
      "chmod 0440 /etc/sudoers.d/$NEW_USER",

      # 4. Set up the secure .ssh directory inside the new user's home folder
      "mkdir -p /home/$NEW_USER/.ssh",
      "echo \"$SSH_KEY\" > /home/$NEW_USER/.ssh/authorized_keys",

      # 5. Lock down permissions so SSH doesn't reject the keys due to lax security
      "chmod 700 /home/$NEW_USER/.ssh",
      "chmod 600 /home/$NEW_USER/.ssh/authorized_keys",
      "chown -R $NEW_USER:$NEW_USER /home/$NEW_USER/.ssh"
    ]
  }

  # Install build tools and required packages.
  provisioner "shell" {
    inline = [
      "set -ex",
      "echo 'pycontrol' > /etc/hostname",
      "sed -i 's/127.0.1.1.*/127.0.1.1\tpycontrol/g' /etc/hosts",
      "apt-get -o Acquire::Retries=3 update",
      "apt-get -o Acquire::Retries=3 install -y \\",
        "autoconf \\",
        "automake \\",
        "autopoint \\",
        "avahi-daemon \\",
        "chrony \\",
        "cmake \\",
        "dnsmasq \\",
        "g++ \\",
        "gettext \\",
        "git \\",
        "gphoto2 \\",
        "gpsd \\",
        "gpsd-tools \\",
        "hostapd \\",
        "libbenchmark-dev \\",
        "libboost-dev \\",
        "libcurl4-openssl-dev \\",
        "libgd-dev \\",
        "libgettextpo-dev \\",
        "libgphoto2-6 \\",
        "libgphoto2-dev \\",
        "libgtest-dev \\",
        "libltdl-dev \\",
        "libpopt-dev \\",
        "libpopt0 \\",
        "libprotobuf-dev \\",
        "libusb-1.0-0-dev \\",
        "libxml2-dev \\",
        "make \\",
        "ninja-build \\",
        "openssh-server \\",
        "protobuf-compiler \\",
        "python-is-python3 \\",
        "python3-dev \\",
        "python3.11-minimal \\",
        "tcpdump \\",
        "",
      "systemctl enable ssh",
    ]
  }

  # Disable auto resizing of the partitions.
  # If you later want to use any extra space on your micro SD card, run this:
  #     sudo raspi-config --expand-rootfs
  provisioner "shell" {
      inline = [
          "set -ex",
          "mv /usr/lib/raspberrypi-sys-mods/firstboot /usr/lib/raspberrypi-sys-mods/first-boot",
          "sed -i 's|firstboot|first-boot|g' /usr/lib/raspberrypi-sys-mods/first-boot",
          "[ -f /boot/cmdline.txt ] && sed -i 's|/firstboot|/first-boot|' /boot/cmdline.txt || true",
          "[ -f /boot/firmware/cmdline.txt ] && sed -i 's|/firstboot|/first-boot|' /boot/firmware/cmdline.txt || true",

          # Seed fake-hwclock so the Pi boots with a recent date instead of 1970
          "date -u '+%Y-%m-%d %H:%M:%S' > /etc/fake-hwclock.data",
      ]
  }

  # Clean so base image stays lightweight.
  provisioner "shell" {
    inline = [
      "set -ex",
      "apt-get autoremove -y",
      "apt-get clean"
    ]
  }
}
