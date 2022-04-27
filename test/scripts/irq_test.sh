#!/bin/bash
#
# Connect to hardware or simulation to run basic tests
#

# Debug
set -x
set -m

# Trap CTRL-C
trap ctrl_c INT
function ctrl_c() {

    # Kill all verilator instances
    if [ -n "$SIM_PID" ]; then
        kill -9 $SIM_PID
    fi
    if [ -n "$REMOTE_PID" ]; then
        kill -9 $REMOTE_PID
    fi
}

# Running on sim
if [ $# -eq 6 ]; then
    # Get local SIM and remote SIM
    SIM=$1
    REMOTE=$2

    # Get test and HW
    EXE=$3
    HW=$4
    TRACE=$5
    ARM_BIN=$6
    
    # Parse port from host
    HOST_PORT=($(echo $4 | tr ':' ' '))
    HOST=${HOST_PORT[0]}
    PORT=${HOST_PORT[1]}
else
    echo "Incorrect args!"
    exit -1
fi

# Skip if running on real hardware
# as we can't force IRQs
if [ $HOST != "127.0.0.1" ]; then
    exit 0
fi

# Start verilator instances
if [ -n "$REMOTE" ]; then
    # Trace if necessary
    if [ "$TRACE" = true ]; then
        $REMOTE -j -g --vcd=${EXE}_remote.vcd --bin-load=$ARM_BIN &
    else
        $REMOTE -j -g --elf-load=$ARM_ELF &
    fi
    REMOTE_PID=$!
    sleep 0.5
fi
if [ -n "$SIM" ]; then
    # Trace if necessary
    if [ "$TRACE" = true ]; then
        $SIM -u$PORT -r --vcd=${EXE}.vcd &
    else
        $SIM -u$PORT -r &
    fi
    SIM_PID=$!
    sleep 1
fi

# Run test
$EXE $HW
RV=$?
echo "Result=" $RV

# Sleep for a second to allow flushing of traces
if [ "$TRACE" = true ]; then
    sleep 1
fi

# Kill sim instances
ctrl_c

# Return test result
exit $RV
