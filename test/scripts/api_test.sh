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

# Running on read hardware
if [ $# -eq 2 ]; then

    # Get test and hardware
    EXE=$1
    HW=$2
# Running on sim
elif [ $# -eq 5 ]; then
    # Get local SIM and remote SIM
    SIM=$1
    REMOTE=$2

    # Get test and HW
    EXE=$3
    HW=$4
    TRACE=$5

    # Parse port from host
    HOST_PORT=($(echo $4 | tr ':' ' '))
    HOST=${HOST_PORT[0]}
    PORT=${HOST_PORT[1]}
else
    echo "Incorrect args!"
    exit -1
fi

# Start verilator instances
if [ -n "$REMOTE" ]; then
    # Trace if necessary
    if [ "$TRACE" = true ]; then
        $REMOTE -j --vcd=${EXE}_remote.vcd &
    else
        $REMOTE -j &
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
