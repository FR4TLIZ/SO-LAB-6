Instructions:

1. Run: make
2. Install the module using: sudo insmod keylogger.ko
3. Verify the installation with: lsmod | grep "keylogger" // This checks if the module was properly installed
4. Change directory to /tmp: cd /tmp
5. Compare the output with the top command using: top
6. List the contents of the directory with: ls
7. Read the keylog file using: cat keylog
8. Remove the module using: sudo rmmod keylogger
9. Verify the removal with: lsmod | grep "keylogger" // This checks if the module was properly uninstalled
10. Clean up with: make clean
