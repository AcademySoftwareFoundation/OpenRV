//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__Exception__h__
#define __IPCore__Exception__h__
#include <TwkExc/Exception.h>

namespace IPCore
{

    TWK_DERIVED_EXCEPTION(ReadFailedExc)
    TWK_DERIVED_EXCEPTION(AllReadFailedExc)
    TWK_DERIVED_EXCEPTION(EvaluationFailedExc)
    TWK_DERIVED_EXCEPTION(RenderFailedExc)

    TWK_DERIVED_EXCEPTION(SingleInputOnlyExc)
    TWK_DERIVED_EXCEPTION(BadIndex)

    TWK_DERIVED_EXCEPTION(BadFrameRangeExc)
    TWK_DERIVED_EXCEPTION(EvaluationExc)
    TWK_DERIVED_EXCEPTION(MetaEvaluationExc)
    TWK_DERIVED_EXCEPTION(SequenceOutOfBoundsExc)
    TWK_DERIVED_EXCEPTION(LayerOutOfBoundsExc)
    TWK_DERIVED_EXCEPTION(PixelBlockSizeMismatchExc);

    TWK_DERIVED_EXCEPTION(RendererNotSupportedExc);
    TWK_DERIVED_EXCEPTION(PixelFormatNotSupportedExc);
    TWK_DERIVED_EXCEPTION(CacheFullExc);
    TWK_DERIVED_EXCEPTION(CacheMissExc);

    TWK_DERIVED_EXCEPTION(AudioFailedExc);
    TWK_DERIVED_EXCEPTION(StereoFailedExc);
    TWK_DERIVED_EXCEPTION(BufferNeedsRefillExc);

    TWK_DERIVED_EXCEPTION(BadPropertyExc);
    TWK_DERIVED_EXCEPTION(ShaderSignatureExc);

} // namespace IPCore

#endif // __IPCore__Exception__h__
