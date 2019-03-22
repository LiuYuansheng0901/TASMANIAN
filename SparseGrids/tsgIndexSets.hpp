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

#ifndef __TASMANIAN_SPARSE_GRID_INDEX_SETS_HPP
#define __TASMANIAN_SPARSE_GRID_INDEX_SETS_HPP

#include <functional>
#include <algorithm>

#include "tsgIOHelpers.hpp"
#include "tsgUtils.hpp"

/*!
 * \internal
 * \file tsgIndexSets.hpp
 * \brief Data structures for storing multi-indexes and floating point values.
 * \author Miroslav Stoyanov
 * \ingroup TasmanianSets
 *
 * Three classes for storage and easy access to multi-indexes and floating point values.
 * The majority of internal data-structures associated with the five types of grids
 * can be represented as two-dimensional data (or array/list of tuples), the three
 * classes here correspond to sorted or unsorted, integer or floating point data.
 */

/*!
 * \internal
 * \ingroup TasmanianSG
 * \addtogroup TasmanianSets Multi-Index data structures
 *
 * \par Fundamental data structures
 * Three classes for storage and easy access to multi-indexes and floating point values.
 * The majority of internal data-structures associated with the five types of grids
 * can be represented as two-dimensional data (or array/list of tuples), the three
 * classes here correspond to sorted or unsorted, integer or floating point data.
 * \endinternal
 */

namespace TasGrid{

/*!
 * \ingroup TasmanianSets
 * \brief Generic 2D data structure divided into contiguous strips of fixed length (similar to a matrix).
 *
 * Many of the internal data-structures relevant to sparse grids can be represented as two dimensional arrays.
 * The data is divided into \b strips of equal \b stride, e.g., number of multi-indexes and number of dimensions.
 * Internally the class uses \b std::vector with type \b T,
 * when used, \b T is almost always \b double or \b int.
 */
template<typename T>
class Data2D{
public:
    //! \brief Default constructor makes an empty data-structure.
    Data2D() : stride(0), num_strips(0){}
    //! \brief Create data-structure with given \b stride and number of \b strips.
    Data2D(int new_stride, int new_num_strips) : stride((size_t) new_stride), num_strips((size_t) new_num_strips), vec(stride * num_strips){}
    //! \brief Create data-structure with given \b stride and number of \b strips.
    Data2D(size_t new_stride, int new_num_strips) : stride(new_stride), num_strips((size_t) new_num_strips), vec(stride * num_strips){}
    //! \brief Create data-structure with given \b stride and number of \b strips and initializes with \b val.
    Data2D(int new_stride, int new_num_strips, T val) : stride((size_t) new_stride), num_strips((size_t) new_num_strips), vec(stride * num_strips, val){}
    //! \brief Default destructor.
    ~Data2D(){}

    //! \brief Returns \b true if the number of strips is zero.
    bool empty() const{ return (num_strips == 0); }

    //! \brief Clear any existing data and allocate a new data-structure with given \b stride and number of \b strips.
    void resize(int new_stride, int new_num_strips){
        stride = (size_t) new_stride;
        num_strips = (size_t) new_num_strips;
        vec.resize(stride * num_strips);
    }

    //! \brief Returns a reference to the \b i-th strip.
    T* getStrip(int i){ return &(vec[i*stride]); }
    //! \brief Returns a const reference to the \b i-th strip.
    T const* getStrip(int i) const{ return &(vec[i*stride]); }
    //! \brief Returns the stride.
    size_t getStride() const{ return stride; }
    //! \brief Returns the number of strips.
    int getNumStrips() const{ return (int) num_strips; }
    //! \brief Returns the total number of entries, stride times number of trips.
    size_t getTotalEntries() const{ return vec.size(); }
    //! \brief Returns a reference to the internal data.
    std::vector<T>& getVector(){ return vec; }
    //! \brief Returns a const reference to the internal data.
    const std::vector<T>& getVector() const{ return vec; }
    //! \brief Clear all used data.
    void clear(){
        stride = 0;
        num_strips = 0;
        vec = std::vector<double>();
    }

    //! \brief Uses std::vector::insert to append \b x, assumes \b x.size() is one stride.
    void appendStrip(const std::vector<T> &x){
        vec.insert(vec.end(), x.begin(), x.end());
        num_strips++;
    }

    //! \brief Uses std::vector::insert to append a strip \b x to the existing data at position \b pos, assumes \b x.size() is one stride.
    void appendStrip(int pos, const std::vector<T> &x){
        vec.insert(vec.begin() + (((size_t) pos) * stride), x.begin(), x.end());
        num_strips++;
    }

    //! \brief Fill the entire vector with the specified \b value.
    void fill(T value){ std::fill(vec.begin(), vec.end(), value); }

private:
    size_t stride, num_strips;
    std::vector<T> vec;
};

/*!
 * \ingroup TasmanianSets
 * \brief Class that stores multi-indexes in sorted (lexicographical) order.
 *
 * At the core of each sparse grid, there are multiple multi-index sets.
 * The organization of the data is similar to the \b Data2D class, but at any time the indexes
 * are stored in a lexicographical order. The main functionality provided here is:
 * - fast O(log(n)) search utilizing the lexicographical order
 * - synchronization between multi-indexes and values (i.e., model outputs)
 * - adding or removing indexes while preserving the order
 * - basic file I/O
 */
class MultiIndexSet{
public:
    //! \brief Default constructor, makes an empty set.
    MultiIndexSet() : num_dimensions(0), cache_num_indexes(0){}
    //! \brief Constructor, makes a set by \b moving out of the vector, the vector must be already sorted.
    MultiIndexSet(size_t cnum_dimensions, std::vector<int> &new_indexes) :
        num_dimensions(cnum_dimensions), cache_num_indexes((int)(new_indexes.size() / cnum_dimensions)), indexes(std::move(new_indexes)){}
    //! \brief Copy a collection of unsorted indexes into a sorted multi-index set, sorts during the copy.
    MultiIndexSet(Data2D<int> &data) : num_dimensions((size_t) data.getStride()), cache_num_indexes(0){ setData2D(data); }
    //! \brief Default destructor.
    ~MultiIndexSet(){}

    //! \brief Write the set to ASCII or binary stream, use with std::ofstream and std::ifstream.

    //! The format consists of two `int` values corresponding to the number of dimensions and number of indexes,
    //! followed by all the entries of the array on a single line separated by a space, or dump of a single write command.
    template<bool useAscii> void write(std::ostream &os) const;

    //! \brief Read the from the stream, must know whether to use ASCII or binary format.

    //! Uses the same format as \b write<bool>
    template<bool useAscii> void read(std::istream &os);

    //! \brief Returns **true** if there are no multi-indexes in the set, **false** otherwise
    inline bool empty() const{ return indexes.empty(); }

    //! \brief Returns the number of dimensions
    inline size_t getNumDimensions() const{ return num_dimensions; }
    //! \brief Returns the number of indexes
    inline int getNumIndexes() const{ return cache_num_indexes; }

    //! \brief Add more indexes to a non-empty set, \b addition must be sorted and the set must be initialized.
    void addSortedIndexes(std::vector<int> const &addition);
    //! \brief If empty, copy \b addition, otherwise merge the indexes of \b addition into this set.
    inline void addMultiIndexSet(MultiIndexSet const  &addition){
        num_dimensions = addition.getNumDimensions();
        addSortedIndexes(addition.getVector());
    }

    //! \brief Returns a const reference to the internal data
    inline const std::vector<int>& getVector() const{ return indexes; }
    //! \brief Returns a reference to the internal data, must not modify the lexicographical order or the size of the vector
    inline std::vector<int>& getVector(){ return indexes; } // used for remapping during tensor generic points

    //! \brief Returns the slot containing index **p**, returns `-1` if not found
    int getSlot(const int *p) const;
    //! \brief Returns the slot containing index **p**, returns `-1` if not found
    inline int getSlot(const std::vector<int> &p) const{ return getSlot(p.data()); }
    //! \brief Returns **true** if **p** is missing from the set, **false** otherwise
    inline bool missing(const std::vector<int> &p) const{ return (getSlot(p.data()) == -1); }

    //! \brief Returns the **i**-th index of the set, useful to loop over all indexes or to cross reference with values
    inline const int *getIndex(int i) const{ return &(indexes[((size_t) i) * num_dimensions]); }

    /*! \brief Return a new multi-index set that holds the indexed present in this set, but missing in \b substract.
     *
     * Assumes that \b substract has the same dimension as this set.
     * The implementation uses an algorithm similar to merge with complexity linear in the number of multi-indexes of the two sets,
     * i.e., does not use \b missing() which would add a logarithmic factor.
     */
    MultiIndexSet diffSets(const MultiIndexSet &substract);

    //! \brief Removes \b p from the set (if exists).
    void removeIndex(const std::vector<int> &p);

    //! \brief Returns the maximum single index in the set.
    int getMaxIndex() const{ return (empty()) ? 0 : *std::max_element(indexes.begin(), indexes.end()); }

protected:
    //! \brief Copy and sort the indexes from the \b data, called only from the constructor.
    void setData2D(Data2D<int> const &data);

private:
    size_t num_dimensions;
    int cache_num_indexes;
    std::vector<int> indexes;
};

/*!
 * \ingroup TasmanianSets
 * \brief Class that stores values, i.e., model outputs, the order of the values is in sync with the order of some **MultiIndexSet**
 *
 * The StorageSet stores the floating-point values (model outputs) in a contiguous order, suitable for interfacing with BLAS/cuBlas.
 * The values associated with one model simulation are adjacent to each other in a stride of size \b num_outputs.
 * Each grid type has one instance of the \b StorageSet and the order of the stored values is kept in sync
 * with the \b points multi-index set.
 * Synchronization is achieved by merging values before merging multi-indexes, when
 * the \b addValues() is called with the old and new multi-index sets and the new values.
 */
class StorageSet{
public:
    //! \brief Default constructor, makes an empty set.
    StorageSet();
    //! \brief Default destructor.
    ~StorageSet();

    /*!
     * \brief Write the set to ASCII or binary stream, use with std::ofstream and std::ifstream.
     *
     * The format consists of two `int` values corresponding to the number of dimensions and number of indexes,
     * followed by all the entries of the array on a single line separated by a space, or dump of a single write command.
     */
    template<bool useAscii> void write(std::ostream &os) const;

    //! \brief Read the from the stream, must know whether to use ASCII or binary format.
    template<bool useAscii> void read(std::istream &os);

    //! \brief Clear the existing values and assigns new dimensions, does not allocate memory for the new values.
    void resize(int cnum_outputs, int cnum_values);

    //! \brief Returns const reference to the \b i-th value.
    const double* getValues(int i) const;
    //! \brief Returns reference to the \b i-th value.
    double* getValues(int i);
    //! \brief Returns reference to the internal data vector.
    std::vector<double>& getVector(){ return values; }
    //! \brief Returns const reference to the internal data vector.
    const std::vector<double>& getVector() const{ return values; }

    //! \brief Replace the existing values with a copy of **vals**, the size must be at least **num_outputs** times **num_values**
    void setValues(const double vals[]);
    //! \brief Replace the existing values with **vals** using `std::move()`, the size of **vals** must be **num_outputs** times **num_values**
    void setValues(std::vector<double> &vals);

    /*!
     * \brief Add more values to the set, the \b old_set and \b new_set are the associated multi-index sets required to maintain order.
     *
     * Add more values to an existing set of values, where \b old_set and \b new_set indicate the order.
     * The \b old_set contains the ordered multi-indexes associated with the current values,
     * the \b new_set corresponds to the order of the \b new_vals.
     * After the merge, the order of the values will match that of the union of \b old_set and \b new_set.
     *
     * Note that the two multi-index sets cannot have repeated entries.
     */
    void addValues(const MultiIndexSet &old_set, const MultiIndexSet &new_set, const double new_vals[]);

private:
    size_t num_outputs, num_values; // kept as size_t to avoid conversions in products, but each one is small individually
    std::vector<double> values;
};

}

#endif
