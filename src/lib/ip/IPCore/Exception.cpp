//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPCore/Exception.h>

namespace IPCore
{

    TWK_DERIVED_EXCEPTION_IMP(ReadFailedExc)
    TWK_DERIVED_EXCEPTION_IMP(AllReadFailedExc)
    TWK_DERIVED_EXCEPTION_IMP(EvaluationFailedExc)
    TWK_DERIVED_EXCEPTION_IMP(RenderFailedExc)

    TWK_DERIVED_EXCEPTION_IMP(SingleInputOnlyExc)
    TWK_DERIVED_EXCEPTION_IMP(BadIndex)

    TWK_DERIVED_EXCEPTION_IMP(BadFrameRangeExc)
    TWK_DERIVED_EXCEPTION_IMP(EvaluationExc)
    TWK_DERIVED_EXCEPTION_IMP(MetaEvaluationExc)
    TWK_DERIVED_EXCEPTION_IMP(SequenceOutOfBoundsExc)
    TWK_DERIVED_EXCEPTION_IMP(LayerOutOfBoundsExc)
    TWK_DERIVED_EXCEPTION_IMP(PixelBlockSizeMismatchExc);

    TWK_DERIVED_EXCEPTION_IMP(RendererNotSupportedExc);
    TWK_DERIVED_EXCEPTION_IMP(PixelFormatNotSupportedExc);
    TWK_DERIVED_EXCEPTION_IMP(CacheFullExc);
    TWK_DERIVED_EXCEPTION_IMP(CacheMissExc);

    TWK_DERIVED_EXCEPTION_IMP(AudioFailedExc);
    TWK_DERIVED_EXCEPTION_IMP(StereoFailedExc);
    TWK_DERIVED_EXCEPTION_IMP(BufferNeedsRefillExc);

    TWK_DERIVED_EXCEPTION_IMP(BadPropertyExc);
    TWK_DERIVED_EXCEPTION_IMP(ShaderSignatureExc);

} // namespace IPCore
