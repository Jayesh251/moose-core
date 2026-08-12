# Installing MOOSE + JARDesigner with Docker

This is the fastest way to get both MOOSE (the simulator) and JARDesigner
(the model-building web GUI) running, with zero Python setup on your own
machine. Docker packages MOOSE, JARDesigner, and every dependency they need
into one self-contained image that runs identically on Windows, macOS, and
Linux.

The `Dockerfile` and `start.sh` used to build this image live in
[`docker/`](../../../docker) at the root of this repository.

## Step 1: Install Docker

Docker itself needs to be installed once per machine.

### Windows

1. Download **Docker Desktop for Windows** from
   <https://www.docker.com/products/docker-desktop/> and run the installer.
2. Keep **"Use WSL 2 instead of Hyper-V"** checked (the default on modern
   Windows 10/11) — Docker containers are Linux containers under the hood,
   and WSL 2 is the lightweight Linux environment Docker runs them in.
3. Restart if prompted, then launch **Docker Desktop** and wait for the
   whale icon in the system tray to show "Docker Desktop is running".

### macOS

1. Download **Docker Desktop for Mac** from
   <https://www.docker.com/products/docker-desktop/> — choose **Apple
   Silicon** for M1/M2/M3 Macs, or **Intel chip** for older ones.
2. Open the `.dmg` and drag Docker into Applications.
3. Launch it and wait for the whale icon in the menu bar.

### Linux

```bash
sudo apt-get update
sudo apt-get install -y docker.io
sudo systemctl enable --now docker
sudo usermod -aG docker $USER
```

Log out and back in after the last command, so your user account can run
Docker without `sudo`. For non-Debian distributions, see
<https://docs.docker.com/engine/install/>.

**Verify the install (any OS):**

```bash
docker --version
```

## Step 2: Run MOOSE + JARDesigner

```bash
docker run -d \
  --name moose-jardesigner \
  -p 8888:8888 \
  -p 5000:5000 \
  -v moose_workspace:/workspace \
  -v jardesigner_data:/root/.local/share/jardesigner \
  jayesh8050/moose-jardesigner:latest
```

**What each part does:**

| Part | Meaning |
|---|---|
| `docker run` | Start a new container (a running copy of the image). |
| `-d` | Run in the background ("detached"), so the terminal stays free. |
| `--name moose-jardesigner` | A friendly name for the container, so you can refer to it later (`docker stop moose-jardesigner`) instead of a random ID. |
| `-p 8888:8888` | Maps port 8888 on your machine to port 8888 inside the container — this serves JupyterLab. Format is `host:container`. |
| `-p 5000:5000` | Same idea, for port 5000 — this serves JARDesigner. |
| `-v moose_workspace:/workspace` | A named, persistent storage volume connected to `/workspace` inside the container. Notebooks/scripts you save there survive even if the container is later removed. |
| `-v jardesigner_data:/root/.local/share/jardesigner` | Same idea, for JARDesigner's saved projects and uploads. |
| `jayesh8050/moose-jardesigner:latest` | The image to run. Downloaded automatically from Docker Hub the first time, then reused. |

The first run downloads the image (a few hundred MB); after that,
starting/stopping is instant.

## Step 3: Open it in your browser

- **JupyterLab** (for MOOSE Python scripts): <http://localhost:8888>
- **JARDesigner** (model-building GUI): <http://localhost:5000>

## Running only one of the two

Add the mode as the last word on the command:

```bash
# Only MOOSE + JupyterLab
docker run -d --name moose-jardesigner -p 8888:8888 \
  -v moose_workspace:/workspace \
  jayesh8050/moose-jardesigner:latest moose

# Only JARDesigner
docker run -d --name moose-jardesigner -p 5000:5000 \
  -v jardesigner_data:/root/.local/share/jardesigner \
  jayesh8050/moose-jardesigner:latest jardesigner
```

## Stopping, restarting, and removing

```bash
docker stop moose-jardesigner    # stops the container, keeps your data
docker start moose-jardesigner   # starts it again later
docker rm -f moose-jardesigner   # removes the container (data is kept in the named volumes)
```

Your notebooks and JARDesigner projects live in the named volumes
(`moose_workspace`, `jardesigner_data`), not inside the container itself, so
they survive `stop`/`start`/`rm`. To start fresh again later, re-run the
`docker run` command from Step 2 with the same volume names.

## Troubleshooting

**"port is already allocated"** — something is already using port 8888 or
5000 (perhaps another copy of this container is still running). Run
`docker rm -f moose-jardesigner`, then re-run the command from Step 2. To
run two copies at once instead, change the *first* number in a `-p` flag,
e.g. `-p 8889:8888`, and open `http://localhost:8889` instead.

**"container name already in use"** — a container with that name already
exists, even if stopped (Docker keeps the name reserved until removed). Run
`docker rm -f moose-jardesigner`, then re-run.

**Nothing loads in the browser** — wait 10-15 seconds after starting for
JupyterLab/JARDesigner to boot inside the container, then check
`docker logs moose-jardesigner` for startup errors.
