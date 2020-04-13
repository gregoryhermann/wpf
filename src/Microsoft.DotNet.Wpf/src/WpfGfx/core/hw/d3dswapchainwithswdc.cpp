// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CD3DSwapChainWithSwDC implementation
//
//      This class overrides the GetDC method of CD3DSwapChain to implement
//      GetDC using GetRenderTargetData. This approach acheived phenominal perf
//      wins in WDDM.
//

#include "precomp.hpp"



