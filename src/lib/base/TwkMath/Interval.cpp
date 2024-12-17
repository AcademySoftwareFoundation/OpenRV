//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkMath/Interval.h>

namespace TwkMath
{

    //******************************************************************************
    BooleanInterval operator==(BooleanInterval a, BooleanInterval b)
    {
        if (same(a, IVL_FALSE))
        {
            if (same(b, IVL_FALSE))
            {
                return IVL_TRUE;
            }
            else if (same(b, IVL_MAYBE))
            {
                return IVL_MAYBE;
            }
            else
            {
                return IVL_FALSE;
            }
        }
        else if (same(a, IVL_TRUE))
        {
            if (same(b, IVL_TRUE))
            {
                return IVL_TRUE;
            }
            else if (same(b, IVL_MAYBE))
            {
                return IVL_MAYBE;
            }
            else
            {
                return IVL_FALSE;
            }
        }
        else
        {
            // IVL_MAYBE is always MAYBE equal to anything
            return IVL_MAYBE;
        }
    }

    //******************************************************************************
    BooleanInterval operator!=(BooleanInterval a, BooleanInterval b)
    {
        if (same(a, IVL_FALSE))
        {
            if (same(b, IVL_FALSE))
            {
                return IVL_FALSE;
            }
            else if (same(b, IVL_MAYBE))
            {
                return IVL_MAYBE;
            }
            else
            {
                return IVL_TRUE;
            }
        }
        else if (same(a, IVL_TRUE))
        {
            if (same(b, IVL_TRUE))
            {
                return IVL_FALSE;
            }
            else if (same(b, IVL_MAYBE))
            {
                return IVL_MAYBE;
            }
            else
            {
                return IVL_TRUE;
            }
        }
        else
        {
            // Maybe is always MAYBE not equal to anything
            return IVL_MAYBE;
        }
    }

    //******************************************************************************
    BooleanInterval operator&&(BooleanInterval a, BooleanInterval b)
    {
        if (same(a, IVL_FALSE) || same(b, IVL_FALSE))
        {
            return IVL_FALSE;
        }
        else if (same(a, IVL_MAYBE) || same(b, IVL_MAYBE))
        {
            return IVL_MAYBE;
        }
        else
        {
            return IVL_TRUE;
        }
    }

    //******************************************************************************
    BooleanInterval operator||(BooleanInterval a, BooleanInterval b)
    {
        if (same(a, IVL_TRUE) || same(b, IVL_TRUE))
        {
            return IVL_TRUE;
        }
        else if (same(a, IVL_MAYBE) || same(b, IVL_MAYBE))
        {
            return IVL_MAYBE;
        }
        else
        {
            return IVL_FALSE;
        }
    }

} // End namespace TwkMath
