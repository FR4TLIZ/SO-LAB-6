Instructions:

1. Run: ./backupmaker.sh
2. Edit Crontab: crontab -e
3. Add line: 0 2 * * * /home/usr/backupmaker.sh /home/usr/logs
4. Verify the installation with: cat /var/log/mylogs/backup_YYYY-MM-DD.log
