//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkMath__TwkMathSparseIntervalSet__h__
#define __TwkMath__TwkMathSparseIntervalSet__h__
#include <TwkMath/Interval.h>
#include <algorithm>
#include <deque>

namespace TwkMath
{

    //
    //  A run-length encoded set as a container. Good for frames, etc.
    //

    template <class T> class SparseIntervalSet
    {
    public:
        typedef TwkMath::Interval<T> value_type;

        template <class Q> struct Comp
        {
            typedef TwkMath::Interval<Q> I;

            bool operator()(const I& a, const I& b) { return a.min < b.min; }
        };

        typedef std::deque<value_type> container;
        typedef typename container::iterator iterator;
        typedef typename container::const_iterator const_iterator;

        void insert(const T& value);
        bool includes(const T& value) const;

        iterator begin() { return m_container.begin(); }

        iterator end() { return m_container.end(); }

        const_iterator begin() const { return m_container.begin(); }

        const_iterator end() const { return m_container.end(); }

    private:
        container m_container;
    };

    template <class T> bool SparseIntervalSet<T>::includes(const T& value) const
    {
        value_type iv(value, value);
        const_iterator i =
            lower_bound(m_container.begin(), m_container.end(), iv, Comp<T>());

        if (i == m_container.begin())
        {
            return false;
        }
        else if (i == m_container.end())
        {
            return m_container.back().intersects(value);
        }
    }

    template <class T> void SparseIntervalSet<T>::insert(const T& value)
    {
        if (m_container.empty())
        {
            m_container.push_front(value_type(value, value));
        }
        else
        {
            value_type iv(value, value);

            iterator i = lower_bound(m_container.begin(), m_container.end(), iv,
                                     Comp<T>());

            if (i == m_container.begin())
            {
                value_type& first = *i;

                if (first.min == value + 1)
                {
                    first.min = value;
                }
                else
                {
                    m_container.push_front(iv);
                }
            }
            else
            {
                bool atend = i == m_container.end();
                i--;
                value_type& range = *i;

                if (range.intersects(value))
                {
                    return;
                }
                else if (range.max == value - 1)
                {
                    range.max = value;
                    if (atend)
                        return;
                }
                else
                {
                    m_container.insert(i, iv);
                    if (atend || iv.max < i->min)
                        return;
                }
            }

            //
            //  Fix ranges. This will maintain the set ordering.
            //

            for (iterator i = m_container.begin(); (i + 1) < m_container.end();)
            {
                iterator j = i + 1;

                if (i->intersects(j->min))
                {
                    i->max = j->max;
                    m_container.erase(j);
                }
                else
                {
                    ++i;
                }
            }
        }
    }

} // namespace TwkMath

#endif // __TwkMath__TwkMathSparseIntervalSet__h__
