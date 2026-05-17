Building a raspberry-pi-4 OS image with packer-builder-arm
==========================================================

A more custom and time effective approach to creating a raspberry pi OS image is
using the [packer-builder-arm](https://github.com/mkaczanowski/packer-builder-arm) tool.

WARNING: Using the docker container this way exposes your entire `/dev` device tree to the
docker image, so please verify you're using an official, non-malicous image.

# Pull the image
```
podman pull docker.io/mkaczanowski/packer-builder-arm:latest
```

# Verify it's a legitimate image with cosign

Install and verify cosign.

```
curl -O -L "https://github.com/sigstore/cosign/releases/latest/download/cosign-linux-amd64"
./cosign-linux-amd64 version

./cosign-linux-amd64 version
  ______   ______        _______. __    _______ .__   __.
 /      | /  __  \      /       ||  |  /  _____||  \ |  |
|  ,----'|  |  |  |    |   (----`|  | |  |  __  |   \|  |
|  |     |  |  |  |     \   \    |  | |  | |_ | |  . `  |
|  `----.|  `--'  | .----)   |   |  | |  |__| | |  |\   |
 \______| \______/  |_______/    |__|  \______| |__| \__|
cosign: A tool for Container Signing, Verification and Storage in an OCI registry.

GitVersion:    v3.0.6
GitCommit:     f1ad3ee952313be5d74a49d67ba0aa8d0d5e351f    # <-----------------------
GitTreeState:  clean                                       # <-----------------------
BuildDate:     2026-04-06T21:39:58Z
GoVersion:     go1.25.7
Compiler:      gc
Platform:      linux/amd64
```

Take the commit and [view it on github](https://github.com/sigstore/cosign/commit/f1ad3ee952313be5d74a49d67ba0aa8d0d5e351f).

Verify the commit and date are legit. This isn't foolproof but a pretty good paper trail. Verify the tool you use to
verify your docker image! :)

# Use cosign to verify the `packer-build-arm image`

```
./cosign-linux-amd64 verify docker.io/mkaczanowski/packer-builder-arm:latest
```

Hmmm, as of today (2026-05-17), the image is not signed.  I did manually inspect
the build layers with `podman history --no-trunc docker.io/mkaczanowski/packer-builder-arm:latest`
which appear to be 2 years old.  So I'm accepting a risk here by using the
container.  If it were malicious, probably would have been deleted from
docker.io by now.  I've opened [an issue](https://github.com/mkaczanowski/packer-builder-arm/issues/369)
here to ask for signatures.

# Configure the username and ssh public key

You'll want to configure your username and ssh public key for the RPI image.  Update the top of `build.pkr.hcl` with
your username and ssh key.  I recomment creating an ssh key specifically for access your raspberry pi.
```
variable "target_username" {
  type    = string
  default = "nhilton"
}

variable "ssh_public_key" {
  type    = string
  default = "ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABgQC68cYPmB0ZSEyVoKGqnDd8LqGWTKHbDNxxyXZYSgDJB1Ot3sJnjk6+bXYl+qIU+rXxCUe/aPcucWtbhy1YCGXYbs3tNkFp3uAv7R/zipDQHFnYDa/KdQzEvGlH5J8guQRvdtWpYFAXPdhxupg61/73XXJVZd7ytoge+4RuQOqCktTUSGFyhUF3PClDiJpVHhXnSIfy/457ff998kLlsdTRZMq+f4LtuUV7yOabMfBDOmkQnf9ou2d7RA48T6mpmgsaZp6fpUBWMueqTH3x3Lpa/1r4il+VDjvG2ji+tK/q+oPvxinx8reGv2/9eL5BWuxVI1YB1i1j4+JtsLlhViCPZTSMjCcU+ynJvKNT97rNNweBLpUxnpSKWBWDmi33GgGpuEEwaBsoYy0W/4g4rAP3DgIoX6bKN5+osgpfMXVNGyH1wlp2mGmuwFqr6ht4SDLQSwhDrDeiMdgVRbMg1Ji69x/SdFaA0kDnazqlDK+Bh5+9w5P/M+/dvIOjtSbWQm0= nhilton@quadegg"
}
```

# Build the image

The `build.pkr.hcl` file instructs `packer` what do do.  Inspect it, then you
we're ready to build the os image do:

```
make os-img
```

The build takes a long time, as the virutal arm machine must download all the packages needed over the
internet, even if the build fails, all the temporary files are discarded.

After the build completes, you'll have a fresh rpi-os image named pi-os-pycontrol-TIMESTAMP.img.  Next,
you can flash that do a microSD card with:
```
make flash-os-img
```

You'll be prompted to enter the device to write to with `dd`, it's important you can reconize the
drive so you don't accidentally overwrite an important disk on your computer!

BE CAREFUL!  YOU'VE BEEN WARNED!

# Boot the fresh image

The first time the pi boots up, it generates host ssh keys to uniquily idnetify the host on your
network, you probably have seen this before:

```
ssh pycontrol.local
The authenticity of host 'pycontrol.local (192.168.1.249)' can't be established.
ED25519 key fingerprint is SHA256:l+M83f5+ONiv4rDsE8CcgAf5DvV20RoezViToHMzsFY.
This key is not known by any other names.
Are you sure you want to continue connecting (yes/no/[fingerprint])?
```

After generaging new host keys, it reboots.  From then on, this firstboot script is never
executed again.

# Run ansible to configure your fresh PI

Now that you've got a new raspberry pi on your network, try to ssh into to verify no password is
needed:

```
ssh pycontrol.local
hilton@pycontrol:~ $
```

Now to run ansbile to apply the final configuration:

```
make run-ansible
```

Happy coding & eclipse chasing!
