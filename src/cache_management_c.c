/* -*- C -*-
 *
 * Copyright 2011 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S.  Government
 * retains certain rights in this software.
 *
 * Copyright (c) 2016 Intel Corporation. All rights reserved.
 * This software is available to you under the BSD license.
 *
 * This file is part of the Sandia OpenSHMEM software package. For license
 * information, see the LICENSE file in the top level directory of the
 * distribution.
 *
 */

#include "config.h"

#define SHMEM_INTERNAL_INCLUDE
#include "shmem.h"
#include "shmem_internal.h"

#ifdef ENABLE_PROFILING
#include "pshmem.h"

#pragma weak shmem_clear_cache_inv = pshmem_clear_cache_inv
#define shmem_clear_cache_inv pshmem_clear_cache_inv

#pragma weak shmem_clear_cache_line_inv = pshmem_clear_cache_line_inv
#define shmem_clear_cache_line_inv pshmem_clear_cache_line_inv

#pragma weak shmem_set_cache_inv = pshmem_set_cache_inv
#define shmem_set_cache_inv pshmem_set_cache_inv

#pragma weak shmem_set_cache_line_inv = pshmem_set_cache_line_inv
#define shmem_set_cache_line_inv pshmem_set_cache_line_inv

#pragma weak shmem_udcflush = pshmem_udcflush
#define shmem_udcflush pshmem_udcflush

#pragma weak shmem_udcflush_line = pshmem_udcflush_line
#define shmem_udcflush_line pshmem_udcflush_line

#endif /* ENABLE_PROFILING */

void SHMEM_FUNCTION_ATTRIBUTES
shmem_clear_cache_inv(void)
{
    SHMEM_ERR_CHECK_INITIALIZED();

    /* Intentionally a no-op */
}


void SHMEM_FUNCTION_ATTRIBUTES
shmem_set_cache_inv(void)
{
    SHMEM_ERR_CHECK_INITIALIZED();

    /* Intentionally a no-op */
}


void SHMEM_FUNCTION_ATTRIBUTES
shmem_clear_cache_line_inv(void *target)
{
    SHMEM_ERR_CHECK_INITIALIZED();

    /* Intentionally a no-op */
}


void SHMEM_FUNCTION_ATTRIBUTES
shmem_set_cache_line_inv(void *target)
{
    SHMEM_ERR_CHECK_INITIALIZED();

    /* Intentionally a no-op */
}


void SHMEM_FUNCTION_ATTRIBUTES
shmem_udcflush(void)
{
    SHMEM_ERR_CHECK_INITIALIZED();

    /* Intentionally a no-op */
}


void SHMEM_FUNCTION_ATTRIBUTES
shmem_udcflush_line(void *target)
{
    SHMEM_ERR_CHECK_INITIALIZED();

    /* Intentionally a no-op */
}
