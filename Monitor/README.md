Instructions:

1. Run: make
2. Install the module using: sudo insmod monitor_cpu_usage.ko
3. Verify the installation with: lsmod | grep "monitor" // This checks if the module was properly installed
4. View relevant logs using: sudo dmesg | grep -i "cpu usage"
5. Compare the output with the top command using: top
6. Remove the module using: sudo rmmod monitor_cpu_usage
7. Verify the removal with: lsmod | grep "monitor" // This checks if the module was properly uninstalled
8. Clean up with: make clean
