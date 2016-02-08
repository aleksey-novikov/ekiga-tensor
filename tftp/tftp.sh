#!/bin/bash

CONF="/var/lib/tftp.conf"

tftp_config() {
  rm -f "$CONF"
  [ -n "$new_tftp_server_name" ] && echo "TFTP_SERVER_NAME=\"$new_tftp_server_name\"" > "$CONF"
}

tftp_restore() {
  rm -f "$CONF"
}
