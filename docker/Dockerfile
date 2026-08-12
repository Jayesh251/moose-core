FROM python:3.13

LABEL maintainer="MooseNeuro <https://github.com/MooseNeuro>"
LABEL description="MOOSE (Multiscale Object-Oriented Simulation Environment) with JupyterLab and JARDesigner"

ENV QT_QPA_PLATFORM=offscreen \
    PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1

# --- MOOSE runtime dependencies ---
# These are the shared libraries pymoose's prebuilt wheel needs at runtime. No compiler
# or build tools are required here since MOOSE is installed from a wheel, not built from
# source.
RUN apt-get update && apt-get install -y --no-install-recommends \
        libgsl28 \
        libhdf5-310 \
        libgl1 \
        libglib2.0-0 \
    && rm -rf /var/lib/apt/lists/*

# --- MOOSE + JupyterLab ---
RUN pip install --no-cache-dir pymoose==4.3.0 jupyterlab

RUN python -c "import moose; print('MOOSE version:', moose.__version__)"

# --- JARDesigner ---
# jardesigner's own packaging lists pymoose as a dependency, but it is already installed
# above. --no-deps avoids pip reinstalling/upgrading it and pulling in a large, unrelated
# dependency chain a second time for no reason.
RUN pip install --no-cache-dir --no-deps jardesigner \
    && pip install --no-cache-dir \
        Flask \
        flask-cors \
        Flask-SocketIO \
        jsonschema \
        lxml \
        platformdirs \
        requests \
        urllib3 \
        Werkzeug \
        gevent \
        gevent-websocket

RUN which jardesigner

WORKDIR /workspace

COPY start.sh /start.sh
RUN chmod +x /start.sh

# Persist user-facing data across container restarts and removals:
#   /workspace                        - Jupyter notebooks and scripts
#   /root/.local/share/jardesigner    - jardesigner uploads, saved projects, user data
# Declaring these as VOLUME is a safety net: even if a user forgets to mount a volume
# explicitly at `docker run` time, Docker still preserves this data in a managed volume
# instead of silently discarding it when the container is removed.
VOLUME ["/workspace", "/root/.local/share/jardesigner"]

EXPOSE 8888 5000

ENTRYPOINT ["/start.sh"]
CMD ["both"]
