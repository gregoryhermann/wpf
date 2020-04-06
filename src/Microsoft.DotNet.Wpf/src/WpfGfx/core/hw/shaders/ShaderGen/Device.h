// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Abstract:
//      Helpers for standalone executable to generate shader sources.
//
//------------------------------------------------------------------------

#pragma once

//--------------------------------------------------------------------------------
//
// Class:   CFakeDevice
//
// Synopsis:
//      Fake D3DDeviceContext implementation to serve D3DXCreateEffectFromFile.
//      We don't want generator to depend on particular machine configuration,
//      so we don't create real D3DDeviceContext, instead using own implementation.
//      Luckily, D3DX effects need just a few functionality of it.
//
//--------------------------------------------------------------------------------

class CFakeDevice : public D3DDeviceContext
{
public:
    static HRESULT Create(
        __deref_out_ecount(1) D3DDeviceContext **ppDevice
        );

private:
    CFakeDevice();
    ~CFakeDevice();

    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG,AddRef)(THIS);
    STDMETHOD_(ULONG,Release)(THIS);

    /*** D3DDeviceContext methods ***/
    STDMETHOD(TestCooperativeLevel)(THIS);
    STDMETHOD_(UINT, GetAvailableTextureMem)(THIS);
    STDMETHOD(EvictManagedResources)(THIS);
    STDMETHOD(GetDirect3D)(THIS_ IDirect3D9** ppD3D9);
    STDMETHOD(GetDeviceCaps)(THIS_ D3DCAPS9* pCaps);
    STDMETHOD(GetDisplayMode)(THIS_ UINT iSwapChain,D3DDISPLAYMODE* pMode);
    STDMETHOD(GetCreationParameters)(THIS_ D3DDEVICE_CREATION_PARAMETERS *pParameters);
    STDMETHOD(SetCursorProperties)(THIS_ UINT XHotSpot,UINT YHotSpot,D3DSurface* pCursorBitmap);
    STDMETHOD_(void, SetCursorPosition)(THIS_ int X,int Y,DWORD Flags);
    STDMETHOD_(BOOL, ShowCursor)(THIS_ BOOL bShow);
    STDMETHOD(CreateAdditionalSwapChain)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain9** pSwapChain);
    STDMETHOD(GetSwapChain)(THIS_ UINT iSwapChain,IDirect3DSwapChain9** pSwapChain);
    STDMETHOD_(UINT, GetNumberOfSwapChains)(THIS);
    STDMETHOD(Reset)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters);
    STDMETHOD(Present)(THIS_ CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion);
    STDMETHOD(GetBackBuffer)(THIS_ UINT iSwapChain,UINT iBackBuffer,D3DBACKBUFFER_TYPE Type,D3DSurface** ppBackBuffer);
    STDMETHOD(GetRasterStatus)(THIS_ UINT iSwapChain,D3DRASTER_STATUS* pRasterStatus);
    STDMETHOD(SetDialogBoxMode)(THIS_ BOOL bEnableDialogs);
    STDMETHOD_(void, SetGammaRamp)(THIS_ UINT iSwapChain,DWORD Flags,CONST D3DGAMMARAMP* pRamp);
    STDMETHOD_(void, GetGammaRamp)(THIS_ UINT iSwapChain,D3DGAMMARAMP* pRamp);
    STDMETHOD(CreateTexture)(THIS_ UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,D3DTexture** ppTexture,HANDLE* pSharedHandle);
    STDMETHOD(CreateVolumeTexture)(THIS_ UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture9** ppVolumeTexture,HANDLE* pSharedHandle);
    STDMETHOD(CreateCubeTexture)(THIS_ UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture9** ppCubeTexture,HANDLE* pSharedHandle);
    STDMETHOD(CreateVertexBuffer)(THIS_ UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,D3DVertexBuffer** ppVertexBuffer,HANDLE* pSharedHandle);
    STDMETHOD(CreateIndexBuffer)(THIS_ UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,D3DIndexBuffer** ppIndexBuffer,HANDLE* pSharedHandle);
    STDMETHOD(CreateRenderTarget)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,D3DSurface** ppSurface,HANDLE* pSharedHandle);
    STDMETHOD(CreateDepthStencilSurface)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,D3DSurface** ppSurface,HANDLE* pSharedHandle);
    STDMETHOD(UpdateSurface)(THIS_ D3DSurface* pSourceSurface,CONST RECT* pSourceRect,D3DSurface* pDestinationSurface,CONST POINT* pDestPoint);
    STDMETHOD(UpdateTexture)(THIS_ IDirect3DBaseTexture9* pSourceTexture,IDirect3DBaseTexture9* pDestinationTexture);
    STDMETHOD(GetRenderTargetData)(THIS_ D3DSurface* pRenderTarget,D3DSurface* pDestSurface);
    STDMETHOD(GetFrontBufferData)(THIS_ UINT iSwapChain,D3DSurface* pDestSurface);
    STDMETHOD(StretchRect)(THIS_ D3DSurface* pSourceSurface,CONST RECT* pSourceRect,D3DSurface* pDestSurface,CONST RECT* pDestRect,D3DTEXTUREFILTERTYPE Filter);
    STDMETHOD(ColorFill)(THIS_ D3DSurface* pSurface,CONST RECT* pRect,D3DCOLOR color);
    STDMETHOD(CreateOffscreenPlainSurface)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,D3DSurface** ppSurface,HANDLE* pSharedHandle);
    STDMETHOD(SetRenderTarget)(THIS_ DWORD RenderTargetIndex,D3DSurface* pRenderTarget);
    STDMETHOD(GetRenderTarget)(THIS_ DWORD RenderTargetIndex,D3DSurface** ppRenderTarget);
    STDMETHOD(SetDepthStencilSurface)(THIS_ D3DSurface* pNewZStencil);
    STDMETHOD(GetDepthStencilSurface)(THIS_ D3DSurface** ppZStencilSurface);
    STDMETHOD(BeginScene)(THIS);
    STDMETHOD(EndScene)(THIS);
    STDMETHOD(Clear)(THIS_ DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil);
    STDMETHOD(SetTransform)(THIS_ D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix);
    STDMETHOD(GetTransform)(THIS_ D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix);
    STDMETHOD(MultiplyTransform)(THIS_ D3DTRANSFORMSTATETYPE,CONST D3DMATRIX*);
    STDMETHOD(SetViewport)(THIS_ CONST D3DVIEWPORT9* pViewport);
    STDMETHOD(GetViewport)(THIS_ D3DVIEWPORT9* pViewport);
    STDMETHOD(SetMaterial)(THIS_ CONST D3DMATERIAL9* pMaterial);
    STDMETHOD(GetMaterial)(THIS_ D3DMATERIAL9* pMaterial);
    STDMETHOD(SetLight)(THIS_ DWORD Index,CONST D3DLIGHT9*);
    STDMETHOD(GetLight)(THIS_ DWORD Index,D3DLIGHT9*);
    STDMETHOD(LightEnable)(THIS_ DWORD Index,BOOL Enable);
    STDMETHOD(GetLightEnable)(THIS_ DWORD Index,BOOL* pEnable);
    STDMETHOD(SetClipPlane)(THIS_ DWORD Index,CONST float* pPlane);
    STDMETHOD(GetClipPlane)(THIS_ DWORD Index,float* pPlane);
    STDMETHOD(SetRenderState)(THIS_ D3DRENDERSTATETYPE State,DWORD Value);
    STDMETHOD(GetRenderState)(THIS_ D3DRENDERSTATETYPE State,DWORD* pValue);
    STDMETHOD(CreateStateBlock)(THIS_ D3DSTATEBLOCKTYPE Type,IDirect3DStateBlock9** ppSB);
    STDMETHOD(BeginStateBlock)(THIS);
    STDMETHOD(EndStateBlock)(THIS_ IDirect3DStateBlock9** ppSB);
    STDMETHOD(SetClipStatus)(THIS_ CONST D3DCLIPSTATUS9* pClipStatus);
    STDMETHOD(GetClipStatus)(THIS_ D3DCLIPSTATUS9* pClipStatus);
    STDMETHOD(GetTexture)(THIS_ DWORD Stage,IDirect3DBaseTexture9** ppTexture);
    STDMETHOD(SetTexture)(THIS_ DWORD Stage,IDirect3DBaseTexture9* pTexture);
    STDMETHOD(GetTextureStageState)(THIS_ DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue);
    STDMETHOD(SetTextureStageState)(THIS_ DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value);
    STDMETHOD(GetSamplerState)(THIS_ DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD* pValue);
    STDMETHOD(SetSamplerState)(THIS_ DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD Value);
    STDMETHOD(ValidateDevice)(THIS_ DWORD* pNumPasses);
    STDMETHOD(SetPaletteEntries)(THIS_ UINT PaletteNumber,CONST PALETTEENTRY* pEntries);
    STDMETHOD(GetPaletteEntries)(THIS_ UINT PaletteNumber,PALETTEENTRY* pEntries);
    STDMETHOD(SetCurrentTexturePalette)(THIS_ UINT PaletteNumber);
    STDMETHOD(GetCurrentTexturePalette)(THIS_ UINT *PaletteNumber);
    STDMETHOD(SetScissorRect)(THIS_ CONST RECT* pRect);
    STDMETHOD(GetScissorRect)(THIS_ RECT* pRect);
    STDMETHOD(SetSoftwareVertexProcessing)(THIS_ BOOL bSoftware);
    STDMETHOD_(BOOL, GetSoftwareVertexProcessing)(THIS);
    STDMETHOD(SetNPatchMode)(THIS_ float nSegments);
    STDMETHOD_(float, GetNPatchMode)(THIS);
    STDMETHOD(DrawPrimitive)(THIS_ D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount);
    STDMETHOD(DrawIndexedPrimitive)(THIS_ D3DPRIMITIVETYPE,INT BaseVertexIndex,UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount);
    STDMETHOD(DrawPrimitiveUP)(THIS_ D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);
    STDMETHOD(DrawIndexedPrimitiveUP)(THIS_ D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);
    STDMETHOD(ProcessVertices)(THIS_ UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,D3DVertexBuffer* pDestBuffer,IDirect3DVertexDeclaration9* pVertexDecl,DWORD Flags);
    STDMETHOD(CreateVertexDeclaration)(THIS_ CONST D3DVERTEXELEMENT9* pVertexElements,IDirect3DVertexDeclaration9** ppDecl);
    STDMETHOD(SetVertexDeclaration)(THIS_ IDirect3DVertexDeclaration9* pDecl);
    STDMETHOD(GetVertexDeclaration)(THIS_ IDirect3DVertexDeclaration9** ppDecl);
    STDMETHOD(SetFVF)(THIS_ DWORD FVF);
    STDMETHOD(GetFVF)(THIS_ DWORD* pFVF);
    STDMETHOD(CreateVertexShader)(THIS_ CONST DWORD* pFunction,IDirect3DVertexShader9** ppShader);
    STDMETHOD(SetVertexShader)(THIS_ IDirect3DVertexShader9* pShader);
    STDMETHOD(GetVertexShader)(THIS_ IDirect3DVertexShader9** ppShader);
    STDMETHOD(SetVertexShaderConstantF)(THIS_ UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount);
    STDMETHOD(GetVertexShaderConstantF)(THIS_ UINT StartRegister,float* pConstantData,UINT Vector4fCount);
    STDMETHOD(SetVertexShaderConstantI)(THIS_ UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount);
    STDMETHOD(GetVertexShaderConstantI)(THIS_ UINT StartRegister,int* pConstantData,UINT Vector4iCount);
    STDMETHOD(SetVertexShaderConstantB)(THIS_ UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount);
    STDMETHOD(GetVertexShaderConstantB)(THIS_ UINT StartRegister,BOOL* pConstantData,UINT BoolCount);
    STDMETHOD(SetStreamSource)(THIS_ UINT StreamNumber,D3DVertexBuffer* pStreamData,UINT OffsetInBytes,UINT Stride);
    STDMETHOD(GetStreamSource)(THIS_ UINT StreamNumber,D3DVertexBuffer** ppStreamData,UINT* pOffsetInBytes,UINT* pStride);
    STDMETHOD(SetStreamSourceFreq)(THIS_ UINT StreamNumber,UINT Setting);
    STDMETHOD(GetStreamSourceFreq)(THIS_ UINT StreamNumber,UINT* pSetting);
    STDMETHOD(SetIndices)(THIS_ D3DIndexBuffer* pIndexData);
    STDMETHOD(GetIndices)(THIS_ D3DIndexBuffer** ppIndexData);
    STDMETHOD(CreatePixelShader)(THIS_ CONST DWORD* pFunction,IDirect3DPixelShader9** ppShader);
    STDMETHOD(SetPixelShader)(THIS_ IDirect3DPixelShader9* pShader);
    STDMETHOD(GetPixelShader)(THIS_ IDirect3DPixelShader9** ppShader);
    STDMETHOD(SetPixelShaderConstantF)(THIS_ UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount);
    STDMETHOD(GetPixelShaderConstantF)(THIS_ UINT StartRegister,float* pConstantData,UINT Vector4fCount);
    STDMETHOD(SetPixelShaderConstantI)(THIS_ UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount);
    STDMETHOD(GetPixelShaderConstantI)(THIS_ UINT StartRegister,int* pConstantData,UINT Vector4iCount);
    STDMETHOD(SetPixelShaderConstantB)(THIS_ UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount);
    STDMETHOD(GetPixelShaderConstantB)(THIS_ UINT StartRegister,BOOL* pConstantData,UINT BoolCount);
    STDMETHOD(DrawRectPatch)(THIS_ UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo);
    STDMETHOD(DrawTriPatch)(THIS_ UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo);
    STDMETHOD(DeletePatch)(THIS_ UINT Handle);
    STDMETHOD(CreateQuery)(THIS_ D3DQUERYTYPE Type,IDirect3DQuery9** ppQuery);
    STDMETHOD(SetConvolutionMonoKernel)(THIS_ UINT width,UINT height,float* rows,float* columns);
    STDMETHOD(ComposeRects)(THIS_ D3DSurface* pSrc,D3DSurface* pDst,D3DVertexBuffer* pSrcRectDescs,UINT NumRects,D3DVertexBuffer* pDstRectDescs,D3DCOMPOSERECTSOP Operation,int Xoffset,int Yoffset);
    STDMETHOD(PresentEx)(THIS_ CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion,DWORD dwFlags,D3DSurface * pSourceSurfaceOverride);
    STDMETHOD(GetGPUThreadPriority)(THIS_ UINT *pPriority);
    STDMETHOD(SetGPUThreadPriority)(THIS_ UINT Priority);
    STDMETHOD(WaitForVBlank)(THIS_ UINT iSwapChain);
    STDMETHOD(SetMaximumFrameLatency)(THIS_ UINT MaxLatency);
    STDMETHOD(GetMaximumFrameLatency)(THIS_ UINT * pMaxLatency);

    // LH only methods

    STDMETHOD(PresentEx)(const RECT *,const RECT *,HWND,const RGNDATA *,DWORD);
    STDMETHOD(GetGPUThreadPriority)(INT *);
    STDMETHOD(SetGPUThreadPriority)(INT);
    STDMETHOD(CheckDeviceState)(HWND);
    STDMETHOD(CreateRenderTargetEx)(UINT, UINT, D3DFORMAT, D3DMULTISAMPLE_TYPE, DWORD, BOOL, D3DSurface **, HANDLE *, DWORD);
    STDMETHOD(CreateOffscreenPlainSurfaceEx)(UINT, UINT, D3DFORMAT, D3DPOOL, D3DSurface **, HANDLE *, DWORD);
    STDMETHOD(CreateDepthStencilSurfaceEx)(UINT,UINT,D3DFORMAT,D3DMULTISAMPLE_TYPE,DWORD,BOOL,D3DSurface **,HANDLE *,DWORD);

private:
    ULONG m_cRef;
};

