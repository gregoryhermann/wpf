// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_text
//      $Keywords:
//
//  $Description:
//      Contains lockable D3D texture class declaration and the stack based lock
//      helper class
//
//      Contains also CD3DLockableTexturePair class declaration and stack based
//      lock helper class for it.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CD3DLockableTexture);
MtExtern(D3DResource_LockableTexture);

class CD3DTexture;

class CD3DLockableTexture : public CD3DTexture
{
protected:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CD3DLockableTexture));

public:
    DEFINE_RESOURCE_REF_COUNT_BASE

    static HRESULT Create(
        __inout_ecount(1) CD3DResourceManager *pResourceManager,
        __in_ecount(1) CD3DDeviceLevel1* pDevice,
        __in_ecount(1) D3DTexture *pD3DTexture,
        __deref_out_ecount(1) CD3DLockableTexture **ppD3DLockableTexture
        );

    // Only call this method if you are certain that we have a system memory
    // texture.
    HRESULT LockRect(
        __out_ecount(1) D3DLOCKED_RECT* pLockedRect, 
        __in_ecount(1) CONST RECT* pRect,
        DWORD dwFlags
        );

    HRESULT UnlockRect();

    HRESULT AddDirtyRect(
        __in_ecount(1) const RECT &rc
        );

protected:
    CD3DLockableTexture();
    ~CD3DLockableTexture();

    HRESULT Init(
        __inout_ecount(1) CD3DResourceManager *pResourceManager,
        __in_ecount(1) CD3DDeviceLevel1* pDevice,
        __in_ecount(1) D3DTexture *pD3DTexture
        );

#if PERFMETER
    virtual PERFMETERTAG GetPerfMeterTag() const
    {
        return Mt(D3DResource_LockableTexture);
    }
#endif
};

