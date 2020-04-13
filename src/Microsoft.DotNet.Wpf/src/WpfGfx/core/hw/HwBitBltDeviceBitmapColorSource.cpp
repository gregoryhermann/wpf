// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//
//  Class:
//      CHwBitBltDeviceBitmapColorSource
//
//  Synopsis:
//      Provides a color source that can be BitBlt to. It operates differently
//      based upon fIsDependent at creation.
//      
//      If fIsDependent = true, then it is dependent upon another color source 
//      for content, referred to as the primary DBCS. Realize will grab the bitmap 
//      dirty rects and copy from the bitmap's primary DBCS. D3DImage uses a dependent 
//      BBDBCS when drawing on a different adapter in BitBlt mode.
//
//      If fIsDependent = false, Realize will no-op, just like
//      CHwDeviceBitmapColorSource does, because it's always up to date. The
//      device bitmap pushes updates through UpdateSurface. D3DImage uses an
//      independent BBDBCS as its front buffer in BitBlt mode.
//
//------------------------------------------------------------------------------

#include "precomp.hpp"


