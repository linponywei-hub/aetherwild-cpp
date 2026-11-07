#!/bin/sh
set -eu
data_dir="${AETHERWILD_DATA_DIR:-/app/runtime-data}"
case "$data_dir" in
  /app/runtime-data|/var/data) ;;
  *) echo 'Unsupported save directory' >&2; exit 1 ;;
esac
mkdir -p "$data_dir"
chown 10001:10001 "$data_dir"
chmod 700 "$data_dir"
exec setpriv --no-new-privs --reuid=10001 --regid=10001 --clear-groups "$@"
