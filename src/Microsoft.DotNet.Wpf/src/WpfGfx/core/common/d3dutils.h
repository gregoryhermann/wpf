// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      Contains declarations for generic render utility routines.
//
//------------------------------------------------------------------------------

#pragma once

//
// The bitmap cache logic needs the mipmap levels to have a strict ordering 
// policy.
//
// Mipmaps realizations with greater levels must have a greater value.
//
// Example:
//
// TMML_All > TMML_One
//
enum TextureMipMapLevel
{
    TMML_All = 1,
    TMML_One = 0,
    TMML_Unknown
};


//+----------------------------------------------------------------------------
//
//  Function:
//      TextureAddressingAllowsConditionalNonPower2Usage
//
//  Synopsis:
//      Returns true if given texture addressing modes allow use of D3D's
//      conditional non-power of two support.
//

MIL_FORCEINLINE bool TextureAddressingAllowsConditionalNonPower2Usage(
    MilBitmapWrapMode::Enum wrapU,
    MilBitmapWrapMode::Enum wrapV
    )
{
    // Conditional non-power of two support only works when both texture
    // addressing modes are CLAMP (a.k.a. extend edge) despite presence of
    // D3DPTADDRESSCAPS_INDEPENDENTUV.
    return ((wrapU == MilBitmapWrapMode::Extend) && (wrapV == MilBitmapWrapMode::Extend));
}


void PopulateSurfaceDesc(
    DXGI_FORMAT fmtPixelFormat,
    UINT uTextureWidth,
    UINT uTextureHeight,
    __out_ecount(1) D3D11_TEXTURE2D_DESC* pDesc 
    );

D3DFORMAT GetSuperiorSurfaceFormat(
    DXGI_FORMAT d3dFormat,
    BOOL fPalUsesAlpha
    );


