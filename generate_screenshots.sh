set -e

rebble build

rebble wipe
rebble kill
rebble install --emulator aplite
rebble screenshot screenshot_aplite.png

rebble wipe
rebble kill
rebble install --emulator basalt
rebble screenshot screenshot_basalt.png

rebble wipe
rebble kill
rebble install --emulator chalk
rebble screenshot screenshot_chalk.png

rebble wipe
rebble kill
rebble install --emulator diorite
rebble screenshot screenshot_diorite.png

rebble wipe
rebble kill
rebble install --emulator emery
rebble screenshot screenshot_emery.png

rebble wipe
rebble kill
