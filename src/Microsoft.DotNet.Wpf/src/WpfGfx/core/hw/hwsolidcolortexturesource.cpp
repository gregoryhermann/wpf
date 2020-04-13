// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwSolidColorTextureSource implementation
//

#include "precomp.hpp"

MtDefine(CHwSolidColorTextureSource, MILRender, "CHwSolidColorTextureSource");

//+----------------------------------------------------------------------------
//
//  Function:  CHwSolidColorTextureSource::ctor
//
//  Synopsis:  Initializes the device and texture.
//
//-----------------------------------------------------------------------------
CHwSolidColorTextureSource::CHwSolidColorTextureSource(
    __in_ecount(1) CD3DDeviceLevel1 *pD3DDevice
    )
    : CHwTexturedColorSource(pD3DDevice)
{
    m_pLockableTexture = NULL;

    m_color.a = 1.0f;
    m_color.r = 1.0f;
    m_color.g = 1.0f;
    m_color.b = 1.0f;

    m_fValidRealization = false;

    //
    // Future Consideration:  PERF: Investigate setting u and v sampler state only when necessary
    // This color should still render fine when rendered with a sampler state of anything but
    // border. It would render correctly with border as well so long as we could guarantee
    // that the u and v coordinates were always exactly 0.5. This would be risky though.
    //

    SetFilterAndWrapModes(
        MilBitmapInterpolationMode::Linear,
        MilBitmapWrapMode::Extend,
        MilBitmapWrapMode::Extend
        );

    //
    // The DeviceToSource matrix does not matter for solid colors since
    // there is only one texel and we use extend/clamp to map it to the infinite plane.
    //
    m_matXSpaceToTextureUV.SetIdentity();
#if DBG
    DbgMarkXSpaceToTextureUVAsSet(XSpaceIsIrrelevant);
#endif
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwSolidColorTextureSource::dtor
//
//  Synopsis:  Releases the texture.
//
//-----------------------------------------------------------------------------
CHwSolidColorTextureSource::~CHwSolidColorTextureSource()
{
    ReleaseInterfaceNoNULL(m_pLockableTexture);
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwSolidColorTextureSource::Create
//
//  Synopsis:  Creates a CHwSolidColorTextureSource given a device and
//             initializes it.
//
//-----------------------------------------------------------------------------
__checkReturn HRESULT
CHwSolidColorTextureSource::Create(
    __in_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
    __deref_out_ecount(1) CHwSolidColorTextureSource **ppTextureSource
    )
{
    HRESULT hr = S_OK;
    CHwSolidColorTextureSource *pNewTextureSource = NULL;

    *ppTextureSource = NULL;

    pNewTextureSource = new CHwSolidColorTextureSource(pD3DDevice);

    IFCOOM(pNewTextureSource);

    pNewTextureSource->AddRef();

    *ppTextureSource = pNewTextureSource;

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterfaceNoNULL(pNewTextureSource);
    }

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Function:  CHwSolidColorTextureSource::GetSourceType
//
//  Synopsis:  Returns Textured-Constant Source Type
//
//-----------------------------------------------------------------------------
CHwColorSource::TypeFlags
CHwSolidColorTextureSource::GetSourceType() const
{
    return Texture | Constant;
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwSolidColorTextureSource::SendDeviceStates
//
//  Synopsis:  Send the device states to render the solid color texture
//
//-----------------------------------------------------------------------------
HRESULT
CHwSolidColorTextureSource::SendDeviceStates(
    DWORD dwStage,
    DWORD dwSampler
    )
{
    HRESULT hr = S_OK;

    Assert(m_fValidRealization);

    IFC(CHwTexturedColorSource::SendDeviceStates(
        dwStage,
        dwSampler
        ));

    IFC(m_pDevice->SetTexture(dwSampler, m_pLockableTexture));

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwSolidColorTextureSource::Realize
//
//  Synopsis:  Should Realize any data necessary which includes filling the texture.
//
//-----------------------------------------------------------------------------
HRESULT
CHwSolidColorTextureSource::Realize()
{
    HRESULT hr = S_OK;


    // If we don't have a texture, create one

    if (!m_pLockableTexture)
    {
        IFC(CreateLockableTexture());
        m_fValidRealization = false;
    }


    Assert(m_pLockableTexture);


    // If our source data is different than realized
    // update it.

    if (!m_fValidRealization)
    {
        IFC(FillTexture());
    }

    m_fValidRealization = true;


Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwSolidColorTextureSource::CreateTexture
//
//  Synopsis:  Sets up and creates a 1 x 1 Lockable Texture
//
//-----------------------------------------------------------------------------
__checkReturn HRESULT
CHwSolidColorTextureSource::CreateLockableTexture()
{
    HRESULT hr = S_OK;
    D3D11_TEXTURE2D_DESC textureDesc = { 0 };
    CD3DLockableTexture *pTexture = NULL;

    Assert(m_pLockableTexture == NULL);

    // Set up our surface format of 1x1 32 bpp Managed

    textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    textureDesc.Width = 1;
    textureDesc.Height = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.ArraySize = 1;

    IFC(m_pDevice->CreateLockableTexture(
        &textureDesc,
        &pTexture
        ));
    
    Assert(pTexture);

    m_pLockableTexture = pTexture;

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwSolidColorTextureSource::FillTexture
//
//  Synopsis:  Locks the texture and populates it's single texel with our color.
//
//-----------------------------------------------------------------------------
HRESULT
CHwSolidColorTextureSource::FillTexture()
{
    HRESULT hr = S_OK;
    D3DLOCKED_RECT d3dRect;

    Assert(m_pLockableTexture);


    // Lock the Rect, we pass in NULL for the rect to lock since we want the
    // whole texture.

    IFC(m_pLockableTexture->LockRect(
        &d3dRect,
        NULL,
        0
        ));

    DWORD *pdwTexel = reinterpret_cast<DWORD *>(d3dRect.pBits);


    *pdwTexel = Convert_MilColorF_scRGB_To_Premultiplied_MilColorB_sRGB(&m_color);

    IFC(m_pLockableTexture->UnlockRect());

Cleanup:
    RRETURN(hr);
}
