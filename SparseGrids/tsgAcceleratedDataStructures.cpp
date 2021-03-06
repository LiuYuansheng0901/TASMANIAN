/*
 * Copyright (c) 2017, Miroslav Stoyanov
 *
 * This file is part of
 * Toolkit for Adaptive Stochastic Modeling And Non-Intrusive ApproximatioN: TASMANIAN
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 *    and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse
 *    or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * UT-BATTELLE, LLC AND THE UNITED STATES GOVERNMENT MAKE NO REPRESENTATIONS AND DISCLAIM ALL WARRANTIES, BOTH EXPRESSED AND IMPLIED.
 * THERE ARE NO EXPRESS OR IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, OR THAT THE USE OF THE SOFTWARE WILL NOT INFRINGE ANY PATENT,
 * COPYRIGHT, TRADEMARK, OR OTHER PROPRIETARY RIGHTS, OR THAT THE SOFTWARE WILL ACCOMPLISH THE INTENDED RESULTS OR THAT THE SOFTWARE OR ITS USE WILL NOT RESULT IN INJURY OR DAMAGE.
 * THE USER ASSUMES RESPONSIBILITY FOR ALL LIABILITIES, PENALTIES, FINES, CLAIMS, CAUSES OF ACTION, AND COSTS AND EXPENSES, CAUSED BY, RESULTING FROM OR ARISING OUT OF,
 * IN WHOLE OR IN PART THE USE, STORAGE OR DISPOSAL OF THE SOFTWARE.
 */

#ifndef __TASMANIAN_SPARSE_GRID_ACCELERATED_DATA_STRUCTURES_CPP
#define __TASMANIAN_SPARSE_GRID_ACCELERATED_DATA_STRUCTURES_CPP

#include "tsgAcceleratedDataStructures.hpp"

#ifdef Tasmanian_ENABLE_CUDA
#include <cuda_runtime_api.h>
#include <cuda.h>
#include <cublas_v2.h>
#include <cusparse.h>
#endif

#ifdef Tasmanian_ENABLE_MAGMA
#include "magma_v2.h"
#include "magmasparse.h"
#endif

namespace TasGrid{

#ifdef Tasmanian_ENABLE_CUDA
template<typename T> void CudaVector<T>::resize(size_t count){
    if (count != num_entries){ // if the current array is not big enough
        clear(); // resets dynamic_mode
        num_entries = count;
        cudaError_t cudaStat = cudaMalloc(((void**) &gpu_data), num_entries * sizeof(T));
        AccelerationMeta::cudaCheckError((void*) &cudaStat, "CudaVector::resize(), call to cudaMalloc()");
    }
}
template<typename T> void CudaVector<T>::clear(){
    num_entries = 0;
    if (gpu_data != nullptr){ // if I own the data and the data is not null
        cudaError_t cudaStat = cudaFree(gpu_data);
        AccelerationMeta::cudaCheckError((void*) &cudaStat, "CudaVector::clear(), call to cudaFree()");
    }
    gpu_data = nullptr;
}
template<typename T> void CudaVector<T>::load(size_t count, const T* cpu_data){
    resize(count);
    cudaError_t cudaStat = cudaMemcpy(gpu_data, cpu_data, num_entries * sizeof(T), cudaMemcpyHostToDevice);
    AccelerationMeta::cudaCheckError((void*) &cudaStat, "CudaVector::load(), call to cudaMemcpy()");
}
template<typename T> void CudaVector<T>::unload(T* cpu_data) const{
    cudaError_t cudaStat = cudaMemcpy(cpu_data, gpu_data, num_entries * sizeof(T), cudaMemcpyDeviceToHost);
    AccelerationMeta::cudaCheckError((void*) &cudaStat, "CudaVector::unload(), call to cudaMemcpy()");
}

template void CudaVector<double>::resize(size_t);
template void CudaVector<double>::clear();
template void CudaVector<double>::load(size_t, const double*);
template void CudaVector<double>::unload(double*) const;

template void CudaVector<float>::resize(size_t);
template void CudaVector<float>::clear();
template void CudaVector<float>::load(size_t, const float*);
template void CudaVector<float>::unload(float*) const;

template void CudaVector<int>::resize(size_t);
template void CudaVector<int>::clear();
template void CudaVector<int>::load(size_t, const int*);
template void CudaVector<int>::unload(int*) const;

CudaEngine::~CudaEngine(){
    if (own_cublas_handle && cublasHandle != nullptr){
        cublasDestroy((cublasHandle_t) cublasHandle);
        cublasHandle = nullptr;
    }
    if (own_cusparse_handle && cusparseHandle != nullptr){
        cusparseDestroy((cusparseHandle_t) cusparseHandle);
        cusparseHandle = nullptr;
    }
    #ifdef Tasmanian_ENABLE_MAGMA
    if (own_magma_queue && magmaCudaQueue != nullptr){
        magma_queue_destroy((magma_queue*) magmaCudaQueue);
        magmaCudaQueue = nullptr;
        magma_finalize();
    }
    if (magmaCudaStream != nullptr) cudaStreamDestroy((cudaStream_t) magmaCudaStream); // always owned, created only with owned magma queue
    magmaCudaStream = nullptr;
    #endif
}
void CudaEngine::cuBlasPrepare(){
    if (cublasHandle == nullptr){
        cublasHandle_t cbh;
        cublasCreate(&cbh);
        cublasHandle = (void*) cbh;
        own_cublas_handle = true;
    }
}
void CudaEngine::cuSparsePrepare(){
    if (cusparseHandle == nullptr){
        cusparseHandle_t csh;
        cusparseCreate(&csh);
        cusparseHandle = (void*) csh;
        own_cusparse_handle = true;
    }
}
void CudaEngine::magmaPrepare(){
    #ifdef Tasmanian_ENABLE_MAGMA
    if (magmaCudaQueue == nullptr){
        magma_init();
        cuBlasPrepare();
        cuSparsePrepare();
        magma_queue_create_from_cuda(gpu, (cudaStream_t) magmaCudaStream, (cublasHandle_t) cublasHandle, (cusparseHandle_t) cusparseHandle, ((magma_queue**) &magmaCudaQueue));
        own_magma_queue = true;
    }
    #endif
}
#ifdef Tasmanian_ENABLE_MAGMA
inline void magma_gemm(magma_trans_t transa, magma_trans_t transb, int M, int N, int K, double alpha, double const A[], int lda,
                       double const B[], int ldb, double beta, double C[], int ldc, magma_queue_t mqueue){
    magma_dgemm(transa, transb, M, N, K, alpha, A, lda, B, ldb, beta, C, ldc, mqueue);
}
inline void magma_gemm(magma_trans_t transa, magma_trans_t transb, int M, int N, int K, float alpha, float const A[], int lda,
                       float const B[], int ldb, float beta, float C[], int ldc, magma_queue_t mqueue){
    magma_sgemm(transa, transb, M, N, K, alpha, A, lda, B, ldb, beta, C, ldc, mqueue);
}
inline void magma_gemv(magma_trans_t transa, int M, int N, double alpha, double const A[], int lda,
                       double const x[], int incx, double beta, double y[], int incy, magma_queue_t mqueue){
    magma_dgemv(transa, M, N, alpha, A, lda, x, incx, beta, y, incy, mqueue);
}
inline void magma_gemv(magma_trans_t transa, int M, int N, float alpha, float const A[], int lda,
                       float const x[], int incx, float beta, float y[], int incy, magma_queue_t mqueue){
    magma_sgemv(transa, M, N, alpha, A, lda, x, incx, beta, y, incy, mqueue);
}
#endif
inline cublasStatus_t cublasgemm(cublasHandle_t handle, cublasOperation_t transa, cublasOperation_t transb, int M, int N, int K,
                                 double const *alpha, const double A[], int lda, const double B[], int ldb, double const *beta, double C[], int ldc){
    return cublasDgemm(handle, transa, transb, M, N, K, alpha, A, lda, B, ldb, beta, C, ldc);
}
inline cublasStatus_t cublasgemm(cublasHandle_t handle, cublasOperation_t transa, cublasOperation_t transb, int M, int N, int K,
                                 float const *alpha, const float A[], int lda, const float B[], int ldb, float const *beta, float C[], int ldc){
    return cublasSgemm(handle, transa, transb, M, N, K, alpha, A, lda, B, ldb, beta, C, ldc);
}
inline cublasStatus_t cublasgemv(cublasHandle_t handle, cublasOperation_t transa, int M, int N,
                                 double const *alpha, const double A[], int lda, const double x[], int incx, double const *beta, double y[], int incy){
    return cublasDgemv(handle, transa, M, N, alpha, A, lda, x, incx, beta, y, incy);
}
inline cublasStatus_t cublasgemv(cublasHandle_t handle, cublasOperation_t transa, int M, int N,
                                 float const *alpha, const float A[], int lda, const float x[], int incx, float const *beta, float y[], int incy){
    return cublasSgemv(handle, transa, M, N, alpha, A, lda, x, incx, beta, y, incy);
}

template<typename T>
void CudaEngine::denseMultiply(int M, int N, int K, typename CudaVector<T>::value_type alpha,
                               const CudaVector<T> &A, const CudaVector<T> &B, typename CudaVector<T>::value_type beta, T C[]){
    #ifdef Tasmanian_ENABLE_MAGMA
    if (magma){
        magmaPrepare();
        if (M > 1){
            if (N > 1){ // matrix mode
                magma_gemm(MagmaNoTrans, MagmaNoTrans, M, N, K,
                           alpha, A.data(), M, B.data(), K, beta, C, M, (magma_queue_t) magmaCudaQueue);
            }else{ // matrix vector, A * v = C
                magma_gemv(MagmaNoTrans, M, K,
                            alpha, A.data(), M, B.data(), 1, beta, C, 1, (magma_queue_t) magmaCudaQueue);
            }
        }else{ // matrix vector B^T * v = C
            magma_gemv(MagmaTrans, K, N,
                        alpha, B.data(), K, A.data(), 1, beta, C, 1, (magma_queue_t) magmaCudaQueue);
        }
        return;
    }
    #endif
    cublasStatus_t stat;
    cuBlasPrepare();
    if (M > 1){
        if (N > 1){ // matrix mode
            stat = cublasgemm((cublasHandle_t) cublasHandle, CUBLAS_OP_N, CUBLAS_OP_N, M, N, K,
                               &alpha, A.data(), M, B.data(), K, &beta, C, M);
        }else{ // matrix vector, A * v = C
            stat= cublasgemv((cublasHandle_t) cublasHandle, CUBLAS_OP_N, M, K,
                              &alpha, A.data(), M, B.data(), 1, &beta, C, 1);
        }
    }else{ // matrix vector B^T * v = C
        stat= cublasgemv((cublasHandle_t) cublasHandle, CUBLAS_OP_T, K, N,
                          &alpha, B.data(), K, A.data(), 1, &beta, C, 1);
    }
    AccelerationMeta::cublasCheckError((void*) &stat, "while calling CudaEngine::denseMultiply()");
}
template void CudaEngine::denseMultiply<double>(int M, int N, int K, typename CudaVector<double>::value_type alpha,
                                                const CudaVector<double> &A, const CudaVector<double> &B,
                                                typename CudaVector<double>::value_type beta, double C[]);
template void CudaEngine::denseMultiply<float>(int M, int N, int K, typename CudaVector<float>::value_type alpha,
                                               const CudaVector<float> &A, const CudaVector<float> &B,
                                               typename CudaVector<float>::value_type beta, float C[]);

// ----------- Sparse Linear Algebra ------------------- //
inline
cusparseStatus_t cusparsecsrmm2(cusparseHandle_t handle, cusparseOperation_t transa, cusparseOperation_t transb,
                                int M, int N, int K, int nnz, double const *alpha,
                                cusparseMatDescr_t const matdesc, double const vals[], int const pntr[], int const indx[],
                                double const B[], int ldb, double const *beta, double C[], int ldc){
    return cusparseDcsrmm2(handle, transa, transb, M, N, K, nnz, alpha, matdesc, vals, pntr, indx, B, ldb, beta, C, ldc);
}
inline cusparseStatus_t cusparsecsrmm2(cusparseHandle_t handle, cusparseOperation_t transa, cusparseOperation_t transb,
                                       int M, int N, int K, int nnz, float const *alpha,
                                       cusparseMatDescr_t const matdesc, float const vals[], int const pntr[], int const indx[],
                                       float const B[], int ldb, float const *beta, float C[], int ldc){
    return cusparseScsrmm2(handle, transa, transb, M, N, K, nnz, alpha, matdesc, vals, pntr, indx, B, ldb, beta, C, ldc);
}
inline cublasStatus_t cublasgeam(cublasHandle_t handle, cublasOperation_t transa, cublasOperation_t transb,
                                 int M, int N, double const *alpha, double const A[], int lda,
                                 double const *beta, double const B[], int ldb, double C[], int ldc){
    return cublasDgeam(handle, transa, transb, M, N, alpha, A, lda, beta, B, ldb, C, ldc);
}
inline cublasStatus_t cublasgeam(cublasHandle_t handle, cublasOperation_t transa, cublasOperation_t transb,
                                 int M, int N, float const *alpha, float const A[], int lda,
                                 float const *beta, float const B[], int ldb, float C[], int ldc){
    return cublasSgeam(handle, transa, transb, M, N, alpha, A, lda, beta, B, ldb, C, ldc);
}
inline cusparseStatus_t cusparsecsrmv(cusparseHandle_t handle, cusparseOperation_t transa, int M, int N, int nnz,
                                      double const *alpha, cusparseMatDescr_t const matdesc, double const vals[], int const pntr[], int const indx[],
                                      double const x[], double const *beta, double y[]){
    return cusparseDcsrmv(handle, transa, M, N, nnz, alpha, matdesc, vals, pntr, indx, x, beta, y);
}
inline cusparseStatus_t cusparsecsrmv(cusparseHandle_t handle, cusparseOperation_t transa, int M, int N, int nnz,
                                      float const *alpha, cusparseMatDescr_t const matdesc, float const vals[], int const pntr[], int const indx[],
                                      float const x[], float const *beta, float y[]){
    return cusparseScsrmv(handle, transa, M, N, nnz, alpha, matdesc, vals, pntr, indx, x, beta, y);
}
template<typename T>
cusparseStatus_t cusparsegemvi_bufferSize(cusparseHandle_t handle, cusparseOperation_t transa, int M, int N, int nnz, int *buff_size){
    static_assert(std::is_same<T, double>::value || std::is_same<T, float>::value, "cusparsegemvi_bufferSize() works only with float and double");
    if (std::is_same<T, double>::value){
        return cusparseDgemvi_bufferSize(handle, transa, M, N, nnz, buff_size);
    }else{
        return cusparseSgemvi_bufferSize(handle, transa, M, N, nnz, buff_size);
    }
}
inline cusparseStatus_t cusparsegemvi(cusparseHandle_t handle, cusparseOperation_t  transa,
               int M, int N, double const *alpha, double const A[], int lda,
               int nnz, const double x[], int const indx[],
               double const *beta, double y[], cusparseIndexBase_t index_base, void *buff){
    return cusparseDgemvi(handle, transa, M, N, alpha, A, lda, nnz, x, indx, beta, y, index_base, buff);
}
inline cusparseStatus_t cusparsegemvi(cusparseHandle_t handle, cusparseOperation_t  transa,
               int M, int N, float const *alpha, float const A[], int lda,
               int nnz, const float x[], int const indx[],
               float const *beta, float y[], cusparseIndexBase_t index_base, void *buff){
    return cusparseSgemvi(handle, transa, M, N, alpha, A, lda, nnz, x, indx, beta, y, index_base, buff);
}


template<typename T>
void CudaEngine::sparseMultiply(int M, int N, int K, typename CudaVector<T>::value_type alpha, const CudaVector<T> &A,
                                const CudaVector<int> &pntr, const CudaVector<int> &indx, const CudaVector<T> &vals,
                                typename CudaVector<T>::value_type beta, T C[]){
    #ifdef Tasmanian_ENABLE_MAGMA
    //if (magma){ // TODO: Enable more MAGMA sparse capabilities
    //    return;
    //}
    #endif
    cusparseStatus_t sparse_stat;
    cuSparsePrepare();
    if (N > 1){ // dense matrix has many columns
        if (M > 1){ // dense matrix has many rows, use matrix-matrix algorithm
            cusparseMatDescr_t mat_desc;
            sparse_stat = cusparseCreateMatDescr(&mat_desc);
            AccelerationMeta::cusparseCheckError((void*) &sparse_stat, "cusparseCreateMatDescr() in CudaEngine::sparseMultiply()");
            cusparseSetMatType(mat_desc, CUSPARSE_MATRIX_TYPE_GENERAL);
            cusparseSetMatIndexBase(mat_desc, CUSPARSE_INDEX_BASE_ZERO);
            cusparseSetMatDiagType(mat_desc, CUSPARSE_DIAG_TYPE_NON_UNIT);

            CudaVector<T> tempC(M, N);
            sparse_stat = cusparsecsrmm2((cusparseHandle_t) cusparseHandle,
                                         CUSPARSE_OPERATION_NON_TRANSPOSE, CUSPARSE_OPERATION_TRANSPOSE, N, M, K, (int) indx.size(),
                                         &alpha, mat_desc, vals.data(), pntr.data(), indx.data(), A.data(), M, &beta, tempC.data(), N);
            AccelerationMeta::cusparseCheckError((void*) &sparse_stat, "cusparseXcsrmm2() in CudaEngine::sparseMultiply()");

            cusparseDestroyMatDescr(mat_desc);

            cuBlasPrepare();
            T talpha = 1.0, tbeta = 0.0;
            cublasStatus_t dense_stat;
            dense_stat = cublasgeam((cublasHandle_t) cublasHandle,
                                     CUBLAS_OP_T, CUBLAS_OP_T, M, N, &talpha, tempC.data(), N, &tbeta, tempC.data(), N, C, M);
            AccelerationMeta::cublasCheckError((void*) &dense_stat, "cublasXgeam() in CudaEngine::sparseMultiply()");
        }else{ // dense matrix has only one row, use sparse matrix times dense vector
            cusparseMatDescr_t mat_desc;
            sparse_stat = cusparseCreateMatDescr(&mat_desc);
            AccelerationMeta::cusparseCheckError((void*) &sparse_stat, "cusparseCreateMatDescr() in CudaEngine::sparseMultiply()");
            cusparseSetMatType(mat_desc, CUSPARSE_MATRIX_TYPE_GENERAL);
            cusparseSetMatIndexBase(mat_desc, CUSPARSE_INDEX_BASE_ZERO);
            cusparseSetMatDiagType(mat_desc, CUSPARSE_DIAG_TYPE_NON_UNIT);

            sparse_stat = cusparsecsrmv((cusparseHandle_t) cusparseHandle,
                                        CUSPARSE_OPERATION_NON_TRANSPOSE, N, K, (int) indx.size(),
                                        &alpha, mat_desc, vals.data(), pntr.data(), indx.data(), A.data(), &beta, C);
            AccelerationMeta::cusparseCheckError((void*) &sparse_stat, "cusparseXcsrmv() in CudaEngine::sparseMultiply()");

            cusparseDestroyMatDescr(mat_desc);
        }
    }else{ // sparse matrix has only one column, use dense matrix times sparse vector
        // quote from Nvidia CUDA cusparse manual at https://docs.nvidia.com/cuda/cusparse/index.html#cusparse-lt-t-gt-gemvi
        // "This function requires no extra storage for the general matrices when operation CUSPARSE_OPERATION_NON_TRANSPOSE is selected."
        // Yet, buffer is required when num_nz exceeds 32 even with CUSPARSE_OPERATION_NON_TRANSPOSE
        int buffer_size;
        sparse_stat = cusparsegemvi_bufferSize<T>((cusparseHandle_t) cusparseHandle, CUSPARSE_OPERATION_NON_TRANSPOSE,
                                                   M, K, (int) indx.size(), &buffer_size);
        AccelerationMeta::cusparseCheckError((void*) &sparse_stat, "cusparseXgemvi_bufferSize() in CudaEngine::sparseMultiply()");
        CudaVector<T> gpu_buffer((size_t) buffer_size);

        sparse_stat = cusparsegemvi((cusparseHandle_t) cusparseHandle,
                                     CUSPARSE_OPERATION_NON_TRANSPOSE, M, K, &alpha, A.data(), M, (int) indx.size(), vals.data(),
                                     indx.data(), &beta, C, CUSPARSE_INDEX_BASE_ZERO, gpu_buffer.data());
        AccelerationMeta::cusparseCheckError((void*) &sparse_stat, "cusparseXgemvi() in CudaEngine::sparseMultiply()");
    }
}
template void CudaEngine::sparseMultiply<double>(int M, int N, int K, typename CudaVector<double>::value_type alpha, const CudaVector<double> &A,
                                                 const CudaVector<int> &pntr, const CudaVector<int> &indx, const CudaVector<double> &vals,
                                                 typename CudaVector<double>::value_type beta, double C[]);
template void CudaEngine::sparseMultiply<float>(int M, int N, int K, typename CudaVector<float>::value_type alpha, const CudaVector<float> &A,
                                                const CudaVector<int> &pntr, const CudaVector<int> &indx, const CudaVector<float> &vals,
                                                typename CudaVector<float>::value_type beta, float C[]);
void CudaEngine::setDevice() const{ cudaSetDevice(gpu); }
#endif

std::map<std::string, TypeAcceleration> AccelerationMeta::getStringToAccelerationMap(){
    return {
        {"none",        accel_none},
        {"cpu-blas",    accel_cpu_blas},
        {"gpu-default", accel_gpu_default},
        {"gpu-cublas",  accel_gpu_cublas},
        {"gpu-cuda",    accel_gpu_cuda},
        {"gpu-rocblas", accel_gpu_rocblas},
        {"gpu-hip",     accel_gpu_hip},
        {"gpu-magma",   accel_gpu_magma}};
}

TypeAcceleration AccelerationMeta::getIOAccelerationString(const char * name){
    try{
        return getStringToAccelerationMap().at(name);
    }catch(std::out_of_range &){
        return accel_none;
    }
}
const char* AccelerationMeta::getIOAccelerationString(TypeAcceleration accel){
    switch (accel){
        case accel_cpu_blas:       return "cpu-blas";
        case accel_gpu_default:    return "gpu-default";
        case accel_gpu_cublas:     return "gpu-cublas";
        case accel_gpu_cuda:       return "gpu-cuda";
        case accel_gpu_magma:      return "gpu-magma";
        default: return "none";
    }
}
int AccelerationMeta::getIOAccelerationInt(TypeAcceleration accel){
    switch (accel){
        case accel_cpu_blas:       return 1;
        case accel_gpu_default:    return 3;
        case accel_gpu_cublas:     return 4;
        case accel_gpu_cuda:       return 5;
        case accel_gpu_magma:      return 6;
        default: return 0;
    }
}
TypeAcceleration AccelerationMeta::getIOIntAcceleration(int accel){
    switch (accel){
        case 1:  return accel_cpu_blas;
        case 3:  return accel_gpu_default;
        case 4:  return accel_gpu_cublas;
        case 5:  return accel_gpu_cuda;
        case 6:  return accel_gpu_magma;
        default: return accel_none;
    }
}
bool AccelerationMeta::isAccTypeGPU(TypeAcceleration accel){
    switch (accel){
        case accel_gpu_default:
        case accel_gpu_cublas:
        case accel_gpu_cuda:
        case accel_gpu_magma: return true;
        default:
            return false;
    }
}

TypeAcceleration AccelerationMeta::getAvailableFallback(TypeAcceleration accel){
    // sparse grids are evaluated in 2 stages:
    // - s1: convert multi-index to matrix B
    // - s2: multiply matrix B by stored matrix A
    // Mode   | Stage 1 device | Stage 2 device | Library for stage 2
    // CUBLAS |      CPU       |     GPU        | Nvidia cuBlas (or cuSparse)
    // CUDA   |      GPU       |     GPU        | Nvidia cuBlas (or cuSparse)
    // MAGMA  |      GPU*      |     GPU        | UTK magma and magma_sparse
    // BLAS   |      CPU       |     CPU        | BLAS
    // none   | all done on CPU, still using OpenMP (if available)
    // *if CUDA is not simultaneously available with MAGMA, then MAGMA will use the CPU for stage 1
    // Note: using CUDA without either cuBlas or MAGMA is a bad idea (it will still work, just slow)

    // accel_gpu_default should always point to the potentially "best" option (currently MAGMA)
    if (accel == accel_gpu_default) accel = accel_gpu_magma;
    #if !defined(Tasmanian_ENABLE_CUDA) || !defined(Tasmanian_ENABLE_MAGMA) || !defined(Tasmanian_ENABLE_BLAS)
    // if any of the 3 acceleration modes is missing, then add a switch statement to guard against setting that mode
    switch(accel){
        #ifndef Tasmanian_ENABLE_CUDA
        // if CUDA is missing: just use the CPU
        case accel_gpu_cublas:
        case accel_gpu_cuda:
            #ifdef Tasmanian_ENABLE_BLAS
            accel = accel_cpu_blas;
            #else
            accel = accel_none;
            #endif
            break;
        #endif // Tasmanian_ENABLE_CUDA
        #ifndef Tasmanian_ENABLE_MAGMA
        // MAGMA tries to use CUDA kernels with magma linear algebra, this CUDA is the next best thing
        case accel_gpu_magma:
            #ifdef Tasmanian_ENABLE_CUDA
            accel = accel_gpu_cuda;
            #elif defined(Tasmanian_ENABLE_BLAS)
            accel = accel_cpu_blas;
            #else
            accel = accel_none;
            #endif
            break;
        #endif // Tasmanian_ENABLE_MAGMA
        #ifndef Tasmanian_ENABLE_BLAS
        // if BLAS is missing, do not attempt to use the GPU but go directly to "none" mode
        case accel_cpu_blas:
            accel = accel_none;
            break;
        #endif // Tasmanian_ENABLE_BLAS
        default: // compiler complains if there is no explicit "default", even if empty
            break;
    }
    #endif
    return accel;
}

#ifdef Tasmanian_ENABLE_CUDA
void AccelerationMeta::cudaCheckError(void *cudaStatus, const char *info){
    if (*((cudaError_t*) cudaStatus) != cudaSuccess){
        std::string message = "ERROR: cuda failed at ";
        message += info;
        message += " with error: ";
        message += cudaGetErrorString(*((cudaError_t*) cudaStatus));
        throw std::runtime_error(message);
    }
}
void AccelerationMeta::cublasCheckError(void *cublasStatus, const char *info){
    if (*((cublasStatus_t*) cublasStatus) != CUBLAS_STATUS_SUCCESS){
        std::string message = "ERROR: cuBlas failed with code: ";
        if (*((cublasStatus_t*) cublasStatus) == CUBLAS_STATUS_NOT_INITIALIZED){
            message += "CUBLAS_STATUS_NOT_INITIALIZED";
        }else if (*((cublasStatus_t*) cublasStatus) == CUBLAS_STATUS_ALLOC_FAILED){
            message += "CUBLAS_STATUS_ALLOC_FAILED";
        }else if (*((cublasStatus_t*) cublasStatus) == CUBLAS_STATUS_INVALID_VALUE){
            message += "CUBLAS_STATUS_INVALID_VALUE";
        }else if (*((cublasStatus_t*) cublasStatus) == CUBLAS_STATUS_ARCH_MISMATCH){
            message += "CUBLAS_STATUS_ARCH_MISMATCH";
        }else if (*((cublasStatus_t*) cublasStatus) == CUBLAS_STATUS_MAPPING_ERROR){
            message += "CUBLAS_STATUS_MAPPING_ERROR";
        }else if (*((cublasStatus_t*) cublasStatus) == CUBLAS_STATUS_EXECUTION_FAILED){
            message += "CUBLAS_STATUS_EXECUTION_FAILED";
        }else if (*((cublasStatus_t*) cublasStatus) == CUBLAS_STATUS_INTERNAL_ERROR){
            message += "CUBLAS_STATUS_INTERNAL_ERROR";
        }else if (*((cublasStatus_t*) cublasStatus) == CUBLAS_STATUS_NOT_SUPPORTED){
            message += "CUBLAS_STATUS_NOT_SUPPORTED";
        }else if (*((cublasStatus_t*) cublasStatus) == CUBLAS_STATUS_LICENSE_ERROR){
            message += "CUBLAS_STATUS_LICENSE_ERROR";
        }else{
            message += "UNKNOWN";
        }
        message += " at ";
        message += info;
        throw std::runtime_error(message);
    }
}
void AccelerationMeta::cusparseCheckError(void *cusparseStatus, const char *info){
    if (*((cusparseStatus_t*) cusparseStatus) != CUSPARSE_STATUS_SUCCESS){
        std::string message = "ERROR: cuSparse failed with code: ";
        if (*((cusparseStatus_t*) cusparseStatus) == CUSPARSE_STATUS_NOT_INITIALIZED){
            message += "CUSPARSE_STATUS_NOT_INITIALIZED";
        }else if (*((cusparseStatus_t*) cusparseStatus) == CUSPARSE_STATUS_ALLOC_FAILED){
            message += "CUSPARSE_STATUS_ALLOC_FAILED";
        }else if (*((cusparseStatus_t*) cusparseStatus) == CUSPARSE_STATUS_INVALID_VALUE){
            message += "CUSPARSE_STATUS_INVALID_VALUE";
        }else if (*((cusparseStatus_t*) cusparseStatus) == CUSPARSE_STATUS_ARCH_MISMATCH){
            message += "CUSPARSE_STATUS_ARCH_MISMATCH";
        }else if (*((cusparseStatus_t*) cusparseStatus) == CUSPARSE_STATUS_INTERNAL_ERROR){
            message += "CUSPARSE_STATUS_INTERNAL_ERROR";
        }else if (*((cusparseStatus_t*) cusparseStatus) == CUSPARSE_STATUS_MATRIX_TYPE_NOT_SUPPORTED){
            message += "CUSPARSE_STATUS_MATRIX_TYPE_NOT_SUPPORTED";
        }else if (*((cusparseStatus_t*) cusparseStatus) == CUSPARSE_STATUS_EXECUTION_FAILED){
            message += "CUSPARSE_STATUS_EXECUTION_FAILED";
        }else{
            message += "UNKNOWN";
        }
        message += " at ";
        message += info;
        throw std::runtime_error(message);
    }
}
int AccelerationMeta::getNumCudaDevices(){
    int gpu_count = 0;
    cudaGetDeviceCount(&gpu_count);
    return gpu_count;
}
void AccelerationMeta::setDefaultCudaDevice(int deviceID){
    cudaSetDevice(deviceID);
}
unsigned long long AccelerationMeta::getTotalGPUMemory(int deviceID){
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, deviceID);
    return prop.totalGlobalMem;
}
std::string AccelerationMeta::getCudaDeviceName(int deviceID){
    if ((deviceID < 0) || (deviceID >= getNumCudaDevices())) return std::string();

    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, deviceID);

    return std::string(prop.name);
}
template<typename T> void AccelerationMeta::recvCudaArray(size_t num_entries, const T *gpu_data, std::vector<T> &cpu_data){
    cpu_data.resize(num_entries);
    cudaError_t cudaStat = cudaMemcpy(cpu_data.data(), gpu_data, num_entries * sizeof(T), cudaMemcpyDeviceToHost);
    AccelerationMeta::cudaCheckError((void*) &cudaStat, "cudaRecv(type, type)");
}
template<typename T> void AccelerationMeta::delCudaArray(T *x){
    cudaError_t cudaStat = cudaFree(x);
    AccelerationMeta::cudaCheckError((void*) &cudaStat, "AccelerationMeta::delCudaArray(), call to cudaFree()");
}

void* AccelerationMeta::createCublasHandle(){
    cublasHandle_t handle;
    cublasCreate(&handle);
    return (void*) handle;
}
void AccelerationMeta::deleteCublasHandle(void *handle){
    cublasDestroy((cublasHandle_t) handle);
}

template void AccelerationMeta::recvCudaArray<double>(size_t num_entries, const double*, std::vector<double>&);
template void AccelerationMeta::recvCudaArray<float>(size_t num_entries, const float*, std::vector<float>&);
template void AccelerationMeta::recvCudaArray<int>(size_t num_entries, const int*, std::vector<int>&);

template void AccelerationMeta::delCudaArray<double>(double*);
template void AccelerationMeta::delCudaArray<float>(float*);
template void AccelerationMeta::delCudaArray<int>(int*);


AccelerationDomainTransform::AccelerationDomainTransform(std::vector<double> const &transform_a, std::vector<double> const &transform_b){
    // The points are stored contiguously in a vector with stride equal to num_dimensions
    // Using the contiguous memory in a contiguous fashion on the GPU implies that thread 0 works on dimension 0, thread 1 on dim 1 ...
    // But the number of dimensions is often way less than the number of threads
    // Therefore, we lump vectors together into large vectors of sufficient dimension
    // The dimension is least 512, but less than max CUDA threads 1024
    // The domain transforms are padded accordingly
    num_dimensions = (int) transform_a.size();
    padded_size = num_dimensions;
    while(padded_size < 512) padded_size += num_dimensions;

    std::vector<double> rate(padded_size);
    std::vector<double> shift(padded_size);
    int c = 0;
    for(int i=0; i<padded_size; i++){
        // instead of storing upper/lower limits (as in TasmanianSparseGrid) use rate and shift
        double diff = transform_b[c] - transform_a[c];
        rate[i] = 2.0 / diff;
        shift[i] = (transform_b[c] + transform_a[c]) / diff;
        c++;
        c = (c % num_dimensions);
    }

    gpu_trans_a.load(rate);
    gpu_trans_b.load(shift);
}

template<typename T>
void AccelerationDomainTransform::getCanonicalPoints(bool use01, const T *gpu_transformed_x, int num_x, CudaVector<T> &gpu_canonical_x){
    gpu_canonical_x.resize(((size_t) num_dimensions) * ((size_t) num_x));
    TasCUDA::dtrans2can(use01, num_dimensions, num_x, padded_size, gpu_trans_a.data(), gpu_trans_b.data(), gpu_transformed_x, gpu_canonical_x.data());
}

template void AccelerationDomainTransform::getCanonicalPoints<float>(bool, float const[], int, CudaVector<float>&);
template void AccelerationDomainTransform::getCanonicalPoints<double>(bool, double const[], int, CudaVector<double>&);
#endif // Tasmanian_ENABLE_CUDA


}

#endif
