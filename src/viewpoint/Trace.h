/******************************************************************************
* @file Trace.h
* @brief Trace data storage with flexible memory allocation strategies
*
* Part of the ViewPoint Arduino library for high-speed serial plotting.
*
* Supports three allocation strategies:
* 1. Library-managed: Library allocates and owns the buffer
* 2. External buffer: User provides buffer, library uses directly
* 3. Direct pointer: User sets pointer to existing data array
*
*   This file is a part of the VoidLoop ViewPoint project, which is one of the 
*   projects brought to the education and hobbyist community by VoidLoop.    
*   Please see www.voidloop.com for updates.
*
* Contributors:
* Initial vision, feature definition, debugging and funding: Greg Kovacs
* Architecture, Development, debugging and support: Zachariah Magee
* 
* Thanks to our many collaborators and friends!
* 
* MIT License (MIT)
* 
* Copyright (c) 2026 VoidLoop
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
 ******************************************************************************/

#ifndef _VIEWPOINT_TRACE_H_
#define _VIEWPOINT_TRACE_H_

#include "Types.h"
#include "Colors.h"
#include <math.h>
#include <string.h>

namespace viewpoint {

/**
 * @brief Memory ownership model for Trace data
 */
enum class MemoryMode : uint8_t {
    None,           // No data allocated
    LibraryOwned,   // Library allocated with new[]
    External,       // User-provided buffer (library doesn't free)
    Pointer         // Direct pointer to user data (no size tracking)
};

/**
 * @brief Data type indicator for single vs paired values
 */
enum class DataType : uint8_t {
    Single,   // Single float per sample (y value)
    Paired    // Two floats per sample (x,y or rho,theta)
};

/**
 * Use to define **Derived Traces** or traces that only exist within ViewPoint app
 * and are create from a source trace. Both the source trace and the derived trace
 * can be viewed simultaneously
 */
enum class Filter : uint8_t {
    Average, // Moving average
    MaxHold  // Max Hold - Peak Hold
};

struct DerivedTrace {
    uint16_t source = 0;
    Filter filter = Filter::Average;
    float param = 0.0f;
    uint32_t color = NO_COLOR;
    char label[VIEWPOINT_LABEL_SIZE] = { 0 };
};


class Trace {
public:
    static constexpr uint8_t TRACE_DIRTY_LABEL = 1u << 0;
    static constexpr uint8_t TRACE_DIRTY_COLOR = 1u << 1;

    /**
     * @brief Default constructor - no allocation
     */
    Trace()
        : id_(-1)
        , color_(NO_COLOR)
        , data1_(nullptr)
        , data2_(nullptr)
        , size_(0)
        , capacity_(0)
        , memoryMode_(MemoryMode::None)
        , dataType_(DataType::Single)
    {
        label_[0] = '\0';
    }

    /**
     * @brief Create trace with library-managed memory
     * @param id Trace identifier
     * @param capacity Initial buffer capacity (default 500)
     */
    explicit Trace(int id, size_t capacity = 500)
        : id_(id)
        , color_(NO_COLOR)
        , data1_(nullptr)
        , data2_(nullptr)
        , size_(0)
        , capacity_(0)
        , memoryMode_(MemoryMode::None)
        , dataType_(DataType::Single)
    {
        label_[0] = '\0';
        allocate(capacity);
    }

    /**
     * @brief Create trace with external single-value buffer
     * @param id Trace identifier
     * @param buffer User-provided buffer (not freed by Trace)
     * @param bufferSize Size of buffer in floats
     */
    Trace(int id, float* buffer, size_t bufferSize)
        : id_(id)
        , color_(NO_COLOR)
        , data1_(buffer)
        , data2_(nullptr)
        , size_(bufferSize)
        , capacity_(bufferSize)
        , memoryMode_(MemoryMode::External)
        , dataType_(DataType::Single)
    {
        label_[0] = '\0';
    }

    /**
     * @brief Create trace with external paired-value buffers
     * @param id Trace identifier
     * @param buffer1 User-provided buffer for first values (x or rho)
     * @param buffer2 User-provided buffer for second values (y or theta)
     * @param bufferSize Size of each buffer in floats
     */
    Trace(int id, float* buffer1, float* buffer2, size_t bufferSize)
        : id_(id)
        , color_(NO_COLOR)
        , data1_(buffer1)
        , data2_(buffer2)
        , size_(bufferSize)
        , capacity_(bufferSize)
        , memoryMode_(MemoryMode::External)
        , dataType_(DataType::Paired)
    {
        label_[0] = '\0';
    }

    /**
     * @brief Destructor - frees memory only if library-owned
     */
    ~Trace() {
        freeIfOwned();
    }

    
    // Move semantics (no copying to prevent double-free)
    

    Trace(Trace&& other) noexcept
        : id_(other.id_)
        , color_(other.color_)
        , data1_(other.data1_)
        , data2_(other.data2_)
        , size_(other.size_)
        , capacity_(other.capacity_)
        , memoryMode_(other.memoryMode_)
        , dataType_(other.dataType_)
        , metaDirty_(other.metaDirty_)
    {
        memcpy(label_, other.label_, VIEWPOINT_LABEL_SIZE);
        // Clear other's pointers to prevent double-free
        other.data1_ = nullptr;
        other.data2_ = nullptr;
        other.memoryMode_ = MemoryMode::None;
    }

    Trace& operator=(Trace&& other) noexcept {
        if (this != &other) {
            freeIfOwned();
            id_ = other.id_;
            color_ = other.color_;
            data1_ = other.data1_;
            data2_ = other.data2_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            memoryMode_ = other.memoryMode_;
            dataType_ = other.dataType_;
            metaDirty_ = other.metaDirty_;
            memcpy(label_, other.label_, VIEWPOINT_LABEL_SIZE);
            other.data1_ = nullptr;
            other.data2_ = nullptr;
            other.memoryMode_ = MemoryMode::None;
        }
        return *this;
    }

    // Disable copy
    Trace(const Trace&) = delete;
    Trace& operator=(const Trace&) = delete;

    
    // Data addition - Single values
    

    /**
     * @brief Add a single value to the trace
     * @param value The value to add
     */
    void addData(float value) {
        ensureCapacity(size_ + 1);
        if (data1_ && size_ < capacity_) {
            data1_[size_++] = value;
            dataType_ = DataType::Single;
        }
    }

    /**
     * @brief Add an array of single values
     * @param data Array of values
     * @param count Number of values to add
     */
    void addData(const float* data, size_t count) {
        if (!data || count == 0) return;
        ensureCapacity(size_ + count);
        if (data1_) {
            size_t toAdd = (size_ + count <= capacity_) ? count : (capacity_ - size_);
            memcpy(data1_ + size_, data, toAdd * sizeof(float));
            size_ += toAdd;
            dataType_ = DataType::Single;
        }
    }

    
    // Data addition - Paired values (scatter/polar)
    

    /**
     * @brief Add a pair of values (x,y or rho,theta)
     * @param v1 First value (x or rho)
     * @param v2 Second value (y or theta)
     */
    void addData(float v1, float v2) {
        ensureCapacity(size_ + 1, true);
        if (data1_ && data2_ && size_ < capacity_) {
            data1_[size_] = v1;
            data2_[size_] = v2;
            size_++;
            dataType_ = DataType::Paired;
        }
    }

    /**
     * @brief Add arrays of paired values
     * @param d1 Array of first values
     * @param d2 Array of second values
     * @param count Number of pairs to add
     */
    void addData(const float* d1, const float* d2, size_t count) {
        if (!d1 || !d2 || count == 0) return;
        ensureCapacity(size_ + count, true);
        if (data1_ && data2_) {
            size_t toAdd = (size_ + count <= capacity_) ? count : (capacity_ - size_);
            memcpy(data1_ + size_, d1, toAdd * sizeof(float));
            memcpy(data2_ + size_, d2, toAdd * sizeof(float));
            size_ += toAdd;
            dataType_ = DataType::Paired;
        }
    }

    /**
     * @brief Insert a break marker (Scatter/Polar only)
     *
     * Inserts a NaN,0 pair that causes the renderer to start a new
     * line segment without connecting to the previous point.
     */
    void addBreak() {
        addData(NAN, 0.0f);
    }


    // Data setting - Replace all data
    

    /**
     * @brief Set data pointer (zero-copy for external data)
     * @param data Pointer to data array
     * @param count Number of elements
     */
    void setData(float* data, size_t count) {
        freeIfOwned();
        data1_ = data;
        data2_ = nullptr;
        size_ = count;
        capacity_ = count;
        memoryMode_ = MemoryMode::Pointer;
        dataType_ = DataType::Single;
    }

    /**
     * @brief Set paired data pointers (zero-copy for external data)
     * @param d1 Pointer to first data array
     * @param d2 Pointer to second data array
     * @param count Number of elements in each array
     */
    void setData(float* d1, float* d2, size_t count) {
        freeIfOwned();
        data1_ = d1;
        data2_ = d2;
        size_ = count;
        capacity_ = count;
        memoryMode_ = MemoryMode::Pointer;
        dataType_ = DataType::Paired;
    }

    
    // Data access
    

    /**
     * @brief Clear stored data (resets size, keeps capacity)
     */
    void clear() {
        size_ = 0;
    }

    /**
     * @brief Get single value at index
     */
    float get(size_t index) const {
        if (data1_ && index < size_) {
            return data1_[index];
        }
        return 0.0f;
    }

    /**
     * @brief Get second value at index (for paired data)
     */
    float get2(size_t index) const {
        if (data2_ && index < size_) {
            return data2_[index];
        }
        return 0.0f;
    }

    /**
     * @brief Array access operator
     */
    float operator[](size_t index) const {
        return get(index);
    }

    
    // Configuration
    

    /**
     * @brief Set the trace label
     */
    void setLabel(const char* label) {
        if (label) {
            strncpy(label_, label, VIEWPOINT_LABEL_SIZE - 1);
            label_[VIEWPOINT_LABEL_SIZE - 1] = '\0';
        } else {
            label_[0] = '\0';
        }
        metaDirty_ |= TRACE_DIRTY_LABEL;
    }

    /**
     * @brief Set the trace color (0xRRGGBB)
     */
    void setColor(uint32_t color) {
        color_ = color;
        metaDirty_ |= TRACE_DIRTY_COLOR;
    }

    /**
     * @brief Set trace ID
     */
    void setId(int id) {
        id_ = id;
        if (!hasColor(color_)) {
            color_ = defaultTraceColor(id);
            metaDirty_ |= TRACE_DIRTY_COLOR;
        }
    }

    
    // Accessors
    

    int id() const { return id_; }

    bool hasLabel() const { return label_[0] != '\0'; }
    const char* label() const { return label_; }

    uint32_t color() const { return color_; }

    uint8_t metaDirty() const { return metaDirty_; }
    void clearMetaDirty() { metaDirty_ = 0; }

    const float* data() const { return data1_; }
    float* data() { return data1_; }

    const float* data2() const { return data2_; }
    float* data2() { return data2_; }

    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }
    bool empty() const { return size_ == 0; }

    DataType dataType() const { return dataType_; }
    bool isPaired() const { return dataType_ == DataType::Paired; }

    MemoryMode memoryMode() const { return memoryMode_; }
    bool ownsMemory() const { return memoryMode_ == MemoryMode::LibraryOwned; }

private:
    
    // Memory management
    

    void allocate(size_t newCapacity) {
        freeIfOwned();
        if (newCapacity > 0) {
            data1_ = new float[newCapacity];
            capacity_ = newCapacity;
            memoryMode_ = MemoryMode::LibraryOwned;
        }
    }

    void allocatePaired(size_t newCapacity) {
        freeIfOwned();
        if (newCapacity > 0) {
            data1_ = new float[newCapacity];
            data2_ = new float[newCapacity];
            capacity_ = newCapacity;
            memoryMode_ = MemoryMode::LibraryOwned;
        }
    }

    void freeIfOwned() {
        if (memoryMode_ == MemoryMode::LibraryOwned) {
            delete[] data1_;
            delete[] data2_;
        }
        data1_ = nullptr;
        data2_ = nullptr;
        capacity_ = 0;
        size_ = 0;
        memoryMode_ = MemoryMode::None;
    }

    void ensureCapacity(size_t needed, bool paired = false) {
        // Only grow if library-owned or not yet allocated
        if (memoryMode_ == MemoryMode::External || memoryMode_ == MemoryMode::Pointer) {
            return; // Can't resize external buffers
        }

        if (needed <= capacity_) {
            // Need to allocate data2 for paired mode?
            if (paired && !data2_ && memoryMode_ == MemoryMode::LibraryOwned) {
                data2_ = new float[capacity_];
            }
            return;
        }

        // Grow capacity (double or use needed, whichever is larger)
        size_t newCapacity = capacity_ * 2;
        if (newCapacity < needed) newCapacity = needed;
        if (newCapacity < 64) newCapacity = 64; // Minimum allocation

        // Allocate new buffers
        float* newData1 = new float[newCapacity];
        float* newData2 = nullptr;

        if (paired || data2_) {
            newData2 = new float[newCapacity];
        }

        // Copy existing data
        if (data1_ && size_ > 0) {
            memcpy(newData1, data1_, size_ * sizeof(float));
        }
        if (data2_ && newData2 && size_ > 0) {
            memcpy(newData2, data2_, size_ * sizeof(float));
        }

        // Free old buffers if owned
        if (memoryMode_ == MemoryMode::LibraryOwned) {
            delete[] data1_;
            delete[] data2_;
        }

        data1_ = newData1;
        data2_ = newData2;
        capacity_ = newCapacity;
        memoryMode_ = MemoryMode::LibraryOwned;
    }

    
    // Member variables
    int id_;
    uint32_t color_;
    char label_[VIEWPOINT_LABEL_SIZE];

    float* data1_;
    float* data2_;
    size_t size_;
    size_t capacity_;

    MemoryMode memoryMode_;
    DataType dataType_;
    uint8_t metaDirty_ = 0;
};

} // namespace viewpoint

#endif // _VIEWPOINT_TRACE_H_
