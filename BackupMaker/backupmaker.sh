#!/bin/bash

mkdir -p /home/usr/backup
mkdir -p /var/log/mylogs

log_dir="var/log/mylogs"

file_name="backup_$(date + '%Y-%m-%d').tar.gz"

log_name="backup_$(date + '%Y-%m-%d').log"

tar -czvf /home/usr/backups/"$file_name" "$@" > "$log_dir/$log_name"
