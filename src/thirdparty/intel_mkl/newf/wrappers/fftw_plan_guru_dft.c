/*******************************************************************************
* Copyright 2005-2022 Intel Corporation.
*
* This software and the related documents are Intel copyrighted  materials,  and
* your use of  them is  governed by the  express license  under which  they were
* provided to you (License).  Unless the License provides otherwise, you may not
* use, modify, copy, publish, distribute,  disclose or transmit this software or
* the related documents without Intel's prior written permission.
*
* This software and the related documents  are provided as  is,  with no express
* or implied  warranties,  other  than those  that are  expressly stated  in the
* License.
*******************************************************************************/

/*
 *
 * fftw_plan_guru_dft - FFTW3 wrapper to Intel(R) oneAPI Math Kernel Library (Intel(R) oneMKL).
 *
 ******************************************************************************
 */

#include "fftw3_mkl.h"
#ifdef DFT_ENABLE_OFFLOAD
#include "oneapi/mkl/export.hpp"
#include "fftw3_omp_offload_common.h"

DLL_EXPORT fftw_plan
fftw_plan_guru_dft_omp_offload(int rank, const fftw_iodim *dims, int howmany_rank,
                               const fftw_iodim *howmany_dims, fftw_complex *in,
                               fftw_complex *out, int sign, unsigned flags,
                               void* interopObj)
#else
fftw_plan
fftw_plan_guru_dft(int rank, const fftw_iodim *dims, int howmany_rank,
                   const fftw_iodim *howmany_dims, fftw_complex *in,
                   fftw_complex *out, int sign, unsigned flags)
#endif
{
    fftw_iodim64 dims64[MKL_MAXRANK];
    fftw_iodim64 howmany_dims64[MKL_ONE];
    int i;

    if (rank > MKL_MAXRANK || howmany_rank > MKL_ONE)
        return NULL;

    if (dims == NULL || (howmany_rank > 0 && howmany_dims == NULL))
        return NULL;

    for (i = 0; i < rank; ++i)
    {
        dims64[i].n = dims[i].n;
        dims64[i].is = dims[i].is;
        dims64[i].os = dims[i].os;
    }
    for (i = 0; i < howmany_rank; ++i)
    {
        howmany_dims64[i].n = howmany_dims[i].n;
        howmany_dims64[i].is = howmany_dims[i].is;
        howmany_dims64[i].os = howmany_dims[i].os;
    }

#ifdef DFT_ENABLE_OFFLOAD
    if(mkl_dfti_is_ilp64 == 0)
      return fftw_plan_guru64_dft_omp_offload_impl_lp64(rank, dims64, howmany_rank, howmany_dims64,
                                                   in, out, sign, flags, interopObj);
    else
      return fftw_plan_guru64_dft_omp_offload_impl_ilp64(rank, dims64, howmany_rank, howmany_dims64,
                                                   in, out, sign, flags, interopObj);

#else
    return fftw_plan_guru64_dft(rank, dims64, howmany_rank, howmany_dims64, in,
                                out, sign, flags);
#endif
}
