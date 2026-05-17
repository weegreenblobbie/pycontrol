# Docker image for unit testing and gemini-cli

I've been able to build and run the unit tests and invoke gemini-cli using the
docker container built from the `Dockerfile` in this directory.

To build it run `podman build -t gemini-pycontrol-agent .`

This will result in a docker image named `localhost/gemini-pycontrol-agent`.

You can start this container up and get a BASH shell with:
```
docker run -it --rm \
     -e TERM=xterm-256color \
     -v /home/nhilton/development/pycontrol:/workspace \
     -w /workspace \
     localhost/gemini-pycontrol-agent /bin/bash
```

You'll need to update the path to your pycontrol checkout in the command above.
This provides a sandbox for building and running the C++ and Python unit tests.

I built this container so I can invoke the `gemini-cli` tool, using `gemini`
on the command line requires an API key from Google.  If you have such a key
you can launch this container with the API key to easily use the agent like 
this:

```
docker run -it --rm \
     -e GEMINI_API_KEY=******************************* \
     -e TERM=xterm-256color \
     -v /home/nhilton/development/pycontrol:/workspace \
     -w /workspace \
     localhost/gemini-pycontrol-agent
```

