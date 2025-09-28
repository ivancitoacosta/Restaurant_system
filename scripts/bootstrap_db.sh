#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."
APP="${ROOT_DIR}/build/restaurant_app"

if [ ! -x "${APP}" ]; then
    echo "No se encontró el ejecutable en build/. Compile primero (cmake --build build)."
    exit 1
fi

"${APP}" --bootstrap
