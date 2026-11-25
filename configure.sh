#!/bin/bash

set -e

WGFORGE_GROUP="wgforge"
CONF_DIR="/etc/wgforge"
DB_DIR="/var/lib/wgforge"

if [ "$(id -u)" -ne 0 ]; then
    echo "Execute script as root or use sudo" >&2
    exit 1
fi

# Define origin user
if [ -n "${SUDO_USER:-}" ] && [ "$SUDO_USER" != "root" ]; then
    ORIGIN_USER="$SUDO_USER"
elif [ $# -ge 1 ]; then
    ORIGIN_USER="$1"
else
    echo "Use: $0 <username> (or execute via sudo)" >&2
    exit 1
fi

if ! id "$ORIGIN_USER" >/dev/null 2>&1; then
    echo "User: '$ORIGIN_USER' does not exist" >&2
    exit 1
fi

# Create dirs
mkdir -p "$CONF_DIR" "$DB_DIR"

# Create group
if ! getent group "$WGFORGE_GROUP" >/dev/null; then
    groupadd $WGFORGE_GROUP
fi
usermod -aG $WGFORGE_GROUP $ORIGIN_USER

# Set permissions and owners
chmod -R u+rwX,g+rwX,o-rwx "$CONF_DIR" "$DB_DIR"
chown -R "root:$WGFORGE_GROUP" "$CONF_DIR" "$DB_DIR"
