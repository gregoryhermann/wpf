// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      Contains CD3DSwapChain implementation
//

MtExtern(CD3DSwapChain);
MtExtern(D3DResource_SwapChain);

//------------------------------------------------------------------------------
//
//  Class: CD3DSwapChain
//
//  Description:
//     Abstracts the core D3D swapchain.  The main reason to wrap this
//     d3d object is so that we can respond to mode changes, i.e.,   
//     respond to E_DEVICELOST on the Present.
//
//------------------------------------------------------------------------------

class CD3DSwapChain : public CD3DResource
{
    // We present through CD3DDeviceLevel1 instead of the CD3DSwap chain so that
    // we can have CD3DDeviceLevel1 internally call Begin/EndScene instead of
    // exposing this on the CD3DDeviceLevel1 interface

    friend HRESULT CD3DDeviceLevel1::Present(
        __in_ecount(1) CD3DSwapChain const *pD3DSwapChain,
        __in_ecount_opt(1) CMILSurfaceRect const *prcSource,
        __in_ecount_opt(1) CMILSurfaceRect const *prcDest,
        __in_ecount(1) CMILDeviceContext const *pMILDC,
        __in_ecount_opt(1) RGNDATA const * pDirtyRegion,
        DWORD dwD3DPresentFlags
        );

public:
    IDXGISwapChain *GetD3DSwapChainNoRef() { return m_pDXGISwapChain; }

protected:

    inline void __cdecl operator delete(void * pv) { WPFFree(ProcessHeap, pv); }
    __allocator __bcount_opt(cb + cBackBuffers*sizeof(PVOID))
        void * __cdecl operator new(size_t cb, size_t cBackBuffers);
    inline void __cdecl operator delete(void* pv, size_t) { WPFFree(ProcessHeap, pv); }

public:

    static HRESULT Create(
        __inout_ecount(1) CD3DDeviceLevel1 *pDevice,
        __inout_ecount(1) IDXGISwapChain *pDXGISwapChain,
        __in_ecount_opt(1) CMILDeviceContext const *pPresentContext,        
        __deref_out_ecount(1) CD3DSwapChain **ppSwapChain
        );

    //
    // IDirect3DSwapChain9 like helper methods
    //
    
    HRESULT GetBackBuffer(
        __in_range(<, this->m_cBackBuffers) UINT iBackBuffer,
        __deref_out_ecount(1) CD3DTexture **ppBackBuffer
        ) const;

    virtual HRESULT GetDC(
        __in_range(<, this->m_cBackBuffers) UINT iBackBuffer,
        __in_ecount(1) const CMilRectU& rcDirty,
        __deref_out HDC *phdcBackBuffer
        ) const;

    virtual HRESULT ReleaseDC(
        __in_range(<, this->m_cBackBuffers) UINT iBackBuffer,
        __in HDC hdcBackBuffer
        ) const;  

#if DBG
    UINT DbgGetNumBackBuffers() const
    {
        return m_cBackBuffers;
    }
#endif

protected:
    CD3DSwapChain(
        __inout_ecount(1) IDXGISwapChain *pDXGISwapChain,
        __in_range(>, 0) __out_range(==, this->m_cBackBuffers) UINT cBackBuffers);
    virtual ~CD3DSwapChain();

protected:
    virtual HRESULT Init(
        __inout_ecount(1) CD3DDeviceLevel1* pDevice);

private:

#if PERFMETER
    virtual PERFMETERTAG GetPerfMeterTag() const
    {
        return Mt(D3DResource_SwapChain);
    }
#endif


    // 
    // CD3DResource methods
    //

    void ReleaseD3DResources();

private:
    IDXGISwapChain *m_pDXGISwapChain;
    CD3DDeviceLevel1* m_pDevice;
    UINT m_cBackBuffers;
};


