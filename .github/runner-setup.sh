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
# 3. Registers the runner with the repo (requires RUNNER_TOKEN)
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
# b) Get a runner registration token:
#    GitHub → Settings → Actions → Runners → New self-hosted runner
#    Copy the token shown in step 3 of the instructions page.
#
# USAGE
# ─────
# export GITHUB_OWNER=skyne
# export GITHUB_REPO=idklol-client
# export RUNNER_TOKEN=<token-from-github>
# sudo bash .github/runner-setup.sh

set -euo pipefail

RUNNER_USER="ghrunner"
RUNNER_HOME="/home/$RUNNER_USER"
RUNNER_DIR="$RUNNER_HOME/actions-runner"
RUNNER_VERSION="2.321.0"   # https://github.com/actions/runner/releases

: "${GITHUB_OWNER:?Must set GITHUB_OWNER}"
: "${GITHUB_REPO:?Must set GITHUB_REPO}"
: "${RUNNER_TOKEN:?Must set RUNNER_TOKEN}"

echo "═══════════════════════════════════════════"
echo " UE5 CI Runner Setup"
echo " Repo : https://github.com/$GITHUB_OWNER/$GITHUB_REPO"
echo " User : $RUNNER_USER"
echo "═══════════════════════════════════════════"

# ── 1. System packages ────────────────────────────────────────────────────────
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential \
    clang-15 \
    lld-15 \
    cmake \
    ninja-build \
    libssl-dev \
    libcurl4-openssl-dev \
    libc++-dev \
    libc++abi-dev \
    ca-certificates \
    curl \
    git \
    git-lfs \
    jq \
    unzip \
    zip \
    libvulkan1 \
    libvulkan-dev \
    mesa-vulkan-drivers \
    # Docker
    docker.io

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

# ── 4. Register runner with GitHub ────────────────────────────────────────────
sudo -u "$RUNNER_USER" "$RUNNER_DIR/config.sh" \
    --url "https://github.com/$GITHUB_OWNER/$GITHUB_REPO" \
    --token "$RUNNER_TOKEN" \
    --name "$(hostname)-ue5-linux" \
    --labels "self-hosted,Linux,X64,ue5-linux" \
    --work "$RUNNER_DIR/_work" \
    --unattended \
    --replace

# ── 5. Install & start as systemd service ────────────────────────────────────
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
