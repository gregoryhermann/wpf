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
//      Implements CD3DDeviceManager which maintains a list of existing D3D
//      devices via the CD3DDeviceLevel1 wrappers and creates new
//      CD3DDeviceLevel1's as they are needed.
//
//      Also keeps a shared NullRef Device for creation of our Device
//      Independent objects.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

#define ENABLE_NON_LDDM_DWM 1

ExternTag(tagD3DStats);
DeclareTag(tagDisablePureDevice, "MIL-HW", "Disable pure device");
DeclareTag(tagDisableHWGroupAdapterSupport, "MIL-HW", "Disable HW group adapter support");

MtExtern(D3DDevice);

CD3DDeviceManager g_D3DDeviceManager;

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::Create
//
//  Synopsis:
//      Initialize global D3D device manager
//
//------------------------------------------------------------------------------
__checkReturn HRESULT 
CD3DDeviceManager::Create()
{
    HRESULT hr = S_OK;

    Assert(!g_D3DDeviceManager.m_csManagement.IsValid());
    Assert(!g_D3DDeviceManager.m_fD3DLoaded);

    //
    // Call init
    //

    IFC(g_D3DDeviceManager.Init());

Cleanup:
    RRETURN(hr);
}
    
//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::Delete
//
//  Synopsis:
//      Uninitialize global D3D device manager
//
//------------------------------------------------------------------------------
void 
CD3DDeviceManager::Delete()
{
    if (g_D3DDeviceManager.m_fD3DLoaded)
    {
        ReleaseInterface(g_D3DDeviceManager.m_pID3D);
        ReleaseInterface(g_D3DDeviceManager.m_pDisplaySet);
        g_D3DDeviceManager.m_fD3DLoaded = false;
    }
    else
    {
        Assert(!g_D3DDeviceManager.m_pID3D);
        Assert(!g_D3DDeviceManager.m_pDisplaySet);
    }

    ReleaseInterface(g_D3DDeviceManager.m_pNextDisplaySet);

    return;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::Get
//
//  Synopsis:
//      Returns the global D3D device manager and increments possible callers to
//      Get and non-static methods
//
//------------------------------------------------------------------------------
CD3DDeviceManager *CD3DDeviceManager::Get()
{
    g_D3DDeviceManager.IncCallers();
    return &g_D3DDeviceManager;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::Release
//
//  Synopsis:
//      Decrements possible callers to Get and non-static methods
//
//------------------------------------------------------------------------------
void CD3DDeviceManager::Release()
{
    g_D3DDeviceManager.DecCallers();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::NotifyDisplayChange
//
//  Synopsis:
//
//------------------------------------------------------------------------------
void CD3DDeviceManager::NotifyDisplayChange(
    __in_ecount(1) CDisplaySet const * pOldDisplaySet,
    __in_ecount(1) CDisplaySet const * pNewDisplaySet
    )
{
    g_D3DDeviceManager.HandleDisplayChange(pOldDisplaySet, pNewDisplaySet);
}



//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::CD3DDeviceManager
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CD3DDeviceManager::CD3DDeviceManager()
{
    m_cCallers = 0;
    m_pID3D = NULL;
    m_fD3DLoaded = false;
    m_pDisplaySet = NULL;
    m_pNextDisplaySet = NULL;
    m_iFirstUnusable = 0;
    m_pNullRefDevice = NULL;
    m_pSWDevice = NULL;
#if DBG
    m_fDbgCreatingNewDevice = false;
#endif
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::~CD3DDeviceMaanger
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------
CD3DDeviceManager::~CD3DDeviceManager()
{
#if DBG
    Assert(!m_fDbgCreatingNewDevice);

    INT iCount = m_rgDeviceList.GetCount();

    Assert(iCount == 0);
    Assert(m_iFirstUnusable == 0);

    for (INT i=0; i < iCount; i++)
    {
        CD3DDeviceLevel1 *pDeviceLevel1 = m_rgDeviceList[i].pDeviceLevel1;

        Assert(pDeviceLevel1);
        Assert(pDeviceLevel1->GetRefCount() == 0);

        delete pDeviceLevel1;
    }
#endif

    if (m_fD3DLoaded)
    {
        ReleaseInterfaceNoNULL(m_pNullRefDevice);
        ReleaseInterfaceNoNULL(m_pSWDevice);
        ReleaseInterfaceNoNULL(m_pID3D);
    }

    ReleaseInterfaceNoNULL(m_pDisplaySet);
    ReleaseInterfaceNoNULL(m_pNextDisplaySet);

    m_csManagement.DeInit();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::Init
//
//  Synopsis:
//      Initialize static D3D pointers and references
//
//------------------------------------------------------------------------------
HRESULT CD3DDeviceManager::Init(
    )
{
    HRESULT hr = S_OK;

    if (m_fD3DLoaded)
    {
        IFC(E_UNEXPECTED);
    }

    if (m_csManagement.IsValid())
    {
        IFC(E_UNEXPECTED);
    }

    IFC(m_csManagement.Init());

Cleanup:
    Assert(m_csManagement.IsValid() || FAILED(hr));

#pragma prefast(suppress: 15, "m_csManagement is deleted in destructor")
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::GetSWDevice
//
//  Synopsis:
//      Creates and holds onto a single D3DDEVTYPE_SW device.
//
//  Return Value:
//      S_OK if it was successfully able to use an existing or create a new
//           software device.
//
//------------------------------------------------------------------------------
HRESULT CD3DDeviceManager::GetSWDevice(
    __deref_out_ecount(1) CD3DDeviceLevel1 **ppDevice
    )
{
    HRESULT hr = S_OK;

    *ppDevice = NULL;
    
    // Start a critical section to make sure we only create one device.

    CGuard<CCriticalSection> oGuard(m_csManagement);

    D3DDeviceContext *pIDirect3DDevice = NULL;
    CD3DDeviceLevel1 *pDeviceLevel1 = NULL;
    
    if (m_pID3D == NULL)
    {
        //
        // Initialize d3d references
        //

        IFC(InitializeD3DReferences(NULL));
    }

    Assert(m_pID3D);
    Assert(m_pDisplaySet);
    IFC(m_pDisplaySet->EnsureSwRastIsRegistered());

    //
    // Return our existing NULL ref device or create one
    //

    if (m_pSWDevice == NULL)
    {
        D3D_FEATURE_LEVEL requestedfeatureLevels[]
        {
                D3D_FEATURE_LEVEL_11_1,
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_1,
                D3D_FEATURE_LEVEL_10_0,
        };

        D3DDevice* pDevice = nullptr;

        D3D_FEATURE_LEVEL selectedFeatureLevel = D3D_FEATURE_LEVEL_10_0;
        IFC(D3D11CreateDevice(nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            0,
            requestedfeatureLevels,
            _countof(requestedfeatureLevels),
            D3D11_SDK_VERSION,
            &pDevice,
            &selectedFeatureLevel,
            nullptr));

        D3DDeviceContext* pDeviceContext = nullptr;

        ID3D11DeviceContext* pImmediateContext = nullptr;
        pDevice->GetImmediateContext(&pImmediateContext);
        IFC(pImmediateContext->QueryInterface(__uuidof(pDeviceContext), reinterpret_cast<void**>(&pDeviceContext)));
        ReleaseInterface(pImmediateContext);

        IFC(CD3DDeviceLevel1::Create(
            pDevice,
            pDeviceContext,
            m_pDisplaySet->Display(0),
            this,
            0,
            &pDeviceLevel1
            ));

        // All tracked devices are potential non-static method callers
        m_cCallers++;

        // We hold a pointer to the m_pSWDevice without an AddRef (like
        // the other managed devices.)
        m_pSWDevice = pDeviceLevel1;
    }

    m_pSWDevice->AddRef();
    *ppDevice = m_pSWDevice;

Cleanup:
    if (hr == D3DERR_DEVICELOST)
    {
        hr = WGXERR_DISPLAYSTATEINVALID;
    }
    
    ReleaseInterfaceNoNULL(pIDirect3DDevice);
    ReleaseInterfaceNoNULL(pDeviceLevel1);
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::IncCallers
//
//  Synopsis:
//      Increments possible callers to Get and non-static methods
//
//------------------------------------------------------------------------------
void CD3DDeviceManager::IncCallers()
{
    CGuard<CCriticalSection> oGuard(m_csManagement);

    m_cCallers++;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::DecCallers
//
//  Synopsis:
//      Decrements possible callers to Get and non-static methods
//
//------------------------------------------------------------------------------
void CD3DDeviceManager::DecCallers()
{
    CGuard<CCriticalSection> oGuard(m_csManagement);

    Assert(m_cCallers > 0);

    --m_cCallers;

    if (m_cCallers == 0)
    {
        // Managed devices (including the SW device) are callers so
        // there should be none now.
        Assert(m_rgDeviceList.GetCount() == 0);
        Assert(!m_pSWDevice);

        if (m_fD3DLoaded)
        {
            if (m_pID3D)
            {
                ReleaseInterface(m_pNullRefDevice);
                CD3DRegistryDatabase::Cleanup();
                m_pID3D->Release();
                m_pID3D = NULL;
            }
            ReleaseInterface(m_pDisplaySet);
            m_fD3DLoaded = false;
        }

        ReleaseInterface(m_pNextDisplaySet);
        
        Assert(!m_pID3D);
        Assert(!m_pDisplaySet);
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::InitializeD3DReferences
//
//  Synopsis:
//      Initialize static D3D pointers and references
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceManager::InitializeD3DReferences(
    __in_ecount_opt(1) CDisplaySet const *pGivenDisplaySet
    )
{
    HRESULT hr = S_OK;

    CDisplaySet const *pDisplaySet = NULL;
    IDirect3D9 *pID3DNoRef = NULL;

    IFC(g_DisplayManager.DangerousGetLatestDisplaySet(&pDisplaySet));

    Assert(pDisplaySet);

    if (pGivenDisplaySet && pGivenDisplaySet != pDisplaySet)
    {
        //
        // Caller has supplied pGivenDisplaySet that's obsolete.
        // We should not use it because we need not unworkable device,
        // and we should not use newest display set because it'll
        // cause mismatches with the caller that relies upon pGivenDisplaySet
        // and expects us to use it.
        // Just fail.
        // 

        IFC(WGXERR_DISPLAYSTATEINVALID);
    }

    Assert(m_cCallers > 0);
    Assert(m_fD3DLoaded || (m_rgDeviceList.GetCount() == 0));

    m_fD3DLoaded = true;
    m_pDisplaySet = pDisplaySet; // Transfer reference
    pDisplaySet = NULL;

    //
    // Now that we've settle onto current display set we can
    // release m_pNextDisplaySet that was only used to protect
    // against d3d module unloading.
    //

    ReleaseInterface(m_pNextDisplaySet);

Cleanup:
    ReleaseInterfaceNoNULL(pDisplaySet);
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::HandleDisplayChange
//
//  Synopsis:
//      Mark all D3D devices as unusable and release static D3D pointers and
//      references.
//
//------------------------------------------------------------------------------
void
CD3DDeviceManager::HandleDisplayChange(
    __in_ecount(1) CDisplaySet const * pOldDisplaySet,
    __in_ecount(1) CDisplaySet const * pNewDisplaySet
    )
{
    CGuard<CCriticalSection> oGuard(m_csManagement);

    //
    // Check whether we hold on display set that became obsolete;
    // if not so then we don't care.
    //
    if (pOldDisplaySet == m_pDisplaySet)
    {
        // Mark all usable D3D devices as unusable
        while (m_iFirstUnusable > 0)
        {
#if DBG
            UINT dbgOldFirstUnusable = m_iFirstUnusable;
#endif
            CD3DDeviceLevel1 *pDevice = m_rgDeviceList[m_iFirstUnusable - 1].pDeviceLevel1;
            ENTER_DEVICE_FOR_SCOPE(*pDevice);
            pDevice->MarkUnusable(false /* This call is entry protected by above Enter */);
#if DBG
            // We're depending upon MarkUnusable decrementing this number to avoid 
            // an infinite loop.
            Assert(m_iFirstUnusable == dbgOldFirstUnusable - 1);
#endif
        }

        if (m_pSWDevice)
        {
            m_pSWDevice->MarkUnusable(TRUE);
        }

        if (m_fD3DLoaded)
        {
            Assert(m_cCallers > 0);

            if (m_pID3D)
            {
                CD3DRegistryDatabase::Cleanup();
                m_pID3D->Release();
                m_pID3D = NULL;
            }

            //
            // Do not release the load reference on D3D at this point
            // as there may still be devices using D3D.  Note: if m_cCallers
            // is 1 and the device count is 0, there is probably a factory
            // holding it normal reference.
            //
        }
        else
        {
            Assert(!m_pID3D);
        }

        ReleaseInterface(m_pDisplaySet);

        //
        // Hold on pNewDisplaySet to avoid releasing d3d module.
        //

        ReplaceInterface(m_pNextDisplaySet, pNewDisplaySet);
    }
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      ChooseTargetFormat
//
//  Synopsis:
//      Selects render target format based on RT Init Flags
//
//------------------------------------------------------------------------------
void
ChooseTargetFormat(
    MilRTInitialization::Flags dwFlags,  // Presentation options
    __out_ecount(1) D3DFORMAT *pFmt      // The format of the target
    )
{
    if (dwFlags & MilRTInitialization::NeedDestinationAlpha)
    {
        *pFmt = D3DFMT_A8R8G8B8;
    }
    else
    {
        *pFmt = D3DFMT_X8R8G8B8;
    }
}


//+-----------------------------------------------------------------------------
//
//  Function:
//      CheckDisplayFormat
//
//  Synopsis:
//      Determine target format and confirm device support with given mode.
//

HRESULT
CheckDisplayFormat(
    UINT Adapter,
    D3DDEVTYPE DeviceType,
    D3DFORMAT DisplayFormat,
    MilRTInitialization::Flags RTInitFlags
    )
{
    HRESULT hr = S_OK;

    // Is this necessary?
#if 0
    D3DFORMAT TargetFormat;

    ChooseTargetFormat(RTInitFlags, OUT &TargetFormat);

    IFC(pID3D->CheckDeviceType(
        Adapter,
        DeviceType,
        DisplayFormat,
        TargetFormat,
        true // windowed
        ));

Cleanup:
#endif

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::DoesWindowedHwDeviceExist
//
//  Synopsis:
//      Returns 'true' if a non-fullscreen, hardware device exists on the given 
//      context and adapter id.
//
//------------------------------------------------------------------------------

bool
CD3DDeviceManager::DoesWindowedHwDeviceExist(
    UINT uAdapter
    )
{
    bool succeeded = true;

    return succeeded;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::GetAvaliableDevice
//
//  Synopsis:
//      Finds an available CD3DDeviceLevel1 object that satisfies the settings
//      given.
//
//  Notes:
//      pCreateParams->BehaviorFlags may have state of
//      D3DCREATE_DISABLE_DRIVER_MANAGEMENT_EX bit changed upon success.
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceManager::GetAvailableDevice(
    __deref_out_ecount(1) CD3DDeviceLevel1 **ppDeviceLevel1
    ) const
{
    if (m_rgDeviceList.GetCount() == 0)
    {
        return E_FAIL;
    }
    *ppDeviceLevel1 = m_rgDeviceList.At(0).pDeviceLevel1;
    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::GetD3DDeviceAndPresentParams
//
//  Synopsis:
//      Finds or creates a CD3DDeviceLevel1 object that can satisfy the settings
//      it was given.
//
//  Return Value:
//      S_OK if an acceptable state manager was located
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceManager::GetD3DDevice(
    __in_opt HWND hwnd,                                         // destination window

    MilRTInitialization::Flags dwFlags,                         // Presentation/
                                                                // fallback options

    __in_ecount_opt(1) CDisplay const *pDisplay,                // Targeted display -
                                                                // can be NULL for SW

    __deref_out_ecount(1) CD3DDeviceLevel1 **ppDeviceLevel1,    // recieves the located
                                                                // D3D device

    __out_ecount_opt(1) UINT *pDisplayIndex
    )
{
    HRESULT hr = S_OK;

    Assert(m_cCallers > 0);

    *ppDeviceLevel1 = NULL;

    CGuard<CCriticalSection> oGuard(m_csManagement);

    //
    // Ensure we have an adapter index to work with
    //

    UINT uAdapter = 0;

    if (pDisplay)
    {
        uAdapter = pDisplay->GetDisplayIndex();
    }
    else
    {
        uAdapter = 0;
    }

    IFC(InitializeD3DReferences(pDisplay ? pDisplay->DisplaySet() : NULL));

    //
    // Try to find an existing device
    //

    ASSIGN_HR(hr, GetAvailableDevice(ppDeviceLevel1));

    if (FAILED(hr))
    {
        //
        // Create new device
        //

        // 
        IFC(CreateNewDevice(
            nullptr,
            0,
            ppDeviceLevel1
            ));
    }

Cleanup:

    //
    // If the mode has changed at this point, independent of success or failure
    // so far, release any device and return failure.
    //

    if (m_pDisplaySet && m_pDisplaySet->DangerousHasDisplayStateChanged())
    {
        ReleaseInterface(*ppDeviceLevel1);
        MIL_THR(WGXERR_DISPLAYSTATEINVALID);
    }

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::UnusedNotification
//
//  Synopsis:
//      Receives notification that one of its managed objects is no longer in
//      use.  The object may be tracked and saved for later use or
//      UnusedNotification should delete the object.
//
//------------------------------------------------------------------------------
void
CD3DDeviceManager::UnusedNotification(
    __inout_ecount(1) CMILPoolResource *pUnused // pointer to no longer used CD3DDeviceLevel1
    )
{
    if (m_fD3DLoaded)
    {
        CGuard<CCriticalSection> oGuard(m_csManagement);

        CD3DDeviceLevel1 *pDeviceLevel1 = DYNCAST(CD3DDeviceLevel1, pUnused);

        // Is pUnused the SW device?
        if (m_pSWDevice == pDeviceLevel1 && pDeviceLevel1->GetRefCount() == 0)
        {
            delete pDeviceLevel1;
            m_pSWDevice = NULL;

            DecCallers();
        }
        else
        {
            // If not look for it in the list of HW devices.
            UINT uCount = m_rgDeviceList.GetCount();

            for (UINT i = 0; i < uCount; i++)
            {
                if (m_rgDeviceList[i].pDeviceLevel1 == pDeviceLevel1)
                {
                    //
                    // Make sure this object hasn't since been handed
                    //  back out since its last Release.
                    //
                    // Note that if it was handed back out, but was
                    //  again Released so that the current Ref count is
                    //  zero, then we will remove the entry now and the
                    //  pending call to this function will not be able
                    //  to locate the entry.  This is the primary reason
                    //  we never return success - the object may have
                    //  been maanged by this manager, but we won't be
                    //  able to find it in the list.  In fact that object
                    //  should be deleted at that point in time.
                    //

                    if (pDeviceLevel1->GetRefCount() != 0)
                    {
                        break;
                    }

                    //
                    // Object destruction must be done before DecCallers so
                    // that the device may clean up all of its D3D resources
                    // before D3D is potentially unloaded.
                    //

                    NotifyDeviceLost(m_rgDeviceList[i]);
                    delete pDeviceLevel1;

                    //
                    // A match was found so remove it by replacing it
                    // with the last element in the list (if it isn't
                    // already) and then shrinking the list size by 1.
                    //

                    UINT iLast = uCount - 1;

                    if (i < m_iFirstUnusable)
                    {
                        // Move up the last usable
                        m_rgDeviceList[i] = m_rgDeviceList[--m_iFirstUnusable];
                        // Move up the last unusable into the first unusable
                        m_rgDeviceList[m_iFirstUnusable] = m_rgDeviceList[iLast];
                    }
                    else
                    {
                        // Just move up the last unusable
                        m_rgDeviceList[i] = m_rgDeviceList[iLast];
                    }

                    m_rgDeviceList.SetCount(iLast);

                    //
                    // When all of the entries have been freed, shrink
                    // to free all dynamic memory, since the debug
                    // memory trackers don't appreciate being called
                    // at process detatch.
                    //

                    if (iLast == 0)
                    {
                        m_rgDeviceList.ShrinkToSize();
                    }

                    // Remove this previously tracked device as a caller
                    DecCallers();

                    break;
                }
            }
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::UnusableNotification
//
//  Synopsis:
//      Receives notification that one of its managed objects is no longer
//      usable.  The object may not be handed out anymore, but needs to still be
//      tracked until it is know to have deleted all of its D3D resources.
//
//------------------------------------------------------------------------------
void
CD3DDeviceManager::UnusableNotification(
    __inout_ecount(1) CMILPoolResource *pUnusable // pointer to lost CD3DDeviceLevel1
    )
{
    Assert(m_fD3DLoaded);

    CGuard<CCriticalSection> oGuard(m_csManagement);

    const CD3DDeviceLevel1 *pDeviceLevel1 = DYNCAST(CD3DDeviceLevel1, pUnusable);

    //
    // Find entry and move to the unusable section of the list
    //

#if DBG
    bool fFound = false;
#endif

    for (UINT i = 0; i < m_iFirstUnusable; i++)
    {
        if (m_rgDeviceList[i].pDeviceLevel1 == pDeviceLevel1)
        {
            NotifyDeviceLost(m_rgDeviceList[i]);
            
            m_iFirstUnusable--;
            if (i != m_iFirstUnusable)
            {
                D3DDeviceInformation temp = m_rgDeviceList[i];
                // Move last usable into newly unusable's place
                m_rgDeviceList[i] = m_rgDeviceList[m_iFirstUnusable];
                // Move newly unusable into first unusable position
                m_rgDeviceList[m_iFirstUnusable] = temp;
            }

#if DBG
            fFound = true;
#endif
            break;
        }
    }

#if DBG
    //
    // Normally the device should be found in the usable list,
    // but if a mode change has happened then it might have been
    // moved to the unusable section; so, check there too.
    //
    // But only check if we are not currently creating a new device.  During
    // creation the device won't be in the list until after some tests.
    //
    // The SW device is not in the device list so ignore it.
    //

    if (!fFound && !m_fDbgCreatingNewDevice && pUnusable != m_pSWDevice)
    {
        UINT uTotal = m_rgDeviceList.GetCount();

        for (UINT i = m_iFirstUnusable; i < uTotal; i++)
        {
            if (m_rgDeviceList[i].pDeviceLevel1 == pDeviceLevel1)
            {
                TraceTag((tagMILWarning,
                          "Device was lost upon Present after mode change."));
                fFound = true;
                break;
            }
        }

        Assert(fFound);
    }
#endif
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::CreateNewDevice
//
//  Synopsis:
//      CreateNewDevice creates a new D3D device and a new CD3DDeviceLevel1 to
//      use the D3D device, then adds the device wrapper to the tracking list.
//
//  Return Value:
//      S_OK - New D3D device and wrapper were successfully created
//
//------------------------------------------------------------------------------
HRESULT
CD3DDeviceManager::CreateNewDevice(
    __inout_ecount(1) IDXGIAdapter *pAdapter,
    UINT displayIndex,
    __deref_out_ecount(1) CD3DDeviceLevel1 **ppDeviceLevel1
    )
{
    HRESULT hr = S_OK;

    //
    // Before trying to create a device (especially a fullscreen one which can
    // itself set the display modes) make sure it hasn't changed since we
    // acquired mode information.
    //

    if (m_pDisplaySet->DangerousHasDisplayStateChanged())
    {
        IFC(WGXERR_DISPLAYSTATEINVALID);
    }


    D3D_FEATURE_LEVEL requestedfeatureLevels[]
    {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
    };

    D3DDevice* pDevice = nullptr;

    DWORD dwFlags = 0;

    //dwFlags = D3D11_CREATE_DEVICE_DEBUG;

    D3D_FEATURE_LEVEL selectedFeatureLevel = D3D_FEATURE_LEVEL_10_0;
    IFC(D3D11CreateDevice(pAdapter,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        dwFlags,        
        requestedfeatureLevels,
        _countof(requestedfeatureLevels),
        D3D11_SDK_VERSION,
        &pDevice,
        &selectedFeatureLevel,
        nullptr));

    D3DDeviceContext* pDeviceContext = nullptr;

    ID3D11DeviceContext* pImmediateContext = nullptr;
    pDevice->GetImmediateContext(&pImmediateContext);
    IFC(pImmediateContext->QueryInterface(__uuidof(pDeviceContext), reinterpret_cast<void**>(&pDeviceContext)));
    ReleaseInterface(pImmediateContext);

#if DBG
    // Note that device creation is in process incase the device becomes
    // unusable during initialization; so that UnusableNotification won't
    // assert that this new device is not yet in its tracking list. 
    m_fDbgCreatingNewDevice = true;
#endif

    CD3DDeviceLevel1 *pDeviceLevel1 = NULL;

    if (SUCCEEDED(hr))
    {
        MIL_THR(CD3DDeviceLevel1::Create(
            pDevice,
            pDeviceContext,
            m_pDisplaySet->Display(displayIndex),
            this,
            0,
            &pDeviceLevel1
            ));
    }

    if (SUCCEEDED(hr))
    {
        // Make sure there is space for the new entry
        MIL_THR(m_rgDeviceList.ReserveSpace(1));
    }

    if (SUCCEEDED(hr))
    {
        UINT iNewLast = m_rgDeviceList.GetCount();

        // Increase the length
        m_rgDeviceList.SetCount(iNewLast+1);

        // Move the first unusable entry to the end if needed
        if (m_iFirstUnusable < iNewLast)
        {
            m_rgDeviceList[iNewLast] = m_rgDeviceList[m_iFirstUnusable];
        }
        else
        {
            Assert(m_iFirstUnusable == iNewLast);
        }

        // Place the new entry in at the end of the usable list
        m_rgDeviceList[m_iFirstUnusable].pDeviceLevel1 = pDeviceLevel1;
        m_rgDeviceList[m_iFirstUnusable].fIsDeviceLost = false;

        m_iFirstUnusable++;

        // All tracked devices are potential non-static method callers
        m_cCallers++;

        //
        // Return the value and assign the reference from
        // creation to the returned pointer.
        //

        *ppDeviceLevel1 = pDeviceLevel1;
    }
    else
    {
        if (pDeviceLevel1)
        {
            //
            // This device was created but we were unable to track it; so,
            // Release to get the count back to zero and then delete to
            // actually free it.  During the Release this manager will
            // be called, but won't be able to find this resource in its
            // list and therefore won't delete it.
            //

            Verify(pDeviceLevel1->Release() == 0);
            delete pDeviceLevel1;
        }
    }

Cleanup:

#if DBG
    // Device creation and tracking is complete
    m_fDbgCreatingNewDevice = false;
#endif


    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::AddAdapterStatusListener
//
//  Synopsis:
//      Adds listener to the list. 
//
//------------------------------------------------------------------------------

HRESULT 
CD3DDeviceManager::AddAdapterStatusListener(__in IAdapterStatusListener *pListener)
{
    CGuard<CCriticalSection> oGuard(m_csManagement);
    
    RRETURN(m_rgAdapterStatusListeners.Add(pListener));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::RemoveAdapterStatusListener
//
//  Synopsis:
//      Removes listener to the list. 
//
//------------------------------------------------------------------------------

void 
CD3DDeviceManager::RemoveAdapterStatusListener(__in IAdapterStatusListener *pListener)
{
    CGuard<CCriticalSection> oGuard(m_csManagement);
        
    m_rgAdapterStatusListeners.Remove(pListener);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DDeviceManager::NotifyDeviceLost
//
//  Synopsis:
//      Notifies all listeners that the device in "info" has been lost. This should be called
//      whenever a device is being destroyed. fIsDeviceLost protects us from incorrectly
//      overnotifying in the multi-window case. For example, if one window isn't presenting
//      when device lost happens then it won't know that its device is lost until it presents and
//      in the mean time the same device may have already been lost and recreated
//      by another presenting window. We don't want to send another lost message since 
//      rendering for that adapter has been restored.
//
//------------------------------------------------------------------------------

void
CD3DDeviceManager::NotifyDeviceLost(D3DDeviceInformation &info)
{
    if (!info.fIsDeviceLost)
    {
        info.fIsDeviceLost = true;

        for (size_t i = 0; i < m_rgAdapterStatusListeners.GetCount(); ++i)
        {
            m_rgAdapterStatusListeners[i]->NotifyAdapterStatus(
                info.CreateParams.AdapterOrdinal, 
                false       // fIsValid
                );
        }
    }
}







