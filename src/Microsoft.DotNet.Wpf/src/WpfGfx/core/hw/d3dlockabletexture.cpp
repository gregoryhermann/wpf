// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_text
//      $Keywords:
//
//  $Description:
//      Contains CD3DLockableTexture implementation
//
//      Abstract a lockable D3D texture and track it
//       as a D3D resource.
//
//      Also contains CD3DLockableTexturePair implementation
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CD3DLockableTexture, MILRender, "CD3DLockableTexture");
MtDefine(D3DResource_LockableTexture, MILHwMetrics, "Approximate lockable texture size");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DLockableTexture::Create
//
//  Synopsis:
//      Create the CD3DLockableTexture
//
//------------------------------------------------------------------------------
HRESULT 
CD3DLockableTexture::Create(
    __inout_ecount(1) CD3DResourceManager *pResourceManager, 
    __in_ecount(1) CD3DDeviceLevel1* pDevice,
    __in_ecount(1) D3DTexture *pD3DTexture,
    __deref_out_ecount(1) CD3DLockableTexture **ppLockableTexture)
{
    HRESULT hr = S_OK;

    *ppLockableTexture = NULL;

    //
    // Create the CD3DLockableTexture
    //

    *ppLockableTexture = new CD3DLockableTexture();
    IFCOOM(*ppLockableTexture);
    (*ppLockableTexture)->AddRef(); // CD3DLockableTexture::ctor sets ref count == 0

    //
    // Call Init
    //

    IFC((*ppLockableTexture)->Init(pResourceManager, pDevice, pD3DTexture));

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterface(*ppLockableTexture);
    }
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DLockableTexture::CD3DLockableTexture
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CD3DLockableTexture::CD3DLockableTexture()
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DLockableTexture::~CD3DLockableTexture
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------
CD3DLockableTexture::~CD3DLockableTexture()
{
    // InternalDestroy will be called by CD3DTexture destructor.
    //IGNORE_HR(InternalDestroy());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DLockableTexture::Init
//
//  Synopsis:
//      Inits the cached bitmap
//
//------------------------------------------------------------------------------
HRESULT 
CD3DLockableTexture::Init(
    __inout_ecount(1) CD3DResourceManager *pResourceManager,
    __in_ecount(1) CD3DDeviceLevel1* pDevice,
    __in_ecount(1) D3DTexture *pD3DTexture
    )
{
    HRESULT hr = S_OK;

    //
    // Let CD3DTexture handle the init
    //

    IFC(CD3DTexture::Init(pResourceManager, pDevice, pD3DTexture));

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DLockableTexture::LockRect
//
//  Synopsis:
//      Delegate to D3DTexture::LockRect
//
//------------------------------------------------------------------------------
HRESULT 
CD3DLockableTexture::LockRect(
    __out_ecount(1) D3DLOCKED_RECT* pLockedRect, 
    __in_ecount(1) CONST RECT* pRect,
    DWORD dwFlags
    )
{
    HRESULT hr = S_OK;

    D3D11_MAPPED_SUBRESOURCE mappedSubresource;
    m_pDevice->GetDeviceContext()->Map(m_pD3DTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);

    pLockedRect->pBits = mappedSubresource.pData;
    pLockedRect->Pitch = mappedSubresource.RowPitch;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DLockableTexture::UnlockRect
//
//  Synopsis:
//      Delegate to D3DTexture::UnlockRect
//
//------------------------------------------------------------------------------
HRESULT 
CD3DLockableTexture::UnlockRect()
{
    HRESULT hr = S_OK;

    m_pDevice->GetDeviceContext()->Unmap(m_pD3DTexture, 0);

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DLockableTexture::AddDirtyRect
//
//  Synopsis:
//      Delegate to D3DTexture::AddDirtyRect
//
//------------------------------------------------------------------------------
HRESULT 
CD3DLockableTexture::AddDirtyRect(
    __in_ecount(1) const RECT &rc
    )
{
    HRESULT hr = S_OK;
#ifndef DX11
    IFC(m_pD3DTexture->AddDirtyRect(&rc));
#endif

Cleanup:
    RRETURN(hr);
}

