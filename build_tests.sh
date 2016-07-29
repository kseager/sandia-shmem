#!/bin/bash


###################################################
#
#        assumes standard SHMEM and libfabric installation
#
#        input:
#        argument 1: path put source code in
#        argument 2: path to install code in
###################################################

PAR_MAKE="-j 4"

########################## HYDRA ##########################################
#put source into $1/hydra-3.2 and install in $2/hydra
#i.e. mpiexec.hydra = $2/install/hydra/bin

function build_hydra {

    cd $1

    wget http://www.mpich.org/static/downloads/3.2/hydra-3.2.tar.gz > /dev/null
    if [ $? -ne 0 ]; then
        wget -O hydra-3.2.tar.gz "https://docs.google.com/uc?authuser=0&id=0BxdTWEtawABaU3pKSzY5VlY4SzA&export=download"
    fi

    tar xvzf hydra-3.2.tar.gz
    cd hydra-3.2/
    ./configure --prefix=$2/hydra
    make $PAR_MAKE
    make install

    echo "finished hydra: source $1/hydra-3.2 installation $2/hydra"

}

########################## UH TESTS ##########################################

#tests bin and source are in $1
#WARNING BUILDING FORTRAN

function build_UH_tests {

    cd $1

    git clone --depth 10 https://github.com/openshmem-org/tests-uh.git tests-uh
    cd tests-uh
    make $PAR_MAKE C_feature_tests F_feature_tests
    if [ $? -ne 0 ]; then
        echo "UH build failed"
    fi
}


########################## ISX TESTS ##########################################

function build_ISx_tests {

    cd $1
    git clone --depth 10 https://github.com/ParRes/ISx.git ISx
    cd $1/ISx/SHMEM
    make CC=oshcc LDLIBS=-lm

}

########################## PRK TESTS ##########################################

function build_PRK_tests {

    cd $1
    git clone --depth 10 https://github.com/ParRes/Kernels.git PRK
    echo -e "SHMEMCC=oshcc\nSHMEMTOP=$$2\n" > PRK/common/make.defs
    cd $1/PRK
    make allshmem

}

########################## MAIN ##########################################

if [ $# -ne 2 ]; then
    echo "ERROR requires two inputs: 1 source code file dest and 2 installation dest"
fi

#create directories if they don't already exist
mkdir -p $1
mkdir -p $2

echo "placing tests source code into $1 and installations into $2"

build_hydra $1 $2
build_UH_tests $1
build_ISx_tests $1
build_PRK_tests $1 $2
