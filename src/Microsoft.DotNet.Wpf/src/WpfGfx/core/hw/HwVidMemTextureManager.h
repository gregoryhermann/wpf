// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:        
//      Header for CHwVidMemTextureManager
//


//+----------------------------------------------------------------------------
//
//  Class:     
//      CHwVidMemTextureManager
//
//  Synopsis:  
//      Class for managing the transfer of bits to a video memory texture
//      through a system memory surface.
//
//-----------------------------------------------------------------------------

class CHwVidMemTextureManager
{
public:
    CHwVidMemTextureManager();
    ~CHwVidMemTextureManager();

    bool HasRealizationParameters() const;

    void SetRealizationParameters(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        DXGI_FORMAT dxgiFormat,
        UINT uWidth,
        UINT uHeight,
        TextureMipMapLevel eMipMapLevel
        DBG_COMMA_PARAM(bool bDbgConditionalNonPowTwoOkay)
        );

    bool IsSysMemTextureValid() const
    {
        return (m_pSysMemTexture != NULL) && (m_pSysMemTexture->IsValid());
    }

    HRESULT ReCreateAndLockSysMemTexture(
        __out_ecount(1) D3DLOCKED_RECT *pD3DLockedRect
        );

    HRESULT UnlockSysMemTexture(
        );

    HRESULT PushBitsToVidMemTexture();

    __out_ecount_opt(1) CD3DTexture *GetVidMemTextureNoRef()
    {
        return   (m_pVideoMemTexture && m_pVideoMemTexture->IsValid())
               ? m_pVideoMemTexture
               : NULL;
    }

    void PrepareForNewRealization();

private:
    void Initialize();
    void Destroy();

    void ComputeTextureDesc(
        DXGI_FORMAT dxgiFormat,
        UINT uWidth,
        UINT uHeight,
        TextureMipMapLevel eMipMapLevel
        DBG_COMMA_PARAM(bool bDbgConditionalNonPowTwoOkay)
        );

private:
    CD3DDeviceLevel1 *m_pDeviceNoRef;

    CD3DTexture *m_pSysMemTexture;

    CD3DVidMemOnlyTexture *m_pVideoMemTexture;

    D3D11_TEXTURE2D_DESC m_descRequiredForVidMem;
    UINT m_uLevelsForVidMem;

#if DBG
private:
    bool m_fDBGSysMemTextureIsLocked;
#endif
};


