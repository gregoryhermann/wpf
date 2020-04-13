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
//      Contains CD3DDeviceLevel1 implementation
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

class CD3DSwapChain;
class CD3DDeviceVertexBuffer;
class CD3DDeviceIndexBuffer;
interface IMediaDeviceConsumer;

MtExtern(CD3DDeviceLevel1);

#include "uce\global.h"

#define NUM_PRESENTS_BEFORE_GPU_MARKER_FLUSH 3

#define GPUMARKER_FLAGS_MARKERS_ENABLED             0x1
#define GPUMARKER_FLAGS_MARKERS_TESTED              0x10
#define GPUMARKER_FLAGS_MARKER_CONSUMED             0x100

//+-----------------------------------------------------------------------------
//
//  Defines:
//      Device creation failure tracking
//
//------------------------------------------------------------------------------

#if DBG==1
#define TRACE_DEVICECREATE_FAILURE(adapter, msg,  hr) \
    CD3DDeviceLevel1::DbgTraceDeviceCreationFailure(adapter, msg, hr);
#else
#define TRACE_DEVICECREATE_FAILURE(adapter, msg,  hr)
#endif

// Forward declaration for class used for monitor GPU progress
class CGPUMarker;
//+-----------------------------------------------------------------------------
//
//  Class:
//      CD3DDeviceLevel1
//
//  Synopsis:
//      Abstracts the core D3D device to provide the following functionality:
//          0. Assert on invalid input
//          1. Restrict access to methods of D3DDeviceContext to those
//             available on level 1 graphics cards. (Level1 is the base support
//             we require to hw accelerate)
//          2. Provide correct information for GetDeviceCaps
//          3. Centralize resource creation so that it can be tracked.
//             Tracking created resources is important for responding to mode
//             changes
//          4. Respond to mode changes on present call
//          5. Provide testing functionality for determining if a graphics
//             card meets the level1 criteria for hw acceleration
//
//      Note that CD3DDeviceLevel1 is base class for more capable hw devices.
//
//------------------------------------------------------------------------------

class CD3DDeviceLevel1
      // costly shared data is stored here since some of this data has
      // device affinity.  It also holds the cache index, which needs
      // to be released after all resources and therefore should be the
      // last to be destroyed and making it the first base class.
    : public CHwSurfaceRenderTargetSharedData,
      public CD3DRenderState,
      public CMILPoolResource
{
private:

    struct TargetFormatTestStatus {
        HRESULT hrTest;
        HRESULT hrTestGetDC;
    };

public:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CD3DDeviceLevel1));

    static HRESULT Create(
        __inout_ecount(1) D3DDevice* pID3DDevice,
        __inout_ecount(1) D3DDeviceContext *pID3DDeviceContext,
        __in_ecount(1) const CDisplay *pPrimaryDisplay,
        __in_ecount(1) IMILPoolManager *pManager,
        DWORD dwBehaviorFlags,
        __deref_out_ecount(1) CD3DDeviceLevel1 ** const ppDevice
        );

    virtual ~CD3DDeviceLevel1();

    //+-------------------------------------------------------------------------
    //
    //  Members:
    //      Enter and Leave
    //
    //  Synopsis:
    //      Thread protection marking methods
    //
    //      Call Enter when device is about to used in a way that requires
    //      exclusive access and Leave when leaving that context.  This is most
    //      commonly done when handling a drawing routine, which the caller is
    //      required to provide protection for.
    //
    //--------------------------------------------------------------------------

    void Enter();
    void Leave();

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      IsProtected
    //
    //  Synopsis:
    //      Return true if this thread is protected
    //
    //--------------------------------------------------------------------------

    bool IsProtected(
        bool fForceEntryConfirmation    // Ignore lack of multithreaded usage
                                        // flag when checking for protection.
                                        // This is to be used by internal
                                        // callers that may violate caller
                                        // provided protection.
        ) const;


    D3DDevice* GetDevice() { return m_pD3DDevice; }
    D3DDeviceContext* GetDeviceContext() { return m_pD3DDeviceContext; }

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      IsEntered
    //
    //  Synopsis:
    //      Return true if this thread has been marked/entered as protected
    //
    //--------------------------------------------------------------------------

    bool IsEntered() const;

    bool IsEnsuringCorrectMultithreadedRendering() const
        { return m_csDeviceEntry.IsValid(); }

    // TODO, return the right answer
    bool IsSWDevice() const { return false; }
    bool IsHWDevice() const { return !IsSWDevice(); }
    bool Is128BitFPTextureSupported() const { return true; }
    bool SupportsBorderColor() const { return true; }
    bool CanMaskColorChannels() const { return true; }
    bool CanHandleBlendFactor() const { return true; }
    UINT GetMaxTextureWidth() const { return D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION; }
    UINT GetMaxTextureHeight() const { return D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION; }
    bool SupportsNonPow2Unconditionally() const { return true; }
    bool SupportsNonPow2Conditionally() const { return false; }
    //+-------------------------------------------------------------------------
    //
    //  Macro:
    //      ENTER_DEVICE_FOR_SCOPE
    //
    //  Synopsis:
    //      Provide simple mechanism to mark device as protected for the
    //      duration of scope
    //
    //--------------------------------------------------------------------------

    #define ENTER_DEVICE_FOR_SCOPE(Device) \
        CGuard<CD3DDeviceLevel1> oDeviceEntry(Device)

    //+-------------------------------------------------------------------------
    //
    //  Macro:
    //      ENTER_USE_CONTEXT_FOR_SCOPE
    //
    //  Synopsis:
    //      Provide simple mechanism to enter/exit a use context in the scope
    //
    //--------------------------------------------------------------------------

    #define ENTER_USE_CONTEXT_FOR_SCOPE(Device) \
        CD3DUseContextGuard oUseContext(Device)


    //+-------------------------------------------------------------------------
    //
    //  Macro:
    //      AssertDeviceEntry
    //
    //  Synopsis:
    //      Provide simple mechanism to check device protection is taken for the
    //      duration of scope
    //
    //--------------------------------------------------------------------------

    #define AssertDeviceEntry(Device) \
        AssertEntry((Device).m_oDbgEntryCheck); \
        Assert((Device).IsProtected(false /* Do not force entry check */))

    //+-------------------------------------------------------------------------
    //
    //  Macro:
    //      AssertNoDeviceEntry
    //
    //  Synopsis:
    //      Check device protection is not been taken at this time nor for any
    //      other thread for the duration of scope; however this thread may
    //      enter the device protection from this scope
    //
    //--------------------------------------------------------------------------

    #define AssertNoDeviceEntry(Device) \
        AssertEntry((Device).m_oDbgEntryCheck); \
        Assert(!((Device).IsEntered()))


    void AssertRenderFormatIsTestedSuccessfully(
        DXGI_FORMAT fmtRenderTarget
        );

    HRESULT Set3DTransforms(
        __in_ecount(1) CMILMatrix const *pWorldTransform3D,
        __in_ecount(1) CMILMatrix const *pViewTransform3D,
        __in_ecount(1) CMILMatrix const *pProjectionTransform3D,
        __in_ecount(1) CMatrix<CoordinateSpace::Projection3D,CoordinateSpace::Device> const &matHomogeneousTo2DDevice
        );

    HRESULT SetRenderTarget(__in_ecount(1) CD3DTexture *pD3DTexture);

    void ReleaseUseOfRenderTarget(
        __in_ecount(1) const CD3DTexture *pD3DRenderTarget
        );

    void MarkUnusable(
        bool fMayBeMultithreadedCall
        );

    HRESULT CreateRenderTarget(
        UINT uiWidth,
        UINT uiHeight,
        DXGI_FORMAT dxgiFmt,
        UINT uiMultisampleCount,
        UINT uiMultisampleQuality,
        __deref_out_ecount(1) CD3DTexture ** const ppD3DTexture
        );

    HRESULT CreateRenderTargetUntracked(
        UINT uiWidth,
        UINT uiHeight,
        DXGI_FORMAT dxgiFmt,
        UINT uiMultisampleCount,
        UINT uiMultisampleQuality,
        __deref_out_ecount(1) D3DTexture ** const ppD3DTexture
        );

    HRESULT CreateVertexBuffer(
        UINT Length,
        D3D11_USAGE Usage,
        UINT uiCpuAccessFlags,
        __deref_out_ecount(1) D3DVertexBuffer ** const ppVertexBuffer
        );

    HRESULT CreateIndexBuffer(
        UINT Length,
        D3D11_USAGE Usage,
        UINT uiCpuAccessFlags,
        __deref_out_ecount(1) D3DIndexBuffer** const ppIndexBuffer
    );

    HRESULT CreateTexture(
        __in_ecount(1) const D3D11_TEXTURE2D_DESC* pTextureDesc,
        UINT uLevels,
        __deref_out_ecount(1) D3DTexture ** const ppD3DTexture
        );

    HRESULT CreatePixelShaderFromResource(
        UINT uResourceId,
        __deref_out_ecount(1) ID3D11PixelShader** const ppPixelShader
        );

    HRESULT CreateVertexShaderFromResource(
        D3DVertexType vertexType,
        UINT uResourceId,
        __deref_out_ecount(1) ID3D11VertexShader** const ppVertexShader
        );
        
    HRESULT CompilePipelineVertexShader(
        D3DVertexType vertexType,
        __in_bcount(cbHLSLSource) PCSTR pHLSLSource,
        UINT cbHLSLSource,
        __deref_out_ecount(1) ID3D11VertexShader** const ppVertexShader
        );

    HRESULT CompilePipelinePixelShader(
        __in_bcount(cbHLSLSource) PCSTR pHLSLSource,
        UINT cbHLSLSource,
        __deref_out_ecount(1) ID3D11PixelShader** const ppPixelShader
        );

    HRESULT CreateVertexShader(
        D3DVertexType vertexType,
        __in const DWORD *pdwfnVertexShader,
        __in DWORD cbVertexShader,
        __deref_out_ecount(1) ID3D11VertexShader** const ppOutShader
        );

    HRESULT CreatePixelShader(
        __in const DWORD *pdwfnPixelShader,
        __in DWORD cbPixelShader,
        __deref_out_ecount(1) ID3D11PixelShader**const ppOutShader
        );

    HRESULT CreateLockableTexture(
        __in_ecount(1) const D3D11_TEXTURE2D_DESC*pSurfDesc,
        __deref_out_ecount(1) CD3DLockableTexture ** const ppLockableTexture
        );

    HRESULT CreateSysMemUpdateTexture(
        UINT uWidth,
        UINT uHeight,
        DXGI_FORMAT fmtTexture,
        __in_xcount_opt(uWidth * uHeight * D3DFormatSize(fmtTexture)) void *pvPixels,
        __deref_out_ecount(1) D3DTexture ** const ppD3DSysMemTexture
        );

    HRESULT UpdateTextureRegion(
        __in_ecount(1) D3DTexture * pD3DSysMemSrcTexture,
        UINT srcSubresource,
        __in_ecount_opt(1) const RECT *pSourceRect,
        __inout_ecount(1) D3DTexture * pD3DPoolDefaultDestTexture,
        UINT destSubresource,
        __in_ecount_opt(1) const POINT *pDestPoint
        );

    HRESULT UpdateTexture(
        __in_ecount(1) D3DTexture *pD3DSysMemSrcTexture,
        __inout_ecount(1) D3DTexture *pD3DPoolDefaultDestTexture
        );

    HRESULT CreateDepthBuffer(
        UINT uWidth,
        UINT uHeight,
        UINT uiMultisampleCount,
        UINT uiMultisampleQuality,
        __deref_out_ecount(1) CD3DTexture ** const ppTexture
        );

    void CleanupFreedResources();

    HRESULT Present(
        __in_ecount(1) CD3DSwapChain const *pD3DSwapChain,
        __in_ecount_opt(1) CMILSurfaceRect const *prcSource,
        __in_ecount_opt(1) CMILSurfaceRect const *prcDest,
        __in_ecount(1) CMILDeviceContext const *pMILDC,
        __in_ecount_opt(1) RGNDATA const * pDirtyRegion,
        DWORD dwD3DPresentFlags
        );

    HRESULT SetTexture(
        UINT uiSlot,
        __inout_ecount_opt(1) CD3DTexture *pD3DTexture
        );

    HRESULT SetDepthStencilTexture(
        __in_ecount_opt(1) CD3DTexture *pTexture
        );

    void ReleaseUseOfDepthStencilTexture(
        __in_ecount_opt(1) CD3DTexture *pTexture
        );

    HRESULT ClearColor(
        DWORD dwCount,
        __in_ecount_opt(dwCount) const D3D11_RECT* pRects,
        DWORD colorRGBA
        );

     HRESULT ClearDepthStencil(
         FLOAT rZValue,
         int iStencilValue
         );

    HRESULT ColorFill(
        __inout_ecount(1) CD3DTexture *pTexture,
        __in_ecount_opt(1) const RECT *pRect,
        DWORD colorRGBA
        );

    CD3DResourceManager *GetResourceManager()
    {
        return &m_resourceManager;
    }

    HRESULT CopyD3DTexture(
        __in_ecount(1) D3DTexture *pD3DSourceTexture,
        __inout_ecount(1) D3DTexture *pD3DDestinationTexture
        );

    HRESULT GetSupportedTextureFormat(
        MilPixelFormat::Enum fmtBitmapSource,          // Current format of bitmap
        MilPixelFormat::Enum fmtDestinationSurface,    // Format of surface onto
                                                 // which the bitmap will be
                                                 // drawn
        bool fForceAlpha,
        __out_ecount(1) MilPixelFormat::Enum &fmtTextureSource    // Format of the
                                                            // texture to hold
                                                            // the bitmap
        ) const;

    DXGI_SAMPLE_DESC GetSupportedMultisampleType(
        MilPixelFormat::Enum fmtDestinationSurface        // Format of target surface
        ) const;

    HRESULT SetClipRect(
        __in_ecount_opt(1) const CMILSurfaceRect *prcClip
        );

    void GetClipRect(
        __out_ecount(1) MilPointAndSizeL * const prcClipRect
        ) const;

    // Our Adapter LUIDs come from D3D when an extended device is avaible
    // (WDDM). On XP we generate them ourselves, so this method is safe to use
    // for device identity regardless of driver model. However, in XPDM, these
    // auto-generated LUIDs will be different for every display set created.
    LUID GetD3DAdapterLUID() const { return m_luidD3DAdapter; }

    //
    // device capability accessors
    //

    TierType GetTier() const { return m_Tier; }
    bool ShouldAttemptMultisample() const 
    {
        return true;
    }

    __range(1,UINT_MAX) DWORD GetMaxDesiredAnisotropicFilterLevel() const 
    {
        return 4;
    }

    __out_ecount(1) CHwD3DVertexBuffer *Get3DVertexBuffer()
        { return m_pHwVertexBuffer; }

    __out_ecount(1) CHwD3DIndexBuffer *Get3DIndexBuffer()
        { return m_pHwIndexBuffer; }

    BOOL SupportsScissorRect() const {return TRUE;}

    CD3DGlyphBank* GetGlyphBank()
    {
        return &m_glyphBank;
    }

    HRESULT StartPrimitive(CD3DVertexBufferDUV2 **ppOutBuffer)
    {
        *ppOutBuffer = &m_vbBufferDUV2;

        m_vbBufferDUV2.Clear();

        return SetInputLayoutFormat(CD3DVertexXYZDUV2::Format());
    }

    HRESULT StartPrimitive(CD3DVertexBufferDUV6 **ppOutBuffer)
    {
        m_vbBufferDUV6.Clear();

        *ppOutBuffer = &m_vbBufferDUV6;
        return SetInputLayoutFormat(CD3DVertexXYZDUV6::Format());
    }

    HRESULT StartPrimitive(CD3DVertexBufferXYZNDSUV4 **ppOutBuffer)
    {
        m_vbBufferXYZNDSUV4.Clear();

        *ppOutBuffer = &m_vbBufferXYZNDSUV4;
        return SetInputLayoutFormat(CD3DVertexXYZNDSUV4::Format());
    }

    HRESULT EndPrimitiveFan(__in_ecount(1) CD3DVertexBuffer * pBuffer)
    {
        return FlushBufferFan(pBuffer);
    }

    CHwTVertexBuffer<CD3DVertexXYZDUV2> *GetVB_XYZDUV2()
    {
        return &m_vBufferXYZDUV2;
    }

    CHwTVertexBuffer<CD3DVertexXYZDUV8> *GetVB_XYZRHWDUV8()
    {
        return &m_vBufferXYZRHWDUV8;
    }

    HRESULT DrawIndexedTriangleList(
        UINT uBaseVertexIndex,
        UINT uMinIndex,
        UINT cVertices,
        UINT uStartIndex,
        UINT cPrimitives
        );

    HRESULT DrawTriangleList(
        UINT uStartVertex,
        UINT cPrimitives
        );

    HRESULT DrawTriangleStrip(
        UINT uStartVertex,
        UINT cPrimitives
        );

    HRESULT DrawIndexedTriangleListUP(
        __range(1, UINT_MAX) UINT uNumVertices,
        __range(1, UINT_MAX) UINT uPrimitiveCount,
        __in_xcount(sizeof(WORD) * uPrimitiveCount * 3) const WORD* pIndexData,
        __in const void* pVertexStreamZeroData,
        __range(1, UINT_MAX) UINT uVertexStreamZeroStride
        );

    HRESULT DrawPrimitiveUP(
        D3D11_PRIMITIVE_TOPOLOGY primitiveType,
        UINT uPrimitiveCount,
        __in_xcount(
            //
            // Vertex counts are:
            //
            // D3D_PRIMITIVE_TOPOLOGY_LINELIST - PrimitiveCount*2
            // D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP - PrimitiveCount+2
            //
            uVertexStreamZeroStride * ((primitiveType == D3D_PRIMITIVE_TOPOLOGY_LINELIST) ? uPrimitiveCount*2 : uPrimitiveCount+2)
            ) const void* pVertexStreamZeroData,
        UINT uVertexStreamZeroStride
        );

    HRESULT DrawVideoToSurface(
        __in_ecount(1) IAVSurfaceRenderer *pSurfaceRenderer,
        __deref_opt_out_ecount(1) IWGXBitmapSource ** const ppBitmapSource
        );

    HRESULT CheckDeviceState(
        __in_opt HWND hwnd
        );

    HRESULT WaitForVBlank(
        UINT uSwapChain
        );

    HRESULT GetNumQueuedPresents(
        __out_ecount(1) UINT *puNumQueuedPresents
        );

    // Advance the frame for the device.
    // IMPORTANT to call this with increasing frame numbers because
    // it handles releasing HW resources used in the last *TWO* frames.
    void AdvanceFrame(UINT uFrameNumber);

    void Use(__in_ecount(1) const CD3DResource &refResource)
    {
        m_resourceManager.Use(refResource);
    }

    UINT EnterUseContext()
    {
        return m_resourceManager.EnterUseContext();
    }

    void ExitUseContext(UINT uDepth)
    {
        m_resourceManager.ExitUseContext(uDepth);
    }

    bool IsInAUseContext() const
    {
        return m_resourceManager.IsInAUseContext();
    }

#if DBG==1
    static void DbgTraceDeviceCreationFailure(UINT uAdapter, __in PCSTR szMessage, HRESULT hrError);
#endif

private:
    CD3DDeviceLevel1(
        __in_ecount(1) IMILPoolManager *pManager,
        DWORD dwBehaviorFlags
        );

    HRESULT Init(
        __inout_ecount(1) D3DDevice *pID3DDevice,
        __inout_ecount(1) D3DDeviceContext *pID3DDeviceContext,
        __in_ecount(1) CDisplay const *pDisplay
        );

    HRESULT BeginScene();
    HRESULT EndScene();

    HRESULT SetSurfaceToClippingMatrix(
        __in_ecount(1) const MilPointAndSizeL *prcViewport
        );

    HRESULT FlushBufferFan(
        __in_ecount(1) CD3DVertexBuffer*
    );

    void UpdateMetrics(
        UINT uNumVertices,
        UINT uNumPrimitives);

    // Destroys all outstanding markers.  Called on device error
    void ResetMarkers();

    // Insert a marker into the rendering stream so
    // we can determine when the GPU has processed up
    // to this point
    // The marker ID must be greater than any previous
    // id used.  QPC timer values are expected to be used
    HRESULT InsertGPUMarker(
        ULONGLONG ullMarkerId
        );

    // Tests whether all marker with lower ids have been consumed
    HRESULT IsConsumedGPUMarker(
        UINT uMarkerIndex,
        bool fFlushMarkers,
        __out_ecount(1) bool *pfMarkerConsumed
        );

    HRESULT FreeMarkerAndItsPredecessors(UINT uIndex);

    MIL_FORCEINLINE HRESULT HandleDIE(HRESULT hr)
    {
        if (hr == D3DERR_DRIVERINTERNALERROR)
        {
            // Return WGXERR_DISPLAYSTATEINVALID to upstream callers.
            // Present will pick up the D3DERR_DRIVERINTERNALERROR and
            // tear down the device.
            m_hrDisplayInvalid = D3DERR_DRIVERINTERNALERROR;
            hr = WGXERR_DISPLAYSTATEINVALID;
        }

        return hr;
    }

    HRESULT PresentWithD3D(
        __inout_ecount(1) IDXGISwapChain *pSwapChain,
        __in_ecount_opt(1) CMILSurfaceRect const *prcSource,
        __in_ecount_opt(1) CMILSurfaceRect const *prcDest,
        __in_ecount(1) CMILDeviceContext const *pMILDC,
        __in_ecount_opt(1) RGNDATA const * pDirtyRegion,
        DWORD dwD3DPresentFlags,
        __out_ecount(1) bool *pfPresentProcessed
        );

    HRESULT HandlePresentFailure(
         __in_ecount(1) CMILDeviceContext const *pMILDC,
        HRESULT hr);


    HRESULT ConsumePresentMarkers(
        bool fForceFlush
        );

    void SetGPUMarkersAsEnabled()
    {
        m_dwGPUMarkerFlags |= GPUMARKER_FLAGS_MARKERS_ENABLED;
    }

    void DisableGPUMarkers()
    {
        m_dwGPUMarkerFlags &= (~GPUMARKER_FLAGS_MARKERS_ENABLED);
        ResetMarkers();
    }

    bool AreGPUMarkersEnabled() const
    {
        return (m_dwGPUMarkerFlags & GPUMARKER_FLAGS_MARKERS_ENABLED) != 0;
    }

    void SetGPUMarkersAsTested()
    {
        m_dwGPUMarkerFlags |= GPUMARKER_FLAGS_MARKERS_TESTED;
    }

    bool AreGPUMarkersTested() const
    {
        return (m_dwGPUMarkerFlags & GPUMARKER_FLAGS_MARKERS_TESTED) != 0;
    }

    void SetGPUMarkersAsConsumed()
    {
        m_dwGPUMarkerFlags |= GPUMARKER_FLAGS_MARKER_CONSUMED;
    }

    bool HaveGPUMarkersBeenConsumed() const
    {
        return (m_dwGPUMarkerFlags & GPUMARKER_FLAGS_MARKER_CONSUMED) != 0;
    }

private:
    UINT m_cEntry;
    DWORD m_dwThreadId;

    // D3D objects
    D3DDevice        *m_pD3DDevice;
    D3DDeviceContext *m_pD3DDeviceContext;

    // Active render target set on device
    CD3DTexture *m_pCurrentRenderTargetNoRef;
    // Hint for what depth buffer to release when releasing a RT from use.
    // It is meaningless unless m_pCurrentRenderTargetNoRef is not NULL.
    CD3DTexture *m_pDepthStencilBufferForCurrentRTNoRef;

    LUID m_luidD3DAdapter;
    TierType m_Tier;
    DWORD const m_dwD3DBehaviorFlags;

    bool m_fInScene;

    // HRESULT indicating whether the display is invalid
    HRESULT m_hrDisplayInvalid;

    // Current render target desc
    D3D11_TEXTURE2D_DESC m_desc;

    // Vertex Buffers
    CD3DVertexBufferDUV2 m_vbBufferDUV2;
    CD3DVertexBufferDUV6 m_vbBufferDUV6;
    CD3DVertexBufferXYZNDSUV4 m_vbBufferXYZNDSUV4;

    // Matrix applied to surface coordinates to translate to homogeneous clip
    // coordinates.  It is always based on the viewport; so, if the viewport
    // changes this probably needs updated.
    CMatrix<CoordinateSpace::DeviceHPC,CoordinateSpace::D3DHomogeneousClipIPC>
        m_matSurfaceToClip;


    CHwTVertexBuffer<CD3DVertexXYZDUV2>     m_vBufferXYZDUV2;
    CHwTVertexBuffer<CD3DVertexXYZDUV8>     m_vBufferXYZRHWDUV8;

    //
    // Custom VB/IB management
    //

    CHwD3DIndexBuffer *m_pHwIndexBuffer;
    CHwD3DVertexBuffer *m_pHwVertexBuffer;

    // Additional services for tracking resources and render state
    CD3DResourceManager m_resourceManager;

    //
    // Critical section in case we're using the RGBRast device.  We have to
    // protect it from being used by multiple threads simultaneously.
    //
    CCriticalSection m_csDeviceEntry;

    // Glyph rendering
    CD3DGlyphBank m_glyphBank;

    // Per frame metrics
    DWORD m_dwMetricsVerticesPerFrame;
    DWORD m_dwMetricsTrianglesPerFrame;

    // Last marker id specified
    ULONGLONG m_ullLastMarkerId;

    // last marker ID that was consumed
    ULONGLONG m_ullLastConsumedMarkerId;

    // Last frame number given to AdvanceFrame, when this changes
    // we free resources from last two frames as appropriate.
    UINT m_uFrameNumber;

    // Active and free marker lists
    DynArray<CGPUMarker *> m_rgpMarkerActive;
    DynArray<CGPUMarker *> m_rgpMarkerFree;

    UINT m_uNumSuccessfulPresentsSinceMarkerFlush;

    DWORD m_dwGPUMarkerFlags;

    UINT m_presentFailureWindowMessage;

#if DBG_STEP_RENDERING
public:
    void DbgBeginStepRenderingPresent() {
        Assert(!m_fDbgInStepRenderingPresent);
        m_fDbgInStepRenderingPresent = true;
    }
    void DbgEndStepRenderingPresent() {
        Assert(m_fDbgInStepRenderingPresent);
        m_fDbgInStepRenderingPresent = false;
    }
    bool DbgInStepRenderingPresent() const {
        return m_fDbgInStepRenderingPresent;
    }

    HRESULT DbgSaveSurface(
        __in_ecount(1) CD3DSurface *pD3DSurface,
        __in_ecount(1) const MilPointAndSizeL &rcSave
        );

    HRESULT DbgRestoreSurface(
        __inout_ecount(1) CD3DSurface *pD3DSurface,
        __in_ecount(1) const MilPointAndSizeL &rcRestore
        );

    BOOL DbgCanShrinkRectLinear() const {
        return (m_caps.StretchRectFilterCaps & D3DPTFILTERCAPS_MINFLINEAR);
    }

private:
    bool m_fDbgInStepRenderingPresent;
    CD3DSurface *m_pDbgSaveSurface;

#endif DBG_STEP_RENDERING

#if DBG
public:
    // per-frame debug info logging
    CD3DLog m_log;

    // Device thread protection checker
    mutable CAssertEntry    m_oDbgEntryCheck;

private:

    // Rendering stats
    CD3DStats m_d3dStats;
#endif DBG


public: 
    // Shader effect pipeline pereration.
    HRESULT PrepareShaderEffectPipeline();
    HRESULT SetPassThroughPixelShader();
    HRESULT SetRenderTargetForEffectPipeline(__in_ecount(1) CD3DSurface *pD3DSurface);
    HRESULT SetRenderTargetForEffectPipeline(__in_ecount(1) CD3DTexture* pD3DTexture);
private:

    ID3D11VertexShader* m_pEffectPipelineVertexShader; // Vertex shader for shader effects pipeline.
    D3DVertexBuffer* m_pEffectPipelineVertexBuffer; // Scratch buffer for the shader effects pipeline. 
    ID3D11PixelShader* m_pEffectPipelinePassThroughPixelShader; // Pass-through pixel shader for shader effects pipelines.
};

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::UpdateMetrics()
//
//  Synopsis:
//      Update metrics if tracking them
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE void
CD3DDeviceLevel1::UpdateMetrics(UINT uNumVertices, UINT uNumPrimitives)
{
    if (g_pMediaControl)
    {
        m_dwMetricsVerticesPerFrame += uNumVertices;
        m_dwMetricsTrianglesPerFrame += uNumPrimitives;
    }
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      DbgIsPixelZoomMode()
//
//  Synopsis:
//      Is pixel zoom mode enabled
//
//------------------------------------------------------------------------------

#if DBG
extern int c_dbgPixelZoomModeScale;
extern bool DbgIsPixelZoomMode();
#endif




