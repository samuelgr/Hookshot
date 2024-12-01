/***************************************************************************************************
 * Hookshot
 *   General-purpose library for injecting DLLs and hooking function calls.
 ***************************************************************************************************
 * Authored by Samuel Grossman
 * Copyright (c) 2019-2024
 ***********************************************************************************************//**
 * @file Iterator.h
 *   Implementation of various types of iterators for internal containers.
 **************************************************************************************************/

#pragma once

#include <iterator>

namespace Hookshot
{
  /// Iterator type used to denote a position within a contiguous array of objects. Support random
  /// accesses.
  template <typename T> class ContiguousRandomAccessIterator
  {
  public:

    // Type aliases for compliance with STL random-access iterator specifications.
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = int;
    using pointer = T*;
    using reference = T&;

    constexpr ContiguousRandomAccessIterator(void) : buffer(), index() {}

    constexpr ContiguousRandomAccessIterator(T* buffer, int index) : buffer(buffer), index(index) {}

    constexpr T& operator[](int index) const
    {
      return *(*this + index);
    }

    constexpr T& operator*(void) const
    {
      return buffer[index];
    }

    constexpr T* operator->(void) const
    {
      return &buffer[index];
    }

    constexpr ContiguousRandomAccessIterator& operator++(void)
    {
      index += 1;
      return *this;
    }

    constexpr ContiguousRandomAccessIterator operator++(int)
    {
      ContiguousRandomAccessIterator orig = *this;
      index += 1;
      return orig;
    }

    constexpr ContiguousRandomAccessIterator& operator--(void)
    {
      index -= 1;
      return *this;
    }

    constexpr ContiguousRandomAccessIterator operator--(int)
    {
      ContiguousRandomAccessIterator orig = *this;
      index -= 1;
      return orig;
    }

    constexpr ContiguousRandomAccessIterator& operator+=(int indexIncrement)
    {
      index += indexIncrement;
      return *this;
    }

    constexpr ContiguousRandomAccessIterator operator+(int indexIncrement) const
    {
      return ContiguousRandomAccessIterator(buffer, index + indexIncrement);
    }

    constexpr ContiguousRandomAccessIterator& operator-=(int indexIncrement)
    {
      index -= indexIncrement;
      return *this;
    }

    constexpr ContiguousRandomAccessIterator operator-(int indexIncrement) const
    {
      return ContiguousRandomAccessIterator(buffer, index - indexIncrement);
    }

    constexpr int operator-(const ContiguousRandomAccessIterator& rhs) const
    {
      DebugAssert(buffer == rhs.buffer, "Iterators point to different instances.");
      return index - rhs.index;
    }

    constexpr bool operator==(const ContiguousRandomAccessIterator& other) const
    {
      DebugAssert(buffer == other.buffer, "Iterators point to different instances.");
      return (index == other.index);
    }

    constexpr bool operator!=(const ContiguousRandomAccessIterator& other) const
    {
      DebugAssert(buffer == other.buffer, "Iterators point to different instances.");
      return (index != other.index);
    }

    constexpr bool operator<(const ContiguousRandomAccessIterator& rhs) const
    {
      DebugAssert(buffer == rhs.buffer, "Iterators point to different instances.");
      return (index < rhs.index);
    }

    constexpr bool operator<=(const ContiguousRandomAccessIterator& rhs) const
    {
      DebugAssert(buffer == rhs.buffer, "Iterators point to different instances.");
      return (index <= rhs.index);
    }

    constexpr bool operator>(const ContiguousRandomAccessIterator& rhs) const
    {
      DebugAssert(buffer == rhs.buffer, "Iterators point to different instances.");
      return (index > rhs.index);
    }

    constexpr bool operator>=(const ContiguousRandomAccessIterator& rhs) const
    {
      DebugAssert(buffer == rhs.buffer, "Iterators point to different instances.");
      return (index >= rhs.index);
    }

  private:

    /// Pointer directly to the underlying data buffer.
    T* buffer;

    /// Index within the data buffer.
    int index;
  };

  /// Type alias for a constant version of #ContiguousRandomAccessIterator.
  template <typename T> using ContiguousRandomAccessConstIterator =
      ContiguousRandomAccessIterator<const T>;
} // namespace Hookshot
