#ifndef __Mu__PlacementParameter__h__
#define __Mu__PlacementParameter__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/ParameterVariable.h>

namespace Mu
{

    //
    //  class PlacementParameter
    //
    //  A temporary parameter which is referenced by is argument number
    //  instead of a proper name. The PlacementParameter must be erased
    //  before evaluation can occur.
    //

    class PlacementParameter : public ParameterVariable
    {
    public:
        PlacementParameter(int index);

        virtual ~PlacementParameter();

        virtual void output(std::ostream& o) const;
    };

} // namespace Mu

#endif // __Mu__PlacementParameter__h__
