savedcmd_pcd_platform_driver.mod := printf '%s\n'   pcd_platform_driver.o | awk '!x[$$0]++ { print("./"$$0) }' > pcd_platform_driver.mod
