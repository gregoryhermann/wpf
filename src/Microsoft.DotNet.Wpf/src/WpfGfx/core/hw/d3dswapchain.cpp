// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CD3DSwapChain implementation
//
//      Abstracts the core D3D swapchain.  The main reason to wrap this d3d
//      object is so that we can respond to mode changes, i.e.,   respond to
//      D3DERR_DEVICELOST on the Present.
//

#include "precomp.hpp"

MtDefine(CD3DSwapChain, MILRender, "CD3DSwapChain");
MtDefine(D3DResource_SwapChain, MILHwMetrics, "Approximate swap chain size");

MtExtern(CD3DSwapChainWithSwDC);

//+----------------------------------------------------------------------------
//
//  Member:
//      CD3DSwapChain::Create
//
//  Synopsis:
//      Create the CD3DSwapChain wrapper from an IDirect3DSwapChain9
//
//  Notes:
//      This class behaves very differently depending on whether a present
//      context is passed in. If a present context is supplied, the swap chain
//      will implement GetDC by copying the backbuffer to a software GDI DIB
//      section. Even if GetDC is never called, this DIB section would be
//      created.
//

HRESULT 
CD3DSwapChain::Create(
    __inout_ecount(1) CD3DDeviceLevel1 *pDevice,
    __inout_ecount(1) IDXGISwapChain *pDXGISwapChain,
    __in_ecount_opt(1) CMILDeviceContext const *pPresentContext,    
    __deref_out_ecount(1) CD3DSwapChain **ppSwapChain
    )
{
    HRESULT hr = S_OK;

    *ppSwapChain = NULL;

    //
    // Look up back buffer count if not given
    //      code is also in CD3DSwapChainWithSwDC  
    //

    D3DPRESENT_PARAMETERS d3dpp;

    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    IFC(pDXGISwapChain->GetDesc(&swapChainDesc));

    UINT backBufferCount = swapChainDesc.BufferCount;

    //
    // Create the swap chain wrapper
    //

    *ppSwapChain = new(backBufferCount) CD3DSwapChain(
        pDXGISwapChain,
        backBufferCount
    );

    IFCOOM(*ppSwapChain);
    (*ppSwapChain)->AddRef(); // CD3DSwapChain::ctor sets ref count == 0

    //
    // Call init
    //

    IFC((*ppSwapChain)->Init(pDevice));

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterface(*ppSwapChain);
    }
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CD3DSwapChain::operator new
//
//  Synopsis:  Allocate memory for a CD3DSwapChain with cBackBuffers number of
//             back buffer surface.  Extra space is allocated for back buffer
//             pointer array. 
//
//-----------------------------------------------------------------------------

__allocator __bcount_opt(cb + cBackBuffers*sizeof(PVOID) /* Prefast can't deal with type-modifiers, so using PVOID */)
void * __cdecl CD3DSwapChain::operator new(
    size_t cb,
    size_t cBackBuffers
    )
{
    void *pNew = NULL;

    // Calculate extra space for CD3DSurface array (m_rgBackBuffers). The order
    // of memory layout is this followed by back buffer array.
    size_t cbExtraData = cBackBuffers * sizeof(CD3DSurface *);

    // Check for overflow
    if (cbExtraData > cBackBuffers)
    {
        cb += cbExtraData;

        // Check for overflow
        if (cb >= cbExtraData)
        {
            pNew = WPFAlloc(ProcessHeap,
                                 Mt(CD3DSwapChain),
                                 cb);
        }
    }

    return pNew;
}

//+----------------------------------------------------------------------------
//
//  Member:    CD3DSwapChain::CD3DSwapChain
//
//  Synopsis:  ctor
//
//-----------------------------------------------------------------------------

#pragma warning( push )
// Allow use of this in constructor
#pragma warning( disable : 4355 )

CD3DSwapChain::CD3DSwapChain(
    __inout_ecount(1) IDXGISwapChain *pDXGISwapChain,
    __in_range(>, 0) __out_range(==, this->m_cBackBuffers) UINT cBackBuffers
    )
    : m_pDXGISwapChain(pDXGISwapChain),
      m_cBackBuffers(cBackBuffers)
{
    m_pDXGISwapChain->AddRef();

}

#pragma warning( pop )

//+----------------------------------------------------------------------------
//
//  Member:    CD3DSwapChain::~CD3DSwapChain
//
//  Synopsis:  dtor
//
//-----------------------------------------------------------------------------
CD3DSwapChain::~CD3DSwapChain()
{
    ReleaseInterfaceNoNULL(m_pDXGISwapChain);
}

//+----------------------------------------------------------------------------
//
//  Member:    CD3DSwapChain::Init
//
//  Synopsis:  Inits the swap chain
//
//-----------------------------------------------------------------------------
HRESULT 
CD3DSwapChain::Init(
    __inout_ecount(1) CD3DDeviceLevel1* pDevice)
{
    HRESULT hr = S_OK;

    m_pDevice = pDevice;

    //
    // Init the base class
    //

    CD3DResource::Init(pDevice->GetResourceManager(), 0);

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:    CD3DSwapChain::GetBackBuffer
//
//  Synopsis:  Get indexed back buffer from array
//
//-------------------------------------------------------------------------
HRESULT 
CD3DSwapChain::GetBackBuffer(
    __in_range(<, this->m_cBackBuffers) UINT iBackBuffer,
    __deref_out_ecount(1) CD3DTexture **ppBackBuffer
    ) const
{
    HRESULT hr = S_OK;

    Assert(IsValid());

    if (iBackBuffer >= m_cBackBuffers)
    {
        IFC(WGXERR_INVALIDPARAMETER);
    }

    D3DTexture* pTexture = nullptr;
    m_pDXGISwapChain->GetBuffer(0, __uuidof(pTexture), (void**)&pTexture);

    CD3DVidMemOnlyTexture* pVidMemOnlyTexture = nullptr;
    MIL_THR(CD3DVidMemOnlyTexture::Create(pTexture, false, m_pDevice, &pVidMemOnlyTexture));
    *ppBackBuffer = pVidMemOnlyTexture;
    ReleaseInterface(pTexture);

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CD3DSwapChain::ReleaseD3DResources
//
//  Synopsis:  Release the swap chain.  Note that all callers of public
//             methods should have called IsValid before using the swapchain,
//             so it should be ok to blow away our internal swap chain here.
//
//             This method may only be called by CD3DResourceManager because
//             there are restrictions around when a call to ReleaseD3DResources
//             is okay.
//
//-----------------------------------------------------------------------------
void 
CD3DSwapChain::ReleaseD3DResources()
{
    // This resource should have been marked invalid already or at least be out
    // of use.
    Assert(!m_fResourceValid || (m_cRef == 0));
    Assert(IsValid() == m_fResourceValid);

    // This context is protected so it is safe to release the D3D resource
    ReleaseInterface(m_pDXGISwapChain);

    return;
}

//+------------------------------------------------------------------------
//
//  Member:
//      CD3DSwapChain::GetDC
// 
//  Synopsis:
//      Gets the DC for the specified backbuffer
//

HRESULT
CD3DSwapChain::GetDC(
    __in_range(<, this->m_cBackBuffers) UINT iBackBuffer,
    __in_ecount(1) const CMilRectU& rcDirty,
    __deref_out HDC *phdcBackBuffer
    ) const
{
    HRESULT hr = E_NOTIMPL;

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CD3DSwapChain::ReleaseDC
//
//  Synopsis:
//      Releases the DC returned by GetDC if necessary
//

HRESULT
CD3DSwapChain::ReleaseDC(
    __in_range(<, this->m_cBackBuffers) UINT iBackBuffer,
    __in HDC hdcBackBuffer
    ) const
{
    return S_OK;
}




