#!/bin/bash


###################################################
#
#        assumes you are in SHMEM installation and have default libfabric installation
#
#        input:
#        argument 1: path find test/binaries in -- same as build source
#        argument 2: SHMEM src pathway
###################################################

#USER SET THIS VARIABLES:
#this should hook into oshrun
export OSHRUN_LAUNCHER="mpiexec.hydra"
RUN_PARAM="-np 4"

########################## UNIT TESTS ##########################################

function run_SHMEM_UNIT_tests {

    cd $2/test/unit

    make VERBOSE=1 TEST_RUNNER="mpiexec.hydra -np 2" check

}

########################## UH TESTS ##########################################

#tests bin and source are in $1
#WARNING BUILDING FORTRAN

#putting output in log file

function run_UH_tests {

    cd $1/tests-uh

    make C_feature_tests-run 2>&1 | tee uh-tests-c-feature-test.log
    if [ $? -ne 0 ]; then
        echo "UH run failed"
        exit 1
    fi
    make F_feature_tests-run 2>&1 | tee uh-tests-f-feature-test.log
    if [ $? -ne 0 ]; then
        echo "UH run failed"
        exit 1
    fi
}


########################## ISX TESTS ##########################################

function run_ISx_tests {

    cd $1/ISx/SHMEM
    oshrun $RUN_PARAM ./bin/isx.strong 134217728 output_strong
    oshrun $RUN_PARAM ./bin/isx.weak 33554432 output_weak
    oshrun $RUN_PARAM ./bin/isx.weak_iso 33554432 output_weak_iso
    make clean

}

########################## PRK TESTS ##########################################

function run_PRK_tests {

    cd $1/PRK
    oshrun $RUN_PARAM ./SHMEM/Stencil/stencil 100 1000 2>&1 | tee stencil.log
    grep -qi "Solution validates" stencil.log
    oshrun $RUN_PARAM ./SHMEM/Synch_p2p/p2p 10 1000 1000 2>&1 | tee p2p.log
    grep -qi "Solution validates" p2p.log
    oshrun $RUN_PARAM ./SHMEM/Transpose/transpose 10 1000 2>&1 | tee transpose.log
    grep -qi "Solution validates" transpose.log
    make clean

}

########################## MAIN ##########################################

if [ $# -ne 2 ]; then
    echo "ERROR requires 2 inputs: 1)source of tests are installed 2)SHMEM src path"
    exit 1
fi

#check that directory exists

echo "testing out of directory $1"

run_SHMEM_UNIT_tests $2
run_UH_tests $1
run_ISx_tests $1
run_PRK_tests $1
