savedcmd_pcd_device_setup.mod := printf '%s\n'   pcd_device_setup.o | awk '!x[$$0]++ { print("./"$$0) }' > pcd_device_setup.mod
