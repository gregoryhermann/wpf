// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_targets
//      $Keywords:
//
//  $Description:
//      CHwHWNDRenderTarget implementation
//
//      This object creates the d3d device for the current hwnd and manages a
//      flipping chain.  Note that instead of using the primary flipping chain,
//      a secondary chain is created and used since secondary chains can be
//      resized without recreating the d3d device.
//
//      This object is also repsonsible for responding to resize and disabling
//      rendering when the hwnd is minimized.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CHwHWNDRenderTarget, MILRender, "CHwHWNDRenderTarget");


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwHWNDRenderTarget::CHwHWNDRenderTarget
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------

CHwHWNDRenderTarget::CHwHWNDRenderTarget(
    __inout_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
    DisplayId associatedDisplay,
    MilWindowLayerType::Enum eWindowLayerType
    ) :
    CHwDisplayRenderTarget(pD3DDevice, DXGI_FORMAT_B8G8R8A8_UNORM, associatedDisplay),
    m_eWindowLayerType(eWindowLayerType)
{
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwHWNDRenderTarget::Init
//
//  Synopsis:
//      1. Create the CD3DDeviceLevel1
//      2. Initialize the CHwDisplayRenderTarget
//
//------------------------------------------------------------------------------
HRESULT 
CHwHWNDRenderTarget::Init(
    __in_ecount_opt(1) HWND hwnd,
    __in_ecount(1) CDisplay const *pDisplay,
    MilRTInitialization::Flags dwFlags
    )
{
    HRESULT hr = S_OK;

    m_hwnd = hwnd;

    IFC(CHwDisplayRenderTarget::Init(
        hwnd,
        pDisplay,
        dwFlags));
    
    // Finish initialization with 0x0 flipping chain.  A call to Resize
    // is required before use.
    //
    
    IFC(UpdateFlippingChain(0, 0));

    // Call base init only after size has been updated by UpdateFlippingChain
    IFC(CBaseRenderTarget::Init());

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwHWNDRenderTarget::SetPosition
//
//  Synopsis:
//      Remember Present position for when UpdateLayeredWindowEx is called.
//
//------------------------------------------------------------------------------

void
CHwHWNDRenderTarget::SetPosition(POINT ptOrigin)
{
    m_MILDC.SetPosition(ptOrigin);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwHWNDRenderTarget::UpdatePresentProperties
//
//  Synopsis:
//      Remember Present transparency properties for when UpdateLayeredWindowEx
//      is called.
//
//------------------------------------------------------------------------------

void
CHwHWNDRenderTarget::UpdatePresentProperties(
    MilTransparency::Flags transparencyFlags,
    BYTE constantAlpha,
    COLORREF colorKey
    )
{
    m_MILDC.SetLayerProperties(transparencyFlags, constantAlpha, colorKey, NULL);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwHWNDRenderTarget::Present
//
//  Synopsis:
//      1. Present the flipping chain
//      2. Update the render target
//
//------------------------------------------------------------------------------
STDMETHODIMP
CHwHWNDRenderTarget::Present(
    __in_ecount(1) const RECT *pRect
    )
{
    HRESULT hr = S_OK;

    IFC(CHwDisplayRenderTarget::Present(pRect));
    
Cleanup:
    RRETURN(hr);
}

STDMETHODIMP
CHwHWNDRenderTarget::ScrollBlt (
    __in_ecount(1) const RECT *prcSource,
    __in_ecount(1) const RECT *prcDest
    )
{
    RRETURN(E_NOTIMPL);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwHWNDRenderTarget::Resize
//
//  Synopsis:
//      Updated the flipping chain size
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CHwHWNDRenderTarget::Resize(
    UINT uWidth,
    UINT uHeight
    )
{
    HRESULT hr = S_OK;

    ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);

    // All calls to Resize make the contents invalid
    DbgSetInvalidContents();

    //
    // Update the flipping chain size
    //

    IFC( UpdateFlippingChain(uWidth, uHeight) );

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwHWNDRenderTarget::UpdateFlippingChain
//
//  Synopsis:
//      If a flipping chain currently exists, replace it.   Otherwise create a
//      new one.
//
//      Note that we use a secondary flipping chain instead of the primary so
//      that we can resize it without recreating a device.
//
//------------------------------------------------------------------------------
HRESULT 
CHwHWNDRenderTarget::UpdateFlippingChain(
    UINT uWidth,
    UINT uHeight
    )
{
    HRESULT hr = S_OK;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};

    Assert(m_pD3DDevice);

    //
    // Release old resources
    //

    if (m_pD3DTargetTexture != nullptr)
    {
        m_pD3DTargetTexture->DestroyAndRelease();
        m_pD3DTargetTexture = nullptr;
    }

    if (m_pD3DIntermediateMultisampleTargetTexture)
    {
        //
        // If new size of less than a quarter of current intermediate use
        // release the intermediate.
        //

        ULONGLONG ullSizeCur =
            m_pD3DIntermediateMultisampleTargetTexture->D3DSurface0Desc().Width *
            m_pD3DIntermediateMultisampleTargetTexture->D3DSurface0Desc().Height;

        ULONGLONG ullSizeNew = uWidth * uHeight;

        if (ullSizeNew < ullSizeCur / 4)
        {
            ReleaseInterface(m_pD3DIntermediateMultisampleTargetTexture);
        }
    }

    //
    // Don't render when minimized or empty
    //

    if (uWidth == 0 || uHeight == 0)
    {
        m_fEnableRendering = FALSE;
        goto Cleanup;
    }

    //
    // Create flipping chain
    //

    if (m_pD3DSwapChain != nullptr)
    {
        m_pD3DSwapChain->DestroyAndRelease();
        m_pD3DSwapChain = nullptr;
    }

    if (m_pD3DSwapChain == nullptr)
    {
        IDXGIFactory2* pFactory = nullptr;
        IFC(CreateDXGIFactory(__uuidof(pFactory), reinterpret_cast<void**>(&pFactory)));

        swapChainDesc.BufferCount = 2;
        swapChainDesc.Width = 0;
        swapChainDesc.Height = 0;
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;

        IDXGISwapChain1* pD3DSwapChain = nullptr;
        IFCSUB1(pFactory->CreateSwapChainForHwnd(
            m_pD3DDevice->GetDevice(),
            m_hwnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &pD3DSwapChain
        ));

        IFC(CD3DSwapChain::Create(
            m_pD3DDevice,
            pD3DSwapChain,
            &m_MILDC,
            &m_pD3DSwapChain
        ));

        pD3DSwapChain->Release();
    }
    else
    {
        m_pD3DSwapChain->GetD3DSwapChainNoRef()->ResizeBuffers(2, uWidth, uHeight, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
        m_pD3DSwapChain = nullptr;
    }

    //
    // Get the current back buffer and update CHwDisplayRenderTarget
    //

    m_rcBounds.left = 0;
    m_rcBounds.top  = 0;
    m_rcBounds.right  = static_cast<LONG>(uWidth);
    m_rcBounds.bottom = static_cast<LONG>(uHeight);

    m_uWidth = uWidth;
    m_uHeight = uHeight;

    IFCSUB1(m_pD3DSwapChain->GetBackBuffer(0, &m_pD3DTargetTexture));

    //
    // Reset dirty list for new frame (expected next) in case there was a
    // failure after some prior Invalidate calls, but before a call to
    // Present to clear the dirty list.
    //

    IFC(ClearInvalidatedRects());

SubCleanup1:

    if (FAILED(hr))
    {
        //
        // Remember if the display is invalid, because we want to be consistent
        // about returning WGXERR_DISPLAYSTATEINVALID during Present.
        //

        if (hr == WGXERR_DISPLAYSTATEINVALID)
        {
            m_hrDisplayInvalid = hr;
        }

        m_fEnableRendering = FALSE;
    }
    else
    {
        m_fEnableRendering = TRUE;
    }

Cleanup:
    RRETURN(hr);
}


