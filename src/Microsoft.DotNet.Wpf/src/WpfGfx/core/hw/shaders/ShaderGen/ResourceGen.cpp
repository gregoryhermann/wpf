// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Abstract:
//      Routines to write generated shader's binary resource.
//
//------------------------------------------------------------------------

#include "precomp.h"

//+----------------------------------------------------------------------------
//
//  Member:    CResourceGen::CResourceGen
//
//  Synopsis:  ctor
//
//-----------------------------------------------------------------------------
CResourceGen::CResourceGen()
{
    m_pFileRc = NULL;
    m_pFileHpp = NULL;
    m_cbTotal = 0;
}

//+----------------------------------------------------------------------------
//
//  Member:    CResourceGen::~CResourceGen
//
//  Synopsis:  dtor
//
//-----------------------------------------------------------------------------
CResourceGen::~CResourceGen()
{
    if (m_pFileRc)
    {
        fprintf(m_pFileRc, "// Total data size = %d (0x%08x) bytes.\n", m_cbTotal, m_cbTotal);
        fclose(m_pFileRc);
    }
    if (m_pFileHpp)
    {
        fclose(m_pFileHpp);
    }
}


//+----------------------------------------------------------------------------
//
//  Member:    CResourceGen::Initialize
//
//  Synopsis:  Create an instance of CResourceGen with text file opened for writing.
//
//-----------------------------------------------------------------------------
HRESULT
CResourceGen::Initialize(
    __in char const *pszFileNameRc,
    __in char const *pszFileNameHpp,
    UINT uEnumStart
    )
{
    HRESULT hr = S_OK;

    fopen_s(&m_pFileRc, pszFileNameRc, "wt");
    if (!m_pFileRc)
    {
        printf("Can't open %s for writing\n", pszFileNameRc);
        IFC(E_FAIL);
    }

    fopen_s(&m_pFileHpp, pszFileNameHpp, "wt");
    if (!m_pFileHpp)
    {
        printf("Can't open %s for writing\n", pszFileNameHpp);
        IFC(E_FAIL);
    }

    fprintf(m_pFileRc, CCodeGen::sc_szTitle);
    fprintf(m_pFileHpp, CCodeGen::sc_szTitle);

    m_uEnumCurrent = uEnumStart;

Cleanup:
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:    CResourceGen::CompileEffect
//
//  Synopsis:  Create an instance of CResourceGen with text file opened for writing.
//
//-----------------------------------------------------------------------------
HRESULT
CResourceGen::CompileEffect(
    __in WCHAR const *pszEffectFileName,
    __in char const* pszTechniqueName,
    __in char const *pszEffectName
    )
{
    HRESULT hr = S_OK;

    m_pszEffectName = pszEffectName;
    m_pszTechniqueName = pszTechniqueName;

    IFC(D3DCompileFromFile(
        pszEffectFileName,
        NULL,
        NULL,
        "main",
        "ps_4_0_level_9_3",
        0,
        0,
        &m_pShader,
        &m_pCompilationErrors
        ));

    IFC(WritePixelShader());

Cleanup:
    ReleaseInterface(m_pShader);
    if (m_pCompilationErrors != NULL)
    {
        // Output compiler errors

        char *szErrors = (char *)m_pCompilationErrors->GetBufferPointer();
        printf("%s", szErrors);

        m_pCompilationErrors->Release();
    }
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:    CResourceGen::WritePixelShader
//
//  Synopsis:
//      Generate code for pixel shader.
//
//-----------------------------------------------------------------------------
HRESULT
CResourceGen::WritePixelShader()
{
    HRESULT hr = S_OK;

    const DWORD* pFunction = (const DWORD * )m_pShader->GetBufferPointer();

    // form shader data array definition header, like
    // g_PixelShader_Foo_Tech_Pass RCDATA
    fprintf(
        m_pFileRc,
        "g_PixelShader_%s_%s_%s RCDATA\n",
        m_pszEffectName,
        m_pszTechniqueName,
        "P0"
        );

    fprintf(
        m_pFileHpp,
        "#define g_PixelShader_%s_%s_%s %d\n",
        m_pszEffectName,
        m_pszTechniqueName,
        "P0",
        m_uEnumCurrent++
        );

    WriteDwordArray(pFunction, m_pShader->GetBufferSize());

//Cleanup:
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:    CResourceGen::WriteDwordArray
//
//  Synopsis:
//      Helper for WritePixelShader() and WriteVertexShader().
//      Generate array data C++ code.
//
//-----------------------------------------------------------------------------
void
CResourceGen::WriteDwordArray(
    __in DWORD const *pFunction,
    __in DWORD cbSize
)
{
    // open array data
    fprintf(m_pFileRc, "{\n");

    // write data array, uRowSize DWORDs per line
    static const UINT uRowSize = 6;

    for (UINT i = 0, n = cbSize/sizeof(DWORD); i < n; i++)
    {
        UINT j = i % uRowSize;
        if (j == 0)
        {
            fprintf(m_pFileRc, "    ");
        }
        fprintf(m_pFileRc, "0x%08xL", pFunction[i]);
        if (i+1 == n)
        {
            fprintf(m_pFileRc, "\n");
        }
        else
        {
            fprintf(m_pFileRc, ",");
            if (j+1 == uRowSize)
            {
                fprintf(m_pFileRc, "\n");
            }
            else
            {
                fprintf(m_pFileRc, " ");
            }
        }
    }

    // close array data
    fprintf(m_pFileRc, "};\n\n");

    // update total size of all arrays
    m_cbTotal += cbSize;
}

