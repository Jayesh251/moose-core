#!/bin/bash
set -e

MODE="${1:-both}"

start_moose() {
    echo ""
    echo "=================================================="
    echo "  MOOSE + JupyterLab is starting..."
    echo "  Open your browser and go to: http://localhost:8888"
    echo "=================================================="
    echo ""
    exec jupyter lab --ip=0.0.0.0 --port=8888 --no-browser --allow-root --IdentityProvider.token=
}

start_jardesigner() {
    echo ""
    echo "=================================================="
    echo "  JARDesigner is starting..."
    echo "  Open your browser and go to: http://localhost:5000"
    echo "=================================================="
    echo ""
    exec jardesigner --no-browser --host 0.0.0.0 --port 5000
}

start_both() {
    echo ""
    echo "=================================================="
    echo "  MOOSE + JupyterLab + JARDesigner is starting..."
    echo "  JupyterLab:  http://localhost:8888"
    echo "  JARDesigner: http://localhost:5000"
    echo "=================================================="
    echo ""
    jupyter lab --ip=0.0.0.0 --port=8888 --no-browser --allow-root --IdentityProvider.token= &
    jardesigner --no-browser --host 0.0.0.0 --port 5000 &
    wait -n
}

case "$MODE" in
    moose)
        start_moose
        ;;
    jardesigner)
        start_jardesigner
        ;;
    both)
        start_both
        ;;
    *)
        echo "Unknown mode: '$MODE'"
        echo "Usage: docker run <image> [moose|jardesigner|both]"
        exit 1
        ;;
esac
