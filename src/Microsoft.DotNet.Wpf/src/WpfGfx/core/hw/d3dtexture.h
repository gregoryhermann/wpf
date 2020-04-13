// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains D3D Texture wrapper class declaration
//

class CD3DTexture : public CD3DResource
{
public:
    static void DetermineLevels(
        __in_ecount(1) const CD3DDeviceLevel1 *pDevice,
        TextureMipMapLevel eMipMapLevel,
        UINT uTextureWidth,
        UINT uTextureHeight,
        __out_ecount(1) UINT *puLevels
        );

    void GetTextureSize(
        __out_ecount(1) UINT *puWidth, 
        __out_ecount(1) UINT *puHeight
        ) const;

    __out_ecount(1) D3DTexture *GetD3DTextureNoRef() const
    {
        Assert(IsValid());
        Assert(m_pD3DTexture);

        return m_pD3DTexture;
    }
    
    const D3D11_TEXTURE2D_DESC& D3DSurface0Desc() const { return m_sdLevel0; }

    ID3D11DepthStencilView* GetDepthStencilViewNoAddRef();
    ID3D11RenderTargetView* GetRenderTargetViewNoAddRef();
    ID3D11ShaderResourceView* GetShaderResourceViewNoAddRef();

    __range(1,32) UINT Levels() const { return m_cLevels; }

    HRESULT UpdateMipmapLevels();

    override bool RequiresDelayedRelease() const
    {
        return true;
    }

    HRESULT ReadIntoSysMemBuffer(
         __in_ecount(1) const CMilRectU &rcSource,
         UINT cClipRects,
         __in_ecount_opt(cClipRects) const CMilRectU *rgClipRects,
         MilPixelFormat::Enum fmtBitmapOut,
         UINT nStrideOut,
         DBG_ANALYSIS_PARAM_COMMA(UINT cbBufferOut)
         __out_bcount_full(cbBufferOut) BYTE *pbBufferOut
         );

protected:
    CD3DTexture();
    ~CD3DTexture();

    HRESULT Init(
        __inout_ecount(1) CD3DResourceManager *pResourceManager,
        __in_ecount(1) CD3DDeviceLevel1* pDevice,
        __inout_ecount(1) D3DTexture *pD3DTexture
        );

    HRESULT InitResource(
        __inout_ecount(1) CD3DResourceManager *pResourceManager,
        __inout_ecount(1) D3DTexture *pD3DTexture
        );

private:

    // 
    // CD3DResource methods
    //

    // Should only be called by CD3DResourceManager (destructor is okay, too)
    void ReleaseD3DResources();
    
protected:

    D3DTexture *m_pD3DTexture;
    D3D11_TEXTURE2D_DESC m_sdLevel0;
    CD3DDeviceLevel1* m_pDevice;
    ID3D11DepthStencilView* m_pDepthStencilView;
    ID3D11RenderTargetView* m_pRenderTargetView;
    ID3D11ShaderResourceView* m_pShaderResourceView;

private:

    __field_range(1,32) DWORD m_cLevels;
};


