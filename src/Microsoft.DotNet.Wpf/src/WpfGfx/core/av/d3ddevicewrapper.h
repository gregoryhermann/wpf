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
//      Header for the CD3DDeviceWrapper class, which wraps an instance of the
//      D3DDeviceContext interface. This wrapper was written for the purpose of
//      logging D3D calls, but it may also be used to restrict and/or redirect
//      D3D calls.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#pragma once

MtExtern(CD3DDeviceWrapper);

class CD3DDeviceWrapper :
    public D3DDeviceContext,
    public IDirect3DVideoDevice9
{
protected:
    CD3DDeviceWrapper();
    virtual ~CD3DDeviceWrapper();

public:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CD3DDeviceWrapper));

    static HRESULT Create(
        __in_ecount(1) D3DDeviceContext *pD3DDevice,
        __in_ecount(1) IDirect3D9 *pD3DWrapper,
        __deref_out_ecount(1) CD3DDeviceWrapper **ppD3DeviceWrapper
        );

    //
    // IUnknown
    //

    STDMETHOD(QueryInterface)(
        REFIID riid,
        __deref_out_ecount(1) LPVOID* ppvObject
        );

    STDMETHOD_(ULONG, AddRef)(
        );

    STDMETHOD_(ULONG, Release)(
        );

    //
    // D3DDeviceContext
    //

    STDMETHOD(TestCooperativeLevel)(
        ) NOTIMPL_METHOD

    STDMETHOD_(UINT, GetAvailableTextureMem)(
        ) { Assert(FALSE); return 0; }

    STDMETHOD(EvictManagedResources)(
        ) NOTIMPL_METHOD

    STDMETHOD(GetDirect3D)(
        __deref_out_ecount(1) IDirect3D9** ppD3D9
        );

    STDMETHOD(GetDeviceCaps)(
        __out_ecount(1) D3DCAPS9* pCaps
        );

    STDMETHOD(GetDisplayMode)(
        UINT iSwapChain,
        D3DDISPLAYMODE* pMode
        );

    STDMETHOD(GetCreationParameters)(
        __out_ecount(1) D3DDEVICE_CREATION_PARAMETERS *pParameters
        );

    STDMETHOD(SetCursorProperties)(
        UINT XHotSpot,
        UINT YHotSpot,
        D3DSurface* pCursorBitmap
        ) NOTIMPL_METHOD

    STDMETHOD_(void, SetCursorPosition)(
        int X,
        int Y,
        DWORD Flags
        ) { Assert(FALSE); return; }

    STDMETHOD_(BOOL, ShowCursor)(
        BOOL bShow
        ) { Assert(FALSE); return FALSE; }

    STDMETHOD(CreateAdditionalSwapChain)(
        D3DPRESENT_PARAMETERS* pPresentationParameters,
        IDirect3DSwapChain9** pSwapChain
        ) NOTIMPL_METHOD

    STDMETHOD(GetSwapChain)(
        UINT iSwapChain,
        IDirect3DSwapChain9** pSwapChain
        ) NOTIMPL_METHOD

    STDMETHOD_(UINT, GetNumberOfSwapChains)(
        ) { Assert(FALSE); return 0; }

    STDMETHOD(Reset)(
        D3DPRESENT_PARAMETERS* pPresentationParameters
        ) NOTIMPL_METHOD

    STDMETHOD(Present)(
        CONST RECT* pSourceRect,
        CONST RECT* pDestRect,
        HWND hDestWindowOverride,
        CONST RGNDATA* pDirtyRegion
        ) NOTIMPL_METHOD

    STDMETHOD(GetBackBuffer)(
        UINT iSwapChain,
        UINT iBackBuffer,
        D3DBACKBUFFER_TYPE Type,
        D3DSurface** ppBackBuffer
        ) NOTIMPL_METHOD

    STDMETHOD(GetRasterStatus)(
        UINT iSwapChain,
        D3DRASTER_STATUS* pRasterStatus
        ) NOTIMPL_METHOD

    STDMETHOD(SetDialogBoxMode)(
        BOOL bEnableDialogs
        ) NOTIMPL_METHOD

    STDMETHOD_(void, SetGammaRamp)(
        UINT iSwapChain,
        DWORD Flags,
        __in_ecount(1) CONST D3DGAMMARAMP* pRamp
        ) { Assert(FALSE); return; }

    STDMETHOD_(void, GetGammaRamp)(
        UINT iSwapChain,
        __out_ecount(1) D3DGAMMARAMP* pRamp
        ) { Assert(FALSE); return; }

    STDMETHOD(CreateTexture)(
        UINT Width,
        UINT Height,
        UINT Levels,
        DWORD Usage,
        D3DFORMAT Format,
        D3DPOOL Pool,
        __deref_out_ecount(1) D3DTexture** ppTexture,
        __out_ecount(1) HANDLE* pSharedHandle
        );

    STDMETHOD(CreateVolumeTexture)(
        UINT Width,
        UINT Height,
        UINT Depth,
        UINT Levels,
        DWORD Usage,
        D3DFORMAT Format,
        D3DPOOL Pool,
        IDirect3DVolumeTexture9** ppVolumeTexture,
        HANDLE* pSharedHandle
        ) NOTIMPL_METHOD

    STDMETHOD(CreateCubeTexture)(
        UINT EdgeLength,
        UINT Levels,
        DWORD Usage,
        D3DFORMAT Format,
        D3DPOOL Pool,
        IDirect3DCubeTexture9** ppCubeTexture,
        HANDLE* pSharedHandle
        ) NOTIMPL_METHOD

    STDMETHOD(CreateVertexBuffer)(
        UINT Length,
        DWORD Usage,
        DWORD FVF,
        D3DPOOL Pool,
        D3DVertexBuffer** ppVertexBuffer,
        HANDLE* pSharedHandle
        ) NOTIMPL_METHOD

    STDMETHOD(CreateIndexBuffer)(
        UINT Length,
        DWORD Usage,
        D3DFORMAT Format,
        D3DPOOL Pool,
        D3DIndexBuffer** ppIndexBuffer,
        HANDLE* pSharedHandle
        ) NOTIMPL_METHOD

    STDMETHOD(CreateRenderTarget)(
        UINT Width,
        UINT Height,
        D3DFORMAT Format,
        D3DMULTISAMPLE_TYPE MultiSample,
        DWORD MultisampleQuality,
        BOOL Lockable,
        D3DSurface** ppSurface,
        HANDLE* pSharedHandle
        ) NOTIMPL_METHOD

    STDMETHOD(CreateDepthStencilSurface)(
        UINT Width,
        UINT Height,
        D3DFORMAT Format,
        D3DMULTISAMPLE_TYPE MultiSample,
        DWORD MultisampleQuality,
        BOOL Discard,
        D3DSurface** ppSurface,
        HANDLE* pSharedHandle
        ) NOTIMPL_METHOD

    STDMETHOD(UpdateSurface)(
        D3DSurface* pSourceSurface,
        CONST RECT* pSourceRect,
        D3DSurface* pDestinationSurface,
        CONST POINT* pDestPoint
        ) NOTIMPL_METHOD

    STDMETHOD(UpdateTexture)(
        IDirect3DBaseTexture9* pSourceTexture,
        IDirect3DBaseTexture9* pDestinationTexture
        ) NOTIMPL_METHOD

    STDMETHOD(GetRenderTargetData)(
        D3DSurface* pRenderTarget,
        D3DSurface* pDestSurface
        ) NOTIMPL_METHOD

    STDMETHOD(GetFrontBufferData)(
        UINT iSwapChain,
        D3DSurface* pDestSurface
        ) NOTIMPL_METHOD

    STDMETHOD(StretchRect)(
        D3DSurface* pSourceSurface,
        CONST RECT* pSourceRect,
        D3DSurface* pDestSurface,
        CONST RECT* pDestRect,
        D3DTEXTUREFILTERTYPE Filter
        );

    STDMETHOD(ColorFill)(
       __inout_ecount(1)  D3DSurface* pSurface,
        __in_ecount(1) CONST RECT* pRect,
        D3DCOLOR color
        );

    STDMETHOD(CreateOffscreenPlainSurface)(
        UINT Width,
        UINT Height,
        D3DFORMAT Format,
        D3DPOOL Pool,
        __deref_out_ecount(1) D3DSurface** ppSurface,
        __out_ecount(1) HANDLE* pSharedHandle
        );

    STDMETHOD(SetRenderTarget)(
        DWORD RenderTargetIndex,
        D3DSurface* pRenderTarget
        );

    STDMETHOD(GetRenderTarget)(
        DWORD RenderTargetIndex,
        D3DSurface** ppRenderTarget
        );

    STDMETHOD(SetDepthStencilSurface)(
        D3DSurface* pNewZStencil
        ) NOTIMPL_METHOD

    STDMETHOD(GetDepthStencilSurface)(
        D3DSurface** ppZStencilSurface
        ) NOTIMPL_METHOD

    STDMETHOD(BeginScene)(
        ) NOTIMPL_METHOD

    STDMETHOD(EndScene)(
        ) NOTIMPL_METHOD

    STDMETHOD(Clear)(
        DWORD Count,
        CONST D3DRECT* pRects,
        DWORD Flags,
        D3DCOLOR Color,
        float Z,
        DWORD Stencil
        ) NOTIMPL_METHOD

    // error C4995: 'D3DMATRIX': name was marked as #pragma deprecated
    //
    // Ignore deprecation of D3DMATRIX for this prototype because
    // it is defined in the interface this class is implementing
#pragma warning (push)
#pragma warning (disable : 4995)
    STDMETHOD(SetTransform)(
        D3DTRANSFORMSTATETYPE State,
        CONST D3DMATRIX* pMatrix
        ) NOTIMPL_METHOD

    STDMETHOD(GetTransform)(
        D3DTRANSFORMSTATETYPE State,
        D3DMATRIX* pMatrix
        ) NOTIMPL_METHOD

    STDMETHOD(MultiplyTransform)(
        D3DTRANSFORMSTATETYPE,
        CONST D3DMATRIX*
        ) NOTIMPL_METHOD
#pragma warning (pop)

    STDMETHOD(SetViewport)(
        CONST D3DVIEWPORT9* pViewport
        ) NOTIMPL_METHOD

    STDMETHOD(GetViewport)(
        D3DVIEWPORT9* pViewport
        ) NOTIMPL_METHOD

    STDMETHOD(SetMaterial)(
        CONST D3DMATERIAL9* pMaterial
        ) NOTIMPL_METHOD

    STDMETHOD(GetMaterial)(
        D3DMATERIAL9* pMaterial
        ) NOTIMPL_METHOD

    STDMETHOD(SetLight)(
        DWORD Index,
        CONST D3DLIGHT9*
        ) NOTIMPL_METHOD

    STDMETHOD(GetLight)(
        DWORD Index,
        D3DLIGHT9*
        ) NOTIMPL_METHOD

    STDMETHOD(LightEnable)(
        DWORD Index,
        BOOL Enable
        ) NOTIMPL_METHOD

    STDMETHOD(GetLightEnable)(
        DWORD Index,
        BOOL* pEnable
        ) NOTIMPL_METHOD

    STDMETHOD(SetClipPlane)(
        DWORD Index,
        CONST float* pPlane
        ) NOTIMPL_METHOD

    STDMETHOD(GetClipPlane)(
        DWORD Index,
        float* pPlane
        ) NOTIMPL_METHOD

    STDMETHOD(SetRenderState)(
        D3DRENDERSTATETYPE State,
        DWORD Value
        ) NOTIMPL_METHOD

    STDMETHOD(GetRenderState)(
        D3DRENDERSTATETYPE State,
        DWORD* pValue
        ) NOTIMPL_METHOD

    STDMETHOD(CreateStateBlock)(
        D3DSTATEBLOCKTYPE Type,
        IDirect3DStateBlock9** ppSB
        ) NOTIMPL_METHOD

    STDMETHOD(BeginStateBlock)(
        ) NOTIMPL_METHOD

    STDMETHOD(EndStateBlock)(
        IDirect3DStateBlock9** ppSB
        ) NOTIMPL_METHOD

    STDMETHOD(SetClipStatus)(
        CONST D3DCLIPSTATUS9* pClipStatus
        ) NOTIMPL_METHOD

    STDMETHOD(GetClipStatus)(
        D3DCLIPSTATUS9* pClipStatus
        ) NOTIMPL_METHOD

    STDMETHOD(GetTexture)(
        DWORD Stage,
        IDirect3DBaseTexture9** ppTexture
        ) NOTIMPL_METHOD

    STDMETHOD(SetTexture)(
        DWORD Stage,
        IDirect3DBaseTexture9* pTexture
        ) NOTIMPL_METHOD

    STDMETHOD(GetTextureStageState)(
        DWORD Stage,
        D3DTEXTURESTAGESTATETYPE Type,
        DWORD* pValue
        ) NOTIMPL_METHOD

    STDMETHOD(SetTextureStageState)(
        DWORD Stage,
        D3DTEXTURESTAGESTATETYPE Type,
        DWORD Value
        ) NOTIMPL_METHOD

    STDMETHOD(GetSamplerState)(
        DWORD Sampler,
        D3DSAMPLERSTATETYPE Type,
        DWORD* pValue
        ) NOTIMPL_METHOD

    STDMETHOD(SetSamplerState)(
        DWORD Sampler,
        D3DSAMPLERSTATETYPE Type,
        DWORD Value
        ) NOTIMPL_METHOD

    STDMETHOD(ValidateDevice)(
        DWORD* pNumPasses
        ) NOTIMPL_METHOD

    STDMETHOD(SetPaletteEntries)(
        UINT PaletteNumber,
        CONST PALETTEENTRY* pEntries
        ) NOTIMPL_METHOD

    STDMETHOD(GetPaletteEntries)(
        UINT PaletteNumber,
        PALETTEENTRY* pEntries
        ) NOTIMPL_METHOD

    STDMETHOD(SetCurrentTexturePalette)(
        UINT PaletteNumber
        ) NOTIMPL_METHOD

    STDMETHOD(GetCurrentTexturePalette)(
        UINT *PaletteNumber
        ) NOTIMPL_METHOD

    STDMETHOD(SetScissorRect)(
        CONST RECT* pRect
        ) NOTIMPL_METHOD

    STDMETHOD(GetScissorRect)(
        RECT* pRect
        ) NOTIMPL_METHOD

    STDMETHOD(SetSoftwareVertexProcessing)(
        BOOL bSoftware
        ) NOTIMPL_METHOD

    STDMETHOD_(BOOL, GetSoftwareVertexProcessing)(
        ) { Assert(FALSE); return FALSE; }

    STDMETHOD(SetNPatchMode)(
        float nSegments
        ) NOTIMPL_METHOD

    STDMETHOD_(float, GetNPatchMode)(
        ) { Assert(FALSE); return 0.0f; }

    STDMETHOD(DrawPrimitive)(
        D3DPRIMITIVETYPE PrimitiveType,
        UINT StartVertex,
        UINT PrimitiveCount
        ) NOTIMPL_METHOD

    STDMETHOD(DrawIndexedPrimitive)(
        D3DPRIMITIVETYPE,
        INT BaseVertexIndex,
        UINT MinVertexIndex,
        UINT NumVertices,
        UINT startIndex,
        UINT primCount
        ) NOTIMPL_METHOD

    STDMETHOD(DrawPrimitiveUP)(
        D3DPRIMITIVETYPE PrimitiveType,
        UINT PrimitiveCount,
        CONST void* pVertexStreamZeroData,
        UINT VertexStreamZeroStride
        ) NOTIMPL_METHOD

    STDMETHOD(DrawIndexedPrimitiveUP)(
        D3DPRIMITIVETYPE PrimitiveType,
        UINT MinVertexIndex,
        UINT NumVertices,
        UINT PrimitiveCount,
        CONST void* pIndexData,
        D3DFORMAT IndexDataFormat,
        CONST void* pVertexStreamZeroData,
        UINT VertexStreamZeroStride
        ) NOTIMPL_METHOD

    STDMETHOD(ProcessVertices)(
        UINT SrcStartIndex,
        UINT DestIndex,
        UINT VertexCount,
        D3DVertexBuffer* pDestBuffer,
        IDirect3DVertexDeclaration9* pVertexDecl,
        DWORD Flags
        ) NOTIMPL_METHOD

    STDMETHOD(CreateVertexDeclaration)(
        CONST D3DVERTEXELEMENT9* pVertexElements,
        IDirect3DVertexDeclaration9** ppDecl
        ) NOTIMPL_METHOD

    STDMETHOD(SetVertexDeclaration)(
        IDirect3DVertexDeclaration9* pDecl
        ) NOTIMPL_METHOD

    STDMETHOD(GetVertexDeclaration)(
        IDirect3DVertexDeclaration9** ppDecl
        ) NOTIMPL_METHOD

    STDMETHOD(SetFVF)(
        DWORD FVF
        ) NOTIMPL_METHOD

    STDMETHOD(GetFVF)(
        DWORD* pFVF
        ) NOTIMPL_METHOD

    STDMETHOD(CreateVertexShader)(
        CONST DWORD* pFunction,
        IDirect3DVertexShader9** ppShader
        ) NOTIMPL_METHOD

    STDMETHOD(SetVertexShader)(
        IDirect3DVertexShader9* pShader
        ) NOTIMPL_METHOD

    STDMETHOD(GetVertexShader)(
        IDirect3DVertexShader9** ppShader
        ) NOTIMPL_METHOD

    STDMETHOD(SetVertexShaderConstantF)(
        UINT StartRegister,
        CONST float* pConstantData,
        UINT Vector4fCount
        ) NOTIMPL_METHOD

    STDMETHOD(GetVertexShaderConstantF)(
        UINT StartRegister,
        float* pConstantData,
        UINT Vector4fCount
        ) NOTIMPL_METHOD

    STDMETHOD(SetVertexShaderConstantI)(
        UINT StartRegister,
        CONST int* pConstantData,
        UINT Vector4iCount
        ) NOTIMPL_METHOD

    STDMETHOD(GetVertexShaderConstantI)(
        UINT StartRegister,
        int* pConstantData,
        UINT Vector4iCount
        ) NOTIMPL_METHOD

    STDMETHOD(SetVertexShaderConstantB)(
        UINT StartRegister,
        CONST BOOL* pConstantData,
        UINT BoolCount
        ) NOTIMPL_METHOD

    STDMETHOD(GetVertexShaderConstantB)(
        UINT StartRegister,
        BOOL* pConstantData,
        UINT BoolCount
        ) NOTIMPL_METHOD

    STDMETHOD(SetStreamSource)(
        UINT StreamNumber,
        D3DVertexBuffer* pStreamData,
        UINT OffsetInBytes,
        UINT Stride
        ) NOTIMPL_METHOD

    STDMETHOD(GetStreamSource)(
        UINT StreamNumber,
        D3DVertexBuffer** ppStreamData,
        UINT* OffsetInBytes,
        UINT* pStride
        ) NOTIMPL_METHOD

    STDMETHOD(SetStreamSourceFreq)(
        UINT StreamNumber,
        UINT Divider
        ) NOTIMPL_METHOD

    STDMETHOD(GetStreamSourceFreq)(
        UINT StreamNumber,
        UINT* Divider
        ) NOTIMPL_METHOD

    STDMETHOD(SetIndices)(
        D3DIndexBuffer* pIndexData
        ) NOTIMPL_METHOD

    STDMETHOD(GetIndices)(
        D3DIndexBuffer** ppIndexData
        ) NOTIMPL_METHOD

    STDMETHOD(CreatePixelShader)(
        CONST DWORD* pFunction,
        IDirect3DPixelShader9** ppShader
        ) NOTIMPL_METHOD

    STDMETHOD(SetPixelShader)(
        IDirect3DPixelShader9* pShader
        ) NOTIMPL_METHOD

    STDMETHOD(GetPixelShader)(
        IDirect3DPixelShader9** ppShader
        ) NOTIMPL_METHOD

    STDMETHOD(SetPixelShaderConstantF)(
        UINT StartRegister,
        CONST float* pConstantData,
        UINT Vector4fCount
        ) NOTIMPL_METHOD

    STDMETHOD(GetPixelShaderConstantF)(
        UINT StartRegister,
        float* pConstantData,
        UINT Vector4fCount
        ) NOTIMPL_METHOD

    STDMETHOD(SetPixelShaderConstantI)(
        UINT StartRegister,
        CONST int* pConstantData,
        UINT Vector4iCount
        ) NOTIMPL_METHOD

    STDMETHOD(GetPixelShaderConstantI)(
        UINT StartRegister,
        int* pConstantData,
        UINT Vector4iCount
        ) NOTIMPL_METHOD

    STDMETHOD(SetPixelShaderConstantB)(
        UINT StartRegister,
        CONST BOOL* pConstantData,
        UINT BoolCount
        ) NOTIMPL_METHOD

    STDMETHOD(GetPixelShaderConstantB)(
        UINT StartRegister,
        BOOL* pConstantData,
        UINT BoolCount
        ) NOTIMPL_METHOD

    STDMETHOD(DrawRectPatch)(
        UINT Handle,
        CONST float* pNumSegs,
        CONST D3DRECTPATCH_INFO* pRectPatchInfo
        ) NOTIMPL_METHOD

    STDMETHOD(DrawTriPatch)(
        UINT Handle,
        CONST float* pNumSegs,
        CONST D3DTRIPATCH_INFO* pTriPatchInfo
        ) NOTIMPL_METHOD

    STDMETHOD(DeletePatch)(
        UINT Handle
        ) NOTIMPL_METHOD

    STDMETHOD(CreateQuery)(
        D3DQUERYTYPE Type,
        IDirect3DQuery9** ppQuery
        ) NOTIMPL_METHOD

    STDMETHOD(SetConvolutionMonoKernel)(
        UINT width,
        UINT height,
        float* rows,
        float* columns
        ) NOTIMPL_METHOD

    STDMETHOD(ComposeRects)(
        D3DSurface* pSrc,
        D3DSurface* pDst,
        D3DVertexBuffer* pSrcRectDescs,
        UINT NumRects,
        D3DVertexBuffer* pDstRectDescs,
        D3DCOMPOSERECTSOP Operation,
        int Xoffset,
        int Yoffset
        ) NOTIMPL_METHOD

    STDMETHOD(PresentEx)(
        CONST RECT* pSourceRect,
        CONST RECT* pDestRect,
        HWND hDestWindowOverride,
        CONST RGNDATA* pDirtyRegion,
        DWORD dwFlags
        ) NOTIMPL_METHOD

    //
    // IDirect3DVideoDevice9 Methods
    //

    STDMETHOD( PresentEx ) (   CONST RECT *pSourceRect,
                            CONST RECT *pDestRect,
                            HWND hTargetWindow,
                            CONST RGNDATA *pDestinationRegion,
                            DWORD dwFlags,
                            D3DSurface * pSourceSurfaceOverride) NOTIMPL_METHOD

    STDMETHOD( GetGPUThreadPriority     )
        (INT *pPriority) NOTIMPL_METHOD

    STDMETHOD( SetGPUThreadPriority     )
        (INT Priority) NOTIMPL_METHOD

    STDMETHOD( WaitForVBlank            )
        (UINT SwapChain) NOTIMPL_METHOD

    STDMETHOD( CheckDeviceState )
        (HWND hWindow ) NOTIMPL_METHOD

    STDMETHODIMP CreateRenderTargetEx(UINT cpWidth,
                                        UINT cpHeight,
                                        D3DFORMAT Format, 
                                        D3DMULTISAMPLE_TYPE MultiSample, 
                                        DWORD MultiSampleQuality, 
                                        BOOL Lockable, 
                                        D3DSurface **ppSurface, 
                                        HANDLE* pSharedHandle, 
                                        DWORD Usage) NOTIMPL_METHOD

    STDMETHODIMP CreateOffscreenPlainSurfaceEx(UINT Width, 
                                                UINT Height, 
                                                D3DFORMAT Format, 
                                                D3DPOOL Pool, 
                                                D3DSurface** ppSurface, 
                                                HANDLE* pSharedHandle, 
                                                DWORD Usage) NOTIMPL_METHOD

    STDMETHODIMP CreateDepthStencilSurfaceEx(UINT cpWidth,
                                                UINT cpHeight,
                                                D3DFORMAT Format, 
                                                D3DMULTISAMPLE_TYPE MultiSample, 
                                                DWORD MultiSampleQuality, 
                                                BOOL Discardable, 
                                                D3DSurface **ppSurface, 
                                                HANDLE* pSharedHandle, 
                                                DWORD Usage) NOTIMPL_METHOD

    STDMETHOD(CreateSurface)(
        THIS_ UINT Width,
        UINT Height,
        UINT BackBuffers,
        D3DFORMAT Format,
        D3DPOOL Pool,
        DWORD Usage,
        D3DSurface** ppSurface,
        HANDLE* pSharedHandle
        );

    STDMETHOD( SetMaximumFrameLatency   )
        (UINT MaxLatency) NOTIMPL_METHOD

    STDMETHOD( GetMaximumFrameLatency   )
        (UINT * pMaxLatency) NOTIMPL_METHOD

    STDMETHOD(GetDXVACompressedBufferInfo)(
        THIS_ GUID* pGuid,
        DXVAUncompDataInfo* pUncompData,
        DWORD* pNumBuffers,
        DXVACompBufferInfo* pBufferInfo
        ) NOTIMPL_METHOD;

    STDMETHOD(GetDXVAGuids)(
        THIS_ DWORD* pNumGuids,
        GUID* pGuids
        );

    STDMETHOD(GetDXVAInternalInfo)(
        THIS_ GUID* pGuid,
        DXVAUncompDataInfo* pUncompData,
        DWORD* pMemoryUsed
        ) NOTIMPL_METHOD;

    STDMETHOD(GetUncompressedDXVAFormats)(
        THIS_ GUID* pGuid,
        DWORD* pNumFormats,
        D3DFORMAT* pFormats
        ) NOTIMPL_METHOD;

    STDMETHOD(CreateDXVADevice)(
        THIS_ GUID* pGuid,
        DXVAUncompDataInfo* pUncompData,
        LPVOID pData,
        DWORD DataSize,
        IDirect3DDXVADevice9** ppDXVADevice
        );




    STDMETHOD(GetInternalDevice)(
        __deref_out_ecount(1) D3DDeviceContext **ppD3DDevice
        );

private:
    long m_ulRef;
    D3DDeviceContext *m_pD3DDevice;
    CD3DWrapper *m_pD3DWrapper;
    UINT m_uiID;
};


