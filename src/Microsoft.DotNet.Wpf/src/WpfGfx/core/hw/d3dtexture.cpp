// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CD3DTexture implementation
//
//      Provides basic abstraction of a D3D texture and tracks it as a D3D
//      resource.
//

#include "precomp.hpp"

MtDefine(TextureSurfaceCache, MILRender, "CD3DTexture surface cache array");

//+------------------------------------------------------------------------
//
//  Function:  CD3DTexture::CD3DTexture
//
//  Synopsis:  ctor
//
//-------------------------------------------------------------------------
CD3DTexture::CD3DTexture()
    : m_pD3DTexture(NULL),
    m_pDepthStencilView(NULL),
    m_pRenderTargetView(NULL),
    m_pShaderResourceView(NULL)
{
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DTexture::~CD3DTexture
//
//  Synopsis:  dtor
//
//-------------------------------------------------------------------------
CD3DTexture::~CD3DTexture()
{
    ReleaseInterfaceNoNULL(m_pD3DTexture);
    ReleaseInterfaceNoNULL(m_pDepthStencilView);
    ReleaseInterfaceNoNULL(m_pRenderTargetView);
    ReleaseInterfaceNoNULL(m_pShaderResourceView);
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DTexture::Init
//
//  Synopsis:  Inits the texture wrapper
//
//-------------------------------------------------------------------------
HRESULT 
CD3DTexture::Init(
    __inout_ecount(1) CD3DResourceManager *pResourceManager,
    __in_ecount(1) CD3DDeviceLevel1* pDevice,
    __inout_ecount(1) D3DTexture *pD3DTexture
    )
{
    HRESULT hr = S_OK;

    Assert(m_pD3DTexture == NULL);

    m_pDevice = pDevice;

    //
    // Get texture information
    //
#ifndef DX11
    m_cLevels = pD3DTexture->GetLevelCount();
    if (   (m_cLevels < 1)
        || (m_cLevels > 32))
    {
        IFC(E_FAIL);
    }

    IFC(pD3DTexture->GetLevelDesc(0, &m_sdLevel0));
#else
    D3D11_TEXTURE2D_DESC desc;
    pD3DTexture->GetDesc(&desc);
    m_cLevels = desc.MipLevels;
    if ((m_cLevels < 1)
        || (m_cLevels > 32))
    {
        IFC(E_FAIL);
    }
    m_sdLevel0 = desc;
#endif

    //
    // Init the resouce base class
    //

    IFC(InitResource(pResourceManager, pD3DTexture));

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DTexture::InitResource
//
//  Synopsis:  Inits the CD3DResouce base class
//
//-------------------------------------------------------------------------
HRESULT 
CD3DTexture::InitResource(
    __inout_ecount(1) CD3DResourceManager *pResourceManager,
    __inout_ecount(1) D3DTexture *pD3DTexture
    )
{
    HRESULT hr = S_OK;

    UINT uPixelSize = 0;
    UINT uResourceSize = 0;

    //
    // Compute the size of the resource
    //
    D3D11_TEXTURE2D_DESC desc;
    pD3DTexture->GetDesc(&desc);

    for (UINT uLevel = 0; uLevel < m_cLevels; uLevel++)
    {

        // Lookup pixel size from D3DFORMAT
        uPixelSize = D3DFormatSize(desc.Format);
        if (uPixelSize == 0)
        {
            IFC(D3DERR_WRONGTEXTUREFORMAT);
        }

        // Accumulate each levels size
        uResourceSize += ((desc.Width * desc.Height) >> (uLevel * 2)) * uPixelSize;
    }
    
    //
    // Init the base class
    //

    CD3DResource::Init(pResourceManager, uResourceSize);

    //
    // Save the D3D texture reference 
    //

    m_pD3DTexture = pD3DTexture;
    m_pD3DTexture->AddRef();

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DTexture::ReleaseD3DResources
//
//  Synopsis:  Release the texture.
//
//-------------------------------------------------------------------------
void 
CD3DTexture::ReleaseD3DResources()
{
    ReleaseInterface(m_pD3DTexture);
    ReleaseInterface(m_pDepthStencilView);
    ReleaseInterface(m_pRenderTargetView);
    ReleaseInterface(m_pShaderResourceView);

    return;
}


//+------------------------------------------------------------------------
//
//  Function:  CD3DTexture::GetTextureSize
//
//  Synopsis:  Return the dimensions of the texture.
//
//-------------------------------------------------------------------------
void
CD3DTexture::GetTextureSize(
    __out_ecount(1) UINT *puWidth, 
    __out_ecount(1) UINT *puHeight
    ) const
{
    Assert(IsValid());

    *puWidth = m_sdLevel0.Width;
    *puHeight = m_sdLevel0.Height;
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DTexture::UpdateMipmapLevels
//
//  Synopsis:  Update nonzero mipmap levels of the texture based on the zero
//             level.
//
//-------------------------------------------------------------------------
HRESULT CD3DTexture::UpdateMipmapLevels()
{
    HRESULT hr = S_OK;

    if (m_cLevels > 1)
    {
        m_pDevice->GetDeviceContext()->GenerateMips(GetShaderResourceViewNoAddRef());
    }
    
    return hr;
}

ID3D11DepthStencilView* CD3DTexture::GetDepthStencilViewNoAddRef()
{
    if (m_pDepthStencilView == nullptr)
    {
        m_pDevice->GetDevice()->CreateDepthStencilView(GetD3DTextureNoRef(), nullptr, &m_pDepthStencilView);
    }
    return m_pDepthStencilView;
}

ID3D11RenderTargetView* CD3DTexture::GetRenderTargetViewNoAddRef()
{
    if (m_pRenderTargetView == nullptr)
    {
        m_pDevice->GetDevice()->CreateRenderTargetView(GetD3DTextureNoRef(), nullptr, &m_pRenderTargetView);
    }
    return m_pRenderTargetView;
}

ID3D11ShaderResourceView* CD3DTexture::GetShaderResourceViewNoAddRef()
{
    if (m_pShaderResourceView == nullptr)
    {
    	m_pDevice->GetDevice()->CreateShaderResourceView( GetD3DTextureNoRef(), nullptr, &m_pShaderResourceView);
    }
    return m_pShaderResourceView;
}



//+----------------------------------------------------------------------------
//
//  Member:    
//      CD3DTexture::DetermineLevels
//
//  Synopsis:  
//      Determines the levels for a texture that might be mipmapped.
//

void
CD3DTexture::DetermineLevels(
    __in_ecount(1) const CD3DDeviceLevel1 *pDevice,
    TextureMipMapLevel eMipMapLevel,
    UINT uTextureWidth,
    UINT uTextureHeight,
    __out_ecount(1) UINT *puLevels
    )
{
    if (eMipMapLevel == TMML_One)
    {
        *puLevels = 1;
    }
    else
    {
        UINT uMaxSize = max(uTextureWidth, uTextureHeight);
        *puLevels = Log2(uMaxSize) + 1;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:    CD3DTexture::ReadIntoSysMemBuffer
//
//  Synopsis:  Reads the surface's contents in the specified source rectangle
//             and writes it into the provided buffer.
//
//             If cClipRects is non-zero, then writes are restricted to the
//             intersection of rcSource and rectangles in rgClipRects.  For
//             best performance clipping rectangles should not overlap one
//             another.
//
//             Note: the output buffer might be sparsely filled if nStrideOut
//                   is larger than the stride of the source in the given
//                   rectangle.
//
//             IMPORTANT: HwUtil.cpp's ReadRenderTargetIntoSysMemBuffer borrows 
//             heavily from this so it should be kept in sync with this 
//             implementation
//             
//-----------------------------------------------------------------------------
HRESULT
CD3DTexture::ReadIntoSysMemBuffer(
    __in_ecount(1) const CMilRectU &rcSource,
    UINT cClipRects,
    __in_ecount_opt(cClipRects) const CMilRectU *rgClipRects,
    MilPixelFormat::Enum fmtOut,
    UINT nStrideOut,
    DBG_ANALYSIS_PARAM_COMMA(UINT cbBufferOut)
    __out_bcount_full(cbBufferOut) BYTE *pbBufferOut
    )
{
    HRESULT hr = S_OK;
    D3DTexture *pD3DLockableTexture = NULL;
    
    bool fNeedToManuallyCopyBits = true;
    UINT nStrideCopy;
    POINT destPoint = { 0, 0 };

    UINT const uSourceWidth = rcSource.Width();
    UINT const uSourceHeight = rcSource.Height();

    RECT const rcDest = { 0, 0, uSourceWidth, uSourceHeight };

    BYTE const BitsPerPixel = GetPixelFormatSize(fmtOut);

    if (BitsPerPixel % BITS_PER_BYTE)
    {
        TraceTag((tagMILWarning,
                  "Call to CHwDeviceBitmapColorSource::CopyPixels requested fraction byte copy"));
        IFC(WGXERR_INVALIDPARAMETER);
    }

    IFC(HrCalcByteAlignedScanlineStride(uSourceWidth, BitsPerPixel, nStrideCopy));

#if DBG_ANALYSIS
    Assert(nStrideCopy <= nStrideOut);
    Assert(SUCCEEDED(HrCheckBufferSize(fmtOut, nStrideOut, uSourceWidth, uSourceHeight, cbBufferOut)));
#endif

    Assert(Device().IsInAUseContext());

    DXGI_FORMAT dxgiFmtOut = PixelFormatToD3DFormat(fmtOut);

    //
    // Create a lockable copy or wrapper around bit buffer and copy
    //

    void* pvSysMemPixels = NULL;

    //
    // Shared surf create should take stride
    // CreateSysMemUpdateSurface with WDDM can provide a wrapper around a
    // system memory buffer owned by this module.  But surface width in
    // bytes must equal the stride.  Since GetRenderTargetData also has
    // surface size restrictions and nothing can be done about the out
    // buffer properties, check that out buffer meets requirements before
    // creating the wrapper.  If the check fails (or WDDM isn't available)
    // have D3D allocate system memory and do an extra system memory to
    // system memory transfer.
    //
    // Independent of the above, if there are clip rects, then handle
    // clipping with CPU copies rather than attempting several
    // GetRenderTargetData calls which probably still require a CPU
    // transfer.
    //

    IFC(Device().CreateSysMemUpdateTexture(
        uSourceWidth, 
        uSourceHeight, 
        dxgiFmtOut,
        pvSysMemPixels, 
        &pD3DLockableTexture));
        
    Device().UpdateTextureRegion(m_pD3DTexture, 0, (const RECT*) &rcSource, pD3DLockableTexture, 0, &destPoint);

    //
    // Manually lock the surface and copy the bits to the destination if we
    // weren't able to get it done automatically by GetRenderTargetData.
    //
    if (fNeedToManuallyCopyBits)
    {
        D3D11_MAPPED_SUBRESOURCE mappedSubresource;
        m_pDevice->GetDeviceContext()->Map(pD3DLockableTexture, 0, D3D11_MAP_READ, 0, &mappedSubresource);

        UINT const BytesPerPixel = BitsPerPixel / BITS_PER_BYTE;

    #if DBG_ANALYSIS
        BYTE *pbAnalysisBufferOutOrig = pbBufferOut;
    #endif

        if (   cClipRects == 0
            || rgClipRects == NULL)
        {
            cClipRects = 1;
            rgClipRects = &rcSource;
        }

        do
        {
            CMilRectU rcCopy = *rgClipRects;
            if (rcCopy.Intersect(rcSource))
            {
                // Adjust pbBufferOut and locked D3D buffer pointer by amount
                // inset by intersection with source rect.  None of this math
                // should overflow since rcCopy is restricted to rcSource which
                // has already been validated as a good bounding rectangle.
                UINT cbBufferInset, cbLeftInset;

                IFC(MultiplyUINT(BytesPerPixel, rcCopy.left - rcSource.left, OUT cbLeftInset));

                IFC(MultiplyUINT(nStrideOut, rcCopy.top - rcSource.top, OUT cbBufferInset));
                IFC(AddUINT(cbBufferInset, cbLeftInset, OUT cbBufferInset));
                BYTE *pbOutBuffer = pbBufferOut + cbBufferInset;

                IFC(MultiplyUINT(mappedSubresource.RowPitch, rcCopy.top - rcSource.top, OUT cbBufferInset));
                IFC(AddUINT(cbBufferInset, cbLeftInset, OUT cbBufferInset));
                BYTE *pbInBuffer = static_cast<BYTE *>(mappedSubresource.pData) + cbBufferInset;

                IFC(MultiplyUINT(BytesPerPixel, rcCopy.Width(), OUT nStrideCopy));

                for (UINT i = rcCopy.top; i < rcCopy.bottom; i++)
                {
                    Assert(pbOutBuffer + nStrideCopy <= pbAnalysisBufferOutOrig + cbBufferOut);

                    GpMemcpy(pbOutBuffer, pbInBuffer, nStrideCopy);
                    pbOutBuffer += nStrideOut;
                    pbInBuffer += mappedSubresource.RowPitch;
                }
            }
        } while (rgClipRects++, --cClipRects > 0);

        m_pDevice->GetDeviceContext()->Unmap(pD3DLockableTexture, 0);
    }
    
Cleanup:
    ReleaseInterface(pD3DLockableTexture);
    
    RRETURN(hr);
}

