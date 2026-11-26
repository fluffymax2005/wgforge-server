#!/bin/sh

# .rpm based packages

set -e

WGFORGE_GROUP="wgforge"
CONF_DIR="/etc/wgforge"
DB_DIR="/var/lib/wgforge"

if [ "$1" -ge 1 ]; then
    if ! getent group "$WGFORGE_GROUP" >/dev/null; then
        groupadd -r "$WGFORGE_GROUP"
    fi

    mkdir -p "$CONF_DIR" "$DB_DIR"
    chown -R root:"$WGFORGE_GROUP" "$CONF_DIR" "$DB_DIR"
    chmod -R 2770 "$CONF_DIR" "$DB_DIR"

    echo "================================================================="
    echo " wgforge setup complete."
    echo " To grant access to a user, add them to the '$WGFORGE_GROUP' group:"
    echo "   sudo usermod -aG $WGFORGE_GROUP <username>"
    echo "================================================================="
fi