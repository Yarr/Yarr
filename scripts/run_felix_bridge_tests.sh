
# Launch felix_client_bridge to connect an emulator to FelixClient
# Bus paths match hw controller config
bin/felix_client_bridge --bus-dir /tmp/bus --bus-groupname FELIX -f configs/controller/emuCfg_star.json & pid_bridge=($!)
echo "TESTS: WAITING for FCB (on PID ${pid_bridge}) to start"
sleep 3

echo "TESTS: Check Felix Bus"
find /tmp/bus -type f
find /tmp/bus -name "*.ndjson" | xargs cat

HW_CONTROLLER_IN=configs/controller/felix_client_strips.json
HW_CONTROLLER=configs/controller/felix_client_strips_patch.json
jq '.ctrlCfg.cfg.FelixClient.skipFelixReg = true' ${HW_CONTROLLER_IN} > ${HW_CONTROLLER}
EMU_CONNECTIVITY=configs/connectivity/example_star_setup.json

# Run scans with the FelixClient controller
echo "TESTS: Run generic felix client test"
# Send a couple of L0A
bin/testFelixClient ${HW_CONTROLLER} -r 0 -t 0 -w 4 -d 0x1e72d865 -n 2 -l configs/logging/default.json

echo "TESTS: Check Felix Bus after tests"
find /tmp/bus -type f
find /tmp/bus -name "*.ndjson" | xargs cat

echo "TESTS: Now shutdown FCB"

# Terminate felix_client_bridge using SIGUSR1
kill -10 ${pid_bridge}

wait ${pid_bridge}

echo "TESTS: Complete"
