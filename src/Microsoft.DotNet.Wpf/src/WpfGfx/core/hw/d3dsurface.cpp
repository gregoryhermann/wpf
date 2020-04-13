// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CD3DSurface implementation
//
//      Provides basic abstraction of a D3D surface and tracks it as a D3D
//      resource.
//

#include "precomp.hpp"

MtDefine(CD3DSurface, MILRender, "CD3DSurface");
MtDefine(D3DResource_Surface, MILHwMetrics, "Approximate surface sizes");

//+------------------------------------------------------------------------
//
//  Member:    CD3DSurface::Create
//
//  Synopsis:  Create a CD3DSurface object to wrap a D3D texture
//
//-------------------------------------------------------------------------
HRESULT 
CD3DSurface::Create(
    __inout_ecount(1) CD3DResourceManager *pResourceManager,
    __inout_ecount(1) D3DSurface *pD3DSurface,
    __deref_out_ecount(1) CD3DSurface **ppSurface
    )
{
    HRESULT hr = S_OK;

    *ppSurface = NULL;

    //
    // Create the D3D surface wrapper
    //

    *ppSurface = new CD3DSurface(pD3DSurface);
    IFCOOM(*ppSurface);
    (*ppSurface)->AddRef(); // CD3DSurface::ctor sets ref count == 0

    //
    // Call init
    //

    IFC((*ppSurface)->Init(
        pResourceManager
        ));

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterface(*ppSurface);
    }
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:    CD3DSurface::CD3DSurface
//
//  Synopsis:  ctor
//
//-------------------------------------------------------------------------
CD3DSurface::CD3DSurface(
    __inout_ecount(1) D3DSurface * const pD3DSurface
    )
    : m_pD3DSurface(pD3DSurface)
{
    m_pD3DSurface->AddRef();
}

//+------------------------------------------------------------------------
//
//  Member:    CD3DSurface::~CD3DSurface
//
//  Synopsis:  dtor
//
//-------------------------------------------------------------------------
CD3DSurface::~CD3DSurface()
{
    ReleaseInterfaceNoNULL(m_pD3DSurface);
}

//+------------------------------------------------------------------------
//
//  Member:    CD3DSurface::Init
//
//  Synopsis:  Inits the surface wrapper
//
//-------------------------------------------------------------------------
HRESULT 
CD3DSurface::Init(
    __inout_ecount(1) CD3DResourceManager *pResourceManager
    )
{
    HRESULT hr = S_OK;
    UINT uResourceSize;

    //
    // Compute the size of the resource
    //

    IFC(m_pD3DSurface->GetDesc(&m_d3dsd));

    UINT uSamplesPerPixel = 1;
    
    uSamplesPerPixel = static_cast<UINT>(m_d3dsd.SampleDesc.Count);
    
    uResourceSize = D3DFormatSize(m_d3dsd.Format) * m_d3dsd.Width * m_d3dsd.Height * uSamplesPerPixel;
    Assert(uResourceSize > 0);

    //
    // Init the base class
    //

    CD3DResource::Init(pResourceManager, uResourceSize);

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:    CD3DSurface::ReleaseD3DResources
//
//  Synopsis:  Release the surface.
//
//             This method may only be called by CD3DResourceManager because
//             there are various restrictions around when a call to
//             ReleaseD3DResources is okay.
//
//-------------------------------------------------------------------------
void 
CD3DSurface::ReleaseD3DResources()
{
    // This resource should have been marked invalid already or at least be out
    // of use.
    Assert(!m_fResourceValid || (m_cRef == 0));
    Assert(IsValid() == m_fResourceValid);

    // This context is protected so it is safe to release the D3D resource
    ReleaseInterface((*const_cast<D3DSurface **>(&m_pD3DSurface)));

    return;
}

//+------------------------------------------------------------------------
//
//  Member:    CD3DSurface::GetSurfaceSize
//
//  Synopsis:  Return the dimensions of the surface.
//
//-------------------------------------------------------------------------
void
CD3DSurface::GetSurfaceSize(
    __out_ecount(1) UINT *puWidth,
    __out_ecount(1) UINT *puHeight
    ) const
{
    Assert(IsValid());

    *puWidth = m_d3dsd.Width;
    *puHeight = m_d3dsd.Height;
}

//+------------------------------------------------------------------------
//
//  Member:    CD3DSurface::GetDC
//
//  Synopsis:  Retrieve a DC wrapping this surface
//
//-------------------------------------------------------------------------
HRESULT 
CD3DSurface::GetDC(
    __out_ecount(1) HDC *phdc
    )
{
    HRESULT hr = S_OK;

    HDC hBackBuffer = NULL;

    *phdc = NULL;

#ifndef DX11
    MIL_THR(m_pD3DSurface->GetDC(
        &hBackBuffer
        ));
#else
    MIL_THR(m_pD3DSurface->GetDC(
        FALSE,
        &hBackBuffer
    ));
#endif

    //
    // D3D isn't supposed to return E_FAIL to us.  We want to see if we can 
    // reinterpret it.
    //
    if (hr == E_FAIL)
    {
        //
        // Check to see if we're close to being out of available GDI handles.
        // If we are we'll evaluate that as E_OUTOFMEMORY, otherwise we'll
        // return D3DERR_DRIVERINTERNALERROR.
        //
        IFC(CheckGUIHandleQuota(
            GR_GDIOBJECTS,
            E_OUTOFMEMORY,
            D3DERR_DRIVERINTERNALERROR
            ));
    }

    //
    // If we've succeeded assign the Handle to return.
    //
    if (SUCCEEDED(hr))
    {
        *phdc = hBackBuffer;
        hBackBuffer = NULL;
    }

Cleanup:
    //
    // If we have a backbuffer handle at this point it means 
    // something failed.
    //
    if (hBackBuffer)
    {
#ifndef DX11
        IGNORE_HR(m_pD3DSurface->ReleaseDC(hBackBuffer));
#else
        IGNORE_HR(m_pD3DSurface->ReleaseDC(nullptr));
#endif
    }

    RRETURN(hr);
}



