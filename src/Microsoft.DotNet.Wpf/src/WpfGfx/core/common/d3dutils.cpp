// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      Contains generic d3d utility routines.
//

#include "precomp.hpp"

//+------------------------------------------------------------------------
//
//  Function:  PopulateSurfaceDesc
//
//  Synopsis:  Creates a D3DSURFACE_DESC from the specified parameters
//
// Return Value:
//
//  None
//
//-------------------------------------------------------------------------
void 
PopulateSurfaceDesc(
    DXGI_FORMAT fmtPixelFormat,
    UINT uTextureWidth,
    UINT uTextureHeight,
    __out_ecount(1) D3D11_TEXTURE2D_DESC* pDesc 
    )
{
    ZeroMemory(pDesc, sizeof(*pDesc));
    pDesc->SampleDesc.Count = 1;
    pDesc->ArraySize = 1;
    pDesc->Format = fmtPixelFormat;
    pDesc->Width = uTextureWidth;
    pDesc->Height = uTextureHeight;
    pDesc->MipLevels = 1;
}

//+------------------------------------------------------------------------
//
//  Function:  GetMinimalTextureDesc
//
//  Synopsis:  Returns a surface description for a texture the
//              given D3D device should be capable of creating that
//              can retain all data a texture with given
//              description could hold.
//
// Return Value:
//
//   S_OK - suitable texture description was found,
//   S_FALSE - smaller texture description was found,
//            otherwise, varying D3DERR_Xxx values.
//

DeclareTagEx(tagTextureFixup, "MIL_HW", "Texture Create Description Fixup", FALSE /* fEnabled */);

//+------------------------------------------------------------------------
//
//  Function:  GetSuperiorSurfaceFormat
//
//  Synopsis:  Returns a surface format that is superior to the given
//         format in that it can hold more color information.
//
// Return Value:
//
//   D3DFMT_UNKNOWN - no superior format was found.
//
//-------------------------------------------------------------------------
D3DFORMAT
GetSuperiorSurfaceFormat(
    D3DFORMAT d3dFormat,    // In: base D3D surface format
    BOOL fPalUsesAlpha      // In: set to TRUE when an associated
                            //      palette is not completely opaque
    )
{
    switch (d3dFormat)
    {
    case D3DFMT_P8:
        if (fPalUsesAlpha)
        {
            return D3DFMT_A8R8G8B8;
        }
        else
        {
            return D3DFMT_R8G8B8;
        }

    case D3DFMT_X1R5G5B5:
        return D3DFMT_R5G6B5;

    case D3DFMT_R5G6B5:
        return D3DFMT_R8G8B8;

    case D3DFMT_R8G8B8:
        return D3DFMT_X8R8G8B8;

    case D3DFMT_X8R8G8B8:
        return D3DFMT_A8R8G8B8;

    case D3DFMT_A8P8:
        return D3DFMT_A8R8G8B8;

    case D3DFMT_A1R5G5B5:
        return D3DFMT_A8R8G8B8;

    case D3DFMT_A8R8G8B8:
    default:
        break;
    }

    return D3DFMT_UNKNOWN;
}





