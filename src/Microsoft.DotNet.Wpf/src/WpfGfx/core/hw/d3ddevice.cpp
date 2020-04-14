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
//      Implementation of CD3DDeviceLevel1
//
//      Abstracts the core D3D device to provide the following functionality:
//         1. Restrict access to methods of D3DDeviceContext to those
//            available on level 1 graphics cards. (Level1 is the base support
//            we require to hw accelerate)
//         2. Provide correct information for GetDeviceCaps
//         3. Centralize resource creation so that it can be tracked.
//            Tracking created resources is important for responding to mode
//            changes
//         4. Respond to mode changes on present call
//         5. Provide testing functionality for determining if a graphics
//            card meets the level1 criteria for hw acceleration
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"
#include "HwGraphicsCards.h"

using namespace dxlayer;

//   What depth buffer should be used
//  since we no longer need the stencil.  Is 16 or 32 better than 24?

const DXGI_FORMAT kD3DDepthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

MtDefine(CD3DDeviceLevel1, MILRender, "CD3DDeviceLevel1");
MtDefine(NormalTraceVectors, MILRender, "Normal vectors drawn for testing");
MtDefine(MILHwMetrics, Metrics, "MIL HW");
MtDefine(VerticesPerFrame, MILHwMetrics, "Vertices per frame");
MtDefine(TrianglesPerFrame, MILHwMetrics, "Triangles per frame");

MtDefine(D3DAllocatedData, MILRender, "Data Allocated by D3D");

MtDefine(D3DRGBRasterizer,        D3DAllocatedData, "D3D RGB rasterizer");
MtDefine(D3DSwapChain,            D3DAllocatedData, "Data Allocated for a D3D swap chain");
MtDefine(D3DColorFill,              D3DAllocatedData, "Data Allocated for a D3D ColorFill");
MtDefine(D3DTexture,              D3DAllocatedData, "Data Allocated for a D3D Texture");
MtDefine(D3DDevice,               D3DAllocatedData, "Data Allocated for a D3D Device");
MtDefine(D3DDrawPrimitiveUP,      D3DAllocatedData, "Data Allocated for a D3D DrawPrimitiveUP Call");
MtDefine(D3DStencilSurface,       D3DAllocatedData, "Data Allocated for a D3D Stencil Surface");
MtDefine(D3DDrawIndexedPrimitive, D3DAllocatedData, "Data Allocated for a D3D DrawIndexedPrimitive call");
MtDefine(D3DDrawIndexedPrimitiveUP, D3DAllocatedData, "Data Allocated for a D3D DrawIndexedPrimitiveUP call");
MtDefine(D3DDrawPrimitive,        D3DAllocatedData, "Data Allocated for a D3D DrawPrimitive call");
MtDefine(D3DVertexBuffer,         D3DAllocatedData, "Data Allocated for a D3D Vertex Buffer");
MtDefine(D3DIndexBuffer,          D3DAllocatedData, "Data Allocated for a D3D Index Buffer");
MtDefine(matrix_t_get_scaling,    D3DAllocatedData, "Data Allocated for Matrix Scaling");
MtDefine(D3DRenderTarget,         D3DAllocatedData, "Data Allocated for a D3D RenderTarget");
MtDefine(D3DStateBlock,           D3DAllocatedData, "Data Allocated for a D3D StateBlock");

MtDefine(D3DCompiler,             D3DAllocatedData, "Data Allocated for a compiled shader");

DeclareTag(tagD3DStats, "MIL-HW", "Output d3d stats");
DeclareTag(tagPixelZoomMode, "MIL-HW", "Pixel zoom mode");
DeclareTag(tagLowPrimitiveCount, "MIL-HW", "Lower primitive count limit");
DeclareTag(tagInjectDIE, "MIL-HW", "Inject D3DERR_DRIVERINTERNALERROR failures");

void DbgInjectDIE(__inout_ecount(1) HRESULT * const pHR);

#define GPU_MARKERS_MAX_ARRAY_SIZE 35

#if !DBG
MIL_FORCEINLINE void DbgInjectDIE(__inout_ecount(1) HRESULT *pHR)
{
}
#endif

extern HINSTANCE g_DllInstance;
extern volatile DWORD g_dwTextureUpdatesPerFrame;
LONG g_lPixelsFilledPerFrame = 0;

//+-----------------------------------------------------------------------------
//
//  D3DDevice allocation macros
//
//  Synopsis:
//      Put these around any D3D interface call that can result in
//      D3DERR_OUTOFVIDEOMEMORY. These macros assume that "hr" contains the
//      HRRESULT from the device call.
//
//      For example:
//
//           BEGIN_DEVICE_ALLOCATION
//
//           MIL_THR(m_pD3DDevice->CreateTexture(...));
//
//           END_DEVICE_ALLOCATION
//
//------------------------------------------------------------------------------

#define BEGIN_DEVICE_ALLOCATION \
    do \
    { \

#define END_DEVICE_ALLOCATION \
    } while (m_resourceManager.FreeSomeVideoMemory((hr))); \

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
// Pick an odd number for the zoom factor so that it is clear what
// side of the center of the pixel an edge lies on.
int c_dbgPixelZoomModeScale = 11;

// Top left corner zoom mode
POINT g_dbgMousePosition = {0, 0};

// HWND from last present
HWND g_dbgHWND;

bool
DbgIsPixelZoomMode()
{
    wpf::util::DpiAwarenessScope<HWND> dpiScope(g_dbgHWND);

    if (IsTagEnabled(tagPixelZoomMode))
    {
        short keyState = GetKeyState(MK_RBUTTON);

        // The high order bit of keyState indicates that the button is down, so
        // check it here.

        if (keyState & 8000)
        {
            //
            // Mouse button is down, so capture the mouse position and return false.
            //
            // Note that this code assumes that everything succeeds which is ok for
            // our special trace tag
            //

            GetCursorPos(&g_dbgMousePosition);
            ScreenToClient(g_dbgHWND, &g_dbgMousePosition);
            return false;
        }
        else
        {
            return true;
        }
    }

    return false;
}
#endif


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::Create
//
//  Synopsis:
//      Create the d3ddevice and test for level1
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::Create(
    __inout_ecount(1) D3DDevice *pID3DDevice,
    __inout_ecount(1) D3DDeviceContext *pID3DDeviceContext,
    __in_ecount(1) const CDisplay *pPrimaryDisplay,
    __in_ecount(1) IMILPoolManager *pManager,
    DWORD dwBehaviorFlags,
    __deref_out_ecount(1) CD3DDeviceLevel1 ** const ppDevice
    )
{
    HRESULT hr = S_OK;

    *ppDevice = NULL;

    //
    // Create CD3DDeviceLevel1
    //

    *ppDevice = new CD3DDeviceLevel1(pManager, dwBehaviorFlags);
    IFCOOM(*ppDevice);

    //
    // Call init
    //

    IFC((*ppDevice)->Init(pID3DDevice, pID3DDeviceContext, pPrimaryDisplay));

Cleanup:
    if (FAILED(hr))
    {
        // We need to delete here since we've partially initialized the device
        // and the Release call relies on the pool manager being hooked up.
        delete (*ppDevice);
        *ppDevice = NULL;
    }
    else
    {
        (*ppDevice)->AddRef(); // CD3DDeviceLevel1::ctor sets ref count == 0
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CD3DDeviceLevel1
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CD3DDeviceLevel1::CD3DDeviceLevel1(
    __in_ecount(1) IMILPoolManager *pManager,
    DWORD dwBehaviorFlags
    ) : CMILPoolResource(pManager),
        m_Tier(MIL_TIER(0,0)),
        m_dwD3DBehaviorFlags(dwBehaviorFlags),
        m_uFrameNumber(0),
        m_ullLastMarkerId(0),
        m_ullLastConsumedMarkerId(0),
        m_uNumSuccessfulPresentsSinceMarkerFlush(0)
{
    m_cEntry = 0;
    m_dwThreadId = 0;

    m_pD3DDevice = NULL;
    m_pD3DDeviceContext = NULL;

    m_pCurrentRenderTargetNoRef = NULL;
    m_pDepthStencilBufferForCurrentRTNoRef = NULL;

    m_fInScene = false;

    m_hrDisplayInvalid = S_OK;

    m_dwGPUMarkerFlags = 0;

    if (g_pMediaControl)
    {
        m_dwMetricsVerticesPerFrame = 0;
        m_dwMetricsTrianglesPerFrame = 0;
    }

    ZeroMemory(&m_desc, sizeof(m_desc));

    m_uCacheIndex = CMILResourceCache::InvalidToken;

    m_matSurfaceToClip.reset_to_identity();

    m_pHwIndexBuffer = NULL;
    m_pHwVertexBuffer = NULL;

#if DBG_STEP_RENDERING
    m_fDbgInStepRenderingPresent = false;
    m_pDbgSaveSurface = NULL;
#endif DBG_STEP_RENDERING
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::Init
//
//  Synopsis:
//      1. Creates a D3D device
//      2. Tests it for level1 support
//      3. Initializes this class
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::Init(
    __inout_ecount(1) D3DDevice* pID3DDevice,
    __inout_ecount(1) D3DDeviceContext *pID3DDeviceContext,
    __in_ecount(1) const CDisplay *pDisplay
    )
{
    HRESULT hr = S_OK;   

    // Initialize the resource manager as early as possible since the resource manager asserts on
    // shutdown that it has a valid device associated. If not, failures in the hardware detection code
    // below will lead to asserts firing in the D3DResourceManager code on shutdown.
    m_resourceManager.Init(this);

    //
    // Initialize basic members
    //

    m_luidD3DAdapter = pDisplay->GetLUID();

    //
    // Determine Graphics Acceleration Tier
    //

    m_Tier = MIL_TIER(1, 0);

    {
    #pragma warning(push)
    // error C4328: 'CGuard<LOCK>::CGuard(LOCK &)' : indirection alignment of formal parameter 1 (16) is greater than the actual argument alignment (8)
    #pragma warning(disable : 4328)
    ENTER_DEVICE_FOR_SCOPE(*this);
    #pragma warning(pop)

    m_pD3DDevice = pID3DDevice;
    m_pD3DDevice->AddRef();

    m_pD3DDeviceContext = pID3DDeviceContext;
    m_pD3DDeviceContext->AddRef();

    //
    // Check for primitive count limiting trace tag
    //

    //
    // Detect supported target formats
    //

    //
    // Request a global cache index
    //

    IGNORE_HR(AcquireIndex());

    //
    // Initialize render state object
    //

    IFC(CD3DRenderState::Init(
        this,
        m_pD3DDeviceContext
        ));


    //
    // Initialize hw surface render target shared data
    //

    IFC(InitSharedData(this));

    //
    // Initialize glyph bank
    //

    IFC(m_glyphBank.Init(this, &m_resourceManager));

    //
    // Create our fast path IB/VB
    //

    //
    // Size of the vertex and index buffers.  Since we're filling the buffers with
    // sets of 3 indices and sometimes sets of 3 vertices, keeping them both a
    // multiple of 3 helps the math and efficiency of the buffers slightly.
    //
    // The size of these buffers can be explored for performance characteristics.
    // Making the index buffer small will hurt performance because we will have to
    // discard it and retrieve a new one more often.  Making the vertex buffer
    // smaller will probably have a more dramatic performance impact as it causes
    // us to render non-indexed primitives.
    //
    // 20001 was picked because it's around 625kb for the vertex buffer.  This is
    // fairly large, but real scenario testing should be done to find out our
    // optimal size.
    //

    //
    // Note that all cards we support can accept VB's of 64k vertices or
    // more.  If we happen to try this creation on a card that doesn't support
    // these sizes, we end up failing to create the device and falling
    // back to software as expected.
    //


    const UINT uHwVertexBufferSize = 20001*sizeof(CD3DVertexXYZDUV2);
    const UINT uHwIndexBufferSize = 20001*3*sizeof(WORD);

    IFC(CHwD3DIndexBuffer::Create(
        &m_resourceManager,
        this,
        uHwIndexBufferSize,
        &m_pHwIndexBuffer
        ));

    IFC(CHwD3DVertexBuffer::Create(
        &m_resourceManager,
        this,
        uHwVertexBufferSize,
        &m_pHwVertexBuffer
        ));

    m_presentFailureWindowMessage = RegisterWindowMessage(L"NeedsRePresentOnWake");

    } // Leave device scope

Cleanup:

    if (FAILED(hr))
    {
        ReleaseInterface(m_pD3DDevice);
        ReleaseInterface(m_pD3DDeviceContext);
    }
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::~CD3DDeviceLevel1
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------
CD3DDeviceLevel1::~CD3DDeviceLevel1()
{
    Assert(m_cEntry == 0);
    Assert(m_dwThreadId == 0);
    Enter();

    ResetMarkers();

    ReleaseInterface(m_pEffectPipelineVertexShader);
    ReleaseInterface(m_pEffectPipelineVertexBuffer);
    ReleaseInterface(m_pEffectPipelinePassThroughPixelShader);
    m_pCurrentRenderTargetNoRef = NULL;   // No longer care about what is set.
                                          // This makes sure
                                          // ReleaseUseOfRenderTarget does
                                          // nothing.

    ReleaseInterface(m_pHwIndexBuffer);
    ReleaseInterface(m_pHwVertexBuffer);

    m_resourceManager.DestroyAllResources();

    // m_pDepthStencilBufferForCurrentRTNoRef should have been released from
    // use at this point
    Assert(m_pDepthStencilBufferForCurrentRTNoRef == NULL);

    ReleaseInterface(m_pD3DDevice);

#if DBG
    // Only need to leave for entry assert checks in ~CAssertEntry
    Leave();
#endif

}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::Enter and Leave
//
//  Synopsis:
//      Thread protection marking methods
//
//      Call Enter when device is about to used in a way that requires exclusive
//      access and Leave when leaving that context.  This is most commonly done
//      when handling a drawing routine, which the caller is required to provide
//      protection for.
//
//      Enter and Leave must be paired exactly.
//

void
CD3DDeviceLevel1::Enter(
    )
{
    if (IsEnsuringCorrectMultithreadedRendering())
    {
        m_csDeviceEntry.Enter();
    }

    // This call should be protected from double thread entry by the caller.

#if DBG
    // Attempt to catch simultaneous entry from two threads
    m_oDbgEntryCheck.Enter();
#endif

    m_cEntry++;
    m_dwThreadId = GetCurrentThreadId();

    Assert(m_cEntry > 0);
}

void
CD3DDeviceLevel1::Leave(
    )
{
    // This call should be protected from double thread entry by the caller
    // just like Enter was.

    Assert(m_cEntry > 0);

    // Should leave using same thread we entered on.
    Assert(m_dwThreadId == GetCurrentThreadId());

    if (--m_cEntry == 0)
    {
        m_dwThreadId = 0;
    }

#if DBG
    // Attempt to catch simultaneous entry from two threads
    m_oDbgEntryCheck.Leave();
#endif

    if (IsEnsuringCorrectMultithreadedRendering())
    {
        m_csDeviceEntry.Leave();
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::IsProtected
//
//  Synopsis:
//      Return true if this context is protected
//

bool
CD3DDeviceLevel1::IsProtected(
    bool fForceEntryConfirmation    // Ignore lack of multithreaded usage flag
                                    // when checking for protection.
    ) const
{
    // Check if we are always protected (and entry confirmation isn't required)
    // or if not if this thread has been marked/entered.
    bool fProtected = (!fForceEntryConfirmation)
                       || (m_dwThreadId == GetCurrentThreadId());

    if (fProtected)
    {
        AssertEntry(m_oDbgEntryCheck);
        if (   (m_dwD3DBehaviorFlags & D3DCREATE_MULTITHREADED)
            || fForceEntryConfirmation)
        {
            Assert(m_cEntry > 0);
            Assert(m_dwThreadId == GetCurrentThreadId());
        }
    }

    return fProtected;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::IsEntered
//
//  Synopsis:
//      Return true if this thread has been marked/entered as protected
//

bool
CD3DDeviceLevel1::IsEntered(
    ) const
{
    // Calling this method implies that either this thread is the only thread
    // that could have entered the device protection.  Therefore we should use
    // the entry check assert.
    AssertEntry(m_oDbgEntryCheck);

    bool fEntered = (m_cEntry > 0);

    if (fEntered)
    {
        // If entered this should be the marked thread
        Assert(m_dwThreadId == GetCurrentThreadId());
    }
    else
    {
        // If not entered there should be no thread ID marked
        Assert(m_dwThreadId == 0);
    }

    return fEntered;
}

void
CD3DDeviceLevel1::AssertRenderFormatIsTestedSuccessfully(
    DXGI_FORMAT fmtRenderTarget
    )
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::MarkUnusable
//
//  Synopsis:
//      Mark this device as unusable and notify manager. Future calls to Present
//      will return device lost.
//
//------------------------------------------------------------------------------
void
CD3DDeviceLevel1::MarkUnusable(
    bool fMayBeMultithreadedCall
    )
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreateRenderTarget
//
//  Synopsis:
//      delegate to D3DDeviceContext::CreateRenderTargetUntracked, then place
//      a resource wrapper around it
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CreateRenderTarget(
    UINT uiWidth,
    UINT uiHeight,
    DXGI_FORMAT dxgiFmt,
    UINT uiMultisampleCount,
    UINT uiMultisampleQuality,
    __deref_out_ecount(1) CD3DTexture** const ppD3DTexture
    )
{
    HRESULT hr = S_OK;
    D3DTexture *pID3DTexture = NULL;

    IFC(CreateRenderTargetUntracked(
        uiWidth,
        uiHeight,
        dxgiFmt,
        uiMultisampleCount,
        uiMultisampleQuality,
        &pID3DTexture
        ));

    CD3DVidMemOnlyTexture* pResult = nullptr;
    IFC(CD3DVidMemOnlyTexture::Create(pID3DTexture, false, this, &pResult));
    *ppD3DTexture = pResult;

Cleanup:
    ReleaseInterface(pID3DTexture);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreateRenderTargetUntracked
//
//  Synopsis:
//      Delegate to D3DDeviceContext::CreateRenderTarget. This method is called
//      "Untracked" because the surface created is not tracked by our resource
//      management system. This version of CreateRenderTarget should only be
//      called if absolutely necessary.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CreateRenderTargetUntracked(
    UINT uiWidth,
    UINT uiHeight,
    DXGI_FORMAT dxgiFormat,
    UINT uiMultisampleCount,
    UINT uiMultisampleQuality,
    __deref_out_ecount(1) D3DTexture ** const ppD3DTexture
    )
{
    HRESULT hr = S_OK;
    D3DTexture *pID3DTexture = NULL;

    *ppD3DTexture = NULL;

    AssertDeviceEntry(*this);

    BEGIN_DEVICE_ALLOCATION;

    {
        MtSetDefault(Mt(D3DRenderTarget));

        D3D11_TEXTURE2D_DESC bufferDesc;
        bufferDesc.ArraySize = 1;
        bufferDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        bufferDesc.CPUAccessFlags = 0;
        bufferDesc.Format = dxgiFormat;
        bufferDesc.Height = uiHeight;
        bufferDesc.MipLevels = 1;
        bufferDesc.MiscFlags = 0;
        bufferDesc.SampleDesc.Count = uiMultisampleCount;
        bufferDesc.SampleDesc.Quality = uiMultisampleQuality;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.Width = uiWidth;
        hr = m_pD3DDevice->CreateTexture2D(&bufferDesc, 0, &pID3DTexture);
    }

    END_DEVICE_ALLOCATION;

    IFC(hr); // placed here to avoid stack capturing multiple times in allocation loop

    *ppD3DTexture = pID3DTexture; // Steal ref
    pID3DTexture = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pID3DTexture);

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreateVertexBuffer
//
//  Synopsis:
//      delegate to D3DDeviceContext::CreateVertexBuffer
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CreateVertexBuffer(
    UINT Length,
    D3D11_USAGE Usage,
    UINT cpuAccessFlags,
    __deref_out_ecount(1) D3DVertexBuffer ** const ppVertexBuffer
    )
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;

    MtSetDefault(Mt(D3DVertexBuffer));

    //
    // Allocate the D3D vertex buffer
    //

    BEGIN_DEVICE_ALLOCATION;

    // Fill in a buffer description.
    D3D11_BUFFER_DESC bufferDesc;
    bufferDesc.Usage = Usage;
    bufferDesc.ByteWidth = Length;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = cpuAccessFlags;
    bufferDesc.MiscFlags = 0;

    hr = m_pD3DDevice->CreateBuffer(
        &bufferDesc, 
        nullptr, 
        ppVertexBuffer);

    END_DEVICE_ALLOCATION;

    MIL_THR(hr); // placed here to avoid stack capturing multiple times in allocation loop

    RRETURN(HandleDIE(hr));
}

HRESULT
CD3DDeviceLevel1::CreateIndexBuffer(
    UINT Length,
    D3D11_USAGE Usage,
    UINT uiCpuAccessFlags,
    __deref_out_ecount(1) D3DIndexBuffer** const ppIndexBuffer
    )
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;

    MtSetDefault(Mt(D3DIndexBuffer));

    //
    // Allocate the D3D index buffer
    //

    BEGIN_DEVICE_ALLOCATION;

    // Fill in a buffer description.
    D3D11_BUFFER_DESC bufferDesc = { 0 };
    bufferDesc.Usage = Usage;
    bufferDesc.ByteWidth = Length;
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.CPUAccessFlags = uiCpuAccessFlags;
    bufferDesc.MiscFlags = 0;

    hr = m_pD3DDevice->CreateBuffer(
        &bufferDesc,
        nullptr,
        ppIndexBuffer);

    END_DEVICE_ALLOCATION;

    MIL_THR(hr); // placed here to avoid stack capturing multiple times in allocation loop

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreateTexture
//
//  Synopsis:
//      delegate to D3DDeviceContext::CreateTexture
//
//  Notes:
//      Shared handle support is a D3D9.L only feature.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CreateTexture(
    __in_ecount(1) const D3D11_TEXTURE2D_DESC* pTextureDesc,
    UINT uLevels,
    __deref_out_ecount(1) D3DTexture** const ppD3DTexture
)
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;

    //
    // Allocate the D3D texture
    //

    {
        MtSetDefault(Mt(D3DTexture));

        BEGIN_DEVICE_ALLOCATION;

        hr = m_pD3DDevice->CreateTexture2D(pTextureDesc, 0, ppD3DTexture);

        END_DEVICE_ALLOCATION;

        DbgInjectDIE(&hr);

        MIL_THR(hr); // placed here to avoid stack capturing multiple times in allocation loop
    }

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreateLockableTexture
//
//  Synopsis:
//      delegate to D3DDeviceContext::CreateTexture
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CreateLockableTexture(
    __in_ecount(1) const D3D11_TEXTURE2D_DESC* pTextureDesc,
    __deref_out_ecount(1) CD3DLockableTexture** const ppLockableTexture
)
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;
    D3DTexture *pD3DTexture = NULL;
    UINT uLevels = 1;

    *ppLockableTexture = NULL;

    //
    // Allocate the D3D texture
    //

    {
        MtSetDefault(Mt(D3DTexture));

        BEGIN_DEVICE_ALLOCATION;

        hr = CD3DDeviceLevel1::CreateTexture(pTextureDesc, uLevels, &pD3DTexture);

        END_DEVICE_ALLOCATION;

        IFC(hr); // placed here to avoid jumping to cleanup immediately on OOVM
    }

    //
    // Create the texture wrapper
    //

    IFC(CD3DLockableTexture::Create(
        &m_resourceManager,
        this,
        pD3DTexture,
        ppLockableTexture
        ));

Cleanup:
    ReleaseInterface(pD3DTexture);
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreateSysMemUpdateSurface
//
//  Synopsis:
//         delegate to D3DDeviceContext::CreateSurface
//
//      *** WARNING *** WARNING *** WARNING *** WARNING ***
//
//         CreateSysMemUpdateSurface only allows a non-NULL pvPixels on
//         Longhorn. Passing a non-NULL pSharedHandle to
//         CreateOffscreenPlainSurface will return E_NOTIMPL on XP and Server
//         2003.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CreateSysMemUpdateTexture(
    UINT uWidth,
    UINT uHeight,
    DXGI_FORMAT fmtTexture,
    __in_xcount_opt(uWidth* uHeight* D3DFormatSize(fmtTexture)) void* pvPixels,
    __deref_out_ecount(1) D3DTexture** const ppD3DSysMemTexture
)
{
    HRESULT hr = S_OK;
    D3DTexture *pID3DTexture = NULL;

    *ppD3DSysMemTexture = NULL;

    //
    // Allocate the D3D surface. Passing the pixels this way creates the
    // surface by referencing these pixels.
    //

    {
        MtSetDefault(Mt(D3DTexture));

        BEGIN_DEVICE_ALLOCATION;

        D3D11_TEXTURE2D_DESC bufferDesc;
        bufferDesc.ArraySize = 1;
        bufferDesc.BindFlags = 0;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
        bufferDesc.Format = fmtTexture;
        bufferDesc.Height = uHeight;
        bufferDesc.MipLevels = 1;
        bufferDesc.MiscFlags = 0;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.SampleDesc.Quality = 0;
        bufferDesc.Usage = D3D11_USAGE_STAGING;
        bufferDesc.Width = uWidth;

        if (pvPixels != nullptr)
        {
            D3D11_SUBRESOURCE_DATA initialData;
            initialData.pSysMem = pvPixels;
            initialData.SysMemPitch = uWidth * D3DFormatSize(fmtTexture);
            initialData.SysMemSlicePitch = 0;
            hr = m_pD3DDevice->CreateTexture2D(&bufferDesc, &initialData, &pID3DTexture);
        }
        else
        {
            hr = m_pD3DDevice->CreateTexture2D(&bufferDesc, nullptr, &pID3DTexture);
        }


        END_DEVICE_ALLOCATION;

        MIL_THR(hr); // placed here to avoid stack capturing multiple times in allocation loop

        IFC(HandleDIE(hr));

        *ppD3DSysMemTexture = pID3DTexture;
        (*ppD3DSysMemTexture)->AddRef();
    }

Cleanup:
    ReleaseInterfaceNoNULL(pID3DTexture);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::UpdateTextureRegion
//
//  Synopsis:
//      delegate to D3DDeviceContext::CopySubresourceRegion
//
//  Note:
//      There is no check for this, but the src texture must be in system memory
//      and the dest texture must be in pool default.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::UpdateTextureRegion(
    __in_ecount(1) D3DTexture * pD3DSysMemSrcTexture,
    UINT srcSubresource,
    __in_ecount_opt(1) const RECT *pSourceRect,
    __inout_ecount(1) D3DTexture * pD3DPoolDefaultDestTexture,
    UINT destSubresource,
    __in_ecount_opt(1) const POINT *pDestPoint
)
{
    HRESULT hr = S_OK;

    AssertDeviceEntry(*this);

    D3D11_BOX sourceCopyBox;
    sourceCopyBox.left = pSourceRect->left;
    sourceCopyBox.right = pSourceRect->right;
    sourceCopyBox.top = pSourceRect->top;
    sourceCopyBox.bottom = pSourceRect->bottom;
    sourceCopyBox.front = 0;
    sourceCopyBox.back = 1;

    m_pD3DDeviceContext->CopySubresourceRegion(pD3DPoolDefaultDestTexture, 0, pDestPoint->x, pDestPoint->y, 0,
                                               pD3DSysMemSrcTexture, 0, &sourceCopyBox);

Cleanup:
    RRETURN(HandleDIE(hr));
}



//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::UpdateSurface
//
//  Synopsis:
//      delegate to D3DDeviceContext::UpdateSurface
//
//  Note:
//      There is no check for this, but the src texture must be in system memory
//      and the dest texture must be in pool default.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::UpdateTexture(
    __in_ecount(1) D3DTexture* pD3DSysMemSrcTexture,
    __inout_ecount(1) D3DTexture* pD3DPoolDefaultDestTexture
)
{
    HRESULT hr = S_OK;

    AssertDeviceEntry(*this);

    m_pD3DDeviceContext->CopyResource(pD3DPoolDefaultDestTexture, pD3DSysMemSrcTexture);

Cleanup:
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::SetRenderTarget
//
//  Synopsis:
//      1. Call EndScene
//      2. Set the render target
//      3. Call BeginScene
//      4. Set the view and projection matrices.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::SetRenderTargetForEffectPipeline(
    __in_ecount(1) CD3DTexture* pD3DTexture
    )
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;

    if (pD3DTexture == m_pCurrentRenderTargetNoRef)
    {
        goto Cleanup;
    }

    //
    // Call EndScene
    //

    if (m_fInScene)
    {
        IFC(EndScene());
    }

    //
    // Set the render target
    //

    m_desc = pD3DTexture->D3DSurface0Desc();

    CD3DRenderState::SetRenderTarget(pD3DTexture->GetRenderTargetViewNoAddRef());
    m_pCurrentRenderTargetNoRef = pD3DTexture;

    //
    // SetRenderTarget resets the Viewport and ScissorClip
    //  for RT index 0; so remember that.
    //

    //
    // Our clip being set is tracked by our CHwRenderStateManager through the
    // CD3DRenderstate object.
    //
    // We have to let it know to set the clip to false.
    //
    SetClipSet(false);

    //
    // Call BeginScene
    //

    IFC(BeginScene());

Cleanup:

    //
    // If any part of the above fails, there is no valid state.
    // Release the use of the current render target is there is one.
    //

    if (FAILED(hr) && m_pCurrentRenderTargetNoRef)
    {
        ReleaseUseOfRenderTarget(m_pCurrentRenderTargetNoRef);
    }

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::SetRenderTarget
//
//  Synopsis:
//      1. Call EndScene
//      2. Set the render target
//      3. Call BeginScene
//      4. Set the view and projection matrices.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::SetRenderTarget(
    __in_ecount(1) CD3DTexture *pD3DTexture
    )
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;
    CMILMatrix matView;
    CMILMatrix matProj;

    //
    // If the render target hasn't changed, don't do anything.
    //

    if (pD3DTexture == m_pCurrentRenderTargetNoRef)
    {
        goto Cleanup;
    }

    //
    // Call EndScene
    //

    if (m_fInScene)
    {
        IFC(EndScene());
    }

    //
    // Set the render target
    //

    m_desc = pD3DTexture->D3DSurface0Desc();

    // There was a Watson report where D3D returned a failure code
    //                 indicating the D3D9 surface ptr was NULL.  From inspection
    //                 this seems impossible - we want this to break in retail at
    //                 the point of failure to aid future investigation of the issue.
    CD3DRenderState::SetRenderTarget(pD3DTexture->GetRenderTargetViewNoAddRef());
    m_pCurrentRenderTargetNoRef = pD3DTexture;

    //
    // SetRenderTarget resets the Viewport and ScissorClip
    //  for RT index 0; so remember that.
    //

    //
    // Our clip being set is tracked by our CHwRenderStateManager through the
    // CD3DRenderstate object.
    //
    // We have to let it know to set the clip to false.
    //
    SetClipSet(false);

    {
        MilPointAndSizeL rcViewport;

        rcViewport.X = 0;
        rcViewport.Y = 0;
        rcViewport.Width = m_desc.Width;
        rcViewport.Height = m_desc.Height;

        //
        // Set the viewport since it has inherently changed.
        //
        IFC(SetViewport(&rcViewport));

        //
        // Set the surface to clipping matrix
        //

        IFC(SetSurfaceToClippingMatrix(
            &rcViewport
            ));
    }

    //
    // Call BeginScene
    //

    IFC(BeginScene());

Cleanup:

    //
    // If any part of the above fails, there is no valid state.
    // Release the use of the current render target is there is one.
    //

    if (FAILED(hr) && m_pCurrentRenderTargetNoRef)
    {
        ReleaseUseOfRenderTarget(m_pCurrentRenderTargetNoRef);
    }

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::ReleaseUseOfRenderTarget
//
//  Synopsis:
//       Releases any cached use the device may have of the given
//      render target, thereby enabling the D3D surface to be cleaned up
//      when it is truely no longer in use.
//
//------------------------------------------------------------------------------
void
CD3DDeviceLevel1::ReleaseUseOfRenderTarget(
    __in_ecount(1) const CD3DTexture* pD3DTexture// [IN] a render target that
                                                      // will no longer be valid
    )
{
    AssertDeviceEntry(*this);

    if (pD3DTexture == m_pCurrentRenderTargetNoRef)
    {
        m_pCurrentRenderTargetNoRef = NULL;

        AssertMsg(m_fInScene,
                  "m_fInScene expected to be true.\r\n"
                  "This may be ignored only if caller is SetRenderTarget."
                  );
        if (m_fInScene)
        {
            IGNORE_HR(EndScene());
        }

        CD3DRenderState::SetRenderTarget(nullptr);
        ReleaseUseOfDepthStencilTexture(m_pDepthStencilBufferForCurrentRTNoRef);

        // Note: We've set the RT to a dummy so there is no point in
        //       beginning a scene now.  The scene will begin once
        //       another RT has been set.
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::Clear
//
//  Synopsis:
//      delegate to D3DDeviceContext::Clear
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::ClearColor(
    DWORD dwCount,
    __in_ecount_opt(dwCount) const D3D11_RECT* pRects,
    DWORD colorRGBA
)
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;

    Assert((dwCount > 0) == (pRects != NULL));

    //
    // Do the clear
    //
    FLOAT colorFloat[4] = {
        ((colorRGBA >> 0) & 0xFF) / 255.0f,
        ((colorRGBA >> 8) & 0xFF) / 255.0f,
        ((colorRGBA >> 16) & 0xFF) / 255.0f,
        ((colorRGBA >> 24) & 0xFF) / 255.0f
    };

    m_pD3DDeviceContext->ClearView(m_pCurrentRenderTargetNoRef->GetRenderTargetViewNoAddRef(),
        colorFloat,
        pRects,
        dwCount
    );

Cleanup:
    RRETURN(HandleDIE(hr));
}

HRESULT
CD3DDeviceLevel1::ClearDepthStencil(
    FLOAT rZValue,
    int iStencilValue
)
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;

    //
    // Do the clear
    //
    m_pD3DDeviceContext->ClearDepthStencilView(m_pDepthStencilBufferForCurrentRTNoRef->GetDepthStencilViewNoAddRef(), 
        0, 
        rZValue, 
        iStencilValue);

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::ColorFill
//
//  Synopsis:
//      delegate to ColorFill
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::ColorFill(
    __inout_ecount(1) CD3DTexture* pTexture,
    __in_ecount_opt(1) const RECT* pRect,
    DWORD colorRGBA
)
{
    HRESULT hr = S_OK;

    MtSetDefault(Mt(D3DColorFill));

    FLOAT colorFloat[4] = {
        ((colorRGBA >> 0) & 0xFF) / 255.0f,
        ((colorRGBA >> 8) & 0xFF) / 255.0f,
        ((colorRGBA >> 16) & 0xFF) / 255.0f,
        ((colorRGBA >> 24) & 0xFF) / 255.0f
    };

    DWORD dwCount = 0;
    if (pRect != nullptr)
    {
        dwCount = 1;
    }

    m_pD3DDeviceContext->ClearView(pTexture->GetRenderTargetViewNoAddRef(),
        colorFloat,
        reinterpret_cast<const D3D11_RECT*>(pRect),
        dwCount
    );

    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CleanupFreedResources
//
//  Synopsis:
//      Free, without delay, unused resources hanging off the device.
//
//------------------------------------------------------------------------------
void
CD3DDeviceLevel1::CleanupFreedResources()
{
    m_resourceManager.DestroyReleasedResourcesFromLastFrame();
    m_resourceManager.DestroyResources(CD3DResourceManager::WithoutDelay);
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      FillCurrentCumulativeMax
//
//  Synopsis:
//      Take points to three DWORDs for Current, Cumulative, Max, and a new
//      value.  Use atomic operations to fill in the first three based on the
//      new value
//
//      Allow the cumulative ones to overflow and wraparound. Expectation is
//      that they will be reset by a monitoring tool before then, and it's ok if
//      they're not.
//
//------------------------------------------------------------------------------

static void
FillCurrentCumulativeMax(
    __out_ecount(1) DWORD *pCurrent,
    __inout_ecount(1) DWORD *pCumulative,
    __inout_ecount(1) DWORD *pMax,
    DWORD newValue)
{
    InterlockedExchange(reinterpret_cast<LONG *>(pCurrent), newValue);
    InterlockedExchangeAdd(reinterpret_cast<LONG *>(pCumulative), newValue);
    DWORD maxValue = *pMax > newValue ? *pMax : newValue;
    InterlockedExchange(reinterpret_cast<LONG *>(pMax), maxValue);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::Present
//
//  Synopsis:
//      1. Call EndScene
//      2. Delegate to CD3DSwapChain::Present
//      3. Call BeginScene
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::Present(
    __in_ecount(1) CD3DSwapChain const *pD3DSwapChain,
    __in_ecount_opt(1) CMILSurfaceRect const *prcSource,
    __in_ecount_opt(1) CMILSurfaceRect const *prcDest,
    __in_ecount(1) CMILDeviceContext const *pMILDC,
    __in_ecount_opt(1) RGNDATA const * pDirtyRegion,
    DWORD dwD3DPresentFlags
    )
{
    AssertDeviceEntry(*this);

    HRESULT hr;

    if (FAILED(m_hrDisplayInvalid))
    {
        // Call MarkUnusable to check if we still need to handle loss now that
        // we have the proper protection.
        MarkUnusable(false /* This call is already entry protected. */);

        // MarkUnusable may have changed the HR but it's still a failure.
        Assert(FAILED(m_hrDisplayInvalid));

        MIL_THR(m_hrDisplayInvalid);
        goto Cleanup;
    }

    Assert(pD3DSwapChain->m_pDXGISwapChain);
    Assert(pD3DSwapChain->IsValid());

#if DBG==1
    if (IsTagEnabled(tagPixelZoomMode))
    {
        //
        // Get the HWND so that we can do screen to client coordinate transforms.  This code means
        // that you actually need to wait for a frame before using the mouse and doesn't work with
        // multiple windows.
        //
        // However, since this is a tracetag only debugging feature, this code is sufficient.
        //
        g_dbgHWND = pMILDC->GetHWND();
    }
#endif

    //if (m_bNeedGarbageCollection)  --- always for now
    {
        m_glyphBank.CollectGarbage();
        //m_bNeedGarbageCollection = false;
    }

    //
    // Update metrics
    //

    if (g_pMediaControl)
    {
        CMediaControlFile* pFile = g_pMediaControl->GetDataPtr();

        // Update metrics, including cumulative and maxmetrics.


        if (m_dwMetricsTrianglesPerFrame > 0)
        {
            //
            // Only update if we've drawn something
            //

            FillCurrentCumulativeMax(&pFile->TrianglesPerFrame,
                                     &pFile->TrianglesPerFrameCumulative,
                                     &pFile->TrianglesPerFrameMax,
                                     m_dwMetricsTrianglesPerFrame);
        }

        // Texture Updates
        FillCurrentCumulativeMax(&pFile->TextureUpdatesPerFrame,
                                 &pFile->TextureUpdatesPerFrameCumulative,
                                 &pFile->TextureUpdatesPerFrameMax,
                                 g_dwTextureUpdatesPerFrame);


        // Pixels
        FillCurrentCumulativeMax(&pFile->PixelsFilledPerFrame,
                                 &pFile->PixelsFilledPerFrameCumulative,
                                 &pFile->PixelsFilledPerFrameMax,
                                 g_lPixelsFilledPerFrame);

        InterlockedExchange(reinterpret_cast<volatile LONG *>(&g_dwTextureUpdatesPerFrame), 0);
        InterlockedExchange(static_cast<LONG *>(&g_lPixelsFilledPerFrame), 0);

        MtSet(Mt(VerticesPerFrame), m_dwMetricsVerticesPerFrame, 0);
        m_dwMetricsVerticesPerFrame = 0;

        MtSet(Mt(TrianglesPerFrame), m_dwMetricsTrianglesPerFrame, 0);
        m_dwMetricsTrianglesPerFrame = 0;
    }

    //
    // Call EndScene
    //

    bool fRestoreScene = m_fInScene;
    bool fPresentProcessed = false;

    if (m_fInScene)
    {
        IFC(EndScene());
    }

#if DBG
    if (IsTagEnabled(tagD3DStats))
    {
        // Query stats
        //m_d3dStats.OnPresent(m_pD3DDevice);
    }
#endif
    IF_D3DLOG(m_log.OnPresent();)

    IFC(PresentWithD3D(
        pD3DSwapChain->m_pDXGISwapChain,
        prcSource,
        prcDest,
        pMILDC,
        pDirtyRegion,
        dwD3DPresentFlags,
        &fPresentProcessed
        ));

    if (fRestoreScene)
    {
        MIL_THR_SECONDARY(BeginScene());
    }

Cleanup:

    RRETURN1(hr, S_PRESENT_OCCLUDED); // DIE already handled
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::PresentWithD3D
//
//  Synopsis:
//      Use D3D to present from the swap chain.
//

HRESULT
CD3DDeviceLevel1::PresentWithD3D(
    __inout_ecount(1) IDXGISwapChain *pDXGISwapChain,
    __in_ecount_opt(1) CMILSurfaceRect const *prcSource,
    __in_ecount_opt(1) CMILSurfaceRect const *prcDest,
    __in_ecount(1) CMILDeviceContext const *pMILDC,
    __in_ecount_opt(1) RGNDATA const * pDirtyRegion,
    DWORD dwD3DPresentFlags,
    __out_ecount(1) bool *pfPresentProcessed
    )
{
    HRESULT hr = S_OK;

    *pfPresentProcessed = false;

    //
    // Call present and check for mode change
    //

    BEGIN_DEVICE_ALLOCATION;

    // TODO -- Consider, dirty region support?
    hr = pDXGISwapChain->Present(0, 0);


    END_DEVICE_ALLOCATION;

    // Once present is complete, state should be reset
    ResetState();
    m_pCurrentRenderTargetNoRef = NULL;
    m_pDepthStencilBufferForCurrentRTNoRef = NULL;

    DbgInjectDIE(&hr);

    if (hr == S_OK)
    {
        *pfPresentProcessed = true;
    }
    else if (hr == S_PRESENT_MODE_CHANGED)
    {
        //
        // Desktop Display Mode has  changed.  (LH Only, pre-LH systems will
        // return D3DERR_DEVICELOST instead.
        //
        // Currently we want to handle this identically to D3DERR_DEVICELOST
        //
        // Future Consideration:  May want to optimize mode change
        //
        // We could optimize this scenario so we don't recreate the device
        // and instead check the new display parameters.
        //

        hr = D3DERR_DEVICELOST;
    }
    else if (hr == S_PRESENT_OCCLUDED)
    {
        // Device is in a normal state but isn't visible.  This is LH Only
        // and can be because of:
        //
        // 1.  Presentation window is minimized.
        //
        // 2.  Another Device entered fullscreen mode on the same monitor,
        //     and this window is completely on that monitor.
        //
        // 3.  The monitor on which the window is displayed is asleep.
        //     (This final case we handle in the UI thread, by listening for
        //      power events and appropriately invalidating the window when
        //      the monitor comes back on). This is facilitated by sending
        //      a custom window message that the UI thread is looking for.

        //
        // In the windowed case we can't keep checking our device state until
        // we're valid again before we render, since all rendering will
        // stop.  If a window is straddling 2 monitors and one side gets
        // occluded the other won't render.  So if we're windowed, we keep
        // rendering as if nothing has happened.
        //

        // To avoid overloading the CPU with repeated failures, we sleep briefly here. If we are failing to present,
        // this is not a problem because the UI will be unresponsive anyway.
        Sleep(100);
        PostMessage(pMILDC->GetHWND(), m_presentFailureWindowMessage, NULL, NULL);

        hr = S_OK;
    }

    //
    // !!! Critical Note: After this point hr may not be S_OK.  Make sure not
    //                    to change to S_OK when making other calls.
    //

    MIL_THR(hr); // placed here to avoid stack capturing multiple times in allocation loop

    if (FAILED(hr))
    {
        hr = HandlePresentFailure(pMILDC, hr);
    }

    RRETURN1(hr, S_PRESENT_OCCLUDED);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::HandlePresentFailure
//
//  Synopsis:
//      This method handles hresult failures for the present methods, e.g. PresentWithGDI or
//      PresentWithD3D.
//
//      To avoid unnecessary overhead this function should only be called if FAILED(hr) is true,
//      i.e. only with a valid HRESULT failure and not a success code.
//------------------------------------------------------------------------------

HRESULT
CD3DDeviceLevel1::HandlePresentFailure(
    __in_ecount(1) CMILDeviceContext const *pMILDC,
    HRESULT hr
    )
{
    Assert(FAILED(hr));

    //
    //  Release what resources we can.
    //  Do not use ReleaseUseOfRenderTarget since it calls EndScene.
    //  There should normally be a valid RT at this point, but
    //  there are some rare corner cases that we should protect
    //  against.
    //

    if (m_pCurrentRenderTargetNoRef)
    {
        m_pCurrentRenderTargetNoRef = NULL;

        m_pD3DDeviceContext->ClearState();

        ReleaseUseOfDepthStencilTexture(m_pDepthStencilBufferForCurrentRTNoRef);
    }


    if (!IsWindow(pMILDC->GetHWND()))
    {
        //
        // There can be a variety of failure codes returned when a window is
        // destroyed while we are trying to draw to it with GDI.  To simplify
        // the callers error handling check for an invalid window and return
        // a failure code indicating such.  Otherwise just return whatever
        // we could discern so far.
        //

        MIL_THR(HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE));
    }
    else if (hr == E_FAIL || hr == D3DERR_DRIVERINTERNALERROR)
    {
        //
        // The code might end up in here for various reasons:
        //
        //    CD3DSurface::GetDC can fail with D3DERR_DRIVERINTERNALERROR, if
        //    this happens we treat this as a D3DERR_DEVICELOST which is handled below.
        //
        //    IDirect3DSwapChain9::Present may return E_FAIL (though it isn't supposed to),
        //    so treat like D3DERR_DRIVERINTERNALERROR & convert to D3DERR_DEVICELOST
        //    which is handled below.
        //
        hr = D3DERR_DEVICELOST;
    }
    else if (hr == E_INVALIDARG)
    {
        //
        //  DWM DX redirection resize
        //  synchronization can return E_INVALIDARG or E_FAIL (handled above).
        //
        RIPW(L"Present returned E_INVALIDARG");
        MIL_THR(WGXERR_NEED_RECREATE_AND_PRESENT);
    }



    if (hr == D3DERR_DEVICELOST ||
        hr == D3DERR_DEVICEHUNG ||  // Hw Adapter timed out and has
                                    // been reset by the OS (LH Only)
                                    //
        hr == D3DERR_DEVICEREMOVED  // Hw Adapter has been removed (LH
                                    // Only)
                                    //
        )
    {
        hr = WGXERR_DISPLAYSTATEINVALID;
        MarkUnusable(false /* This call is already entry protected. */);
    }

    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::SetTexture
//
//  Synopsis:
//      Sets the texture for a particular stage
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::SetTexture(
    UINT uiSlot,
    __inout_ecount_opt(1) CD3DTexture* pD3DTexture
)
{
    AssertDeviceEntry(*this);

    HRESULT hr = S_OK;

    // 2/14/2003 chrisra - Changed the function so if a NULL texture
    // was passed in the stage would be set to NULL

    if (pD3DTexture != NULL)
    {
        Assert(pD3DTexture->IsValid());

        Use(*pD3DTexture);

        //
        // Set base texture at specified stage
        //
    }

    IFC(CD3DRenderState::SetTexture(uiSlot, pD3DTexture->GetShaderResourceViewNoAddRef()));

Cleanup:
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::BeginScene
//
//  Synopsis:
//      delegate to D3DDeviceContext::BeginScene
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::BeginScene()
{
    Assert(!m_fInScene);
    m_fInScene = true;
    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::EndScene
//
//  Synopsis:
//      delegate to D3DDeviceContext::EndScene
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::EndScene()
{
    Assert(m_fInScene);
    m_fInScene = false;

    return S_OK;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::FlushBufferFan
//
//  Synopsis:
//      Draws the vertexbuffer assuming the primitive type is a fan. Does not
//      clear out the vertex information that may be reused in multi-pass
//      schemes
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::FlushBufferFan(
    __in_ecount(1) CD3DVertexBuffer *pBuffer
    )
{
    HRESULT hr = S_OK;
    UINT cVertices = pBuffer->GetNumVertices();

    Assert(m_fInScene);

    //
    // It's possible for the tessellator to output 0 triangles.  For example,
    // if we get a zero area rectangle, this will occur.
    //

    //
    // The number of triangles is equal to the number of vertices - 2, but we
    // don't want to do that operation and then check for cTriangles > 0,
    // because the unsigned subtraction operation could cause wrapping,
    // resulting in us attempting to render approximately UINT_MAX.  So we just
    // check for cVertices > 2 and then calculate the number of triangles
    // after.
    //

    if (cVertices > 2)
    {
        UINT cTriangles = cVertices - 2;

        std::vector<WORD> indexArray;
        indexArray.resize(cTriangles * 3);

        int indexCur = 0;
        for (int idx = 0; idx < cTriangles; ++idx)
        {
            indexArray[indexCur++] = 0;
            indexArray[indexCur++] = idx + 1;
            indexArray[indexCur++] = idx + 2;
        }

        IFC(DrawIndexedTriangleListUP(
            cVertices,
            cTriangles,
            &indexArray[0],
            pBuffer->GetVertices(),
            pBuffer->GetVertexStride()
            ));
    }

Cleanup:
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::Set3DTransforms
//
//  Synopsis:
//      Sends what the current transforms on the card should be to our
//      CD3DRenderState for 3D.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::Set3DTransforms(
    __in_ecount(1) CMILMatrix const *pWorldTransform3D,
    __in_ecount(1) CMILMatrix const *pViewTransform3D,
    __in_ecount(1) CMILMatrix const *pProjectionTransform3D,
    __in_ecount(1) CMatrix<CoordinateSpace::Projection3D,CoordinateSpace::Device> const &matHomogeneousTo2DDevice
    )
{
    HRESULT hr = S_OK;

    CMILMatrix mat2DDeviceToHomogeneous;
    CMILMatrix matProjectionModifier;
    CMILMatrix mat3DViewportProjection;

    IFC(SetWorldTransform(
        pWorldTransform3D
        ));

    IFC(SetViewTransform(
        pViewTransform3D
        ));

    matProjectionModifier = matHomogeneousTo2DDevice * m_matSurfaceToClip;


    // We now have the transform to take us from Homogeneous Clipping Space to
    // the local space of the viewport passed to us.  We apply this to our
    // projection transform, so now our projection transform will take all
    // objects to local space of the viewport.

    mat3DViewportProjection = *pProjectionTransform3D * matProjectionModifier;

    IFC(SetProjectionTransform(
        &mat3DViewportProjection
        ));

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::SetSurfaceToClippingMatrix
//
//  Synopsis:
//      Calculate surface space to homogeneous clipping (~viewport) matrix for
//      2D and 3D rendering
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::SetSurfaceToClippingMatrix(
    __in_ecount(1) const MilPointAndSizeL *prcViewport
    )
{
    HRESULT hr = S_OK;

    //
    // The SurfaceToClip matrix is used to change our coordinate system to
    // the one DX needs...
    //
    // DX one is this (with integer pixel center at edges):
    //
    //             +1.0
    //              ^ +y
    //              |
    //              |
    //              |
    //       <------O------> +x  +1.0
    //              |
    //              |
    //              |
    //              v
    //
    // Ours is this with integers at upper-left of pixel (half-integer pixel
    //  center):
    //
    //     O------------> +x  +Width
    //     |
    //     |
    //     |
    //     |
    //     |
    //     |
    //     v +y
    //   +Height

    //
    // This diagram of the surface and homogeneous in a single dimension (x) may
    // be more helpful.  From this diagram we can directly derive the required
    // surface to homogeneous clipping transform.  The viewport (V) is given in
    // surface coordinate space.  
    // Now lets find a scale and translation matrix that maps surface space to
    // clipping space.
    //
    //     SurfaceToClip = Sx * Tx
    //
    // The scale portion of the transform is found by matching up the clipping
    // range with the viewport range:
    //
    //          Sx = 2 / V.W
    //

    m_matSurfaceToClip.reset_to_identity();
    m_matSurfaceToClip._11 = 2.0f / prcViewport->Width;
    m_matSurfaceToClip._22 = -2.f / prcViewport->Height;
    m_matSurfaceToClip._41 = -1.f;
    m_matSurfaceToClip._42 = 1.f;

    //
    // Set the 2D transforms for the state manager.  The world and view
    // matrices are identity.  The m_matSurfaceToClip matrix is concatentation
    // of two matrices that can be thought of as the view and projection
    // matrices.
    //
    // The other important propery about a projection matix is what is set in
    // z-scale, translate, and reciprical w as they can affect z-clipping.
    // Note that these are all the same as they'd be in an identity matrix.
    //

    IFC(Define2DTransforms(
        &m_matSurfaceToClip
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreatePixelShaderFromResource
//
//  Synopsis:
//      Read precompiled shader binary data from the resource of this
//      executable, pointed by argument uResourceId. Create pixel shader from
//      these data.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CreatePixelShaderFromResource(
    UINT uResourceId,
    __deref_out_ecount(1) ID3D11PixelShader** const ppPixelShader
    )
{
    HRESULT hr = S_OK;
    HGLOBAL hResource = NULL;
    HRSRC hResourceInfo = NULL;
    const DWORD *pData = NULL;
    DWORD dwResourceSize = 0;

    // This routine should not be called if the shader exists.
    // Use EnsurePixelShader() instead, if it might happen.
    Assert(*ppPixelShader == NULL);

    IFCW32(hResourceInfo = FindResource(
        g_DllInstance,
        MAKEINTRESOURCE(uResourceId),
        RT_RCDATA
        ));

    IFCW32(hResource = LoadResource(
        g_DllInstance,
        hResourceInfo
        ));

    IFCW32(dwResourceSize = SizeofResource(
        g_DllInstance, 
        hResourceInfo
        ));

    // This method is nothing more than a cast, so we don't have to worry about error checking here
    pData = reinterpret_cast<const DWORD *>(LockResource(hResource));

    if (pData == NULL)
    {
        AssertMsg(0, "Error-couldn't load shader resource");
        IFC(E_FAIL);
    }

    IFC(CreatePixelShader(pData, dwResourceSize, ppPixelShader));

Cleanup:
    if (hResource)
    {
        GlobalUnlock(hResource);
    }
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreateVertexShaderFromResource
//
//  Synopsis:
//      Reads precompiled shader binary data from the resource of this
//      executable, pointed by argument uResourceId and creates a vertex
//      shader from the binary data.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CreateVertexShaderFromResource(
    D3DVertexType vertexType,
    UINT uResourceId,
    __deref_out_ecount(1) ID3D11VertexShader ** const ppVertexShader
    )
{
    HRESULT hr = S_OK;
    HGLOBAL hResource = NULL;
    HRSRC hResourceInfo = NULL;
    const DWORD *pData = NULL;
    DWORD dwResourceSize = 0;

    Assert(*ppVertexShader == NULL);

    IFCW32(hResourceInfo = FindResource(g_DllInstance, MAKEINTRESOURCE(uResourceId), RT_RCDATA));
    IFCW32(hResource = LoadResource(g_DllInstance, hResourceInfo));

    // This method is nothing more than a cast, so we don't have to worry about error checking here
    pData = reinterpret_cast<const DWORD *>(LockResource(hResource));

    if (pData == NULL)
    {
        AssertMsg(0, "Error-couldn't load shader resource");
        IFC(E_FAIL);
    }

    IFCW32(dwResourceSize = SizeofResource(
        g_DllInstance,
        hResourceInfo
    ));

    IFC(CreateVertexShader(vertexType, pData, dwResourceSize, ppVertexShader));

Cleanup:
    if (hResource)
    {
        GlobalUnlock(hResource);
    }
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CompilePipelineVertexShader
//
//  Synopsis:
//      Compiles a vertex shader from a string.
//

HRESULT
CD3DDeviceLevel1::CompilePipelineVertexShader(
    D3DVertexType vertexType,
    __in_bcount(cbHLSLSource) PCSTR pHLSLSource,
    UINT cbHLSLSource,
    __deref_out_ecount(1) ID3D11VertexShader ** const ppVertexShader
)
{
    MtSetDefault(Mt(D3DCompiler));

    std::shared_ptr<buffer> pShader;
    std::shared_ptr<buffer> pErr;

    std::string profile_name = 
        shader::get_vertex_shader_profile_name(m_pD3DDeviceContext);

    HRESULT hr =
        shader::compile(
            std::string(pHLSLSource, pHLSLSource + cbHLSLSource),
            "VertexShaderImpl",
            profile_name,
            D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY, 
            0,
            pShader, pErr);

    if (SUCCEEDED(hr))
    {
        hr = CreateVertexShader(vertexType, reinterpret_cast<DWORD*>(pShader->get_buffer_data().buffer), pShader->get_buffer_data().buffer_size, ppVertexShader);
    }

    if (FAILED(hr))
    {
        hr = shader::handle_errors_and_transform_hresult(hr, pErr);
    }

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CompilePipelinePixelShader
//
//  Synopsis:
//      Compiles a pixel shader from a string.
//

HRESULT
CD3DDeviceLevel1::CompilePipelinePixelShader(
    __in_bcount(cbHLSLSource) PCSTR pHLSLSource,
    UINT cbHLSLSource,
    __deref_out_ecount(1) ID3D11PixelShader ** const ppPixelShader
    )
{
    MtSetDefault(Mt(D3DCompiler));

    std::shared_ptr<buffer> pShader;
    std::shared_ptr<buffer> pErr;

    std::string profile_name 
        = shader::get_pixel_shader_profile_name(m_pD3DDeviceContext);

    HRESULT hr =
        shader::compile(
            std::string(pHLSLSource, pHLSLSource + cbHLSLSource),
            "PixelShaderImpl",
            profile_name,
            D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY, 
            0,
            pShader, pErr);

    if (SUCCEEDED(hr))
    {
        hr = CreatePixelShader(reinterpret_cast<DWORD*>(pShader->get_buffer_data().buffer), pShader->get_buffer_data().buffer_size, ppPixelShader);
    }

    if (!SUCCEEDED(hr))
    {
        hr = shader::handle_errors_and_transform_hresult(hr, pErr);
    }

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreateVertexShader
//
//  Synopsis:
//      Creates a vertex shader.
//

HRESULT
CD3DDeviceLevel1::CreateVertexShader(
    D3DVertexType vertexType,
    __in const DWORD *pdwfnVertexShader,
    __in DWORD cbVertexShader,
    __deref_out_ecount(1) ID3D11VertexShader ** const ppOutShader
    )
{
    HRESULT hr = S_OK;

    BEGIN_DEVICE_ALLOCATION;

    hr = m_pD3DDevice->CreateVertexShader(
        pdwfnVertexShader,
        cbVertexShader,
        nullptr,
        ppOutShader
        );

    END_DEVICE_ALLOCATION;

    if (SUCCEEDED(hr))
    {
        hr = EnsureInputLayout(vertexType, pdwfnVertexShader, cbVertexShader);
    }

    MIL_THR(hr); // placed here to avoid stack capturing multiple times in allocation loop

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreatePixelShader
//
//  Synopsis:
//      Creates a pixel shader.
//

HRESULT
CD3DDeviceLevel1::CreatePixelShader(
    __in const DWORD *pdwfnPixelShader,
    __in DWORD cbPixelShader,
    __deref_out_ecount(1) ID3D11PixelShader ** const ppOutShader
    )
{
    HRESULT hr = S_OK;

    BEGIN_DEVICE_ALLOCATION;

    hr = m_pD3DDevice->CreatePixelShader(
        pdwfnPixelShader,
        cbPixelShader,
        nullptr,
        ppOutShader
        );

    END_DEVICE_ALLOCATION;

    MIL_THR(hr); // placed here to avoid stack capturing multiple times in allocation loop

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::GetSupportedTextureFormat
//
//  Synopsis:
//      Given a source format and destination format select a format for a
//      texture
//
//      Return WGXERR_UNSUPPORTEDPIXELFORMAT if no acceptable format was found
//      to be supported.  See GatherSupportedTextureFormats for more details on
//      what is acceptable.
//

HRESULT
CD3DDeviceLevel1::GetSupportedTextureFormat(
    MilPixelFormat::Enum fmtBitmapSource,       // Current format of bitmap

    MilPixelFormat::Enum fmtDestinationSurface, // Format of surface onto which
                                          // the bitmap will be drawn
    bool fForceAlpha,                     // Use alpha in texture regardless
                                          // of whether the input used alpha.
    __out_ecount(1)
    MilPixelFormat::Enum &fmtTextureSource      // Format of the texture to
                                          // hold the bitmap
    ) const
{
    //
    // If the destination surface has higher precision than 8 bits per channel,
    // use high precision textures.  Typically, we expect 32bppBGR101010.
    //

    bool fUseAlpha = fForceAlpha || HasAlphaChannel(fmtBitmapSource);

    if (fmtDestinationSurface == MilPixelFormat::BGR32bpp101010)
    {
        //
        // Break down texture format based on source format
        //
        //  There are three possibilities for texture format:
        //      MilPixelFormat::BGR32bpp101010
        //      MilPixelFormat::RGB128bppFloat
        //      MilPixelFormat::PRGBA128bppFloat
        //

        if (fmtBitmapSource == MilPixelFormat::RGB128bppFloat)
        {
            // Special case MilPixelFormat::RGB128bppFloat
            fmtTextureSource = MilPixelFormat::PRGBA128bppFloat;
        }
        else
        {
            //
            // Check if MilPixelFormat::BGR32bpp101010 can handle the source
            // format.  There are two requirements:
            //  1) Pixel format size must be less than 32bppBGR101010
            //  2) There must not be an alpha channel
            //

            Assert(GetPixelFormatSize(MilPixelFormat::BGR32bpp101010) == 32);
            if ((GetPixelFormatSize(fmtBitmapSource) <= 32) && !fUseAlpha)
            {
                //
                // Convert to MilPixelFormat::BGR32bpp101010 as it has enough
                // precision and there is no alpha channel.
                //
                fmtTextureSource = MilPixelFormat::BGR32bpp101010;
            }
            else
            {
                //
                // Convert to MilPixelFormat::PRGBA128bppFloat so that we can retain
                // the alpha channel and/or enough precision.
                //
                //
                fmtTextureSource = MilPixelFormat::PRGBA128bppFloat;
            }
        }

        Assert(   (fmtTextureSource == MilPixelFormat::BGR32bpp101010)
               || (fmtTextureSource == MilPixelFormat::RGB128bppFloat)
               || (fmtTextureSource == MilPixelFormat::PRGBA128bppFloat)
               || (fmtTextureSource == MilPixelFormat::Undefined));
    }
    else
    {

        //
        // Convert formats to a 32bpp BGR format
        //   Note that this will cause precision loss if the source format has
        //   more that 8 bits per channel.
        //

        if (!fUseAlpha)
        {
            // No alpha channel => MilPixelFormat::BGR32bpp
            fmtTextureSource = MilPixelFormat::BGR32bpp;
        }
        else
        {
            // Alpha channel => MilPixelFormat::PBGRA32bpp
            fmtTextureSource = MilPixelFormat::PBGRA32bpp;
        }

        Assert(   (fmtTextureSource == MilPixelFormat::BGR32bpp)
               || (fmtTextureSource == MilPixelFormat::PBGRA32bpp)
               || (fmtTextureSource == MilPixelFormat::BGR32bpp101010)
               || (fmtTextureSource == MilPixelFormat::RGB128bppFloat)
               || (fmtTextureSource == MilPixelFormat::PRGBA128bppFloat)
               || (fmtTextureSource == MilPixelFormat::Undefined));
    }

    HRESULT hr = S_OK;

    if (fmtTextureSource == MilPixelFormat::Undefined)
    {
        MIL_THR(WGXERR_UNSUPPORTEDPIXELFORMAT);
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::GetSupportedMultisampleType
//
//  Synopsis:
//      Given a destination format select a multisample format
//

DXGI_SAMPLE_DESC
CD3DDeviceLevel1::GetSupportedMultisampleType(
    MilPixelFormat::Enum fmtDestinationSurface        // Format of target surface
    ) const
{
    DXGI_SAMPLE_DESC SampleDesc;
    
    SampleDesc.Count = 4;
    SampleDesc.Quality = 0;

    return SampleDesc;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::SetClipRect
//
//  Synopsis:
//      Provide access aligned clipping using SetViewport.
//      Note that the vieport will be reset on SetRenderTarget.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::SetClipRect(
    __in_ecount_opt(1) const CMILSurfaceRect *prcClip
    )
{
    HRESULT hr = S_OK;
    MilPointAndSizeL rcTargetSurface = {0, 0, m_desc.Width, m_desc.Height};
    MilPointAndSizeL rcSurfaceIntersectClip;
    const MilPointAndSizeL *prcNewClip = NULL;

    if (prcClip)
    {
        MilPointAndSizeL rcClip = {
            prcClip->left,
            prcClip->top,
            prcClip->right - prcClip->left,
            prcClip->bottom - prcClip->top
        };

        //
        // Determine the correct new clip rect by intersecting it with
        //  the target bounds and the given clip rect.
        //

        if (!IntersectRect(OUT &rcSurfaceIntersectClip, &rcTargetSurface, &rcClip))
        {
            hr = WGXHR_CLIPPEDTOEMPTY;
            goto Cleanup;
        }

        if (!IsClipSet(&rcSurfaceIntersectClip))
        {
            prcNewClip = &rcSurfaceIntersectClip;
        }
    }
    else

    {
        //
        // If clipping has been previously set then reset it to
        //  the full extents of the target.
        //

        if (IsClipSet())
        {
            prcNewClip = &rcTargetSurface;
        }
    }

    if (prcNewClip)
    {
        if (SupportsScissorRect())
        {
            IFC(SetScissorRect(prcNewClip));
        }
        else
        {
            IFC(SetViewport(prcNewClip));

            IFC(SetSurfaceToClippingMatrix(
                prcNewClip
                ));
        }

        SetClipSet(prcClip != NULL);
        SetClip(*prcNewClip); // don't really need this if !m_fClipSet

        EventWriteSetClipInfo(prcNewClip->X,
                              prcNewClip ->Y,
                              prcNewClip->Width,
                              prcNewClip->Height);
    }

Cleanup:
    RRETURN1(hr, WGXHR_CLIPPEDTOEMPTY);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::GetClipRect
//
//  Synopsis:
//      Return the current clip rect.
//
//------------------------------------------------------------------------------
void
CD3DDeviceLevel1::GetClipRect(
    __out_ecount(1) MilPointAndSizeL * const prcClipRect
    ) const
{
    if (IsClipSet())
    {
        *prcClipRect = GetClip();
    }
    else
    {
        //
        // If not set then there is no clip which is equivalent
        // to a clip exactly the size of the target.
        //

        prcClipRect->X = 0;
        prcClipRect->Y = 0;
        prcClipRect->Width = m_desc.Width;
        prcClipRect->Height = m_desc.Height;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::DbgTraceDeviceCreationFailure
//
//  Synopsis:
//      Output information about the device creation failure
//
//------------------------------------------------------------------------------
#if DBG==1
void
CD3DDeviceLevel1::DbgTraceDeviceCreationFailure(
    UINT uAdapter,
    __in PCSTR szMessage,
    HRESULT hrError
    )
{
    TraceTag((tagError, "MIL-HW(adapter=%d): Can't create d3d rendering device.", uAdapter));
    TraceTag((tagError, "MIL-HW(adapter=%d): %s (hr = 0x%x).", uAdapter, szMessage, hrError));
}
#endif

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CreateDepthBuffer
//
//  Synopsis:
//      delegate to D3DDeviceContext::CreateDepthStencilBuffer
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CreateDepthBuffer(
    UINT uWidth,
    UINT uHeight,
    UINT uiMultisampleCount,
    UINT uiMultisampleQuality,
    __deref_out_ecount(1) CD3DTexture** const ppTexture
)
{
    HRESULT hr = S_OK;
    D3DTexture* pD3DTexture = NULL;


    //   What depth buffer should be used
    //  since we no longer need the stencil.  Is 16 or 32 better than 24?

    {
        MtSetDefault(Mt(D3DStencilSurface));

        BEGIN_DEVICE_ALLOCATION;

        ID3D11Texture2D* pDepthStencil = NULL;
        D3D11_TEXTURE2D_DESC descDepth;
        descDepth.Width = uWidth;
        descDepth.Height = uHeight;
        descDepth.MipLevels = 1;
        descDepth.ArraySize = 1;
        descDepth.Format = kD3DDepthFormat;
        descDepth.SampleDesc.Count = uiMultisampleCount;
        descDepth.SampleDesc.Quality = uiMultisampleQuality;
        descDepth.Usage = D3D11_USAGE_DEFAULT;
        descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        descDepth.CPUAccessFlags = 0;
        descDepth.MiscFlags = 0;
        hr = m_pD3DDevice->CreateTexture2D(&descDepth, NULL, &pD3DTexture);

        END_DEVICE_ALLOCATION;

        IFC(hr); // placed here to avoid jumping to cleanup immediately on OOVM
    }

    CD3DVidMemOnlyTexture* pVidMemOnlyTexture = nullptr;
    IFC(CD3DVidMemOnlyTexture::Create(pD3DTexture, false, this, &pVidMemOnlyTexture));
    *ppTexture = pVidMemOnlyTexture;

Cleanup:
    ReleaseInterfaceNoNULL(pD3DTexture);
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::SetDepthStencilTexture
//
//  Synopsis:
//      Turns ZEnable on and off and delegates to
//      D3DDeviceContext::SetDepthStencilSurface if pSurface is non null
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::SetDepthStencilTexture(
    __in_ecount_opt(1) CD3DTexture *pTexture
    )
{
    HRESULT hr = S_OK;

    //
    // Enable or disable z-buffer
    //

    if (pTexture)
    {
        IFC(SetRenderState(
            D3DRS_ZENABLE,
            D3DZB_TRUE
            ));
    }
    else
    {
        IFC(SetRenderState(
            D3DRS_ZENABLE,
            D3DZB_FALSE
            ));

        IFC(SetRenderState(
            D3DRS_STENCILENABLE,
            false
            ));
    }

    //
    // Set the z-buffer if necessary
    //

    Assert(m_pCurrentRenderTargetNoRef);
    m_pDepthStencilBufferForCurrentRTNoRef = pTexture;

    IFC(SetDepthStencilTextureInternal(pTexture));

Cleanup:
    if (FAILED(hr))
    {
        // We might as well try to turn off clipping in case of failure
        // Otherwise who knows what the clipping state will be
        IGNORE_HR(SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE)); // HRESULT ignored
        IGNORE_HR(SetRenderState(D3DRS_STENCILENABLE, D3DZB_FALSE)); // HRESULT ignored
        IGNORE_HR(SetDepthStencilTextureInternal(NULL));
    }
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::ReleaseUseOfDepthStencilSurface
//
//  Synopsis:
//       Releases any cached use the device may have of the given
//      depth stencil buffer, thereby enabling the D3D surface to be cleaned up
//      when it is truely no longer in use.
//
//------------------------------------------------------------------------------
void
CD3DDeviceLevel1::ReleaseUseOfDepthStencilTexture(
    __in_ecount_opt(1) CD3DTexture* pTexture    // a surface that will no
                                                // longer be valid
    )
{
    AssertDeviceEntry(*this);

    if (pTexture)
    {
        //
        // It's possible for the state manager to have a different
        // depth/stencil buffer than what this has tracked as d/s buffer for
        // current RT.  For example SetDepthStencilSurface can fail and leave
        // m_pDepthStencilBufferForCurrentRTNoRef in an inaccurate state.  So
        // always call to state manager.
        //

        IGNORE_HR(ReleaseUseOfDepthStencilTextureInternal(pTexture));

        //
        // If "current" depth/stencil is released then note that there is no
        // d/s buffer for current RT.
        //

        if (m_pDepthStencilBufferForCurrentRTNoRef == pTexture)
        {
            m_pDepthStencilBufferForCurrentRTNoRef = NULL;
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::DrawIndexedTriangleListUP
//
//  Synopsis:
//      This method creates a fast path for our XYZDUV2 vertices which will use
//      custom VB/IB code instead of DrawIndexedPrimUP.
//
//      Note that the main advantage here is that we bypass the
//      lowvertexcount limit in d3d9 which is set too low at 96.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::DrawIndexedTriangleListUP(
    __range(1, UINT_MAX) UINT uNumVertices,
    __range(1, UINT_MAX) UINT uPrimitiveCount,
    __in_xcount(sizeof(WORD) * uPrimitiveCount * 3) const WORD* pIndexData,
    __in const void* pVertexStreamZeroData,
    __range(1, UINT_MAX) UINT uVertexStreamZeroStride
    )
{
    HRESULT hr = S_OK;
    UINT uiNumIndices = uPrimitiveCount*3;
    void *pvDataVertices = NULL;
    void *pvDataIndices = NULL;
    bool fVBLockAcquired = false;
    bool fIBLockAcquired = false;
    UINT uiCurrentVertex = 0;
    UINT uiCurrentIndex = 0;

    Assert(uNumVertices > 0);
    Assert(uPrimitiveCount > 0);
    Assert(uVertexStreamZeroStride > 0);

    //
    // Add bandwidth contribution
    //

    if (g_pMediaControl)
    {
        const WORD *pIndex = static_cast<const WORD *>(pIndexData);
        const CD3DVertexXYZDUV2 *pVertices = static_cast<const CD3DVertexXYZDUV2 *>(pVertexStreamZeroData);

        for (UINT i = 0; i < uPrimitiveCount; i++)
        {
            CD3DVertexXYZDUV2 v1 = pVertices[pIndex[0]];
            CD3DVertexXYZDUV2 v2 = pVertices[pIndex[1]];
            CD3DVertexXYZDUV2 v3 = pVertices[pIndex[2]];

            //  Area = abs((xB*yA-xA*yB)+(xC*yB-xB*yC)+(xA*yC-xC*yA))/2
            float rArea = fabs((v2.X*v1.Y-v1.X*v2.Y) + (v3.X*v2.Y-v2.X*v3.Y) + (v1.X*v3.Y-v3.X*v1.Y))/2.0f;
            g_lPixelsFilledPerFrame += CFloatFPU::Ceiling(rArea);

            pIndex += 3;
        }
    }

    //
    // Try to lock both the IB/VB for the fast path case
    //

    hr = THR(m_pHwVertexBuffer->Lock(
        uNumVertices,
        uVertexStreamZeroStride,
        &pvDataVertices,
        &uiCurrentVertex
        ));

    if (SUCCEEDED(hr))
    {
        fVBLockAcquired = true;

        hr = THR(m_pHwIndexBuffer->Lock(
            uiNumIndices,
            reinterpret_cast<WORD **>(&pvDataIndices),
            &uiCurrentIndex
            ));

        fIBLockAcquired = SUCCEEDED(hr);
    }

    if (!fVBLockAcquired || !fIBLockAcquired)
    {
        AssertMsg(0, "Lock should not have failed");
        goto Cleanup;
    }

    //
    // Update vertices
    //

    RtlCopyMemory(pvDataVertices, pVertexStreamZeroData, uNumVertices*uVertexStreamZeroStride);

    IFC(m_pHwVertexBuffer->Unlock(uNumVertices));
    fVBLockAcquired = false;

    //
    // Update indices
    //

    RtlCopyMemory(pvDataIndices, pIndexData, sizeof(short)*uiNumIndices);

    IFC(m_pHwIndexBuffer->Unlock());
    fIBLockAcquired = false;

    //
    // Lots of dx device methods (like DrawPrimUp/DrawIndexedPrimUp)
    // will change the current IB/VB.  So, we either need to track all of
    // those or set it on every draw call.
    //
    // It turns out that d3d has a fast path when the IB/VB don't change,
    // so setting it on each draw isn't costing us much.
    //

    IFC(SetVertexBuffer(
        m_pHwVertexBuffer->GetD3DBuffer(),
        uVertexStreamZeroStride
        ));

    IFC(SetIndices(
        m_pHwIndexBuffer->GetD3DBuffer()
        ));

    //
    // Call DrawIndexedPrimitive
    //

    {
        MtSetDefault(Mt(D3DDrawIndexedPrimitive));
        m_pD3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_pD3DDeviceContext->DrawIndexed(
            uPrimitiveCount * 3,
            uiCurrentIndex,
            uiCurrentVertex
        );
    }

    UpdateMetrics(uNumVertices, uPrimitiveCount);

Cleanup:
    if (fVBLockAcquired)
    {
        IGNORE_HR(m_pHwVertexBuffer->Unlock(uNumVertices));
    }
    if (fIBLockAcquired)
    {
        IGNORE_HR(m_pHwIndexBuffer->Unlock());
    }

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::DrawIndexedTriangleList
//
//  Synopsis:
//      Draw whatever is set in the current stream as an indexed triangle list.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::DrawIndexedTriangleList(
    UINT uBaseVertexIndex,
    UINT uMinIndex,
    UINT cVertices,
    UINT uStartIndex,
    UINT cPrimitives
    )
{
    HRESULT hr = S_OK;
    MtSetDefault(Mt(D3DDrawIndexedPrimitive));

    m_pD3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pD3DDeviceContext->DrawIndexed(
        cPrimitives * 3,
        uStartIndex,
        uMinIndex
    );

    UpdateMetrics(cVertices, cPrimitives);

Cleanup:
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::DrawTriangleList
//
//  Synopsis:
//      Draw whatever is set in the current stream as a triangle list.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::DrawTriangleList(
    UINT uStartVertex,
    UINT cPrimitives
    )
{
    HRESULT hr = S_OK;
    MtSetDefault(Mt(D3DDrawPrimitive));

    m_pD3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pD3DDeviceContext->Draw(
        cPrimitives*3,
        uStartVertex
        );

    UpdateMetrics(cPrimitives*3, cPrimitives);

Cleanup:
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::DrawTriangleStrip
//
//  Synopsis:
//      Draw whatever is set in the current stream as a triangle strip.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::DrawTriangleStrip(
    UINT uStartVertex,
    UINT cPrimitives
    )
{
    HRESULT hr = S_OK;
    MtSetDefault(Mt(D3DDrawPrimitive));

    m_pD3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_pD3DDeviceContext->Draw(
        cPrimitives + 2,
        uStartVertex
    );

    UpdateMetrics(cPrimitives + 2, cPrimitives);

Cleanup:
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CopyD3DTexture
//
//  Synopsis:
//      Does a non-filtered copy of all a source texture contents into all of a
//      destination texture contents.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::CopyD3DTexture(
    __in_ecount(1) D3DTexture *pD3DSourceTexture,
    __inout_ecount(1) D3DTexture *pD3DDestinationTexture
    )
{
    HRESULT hr = S_OK;

    m_pD3DDeviceContext->CopyResource(pD3DDestinationTexture, pD3DSourceTexture);

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::DrawPrimitiveUP
//
//  Synopsis:
//      This method creates a fast path for our XYZDUV2 vertices which will use
//      custom VB code instead of DrawPrimUP.
//
//      Note that the main advantage here is that we bypass the
//      lowvertexcount limit in d3d9 which is set too low at 96.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::DrawPrimitiveUP(
    D3D11_PRIMITIVE_TOPOLOGY primitiveType,
    UINT uPrimitiveCount,
    __in_xcount(
        //
        // Vertex counts are:
        //
        // D3D11_PRIMITIVE_TOPOLOGY_LINELIST - PrimitiveCount*2
        // D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP - PrimitiveCount+2
        //
        uVertexStreamZeroStride * ((primitiveType == D3DPT_LINELIST) ? uPrimitiveCount*2 : uPrimitiveCount+2)
        ) const void* pVertexStreamZeroData,
    UINT uVertexStreamZeroStride
    )
{
    HRESULT hr = S_OK;
    void *pvData = NULL;
    UINT uNumVertices = 0;
    bool fLockAcquired = false;
    UINT uiCurrentVertex = 0;

    Assert(uPrimitiveCount > 0);
    Assert(pVertexStreamZeroData);
    Assert(uVertexStreamZeroStride > 0);

    //
    // Compute number of vertices for supported types
    //

    switch (primitiveType)
    {
    case D3D11_PRIMITIVE_TOPOLOGY_LINELIST:
        uNumVertices = uPrimitiveCount*2;
        break;

    case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
        uNumVertices = uPrimitiveCount*3;
        break;

    case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
        {
            uNumVertices = uPrimitiveCount+2;

            //
            // Add bandwidth contribution
            //
            if (g_pMediaControl)
            {
                const CD3DVertexXYZDUV2 *pVertices =
                    static_cast<const CD3DVertexXYZDUV2 *>(pVertexStreamZeroData);

                for (UINT i = 0; i < uPrimitiveCount; i++)
                {
                    CD3DVertexXYZDUV2 v1 = pVertices[0];
                    CD3DVertexXYZDUV2 v2 = pVertices[1];
                    CD3DVertexXYZDUV2 v3 = pVertices[2];

                    //  Area = abs((xB*yA-xA*yB)+(xC*yB-xB*yC)+(xA*yC-xC*yA))/2
                    float rArea = fabs((v2.X*v1.Y-v1.X*v2.Y) + (v3.X*v2.Y-v2.X*v3.Y) + (v1.X*v3.Y-v3.X*v1.Y))/2.0f;
                    g_lPixelsFilledPerFrame += CFloatFPU::Ceiling(rArea);

                    pVertices += 1;
                }
            }
            break;
        }

    default:
        AssertMsg(0, "Unsupported primitive type");
        IFC(E_INVALIDARG);
    }

    //
    // Try to lock both the IB/VB for the fast path case
    //

    hr = THR(m_pHwVertexBuffer->Lock(
        uNumVertices,
        uVertexStreamZeroStride,
        &pvData,
        &uiCurrentVertex
        ));

    fLockAcquired = SUCCEEDED(hr);

    //
    // Update vertices
    //

    RtlCopyMemory(pvData, pVertexStreamZeroData, uNumVertices*uVertexStreamZeroStride);

    IFC(m_pHwVertexBuffer->Unlock(uNumVertices));
    fLockAcquired = false;

    //
    // Lots of dx device methods (like DrawPrimUp/DrawIndexedPrimUp)
    // wil change the current set IB/VB.  So, we either need to track all of
    // those or set it on every draw call.
    //
    // It turns out that d3d has a fast path when the IB/VB don't change,
    // so setting it on each draw isn't costing us much.
    //

    IFC(SetVertexBuffer(
        m_pHwVertexBuffer->GetD3DBuffer(),
        uVertexStreamZeroStride
        ));

    //
    // Call DrawPrimitive
    //

    {
        MtSetDefault(Mt(D3DDrawPrimitive));

        m_pD3DDeviceContext->IASetPrimitiveTopology(primitiveType);

        m_pD3DDeviceContext->Draw(
            uNumVertices,
            uiCurrentVertex
            );
    }

    UpdateMetrics(uNumVertices, uPrimitiveCount);

Cleanup:
    if (fLockAcquired)
    {
        IGNORE_HR(m_pHwVertexBuffer->Unlock(uNumVertices));
    }

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::DrawVideoToSurface
//
//  Synopsis:
//      Takes an IAVSurfaceRenderer and gives it the D3D device that it can use
//      to Draw the Video frame. In special cases, it draws directly to the
//      backbuffer (CD3DSwapChain) in the specified destination rectangle.
//      Otherwise it uses an intermediate surface which is returned in
//      IWGXBitmapSource. Saves device state before calling to get the frame.
//      Restores state after call is done.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::DrawVideoToSurface(
    __in_ecount(1) IAVSurfaceRenderer *pSurfaceRenderer,
    __deref_opt_out_ecount(1) IWGXBitmapSource ** const ppBitmapSource
    )
{
    HRESULT hr = S_OK;

    Assert(m_pD3DDevice);

    AssertDeviceEntry(*this);

    //
    // Get the next frame here
    // This MUST be the very last HResult altering call in DrawVideoToSurface,
    // since the caller of this assumes that it must call EndRender if this
    // succeeds.
    //
    IFC(pSurfaceRenderer->BeginRender(this, ppBitmapSource));

Cleanup:

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::CheckDeviceState
//
//  Synopsis:
//      Checks current state of the device for the hwnd.
//

HRESULT
CD3DDeviceLevel1::CheckDeviceState(
    __in_opt HWND hwnd
    )
{
    HRESULT hr = S_OK;

    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::WaitForVBlank
//
//  Synopsis:
//      Waits until a vblank occurs on the specified swap chain
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::WaitForVBlank(UINT uSwapChainIndex)
{
    BEGIN_MILINSTRUMENTATION_HRESULT_LIST_WITH_DEFAULTS
        WGXERR_NO_HARDWARE_DEVICE
    END_MILINSTRUMENTATION_HRESULT_LIST

    HRESULT hr = WGXERR_NO_HARDWARE_DEVICE;

  Cleanup:
    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::ResetMarkers
//
//  Synopsis:
//      Deletes all markers. called when device is lost
//
//------------------------------------------------------------------------------
void
CD3DDeviceLevel1::ResetMarkers()
{
    m_ullLastConsumedMarkerId = m_ullLastMarkerId;
    UINT uIndex;
    UINT count = m_rgpMarkerFree.GetCount();
    for (uIndex = 0; uIndex < count ; uIndex++)
    {
        delete m_rgpMarkerFree[uIndex];
    }
    m_rgpMarkerFree.Reset();

    count = m_rgpMarkerActive.GetCount();
    for (uIndex = 0; uIndex < count;  uIndex++)
    {
        delete m_rgpMarkerActive[uIndex];
    }
    m_rgpMarkerActive.Reset();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::ConsumePresentMarkers
//
//  Synopsis:
//      Walk through the array of markers testing and freeing markers that have
//      been consumed.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::ConsumePresentMarkers(
    bool fForceFlush
    )
{
    HRESULT hr = S_OK;

    Assert(m_rgpMarkerActive.GetCount() < INT_MAX);

    for (int i = static_cast<int>(m_rgpMarkerActive.GetCount()) - 1; i >= 0; i--)
    {
        bool fMarkerConsumed = false;

        Assert(i < static_cast<int>(m_rgpMarkerActive.GetCount()));

        IFC(IsConsumedGPUMarker(
            i,
            fForceFlush,
            &fMarkerConsumed
            ));

        if (fMarkerConsumed)
        {
            IFC(FreeMarkerAndItsPredecessors(i));

            //
            // We've found the most recent marker that's been consumed and freed
            // all those before it.  We don't need to walk through the list anymore.
            //
            break;
        }

        //
        // Once a flush has occurred, another won't give us any more information,
        // so don't pay for its cost more than once.
        //
        if (fForceFlush)
        {
            fForceFlush = false;
            m_uNumSuccessfulPresentsSinceMarkerFlush = 0;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::AdvanceFrame
//
//  Synopsis:
//      If given a frame counter different from the last one then tells resource
//      manager to reclaim frame resources.
//
//------------------------------------------------------------------------------
void
CD3DDeviceLevel1::AdvanceFrame(UINT uFrameNumber)
{
    if (m_uFrameNumber != uFrameNumber)
    {
        m_uFrameNumber = uFrameNumber;

        m_resourceManager.EndFrame();
        m_resourceManager.DestroyReleasedResourcesFromLastFrame();
        m_resourceManager.DestroyResources(CD3DResourceManager::WithDelay);
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::GetNumQueuedPresents
//
//  Synopsis:
//      If GPUMarkers are enabled it walks through the array of markers testing
//      if they've been consumed and returns the number of outstanding markers.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::GetNumQueuedPresents(
    __out_ecount(1) UINT *puNumQueuedPresents
    )
{
    HRESULT hr = S_OK;

    *puNumQueuedPresents = 0;

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::InsertGPUMarker
//
//  Synopsis:
//      Inserts a marker into the GPU command stream The marker ID must be
//      greater than any previous id used.  QPC timer values are expected to be
//      used
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::InsertGPUMarker(
    ULONGLONG ullMarkerId
    )
{
    HRESULT hr = S_OK;
    CGPUMarker *pMarker = NULL;

    //
    // Our Markers should always be in increasing order, but there are occasions
    // when we can receive a marker out of order.  These include coming back from
    // a locked desktop, where QueryPerformanceCounter doesn't behave properly.
    //
    if (ullMarkerId < m_ullLastMarkerId)
    {
        TraceTag((tagError, "Warning: GPUMarker Received out of increasing order.  Ignoring it."));
        goto Cleanup;
    }

    if (!m_pD3DDevice)
    {
        goto Cleanup;
    }

    //
    // If markers haven't been tested do it.
    //
    if (!AreGPUMarkersTested())
    {
        D3D11_QUERY_DESC desc;
        desc.Query = D3D11_QUERY_EVENT;
        desc.MiscFlags = 0;
        MIL_THR(m_pD3DDevice->CreateQuery(
            &desc,
            NULL
            ));

        //
        // If we fail in the creation of a query, disable markers.
        //
        if (SUCCEEDED(hr))
        {
            SetGPUMarkersAsEnabled();
        }

        SetGPUMarkersAsTested();
    }

    Assert(AreGPUMarkersTested());

    //
    // If markers are disabled on this device just return
    //
    if (!AreGPUMarkersEnabled())
    {
        m_ullLastMarkerId = ullMarkerId;
        goto Cleanup;
    }

    if (m_rgpMarkerFree.GetCount() > 0)
    {
        UINT uIndex = m_rgpMarkerFree.GetCount()-1;
        pMarker = m_rgpMarkerFree[uIndex];
        m_rgpMarkerFree.SetCount(uIndex);
        pMarker->Reset(ullMarkerId);
    }
    else
    {
        ID3D11Query* pQuery = NULL;
        D3D11_QUERY_DESC desc;
        desc.Query = D3D11_QUERY_EVENT;
        desc.MiscFlags = 0;
        IFC(m_pD3DDevice->CreateQuery(&desc, &pQuery));
        pMarker = new CGPUMarker(pQuery, ullMarkerId);
        IFCOOM(pMarker);
    }

    IFC(pMarker->InsertIntoCommandStream(m_pD3DDeviceContext));
    IFC(m_rgpMarkerActive.Add(pMarker));

    pMarker = NULL;

  Cleanup:
    delete pMarker;

    //
    // If we have a backlog of active markers it probably means the hardware isn't reporting
    // the queries properly.  Turn off markers for this device.
    //
    if (m_rgpMarkerActive.GetCount() > GPU_MARKERS_MAX_ARRAY_SIZE)
    {
        TraceTag((tagError, "Backlog of unconsumed markers in the device, turning marking checking off."));

        DisableGPUMarkers();
    }

    if (D3DERR_DEVICELOST == hr
     || D3DERR_NOTAVAILABLE == hr)
    {
        hr = S_OK;
    }

    //
    //  No other HRESULTs are expected, but this code is fairly new,
    // and no harm is done by ignoring them (will just shut ourselves off).
    //
    if (FAILED(hr))
    {
        hr = S_OK;
        DisableGPUMarkers();
    }

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::IsConsumedGPUMarker
//
//  Synopsis:
//      Determines if the marker has been consumed.
//
//  Returns:
//      If the marker was consumed or we can't confirm it was explicitly
//         NOT consumed *pfMarkersConsumed will be true.  If we can
//         confirm it wasn't consumed, it will be false.
//
//------------------------------------------------------------------------------
HRESULT CD3DDeviceLevel1::IsConsumedGPUMarker(
    UINT uMarkerIndex,
    bool fFlushMarkers,
    __out_ecount(1) bool *pfMarkerConsumed
    )
{
    HRESULT hr = S_OK;

    Assert(m_rgpMarkerActive.GetCount() > 0);
    Assert(AreGPUMarkersEnabled());
    Assert(uMarkerIndex < m_rgpMarkerActive.GetCount());

    MIL_THR(m_rgpMarkerActive[uMarkerIndex]->CheckStatus(
        m_pD3DDeviceContext,
        fFlushMarkers,
        pfMarkerConsumed
        ));

    //
    // If we recieve device lost then the card is no longer rendering our content.
    // Assume the marker has been consumed, and reinterpret the HRESULT.
    //
    if (hr == D3DERR_DEVICELOST)
    {
        *pfMarkerConsumed = true;
        hr = S_OK;
    }

    if (SUCCEEDED(hr))
    {
        if (*pfMarkerConsumed == true)
        {
            SetGPUMarkersAsConsumed();
        }
    }

    //
    //  No other HRESULTs are expected, but this code is fairly new,
    // and no harm is done by ignoring them (will just shut ourselves off).
    //
    if (FAILED(hr))
    {
        hr = S_OK;
        DisableGPUMarkers();
        *pfMarkerConsumed = true;
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceLevel1::FreeMarkerAndItsPredecessors
//
//  Synopsis:
//      Move the marker, and all markers below it into the Free list.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::FreeMarkerAndItsPredecessors(
    UINT uIndex
    )
{
    HRESULT hr = S_OK;

    UINT cConsumed = uIndex+1;
    UINT cNew  = m_rgpMarkerActive.GetCount() - cConsumed;
    UINT ui;

    // this marker was consumed
    // Update the last consumed id
    m_ullLastConsumedMarkerId = m_rgpMarkerActive[uIndex]->GetId();

    // remove it and all those with lower ids to the free list
    for (ui = 0; ui < cConsumed; ui++)
    {
        IFC(m_rgpMarkerFree.Add(m_rgpMarkerActive[ui]));
        m_rgpMarkerActive[ui] = NULL;
    }
    // Shift the unconsumed entries to the begining.
    for (ui = 0; ui < cNew; ui++)
    {
        m_rgpMarkerActive[ui] = m_rgpMarkerActive[cConsumed + ui];
    }
    m_rgpMarkerActive.SetCount(cNew);

Cleanup:
    RRETURN(hr);
}

#if DBG
//+-----------------------------------------------------------------------------
//
//  Function:
//      DbgInjectDIE
//
//  Synopsis:
//      If tagInjectDIE is enabled sometimes change S_OK to
//      D3DERR_DRIVERINTERNALERROR for fault testing purposes.
//
//------------------------------------------------------------------------------
void
DbgInjectDIE(
    __inout_ecount(1) HRESULT * const pHR
    )
{
    if (IsTagEnabled(tagInjectDIE))
    {
        const unsigned c_uInjectionRate = 200;
        static unsigned s_count = 0;

        if (    SUCCEEDED(*pHR)
                && !(++s_count%c_uInjectionRate))
        {
            TraceTag((tagError, "MIL-HW: Injecting D3DERR_DRIVERINTERNALERROR!."));
            *pHR = D3DERR_DRIVERINTERNALERROR;
        }
    }
}
#endif


//+-----------------------------------------------------------------------------
//
//  Function:
//      PrepareShaderEffectPipeline
//
//  Synopsis:
//      Configures the device for running a pixel shader effect.
//      (See ImageEffect).
//
//      Returns a scratch vertex buffer to use for the shader pipeline with
//      this device.
//
//------------------------------------------------------------------------------


HRESULT
CD3DDeviceLevel1::PrepareShaderEffectPipeline()
{
    HRESULT hr = S_OK;
    CD3DVertexXYZDUV2* pVertices = NULL;

    ID3D11VertexShader *pVertexShader = NULL;
    if (m_pEffectPipelineVertexShader == NULL)
    {
        IFC(CreateVertexShaderFromResource(CD3DVertexXYZDUV2::Format(), VS_ShaderEffects30, &m_pEffectPipelineVertexShader));
    }

    pVertexShader = m_pEffectPipelineVertexShader;

    if (m_pEffectPipelineVertexBuffer == NULL)
    {
        IFC(CreateVertexBuffer(
            4 * sizeof(CD3DVertexXYZDUV2),
            D3D11_USAGE_DYNAMIC,
            D3D11_CPU_ACCESS_WRITE,
            &m_pEffectPipelineVertexBuffer));
    }

    IFC(SetVertexShader(pVertexShader));
    IFC(SetInputLayoutFormat(CD3DVertexXYZDUV2::Format()));

    D3D11_MAPPED_SUBRESOURCE mappedSubresource;
    IFC(m_pD3DDeviceContext->Map(m_pEffectPipelineVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource));
    pVertices = reinterpret_cast<CD3DVertexXYZDUV2*>(mappedSubresource.pData);

    //   1---3
    //   | \ |
    //   |  \|
    //   0---2
    pVertices[0].SetXYUV0(0, 1.0f, 0, 1.0f);
    pVertices[1].SetXYUV0(0, 0, 0, 0);
    pVertices[2].SetXYUV0(1.0f, 1.0f, 1.0f, 1.0f);
    pVertices[3].SetXYUV0(1.0f, 0, 1.0f, 0);

    m_pD3DDeviceContext->Unmap(m_pEffectPipelineVertexBuffer, 0);

    pVertices = NULL;
    IFC(hr);

    IFC(SetVertexBuffer(m_pEffectPipelineVertexBuffer, sizeof(CD3DVertexXYZDUV2)));

Cleanup:
    if (pVertices)
    {
        m_pD3DDeviceContext->Unmap(m_pEffectPipelineVertexBuffer, 0);
    }

    RRETURN(HandleDIE(hr));
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      SetPassThroughPixelShader
//
//  Synopsis:
//      Sets the shader effect pipeline pixel shader to the
//      default implementation (pass-through).
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceLevel1::SetPassThroughPixelShader()
{
    HRESULT hr = S_OK;

    if (m_pEffectPipelinePassThroughPixelShader == NULL)
    {
        IFC(CreatePixelShaderFromResource(PS_PassThroughShaderEffect, &m_pEffectPipelinePassThroughPixelShader));
    }

    IFC(SetPixelShader(m_pEffectPipelinePassThroughPixelShader));

Cleanup:
    RRETURN(hr);
}

