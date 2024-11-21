Instructions:

Run: make
Install the module using: sudo insmod keylogger.ko
Verify the installation with: lsmod | grep "keylogger" // This checks if the module was properly installed
Change directory to /tmp: cd /tmp
Compare the output with the top command using: top
List the contents of the directory with: ls
Read the keylog file using: cat keylog
Remove the module using: sudo rmmod keylogger
Verify the removal with: lsmod | grep "keylogger" // This checks if the module was properly uninstalled
Clean up with: make clean
