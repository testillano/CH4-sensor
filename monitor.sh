#!/bin/bash

do_reset() {
  trap "" SIGINT # ignore
  while true
  do
    cat << EOF

In case the monitor records extreme values (ppm), the graph may become
 distorted due to scaling. In such a case, you can reset the data
 recorded so far:

Press ENTER to proceed ...

EOF
    read -r dummy
    kill -SIGUSR1 $(pgrep python3) # TODO: improve target definition
  done
}
export -f do_reset

echo
echo "Close Serial Monitor at Arduino IDE, to avoid 'busy resource' error."
echo "Press ENTER to continue ..."
read -r dummy

xterm -T "RESET GRAPH ACTION SHORTCUT" -e bash -c "do_reset" &
python3 plotSerial.py
