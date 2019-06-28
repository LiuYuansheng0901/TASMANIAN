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

#ifndef __TASMANIAN_ADDONS_MPIGRIDSCATTER_HPP
#define __TASMANIAN_ADDONS_MPIGRIDSCATTER_HPP

/*!
 * \internal
 * \file tsgMPIScatterGrid.hpp
 * \brief Sparse Grids send/receive through MPI.
 * \author Miroslav Stoyanov
 * \ingroup TasmanianAddonsCommon
 *
 * Templates that communicate sparse grids through MPI commands.
 * \endinternal
 */

#include "tsgAddonsCommon.hpp"

/*!
 * \ingroup TasmanianAddons
 * \addtogroup TasmanianAddonsMPIGridSend MPI Send/Receive for Sparse Grids
 *
 * Methods to send/receive TasGrid::TasmanianSparseGrid objects.
 * The syntax mimics the raw MPI_Send and MPI_Recv calls,
 * and the templates require Tasmanian_ENABLE_MPI=ON.
 */

#ifdef Tasmanian_ENABLE_MPI

namespace TasGrid{

/*!
 * \internal
 * \ingroup TasmanianAddonsMPIGridSend
 * \brief Coverts a vector to basic stream-buffer.
 *
 * The stream-buffer is the data portion of the stream, this class converts
 * a char-vector to such buffer.
 * The stream buffer will simply assume the begin, end and data-type pointers
 * from the vector, therefore, using iterator invalidating operations on
 * the vector that invalidate will also invalidate the stream-buffer.
 * \endinternal
 */
class VectorToStreamBuffer : public std::basic_streambuf<char, std::char_traits<char>>{
public:
    //! \brief Make a stream-buffer from the \b data vector.
    VectorToStreamBuffer(std::vector<char> &data){
        setg(&*data.begin(), data.data(), &*data.end());
    }
};


/*!
 * \ingroup TasmanianAddonsMPIGridSend
 * \brief Send a grid to another process in the MPI comm.
 */
template<bool binary = true>
int MPIGridSend(TasmanianSparseGrid const &grid, int destination, int tag_size, int tag_data, MPI_Comm comm){
    std::stringstream ss;
    grid.write(ss, binary);

    while(ss.str().size() % 16 != 0) ss << " "; // pad with empty chars to align to 16 bytes, i.e., long double

    unsigned long long data_size = (unsigned long long) ss.str().size();
    auto result = MPI_Send(&data_size, 1, MPI_UNSIGNED_LONG_LONG, destination, tag_size, comm);
    if (result != MPI_SUCCESS) return result;

    return MPI_Send(ss.str().c_str(), (int) (data_size / 16), MPI_LONG_DOUBLE, destination, tag_data, comm);
}

/*!
 * \ingroup TasmanianAddonsMPIGridSend
 * \brief Receive a grid from another process in the MPI comm.
 */
template<bool binary = true>
int MPIGridRecv(TasmanianSparseGrid &grid, int source, int tag_size, int tag_data, MPI_Comm comm, MPI_Status *status){
    unsigned long long data_size;

    auto result = MPI_Recv(&data_size, 1, MPI_UNSIGNED_LONG_LONG, source, tag_size, comm,  status);
    if (result != MPI_SUCCESS) return result;

    std::vector<char> buff((size_t) data_size);
    result = MPI_Recv(buff.data(), (int) buff.size(), MPI_UNSIGNED_CHAR, source, tag_data, comm, status);

    VectorToStreamBuffer data_buffer(buff); // do not modify buff after this point
    std::istream is(&data_buffer);
    grid.read(is, binary);
    return result;
}

}

#endif // Tasmanian_ENABLE_MPI

#endif
