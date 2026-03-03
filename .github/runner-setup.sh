#!/usr/bin/env bash
# .github/runner-setup.sh
#
# Self-hosted GitHub Actions runner setup for the UE5 dedicated server pipeline.
#
# REQUIREMENTS
# ────────────
# Hardware:
#   - x86_64 Linux (Ubuntu 22.04 recommended)
#   - 16+ CPU cores (UE compile is heavily parallelised)
#   - 64+ GB RAM
#   - 300+ GB fast storage (SSD/NVMe):
#       ~150 GB  UE engine install
#       ~60  GB  build intermediates per run (cleaned up after)
#       ~20  GB  DerivedDataCache (persists across runs)
#       ~10  GB  Docker image layers
#
# WHAT THIS SCRIPT DOES
# ─────────────────────
# 1. Installs system dependencies (clang, cmake, Docker, etc.)
# 2. Installs the GitHub Actions runner binary
# 3. Registers the runner with the repo (uses RUNNER_TOKEN or fetches via gh CLI)
# 4. Installs and starts the runner as a systemd service
#
# WHAT YOU MUST DO MANUALLY BEFORE RUNNING THIS SCRIPT
# ─────────────────────────────────────────────────────
# a) Install Unreal Engine 5.5 to /opt/unreal-engine/5.5
#    - Preferred: Epic Games Launcher (requires GUI or Wine) or
#      build from source (github.com/EpicGames/UnrealEngine)
#    - After install, set the UE_ROOT repo variable in:
#      GitHub → Settings → Variables → Actions → New variable
#      Name: UE_ROOT   Value: /opt/unreal-engine/5.5
#
# b) Authenticate gh CLI (optional, only needed if RUNNER_TOKEN is not set):
#    gh auth login
#
# USAGE
# ─────
# export GITHUB_OWNER=skyne
# export GITHUB_REPO=idklol-client
# export RUNNER_TOKEN=<token-from-github>   # optional if gh is already authenticated
# sudo bash .github/runner-setup.sh

set -euo pipefail

if [ "$EUID" -ne 0 ]; then
    echo "Run as root (example: sudo -E bash .github/runner-setup.sh)"
    exit 1
fi

RUNNER_USER="ghrunner"
RUNNER_HOME="/home/$RUNNER_USER"
RUNNER_DIR="$RUNNER_HOME/actions-runner"
RUNNER_VERSION="2.321.0"   # https://github.com/actions/runner/releases

: "${GITHUB_OWNER:?Must set GITHUB_OWNER}"
: "${GITHUB_REPO:?Must set GITHUB_REPO}"

echo "═══════════════════════════════════════════"
echo " UE5 CI Runner Setup"
echo " Repo : https://github.com/$GITHUB_OWNER/$GITHUB_REPO"
echo " User : $RUNNER_USER"
echo "═══════════════════════════════════════════"

# ── 1. System packages ────────────────────────────────────────────────────────
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    libssl-dev \
    ca-certificates \
    curl \
    git \
    gh \
    jq \
    unzip \
    zip \
    libvulkan1 \
    libvulkan-dev \
    mesa-vulkan-drivers \
    docker.io

pick_apt_pkg() {
    for pkg in "$@"; do
        if apt-cache show "$pkg" >/dev/null 2>&1; then
            echo "$pkg"
            return 0
        fi
    done
    return 1
}

OPTIONAL_PKGS=()

if CLANG_PKG="$(pick_apt_pkg clang-18 clang-17 clang-16 clang-15 clang)"; then
    OPTIONAL_PKGS+=("$CLANG_PKG")
else
    echo "Warning: no clang package found in configured apt repositories"
fi

if LLD_PKG="$(pick_apt_pkg lld-18 lld-17 lld-16 lld-15 lld)"; then
    OPTIONAL_PKGS+=("$LLD_PKG")
else
    echo "Warning: no lld package found in configured apt repositories"
fi

if CURLDEV_PKG="$(pick_apt_pkg libcurl4-openssl-dev libcurl4t64-openssl-dev)"; then
    OPTIONAL_PKGS+=("$CURLDEV_PKG")
else
    echo "Warning: no libcurl OpenSSL dev package found; continuing"
fi

if LIBCXX_PKG="$(pick_apt_pkg libc++-dev libc++-18-dev libc++-17-dev libc++-16-dev)"; then
    OPTIONAL_PKGS+=("$LIBCXX_PKG")
else
    echo "Warning: no libc++ dev package found; continuing"
fi

if LIBCXXABI_PKG="$(pick_apt_pkg libc++abi-dev libc++abi-18-dev libc++abi-17-dev libc++abi-16-dev)"; then
    OPTIONAL_PKGS+=("$LIBCXXABI_PKG")
else
    echo "Warning: no libc++abi dev package found; continuing"
fi

if GITLFS_PKG="$(pick_apt_pkg git-lfs)"; then
    OPTIONAL_PKGS+=("$GITLFS_PKG")
else
    echo "Warning: git-lfs not found in apt repositories; continuing"
fi

if [ ${#OPTIONAL_PKGS[@]} -gt 0 ]; then
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends "${OPTIONAL_PKGS[@]}"
fi

# ── 2. Runner OS user ─────────────────────────────────────────────────────────
if ! id "$RUNNER_USER" &>/dev/null; then
    useradd -m -s /bin/bash "$RUNNER_USER"
fi
usermod -aG docker "$RUNNER_USER"

# ── 3. Download & extract Actions runner ──────────────────────────────────────
ARCH=$(uname -m)
case "$ARCH" in
    x86_64)  RUNNER_ARCH="x64" ;;
    aarch64) RUNNER_ARCH="arm64" ;;
    *) echo "Unsupported arch: $ARCH"; exit 1 ;;
esac

RUNNER_PKG="actions-runner-linux-${RUNNER_ARCH}-${RUNNER_VERSION}.tar.gz"
RUNNER_URL="https://github.com/actions/runner/releases/download/v${RUNNER_VERSION}/${RUNNER_PKG}"

mkdir -p "$RUNNER_DIR"
cd "$RUNNER_DIR"

if [ ! -f "./run.sh" ]; then
    echo "Downloading runner $RUNNER_VERSION..."
    curl -fsSL -o "$RUNNER_PKG" "$RUNNER_URL"
    tar -xzf "$RUNNER_PKG"
    rm "$RUNNER_PKG"
fi

chown -R "$RUNNER_USER:$RUNNER_USER" "$RUNNER_DIR"

# ── 4. Resolve registration token ────────────────────────────────────────────
if [ -z "${RUNNER_TOKEN:-}" ]; then
    echo "RUNNER_TOKEN not set. Trying to fetch a registration token via gh CLI..."

    GH_RUN_AS=("gh")
    if [ -n "${SUDO_USER:-}" ] && [ "${SUDO_USER}" != "root" ]; then
        GH_RUN_AS=("sudo" "-H" "-u" "$SUDO_USER" "gh")
    fi

    if ! command -v gh >/dev/null 2>&1; then
        echo "gh CLI not found. Install gh or set RUNNER_TOKEN manually."
        exit 1
    fi

    if ! "${GH_RUN_AS[@]}" auth status >/dev/null 2>&1; then
        echo "gh CLI is not authenticated for user '${SUDO_USER:-root}'."
        echo "Run: gh auth login"
        echo "Or set RUNNER_TOKEN manually."
        exit 1
    fi

    RUNNER_TOKEN="$("${GH_RUN_AS[@]}" api -X POST "repos/$GITHUB_OWNER/$GITHUB_REPO/actions/runners/registration-token" --jq .token)"

    if [ -z "$RUNNER_TOKEN" ] || [ "$RUNNER_TOKEN" = "null" ]; then
        echo "Failed to retrieve runner registration token via gh CLI."
        echo "Check repo permissions (admin/actions write) and try again."
        exit 1
    fi

    echo "Fetched registration token via gh CLI."
fi

# ── 5. Register runner with GitHub ────────────────────────────────────────────
sudo -u "$RUNNER_USER" "$RUNNER_DIR/config.sh" \
    --url "https://github.com/$GITHUB_OWNER/$GITHUB_REPO" \
    --token "$RUNNER_TOKEN" \
    --name "$(hostname)-ue5-linux" \
    --labels "self-hosted,Linux,X64,ue5-linux" \
    --work "$RUNNER_DIR/_work" \
    --unattended \
    --replace

# ── 6. Install & start as systemd service ────────────────────────────────────
"$RUNNER_DIR/svc.sh" install "$RUNNER_USER"
"$RUNNER_DIR/svc.sh" start

echo ""
echo "✓ Runner registered and started."
echo ""
echo "Next steps:"
echo "  1. Install UE 5.5 to /opt/unreal-engine/5.5 (if not done)"
echo "  2. Set UE_ROOT repo variable on GitHub:"
echo "     Settings → Variables → Actions → New variable"
echo "     Name: UE_ROOT   Value: /opt/unreal-engine/5.5"
echo "  3. Ensure runner user can read the engine:"
echo "     chown -R ghrunner /opt/unreal-engine"
echo "  4. Test with: Actions → UE Server — Build & Push → Run workflow"
