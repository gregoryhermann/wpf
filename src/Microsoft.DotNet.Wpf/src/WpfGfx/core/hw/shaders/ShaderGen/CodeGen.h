// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include <d3dcompiler.h>

//+-----------------------------------------------------------------------
//

//
//  Abstract:
//      Routines to write generated C++ code.
//
//------------------------------------------------------------------------

#pragma once

//--------------------------------------------------------------------------------
//
// Class:   CCodeGen
//
// Synopsis:
//      Converts HLSL files to C++.
//
//  Usage pattern:
//
//      // Create the instance
//      CCodeGen codeGen;
//
//      // Initialize, giving output file names
//      IFC(codeGen.Initialize("myShaders.cpp", "myShader.h"));
//
//      // Compile as many files as needed
//      IFC(codeGen.CompileEffect(L"foo.fx", "Foo"));
//      IFC(codeGen.CompileEffect(L"bar.fx", "Zoo"));
//
//      // Destruct the instance
//--------------------------------------------------------------------------------

class CCodeGen
{
public:
    CCodeGen();
    HRESULT Initialize(
        __in char const *pszFileNameCpp,
        __in char const *pszFileNameHpp
        );
    HRESULT CompileEffect(
        __in WCHAR const *pszEffectFileName,
        __in char const* pszTechniqueName,
        __in char const *pszEffectName
        );
    ~CCodeGen();

    static char const sc_szTitle[];

private:
    HRESULT WritePixelShader();
    void WriteDwordArray(
        __in DWORD const* pFunction,
        __in DWORD cbSize
    );

private:
    FILE *m_pFileCpp;
    FILE *m_pFileHpp;
    char const *m_pszEffectName;
    char const* m_pszTechniqueName;
    ID3DBlob* m_pShader;
    UINT m_cbTotal;
};

