/*******************************************************************************
* Copyright 2008-2022 Intel Corporation.
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
 * fftwf_plan_guru64_dft - FFTW wrapper to Intel(R) oneAPI Math Kernel Library (Intel(R) oneMKL).
 *
 ******************************************************************************
 */

#include "fftw3_mkl.h"
#ifdef DFT_ENABLE_OFFLOAD
#include "oneapi/mkl/export.hpp"
#include "fftw3_omp_offload_common.h"

static void execute_fi(fftw_mkl_plan p, void *interopObj);
static void execute_bi(fftw_mkl_plan p, void *interopObj);
static void execute_fo(fftw_mkl_plan p, void *interopObj);
static void execute_bo(fftw_mkl_plan p, void *interopObj);
#else
static void execute_fi(fftw_mkl_plan p);
static void execute_bi(fftw_mkl_plan p);
static void execute_fo(fftw_mkl_plan p);
static void execute_bo(fftw_mkl_plan p);
#endif

#ifdef DFT_ENABLE_OFFLOAD
#ifdef MKL_ILP64
DLL_EXPORT fftwf_plan
fftwf_plan_guru64_dft_omp_offload(int rank, const fftwf_iodim64 *dims, int howmany_rank,
                                  const fftwf_iodim64 *howmany_dims, fftwf_complex *in,
                                  fftwf_complex *out, int sign, unsigned flags,
                                  void *interopObj)
{
    if(mkl_dfti_is_ilp64 == 0)
      return fftwf_plan_guru64_dft_omp_offload_impl_lp64(rank, dims, howmany_rank, howmany_dims,
                                                   in, out, sign, flags, interopObj);
    else
      return fftwf_plan_guru64_dft_omp_offload_impl_ilp64(rank, dims, howmany_rank, howmany_dims,
                                                   in, out, sign, flags, interopObj);
}

fftwf_plan
fftwf_plan_guru64_dft_omp_offload_impl_ilp64(int rank, const fftwf_iodim64 *dims, int howmany_rank,
                     const fftwf_iodim64 *howmany_dims, fftwf_complex *in,
                     fftwf_complex *out, int sign, unsigned flags, void *interopObj)
#else
fftwf_plan
fftwf_plan_guru64_dft_omp_offload_impl_lp64(int rank, const fftwf_iodim64 *dims, int howmany_rank,
                     const fftwf_iodim64 *howmany_dims, fftwf_complex *in,
                     fftwf_complex *out, int sign, unsigned flags, void *interopObj)
#endif // MKL_ILP64
#else
fftwf_plan
fftwf_plan_guru64_dft(int rank, const fftwf_iodim64 *dims, int howmany_rank,
                     const fftwf_iodim64 *howmany_dims, fftwf_complex *in,
                     fftwf_complex *out, int sign, unsigned flags)
#endif
{
    fftw_mkl_plan mkl_plan;
    MKL_LONG periods[MKL_MAXRANK];
    MKL_LONG istrides[MKL_MAXRANK + 1];
    MKL_LONG ostrides[MKL_MAXRANK + 1];
    MKL_LONG s = 0;                    /* status */
    int i;

    if (rank > MKL_MAXRANK || howmany_rank > MKL_ONE)
        return NULL;

    if (dims == NULL || (howmany_rank > 0 && howmany_dims == NULL))
        return NULL;

    mkl_plan = fftw3_mkl.new_plan();
    if (!mkl_plan)
        return NULL;

    istrides[0] = 0;
    ostrides[0] = 0;

    for (i = 0; i < rank; ++i)
    {
        periods[i] = (MKL_LONG)dims[i].n;
        istrides[i + 1] = (MKL_LONG)dims[i].is;
        ostrides[i + 1] = (MKL_LONG)dims[i].os;

        /* check if MKL_LONG is sufficient to hold dims */
        if (periods[i] != dims[i].n)
            goto broken;
        if (istrides[i + 1] != dims[i].is)
            goto broken;
        if (ostrides[i + 1] != dims[i].os)
            goto broken;
    }
    if (rank == 1)
        s = DftiCreateDescriptor(&mkl_plan->desc, DFTI_SINGLE, DFTI_COMPLEX,
                                 (MKL_LONG)rank, periods[0]);
    else
        s = DftiCreateDescriptor(&mkl_plan->desc, DFTI_SINGLE, DFTI_COMPLEX,
                                 (MKL_LONG)rank, periods);
    if (BAD(s))
        goto broken;

    if (flags & FFTW_CONSERVE_MEMORY)
    {
        s = DftiSetValue(mkl_plan->desc, DFTI_WORKSPACE, DFTI_AVOID);
        if (BAD(s))
            goto broken;
    }

    s = DftiSetValue(mkl_plan->desc, DFTI_INPUT_STRIDES, istrides);
    if (BAD(s))
        goto broken;

    if (in == out)
    {
        mkl_plan->io[0] = in;
#ifdef DFT_ENABLE_OFFLOAD
        mkl_plan->execute_offload = (sign == FFTW_FORWARD) ? execute_fi : execute_bi;
#else
        mkl_plan->execute = (sign == FFTW_FORWARD) ? execute_fi : execute_bi;
#endif
        /* check if in-place has valid strides */
        for (i = 0; i < rank; ++i)
        {
            if (dims[i].is != dims[i].os)
                goto broken;
        }
        for (i = 0; i < howmany_rank; ++i)
        {
            if (howmany_dims[i].is != howmany_dims[i].os)
                goto broken;
        }
    }
    else
    {
        mkl_plan->io[0] = in;
        mkl_plan->io[1] = out;
#ifdef DFT_ENABLE_OFFLOAD
        mkl_plan->execute_offload = (sign == FFTW_FORWARD) ? execute_fo : execute_bo;
#else
        mkl_plan->execute = (sign == FFTW_FORWARD) ? execute_fo : execute_bo;
#endif
        s = DftiSetValue(mkl_plan->desc, DFTI_PLACEMENT, DFTI_NOT_INPLACE);
        if (BAD(s))
            goto broken;

        s = DftiSetValue(mkl_plan->desc, DFTI_OUTPUT_STRIDES, ostrides);
        if (BAD(s))
            goto broken;
    }

    if (howmany_rank == 1)
    {
        MKL_LONG howmany = (MKL_LONG)howmany_dims[0].n;
        MKL_LONG idistance = (MKL_LONG)howmany_dims[0].is;
        MKL_LONG odistance = (MKL_LONG)howmany_dims[0].os;

        /* check if MKL_LONG is sufficient to hold dims */
        if (howmany != howmany_dims[0].n)
            goto broken;
        if (idistance != howmany_dims[0].is)
            goto broken;
        if (odistance != howmany_dims[0].os)
            goto broken;

        s = DftiSetValue(mkl_plan->desc, DFTI_NUMBER_OF_TRANSFORMS, howmany);
        if (BAD(s))
            goto broken;

        s = DftiSetValue(mkl_plan->desc, DFTI_INPUT_DISTANCE, idistance);
        if (BAD(s))
            goto broken;

        s = DftiSetValue(mkl_plan->desc, DFTI_OUTPUT_DISTANCE, odistance);
        if (BAD(s))
            goto broken;
    }

    if (fftw3_mkl.nthreads >= 0)
    {
        s = DftiSetValue(mkl_plan->desc, DFTI_THREAD_LIMIT,
                         (MKL_LONG)fftw3_mkl.nthreads);
        if (BAD(s))
            goto broken;
    }

#ifdef DFT_ENABLE_OFFLOAD
    s = mkl_DftiCommitDescriptor_omp_offload(mkl_plan->desc, interopObj);
#else
    s = DftiCommitDescriptor(mkl_plan->desc);
#endif
    if (BAD(s))
        goto broken;

    return (fftwf_plan)mkl_plan;

  broken:
    /* possibly report the reason before returning NULL */
    mkl_plan->destroy(mkl_plan);
    return NULL;
}

#ifdef DFT_ENABLE_OFFLOAD
static void
execute_fi(fftw_mkl_plan p, void *interopObj)
{
    mkl_DftiComputeForward_omp_offload(p->desc, p->io[0], interopObj);
}

static void
execute_bi(fftw_mkl_plan p, void *interopObj)
{
    mkl_DftiComputeBackward_omp_offload(p->desc, p->io[0], interopObj);
}

static void
execute_fo(fftw_mkl_plan p, void *interopObj)
{
    mkl_DftiComputeForward_omp_offload(p->desc, p->io[0], p->io[1], interopObj);
}

static void
execute_bo(fftw_mkl_plan p, void *interopObj)
{
    mkl_DftiComputeBackward_omp_offload(p->desc, p->io[0], p->io[1], interopObj);
}
#else
static void
execute_fi(fftw_mkl_plan p)
{
    DftiComputeForward(p->desc, p->io[0]);
}

static void
execute_bi(fftw_mkl_plan p)
{
    DftiComputeBackward(p->desc, p->io[0]);
}

static void
execute_fo(fftw_mkl_plan p)
{
    DftiComputeForward(p->desc, p->io[0], p->io[1]);
}

static void
execute_bo(fftw_mkl_plan p)
{
    DftiComputeBackward(p->desc, p->io[0], p->io[1]);
}
#endif
