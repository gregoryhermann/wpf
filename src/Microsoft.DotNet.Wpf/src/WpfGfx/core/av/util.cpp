// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_media
//      $Keywords:
//
//  $Description:
//      Provides for simple utility functions. The general rule is that none of
//      the functions in this file can have dependencies on other functions in
//      the file. If this rule is broken, seperate out the functions into their
//      own file.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
#include "precomp.hpp"
#include "util.tmh"

//+-----------------------------------------------------------------------------
//
//  Function:
//      GetLastErrorAsFailHR
//
//  Synopsis:
//      Gets the last error, turns it into an HRESULT and guarantees that if the
//      caller did not behave and set a last error, it will be E_FAIL.
//
//------------------------------------------------------------------------------
HRESULT
GetLastErrorAsFailHR(
    void
    )
{
    DWORD   Status = GetLastError();
    HRESULT hr = HRESULT_FROM_WIN32(Status);

    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
    }

    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      Win32StatusFromHR
//
//  Synopsis:
//      Returns a Win32 status from an HRESULT if the facility is Win32,
//      otherwise the status will just be the HRESULT.
//
//------------------------------------------------------------------------------
DWORD
Win32StatusFromHR(
    __in    HRESULT     hr
    )
{
    DWORD   Status = ERROR_INVALID_FUNCTION;

    if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
    {
        Status = SCODE(hr);
    }
    else
    {
        //
        // Is there a better thing to do here?
        //
        Status = hr;
    }

    return Status;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      CopyHeapString
//
//  Synopsis:
//      Copies the input string and returns a new string allocated on the default
//      heap. The output string is NULL if the input string is NULL. The output
//      string is free'd if it is currently set.
//
//------------------------------------------------------------------------------
HRESULT
CopyHeapString(
    __in_opt    PCWSTR          stringIn,
    __deref_out PWSTR           *pStringOut
    )
{
    HRESULT hr = S_OK;

    PWSTR   newString = NULL;

    if (NULL != *pStringOut)
    {
        delete[] *pStringOut;

        *pStringOut = NULL;
    }

    if (NULL != stringIn)
    {
        size_t  length = 0;
        IFC(SizeTAdd(wcslen(stringIn), 1, &length));

        newString = new WCHAR[length];

        IFCOOM(newString);

        IFC(StringCchCopy(newString, length, stringIn));
    }

    *pStringOut = newString;

Cleanup:

    if (FAILED(hr))
    {
        delete[] newString;
    }

    RRETURN(hr);
}

class MediaDeviceConsumer : public IMediaDeviceConsumer
{
    D3DDeviceContext    *m_pD3DDeviceContext;

public:

    MediaDeviceConsumer() : m_pD3DDeviceContext(NULL)
    {}

    ~MediaDeviceConsumer()
    {
        ReleaseInterface(m_pD3DDeviceContext);
    }

    STDMETHOD_(void, SetD3DDeviceContext)(
        __in    D3DDeviceContext    *pD3DDeviceContext
        )
    {
        ReplaceInterface(m_pD3DDeviceContext, pD3DDeviceContext);
    }

    void
    GetD3DDeviceContext(
        __deref_out     D3DDeviceContext    **ppD3DDeviceContext
        )
    {
        SetInterface(*ppD3DDeviceContext, m_pD3DDeviceContext);
    }
};


void
GetUnderlyingDevice(
    __in        CD3DDeviceLevel1    *pCD3DDeviceLevel1,
    __deref_out D3DDeviceContext    **ppD3DDeviceContext
    )
{
    MediaDeviceConsumer mdc;
#ifndef DX11
    pCD3DDeviceLevel1->InitializeIMediaDeviceConsumer(&mdc);
#endif

    mdc.GetD3DDeviceContext(ppD3DDeviceContext);
}

HRESULT
ConvertSampleToMediaBuffer(
    __in    IMFSample       *pIMFSample,
    __out   CMFMediaBuffer  **ppCMFMediaBuffer
    )
{
    HRESULT         hr = S_OK;
    IMFMediaBuffer  *pIMFMediaBuffer = NULL;

    IFC(pIMFSample->GetBufferByIndex(0, &pIMFMediaBuffer));

    IFC(pIMFMediaBuffer->QueryInterface(
            IID_CMFMediaBuffer,
            reinterpret_cast<void **>(ppCMFMediaBuffer)));

Cleanup:
    ReleaseInterface(pIMFMediaBuffer);

    EXPECT_SUCCESSID(0, hr);
    RRETURN(hr);
}

