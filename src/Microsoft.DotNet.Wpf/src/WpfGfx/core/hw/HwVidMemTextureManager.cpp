// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:        
//      Implmentation for CHwVidMemTextureManager
//


#include "precomp.hpp"


//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwVidMemTextureManager::CHwVidMemTextureManager
//
//  Synopsis:  
//      ctor
//

CHwVidMemTextureManager::CHwVidMemTextureManager()
{
    Initialize();
}


//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwVidMemTextureManager::Initialize
//
//  Synopsis:  
//      Initilizes member variables
//

void CHwVidMemTextureManager::Initialize()
{
    m_pDeviceNoRef = NULL;

    m_pSysMemTexture = NULL;

    m_pVideoMemTexture = NULL;

    ZeroMemory(&m_descRequiredForVidMem, sizeof(m_descRequiredForVidMem));
    m_uLevelsForVidMem = 0;

#if DBG
    m_fDBGSysMemTextureIsLocked = false;
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwVidMemTextureManager::~CHwVidMemTextureManager
//
//  Synopsis:  
//      dtor
//

CHwVidMemTextureManager::~CHwVidMemTextureManager()
{
    Destroy();
}


//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwVidMemTextureManager::Destroy
//
//  Synopsis:  
//      destroys memory held onto by this object
//

void CHwVidMemTextureManager::Destroy()
{
#if DBG
    Assert(!m_fDBGSysMemTextureIsLocked);
#endif

    ReleaseInterfaceNoNULL(m_pSysMemTexture);
    ReleaseInterfaceNoNULL(m_pVideoMemTexture);
}

//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwVidMemTextureManager::HasRealizationParameters
//
//  Synopsis:  
//      Returns whether the realizaton parameters have been set since this
//      class was constructed/destroyed.
//

bool
CHwVidMemTextureManager::HasRealizationParameters() const
{
    return (m_pDeviceNoRef != NULL);
}

//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwVidMemTextureManager::SetRealizationParameters
//
//  Synopsis:  
//      Create method
//

void
CHwVidMemTextureManager::SetRealizationParameters(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    DXGI_FORMAT dxgiFormat,
    UINT uWidth,
    UINT uHeight,
    TextureMipMapLevel eMipMapLevel
    DBG_COMMA_PARAM(bool bDbgConditionalNonPowTwoOkay)
    )
{
    Assert(!HasRealizationParameters());

    m_pDeviceNoRef = pDevice;
        
    ComputeTextureDesc(
        dxgiFormat,
        uWidth,
        uHeight,
        eMipMapLevel
        DBG_COMMA_PARAM(bDbgConditionalNonPowTwoOkay)
        );
}


//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwVidMemTextureManager::PrepareForNewRealization
//
//  Synopsis:  
//      Destroys realizations in this object and sets it up for re-use.
//

void
CHwVidMemTextureManager::PrepareForNewRealization()
{
    Destroy();
    Initialize();
}

//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwVidMemTextureManager::ReCreateAndLockSysMemTexture
//
//  Synopsis:  
//      Creates system memory texture and locks it for updating
//

HRESULT
CHwVidMemTextureManager::ReCreateAndLockSysMemTexture(
    __out_ecount(1) D3DLOCKED_RECT *pD3DLockedRect
    )
{
    HRESULT hr = S_OK;

#if DBG
    Assert(!m_fDBGSysMemTextureIsLocked);
#endif

    D3DTexture *pID3DSysMemTexture = NULL;

    //
    // Create the surface
    //

    if (!IsSysMemTextureValid())
    {
        ReleaseInterface(m_pSysMemTexture);

        IFC(m_pDeviceNoRef->CreateSysMemUpdateTexture(
            m_descRequiredForVidMem.Width,
            m_descRequiredForVidMem.Height,
            m_descRequiredForVidMem.Format,
            NULL, // pvPixels
            &pID3DSysMemTexture
            ));
        
        CD3DLockableTexture* pLockableTexture;
        IFC(CD3DLockableTexture::Create(
            m_pDeviceNoRef->GetResourceManager(),
            m_pDeviceNoRef,
            pID3DSysMemTexture,
            &pLockableTexture
            ));
        m_pSysMemTexture = pLockableTexture;
        ReleaseInterface(pID3DSysMemTexture); // ownership transfered to m_pSysMemSurface
    }

    //
    // Lock the entire surface
    //

    D3D11_MAPPED_SUBRESOURCE mappedSubresource;
    m_pDeviceNoRef->GetDeviceContext()->Map(m_pSysMemTexture->GetD3DTextureNoRef(), 0, D3D11_MAP_READ, 0, &mappedSubresource);
    pD3DLockedRect->pBits = mappedSubresource.pData;
    pD3DLockedRect->Pitch = mappedSubresource.RowPitch;

#if DBG
    m_fDBGSysMemTextureIsLocked = true;
#endif

Cleanup:
    ReleaseInterfaceNoNULL(pID3DSysMemTexture);
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwVidMemTextureManager::UnlockSysMemSurface
//
//  Synopsis:  
//      Unlocks the system memory texture. Should be called if
//      ReCreateAndLockSysMemSurface passed.
//

HRESULT
CHwVidMemTextureManager::UnlockSysMemTexture(
    )
{
    HRESULT hr = S_OK;
    
    Assert(IsSysMemTextureValid());

#if DBG
    Assert(m_fDBGSysMemTextureIsLocked);
    // If the unlock fails, callers should not try to unlock again.
    m_fDBGSysMemTextureIsLocked = false;
#endif

    m_pDeviceNoRef->GetDeviceContext()->Unmap(m_pSysMemTexture->GetD3DTextureNoRef(), 0);

Cleanup:
    RRETURN(hr);
}



//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwVidMemTextureManager::PushBitsToVidMemTexture
//
//  Synopsis:  
//      Create the video memory texture if necessary and send the bits from the
//      system memory texture to it.
//

HRESULT
CHwVidMemTextureManager::PushBitsToVidMemTexture()
{
    HRESULT hr = S_OK;

    D3DSurface *pD3DVidMemSurface = NULL;

    Assert(m_pSysMemTexture->IsValid());

    //
    // Check to see if the video memory texture is already valid
    //

    if (   m_pVideoMemTexture
        && !m_pVideoMemTexture->IsValid()
       )
    {
        ReleaseInterface(m_pVideoMemTexture);
    }

    //
    // (Re)create the video memory texture
    //
    if (NULL == m_pVideoMemTexture)
    {
        IFC(CD3DVidMemOnlyTexture::Create(
            &m_descRequiredForVidMem,
            m_uLevelsForVidMem,
            true, // fIsEvictable
            m_pDeviceNoRef,
            &m_pVideoMemTexture,
            /* HANDLE *pSharedHandle */ NULL
            ));
    }

    //
    // Get the surface from the texture
    //

    m_pDeviceNoRef->UpdateTexture(m_pSysMemTexture->GetD3DTextureNoRef(), m_pVideoMemTexture->GetD3DTextureNoRef());

    //
    // We've dirtied the 0 level and on some cards we need to update the other
    // levels of the mipmaps.  On other cards or if we don't have mipmaps
    // this is a no-op.
    //
    IFC(m_pVideoMemTexture->UpdateMipmapLevels());

Cleanup:
    ReleaseInterfaceNoNULL(pD3DVidMemSurface);

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwVidMemTextureManager::ComputeTextureDesc
//
//  Synopsis:  
//      Compute the texture description used for creating the video memory
//      texture. Make sure that there is nothing wrong with it.
//

void
CHwVidMemTextureManager::ComputeTextureDesc(
    DXGI_FORMAT dxgiFormat,
    UINT uWidth,
    UINT uHeight,
    TextureMipMapLevel eMipMapLevel
    DBG_COMMA_PARAM(bool bDbgConditionalNonPowTwoOkay)
    )
{
    m_descRequiredForVidMem.Format = dxgiFormat;
    
    CD3DTexture::DetermineLevels(
        m_pDeviceNoRef,
        eMipMapLevel,
        uWidth,
        uHeight,
        OUT &m_uLevelsForVidMem
        );

    m_descRequiredForVidMem.Width = uWidth;
    m_descRequiredForVidMem.Height = uHeight;
}


