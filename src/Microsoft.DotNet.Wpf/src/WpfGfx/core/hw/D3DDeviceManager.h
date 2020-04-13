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
//      Defines CD3DDeviceManager which maintains a list of existing D3D devices
//      via the CD3DDeviceLevel1 wrappers and creates new CD3DDeviceLevel1's as
//      they are needed.
//
//      Also keeps a shared NullRef Device for creation of our Device
//      Independent objects.
//
//  $Notes:
//      None at this time.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


class CD3DDeviceLevel1;

// Implement this interface and add yourself to the manager to receive
// notification when adapters are created or destroyed
class IAdapterStatusListener
{
public:
    virtual void NotifyAdapterStatus(UINT uAdapter, bool fIsValid) = 0;
};

class CD3DDeviceManager :
    public IMILPoolManager
{
public:

    static __checkReturn HRESULT Create();
    static void Delete();

    static CD3DDeviceManager *Get();
    static void Release();

    static void NotifyDisplayChange(
        __in_ecount(1) CDisplaySet const * pOldDisplaySet,
        __in_ecount(1) CDisplaySet const * pNewDisplaySet
        );

    CD3DDeviceManager();
    ~CD3DDeviceManager();

    HRESULT GetD3DDevice(
        __in_opt HWND hwnd,
        MilRTInitialization::Flags dwFlags,
        __in_ecount_opt(1) CDisplay const *pDisplay,
        __deref_out_ecount(1) CD3DDeviceLevel1 **ppDeviceLevel1,
        __out_ecount_opt(1) UINT *pDisplayIndex
        );
                      
    HRESULT AddAdapterStatusListener(__in IAdapterStatusListener *pListener);
    void RemoveAdapterStatusListener(__in IAdapterStatusListener *pListener);

    HRESULT GetSWDevice(__deref_out_ecount(1) CD3DDeviceLevel1 **ppDevice);

    void UnusedNotification(
        __inout_ecount(1) CMILPoolResource *pUnused
        );

    void UnusableNotification(
        __inout_ecount(1) CMILPoolResource *pUnusable
        );

    bool DoesWindowedHwDeviceExist(
        UINT uAdapter
        );

private:

    struct D3DDeviceCreationParameters
    {
        UINT AdapterOrdinal;
        MilRTInitialization::Flags RTInitFlags;
    };

    struct D3DDeviceInformation 
    {
        CD3DDeviceLevel1 *pDeviceLevel1;
        D3DDeviceCreationParameters CreateParams;
        bool fIsDeviceLost;

        #if DBG
            D3DPRESENT_PARAMETERS DbgPresentParams;
        #endif
    };

    HRESULT Init();

    HRESULT InitializeD3DReferences(
        __in_ecount_opt(1) CDisplaySet const *pDisplaySet
        );

    void IncCallers();
    void DecCallers();

    void HandleDisplayChange(
        __in_ecount(1) CDisplaySet const * pOldDisplaySet,
        __in_ecount(1) CDisplaySet const * pNewDisplaySet
        );

    HRESULT FindDeviceMatch(
        UINT uStartIndex,
        UINT uLimitPlusOne,
        __deref_opt_out_ecount(1) CD3DDeviceLevel1 **ppDeviceLevel1
        ) const;

    HRESULT GetAvailableDevice(
        __deref_out_ecount(1) CD3DDeviceLevel1 **ppDeviceLevel1
        ) const;

    HRESULT CreateNewDevice(
        __inout_ecount(1) IDXGIAdapter *pAdapter,
        UINT displayIndex,
        __deref_out_ecount(1) CD3DDeviceLevel1 **ppDeviceLevel1
        );


    void NotifyDeviceLost(D3DDeviceInformation &info);


private:

    CCriticalSection m_csManagement;

    ULONG m_cCallers;
    IDirect3D9 *m_pID3D;
    bool m_fD3DLoaded;

    CDisplaySet const *m_pDisplaySet;
    CDisplaySet const *m_pNextDisplaySet;

    D3DDeviceContext *m_pNullRefDevice;

    CD3DDeviceLevel1 *m_pSWDevice;

    DynArray<D3DDeviceInformation> m_rgDeviceList;
    UINT m_iFirstUnusable;

    CPtrArray<IAdapterStatusListener> m_rgAdapterStatusListeners;

#if DBG
private:
    bool m_fDbgCreatingNewDevice;   // Indicator that device creation and
                                    // testing is in progress to avoid
                                    // UnusableNotification assert when device
                                    // is not yet in tracking list.
#endif
};




