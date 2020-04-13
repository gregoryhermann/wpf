// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_d3d
//      $Keywords:
//
//  $Description:
//      Implementation of GPUMarker used to monitor rendering progress
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
#include "precomp.hpp"
MtDefine(CGPUMarker, MILRender, "CGPUMarker");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGPUMarker::CGPUMarker
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------------
CGPUMarker::CGPUMarker(
    __in_ecount(1) ID3D11Query *pQuery, 
    ULONGLONG ullMarkerId
    ) : m_pQuery(pQuery)
{ 
    Reset(ullMarkerId);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGPUMarker::~CGPUMarker
//
//  Synopsis:
//      Destructor
//
//------------------------------------------------------------------------------
CGPUMarker::~CGPUMarker() 
{ 
    ReleaseInterface(m_pQuery);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGPUMarker::Reset
//
//  Synopsis:
//      Resets the marker for reuse
//
//  Returns:
//      S_OK if succeeds
//
//------------------------------------------------------------------------------
void CGPUMarker::Reset(ULONGLONG ullMarkerId)
{
    m_fIssued = FALSE;
    m_fConsumed = FALSE;
    m_ullMarkerId = ullMarkerId;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGPUMarker::InsertIntoCommandStream
//
//  Synopsis:
//      Insert the marker into the command stream
//
//  Returns:
//      S_OK if succeeds
//
//------------------------------------------------------------------------------
HRESULT CGPUMarker::InsertIntoCommandStream(D3DDeviceContext* pContext)
{
    HRESULT hr = S_OK;

    if (!m_pQuery)
    {
        IFC(E_FAIL);
    }

    pContext->Begin(m_pQuery);
    pContext->End(m_pQuery);
    m_fIssued = TRUE;

  Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGPUMarker::CheckStatus
//
//  Synopsis:
//      Checks if a marker has been processed by the GPU
//
//  Returns:
//      *pfConsumed will be true if the marker was consumed, false otherwise.
//
//------------------------------------------------------------------------------
HRESULT CGPUMarker::CheckStatus(
    D3DDeviceContext* pContext,
    BOOL fFlush,
    __out_ecount(1) bool *pfConsumed
    )
{
    HRESULT hr = S_OK;

    *pfConsumed = false;

    if (m_fIssued)
    {
        if (!m_fConsumed)
        {
            DWORD dwFlags = (fFlush ? 0 : D3D11_ASYNC_GETDATA_DONOTFLUSH);
            MIL_THR(pContext->GetData(m_pQuery, nullptr, 0, dwFlags));
            m_fConsumed = (hr == S_OK);

            if (hr == S_FALSE)
            {
                hr = S_OK;
            }
        }

        *pfConsumed = (m_fConsumed != 0);
    }

    RRETURN(hr);
}


