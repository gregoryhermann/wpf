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
//      Contains implementation for CHwPipelineBuilder class.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"


//+-----------------------------------------------------------------------------
//
//  Table:
//      sc_PipeOpProperties
//
//  Synopsis:
//      Table of HwBlendOp properties
//
//------------------------------------------------------------------------------

static const 
struct BlendOperationProperties {
    bool AllowsAlphaMultiplyInEarlierStage;
} sc_BlendOpProperties[] =
{   // HBO_SelectSource
    {
/* AllowsAlphaMultiplyInEarlierStage */ false
    },

    // HBO_Multiply
    {
/* AllowsAlphaMultiplyInEarlierStage */ true
    },

    // HBO_SelectSourceColorIgnoreAlpha
    {
/* AllowsAlphaMultiplyInEarlierStage */ false
    },

    // HBO_MultiplyColorIgnoreAlpha
    {
/* AllowsAlphaMultiplyInEarlierStage */ true
    },

    // HBO_BumpMap
    {
/* AllowsAlphaMultiplyInEarlierStage */ true
    },
    
    // HBO_MultiplyByAlpha
    {
/* AllowsAlphaMultiplyInEarlierStage */ true
    },
    
    // HBO_MultiplyAlphaOnly
    {
/* AllowsAlphaMultiplyInEarlierStage */ true
    },
};

C_ASSERT(ARRAYSIZE(sc_BlendOpProperties)==HBO_Total);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwPipelineBuilder
//
//  Synopsis:
//      Helper class for CHwPipeline that does the actual construction of the
//      pipeline and to which other components interface
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::Builder
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------

CHwPipelineBuilder::CHwPipelineBuilder(
    __in_ecount(1) CHwPipeline * const pHP,
    HwPipeline::Type oType
    )
    : m_pHP(pHP),
      m_oPipelineType(oType)
{
    Assert(pHP);

    m_iCurrentSampler = INVALID_PIPELINE_SAMPLER;
    m_iCurrentStage   = INVALID_PIPELINE_STAGE;

    m_mvfIn = MILVFAttrNone;
    m_mvfGenerated = MILVFAttrNone;

    m_fAntiAliasUsed = false;

    m_eAlphaMultiplyOp = HBO_Nop;

    m_iAlphaMultiplyOkayAtItem = INVALID_PIPELINE_STAGE;
    m_iLastAlphaScalableItem   = INVALID_PIPELINE_ITEM;

    m_iAntiAliasingPiggybackedByItem = INVALID_PIPELINE_ITEM;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::InitializePipelineMembers
//
//  Synopsis:
//      Figure out the alpha multiply operation and obtain vertex info.
//

void 
CHwPipelineBuilder::InitializePipelineMembers(
    MilCompositingMode::Enum eCompositingMode,
    __in_ecount(1) IGeometryGenerator const *pIGeometryGenerator
    )
{
    Assert(m_iCurrentSampler == INVALID_PIPELINE_SAMPLER);
    Assert(m_iCurrentStage   == INVALID_PIPELINE_STAGE);
    Assert(m_iAlphaMultiplyOkayAtItem  == INVALID_PIPELINE_STAGE);
    Assert(m_iLastAlphaScalableItem == INVALID_PIPELINE_STAGE);

    if (eCompositingMode == MilCompositingMode::SourceOverNonPremultiplied ||
        eCompositingMode == MilCompositingMode::SourceInverseAlphaOverNonPremultiplied)
    {
        m_eAlphaMultiplyOp = HBO_MultiplyAlphaOnly;
    }
    else
    {
        m_eAlphaMultiplyOp = HBO_Multiply;
    }    

    pIGeometryGenerator->GetPerVertexDataType(
        OUT m_mvfIn
        );

    m_mvfAvailable = MILVFAttrXYZ | MILVFAttrDiffuse | MILVFAttrSpecular | MILVFAttrUV4;
    m_mvfAvailable &= ~m_mvfIn;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::SendPipelineOperations
//
//  Synopsis:
//      Construct a full rendering pipeline for the given context from scratch
//

HRESULT
CHwPipelineBuilder::SendPipelineOperations(
    __inout_ecount(1) IHwPrimaryColorSource *pIPCS,
    __in_ecount_opt(1) const IMILEffectList *pIEffects,
    __in_ecount(1) const CHwBrushContext    *pEffectContext,
    __inout_ecount(1) IGeometryGenerator *pIGeometryGenerator
    )
{
    HRESULT hr = S_OK;

    // Determine incoming per vertex data included with geometry.

    // Request primary color source to send primary rendering operations

    IFC(pIPCS->SendOperations(this));

    // Setup effects operations if any

    if (pIEffects)
    {
        IFC(ProcessEffectList(
            pIEffects,
            pEffectContext
            ));
    }

    IFC(pIGeometryGenerator->SendGeometryModifiers(this));
    IFC(pIGeometryGenerator->SendLighting(this));

    // Setup operations to handle clipping
    IFC(ProcessClip());

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::Set_BumpMap
//
//  Synopsis:
//      Take the given color source and set it as a bump map for the first
//      texture color source
//
//      This call must be followed by a Set_Texture call specifying the first
//      real color source.
//

HRESULT
CHwPipelineBuilder::Set_BumpMap(
    __in_ecount(1) CHwTexturedColorSource *pBumpMap
    )
{
    HRESULT hr = S_OK;

    // Parameter Assertions
    Assert(pBumpMap->GetSourceType() != CHwColorSource::Constant);

    IFC(E_NOTIMPL);

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::Mul_AlphaMask
//
//  Synopsis:
//      Add a blend operation that uses the given color source's alpha
//      components to scale previous rendering results
//

HRESULT
CHwPipelineBuilder::Mul_AlphaMask(
    __in_ecount(1) CHwTexturedColorSource *pAlphaMaskColorSource
    )
{
    HRESULT hr = S_OK;

    IFC(E_NOTIMPL);

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::ProcessClip
//
//  Synopsis:
//      Set up clipping operations and/or resources
//

HRESULT
CHwPipelineBuilder::ProcessClip(
    )
{
    HRESULT hr = S_OK;


    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::ProcessEffectList
//
//  Synopsis:
//      Read the effect list and add pipeline operations for each one
//
//      This method and the ProcessXxxEffect helper methods make up the logical
//      Hardware Effects Processor component.
//
//  Responsibilities:
//      - Decode effects list to create color sources and specify operation
//        needed to pipeline
//
//  Not responsible for:
//      - Determining operation order or combining operations
//
//  Inputs required:
//      - Effects list
//      - Pipeline builder object (this)
//

HRESULT
CHwPipelineBuilder::ProcessEffectList(
    __in_ecount(1) const IMILEffectList *pIEffects,
    __in_ecount(1) const CHwBrushContext *pEffectContext
    )
{
    HRESULT hr = S_OK;

    UINT cEntries = 0;

    // Get the count of the transform blocks in the effect object.
    IFC(pIEffects->GetCount(&cEntries));

    // Handle only alpha effects

    for (UINT uIndex = 0; uIndex < cEntries; uIndex++)
    {
        CLSID clsid;
        UINT cbSize;
        UINT cResources;

        IFC(pIEffects->GetCLSID(uIndex, &clsid));
        IFC(pIEffects->GetParameterSize(uIndex, &cbSize));
        IFC(pIEffects->GetResourceCount(uIndex, &cResources));

        if (clsid == CLSID_MILEffectAlphaScale)
        {
            IFC(ProcessAlphaScaleEffect(pIEffects, uIndex, cbSize, cResources));
        }
        else if (clsid == CLSID_MILEffectAlphaMask)
        {
            IFC(ProcessAlphaMaskEffect(pEffectContext, pIEffects, uIndex, cbSize, cResources));
        }
        else
        {
            IFC(WGXERR_UNSUPPORTED_OPERATION);
        }
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::ProcessAlphaScaleEffect
//
//  Synopsis:
//      Decode an alpha scale effect and add to pipeline
//

HRESULT
CHwPipelineBuilder::ProcessAlphaScaleEffect(
    __in_ecount(1) const IMILEffectList *pIEffects,
    UINT uIndex,
    UINT cbSize,
    UINT cResources
    )
{
    HRESULT hr = S_OK;

    CHwConstantAlphaScalableColorSource *pNewAlphaColorSource = NULL;

    AlphaScaleParams alphaScale;

    // check the parameter size
    if (cbSize != sizeof(alphaScale))
    {
        AssertMsg(FALSE, "AlphaScale parameter has unexpected size.");
        IFC(WGXERR_UNSUPPORTED_OPERATION);
    }
    else if (cResources != 0)
    {
        AssertMsg(FALSE, "AlphaScale has unexpected number of resources.");
        IFC(WGXERR_UNSUPPORTED_OPERATION);
    }

    IFC(pIEffects->GetParameters(uIndex, cbSize, &alphaScale));

    if (0.0f > alphaScale.scale || alphaScale.scale > 1.0f)
    {
        IFC(WGXERR_UNSUPPORTED_OPERATION);
    }
    else
    {

        IFC(CHwConstantAlphaScalableColorSource::Create(
            m_pHP->m_pDevice,
            alphaScale.scale,
            NULL,
            &m_pHP->m_dbScratch,
            &pNewAlphaColorSource
            ));

        IFC(Mul_ConstAlpha(pNewAlphaColorSource));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pNewAlphaColorSource);

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::ProcessAlphaMaskEffect
//
//  Synopsis:
//      Decode an alpha mask effect and add to pipeline
//

HRESULT
CHwPipelineBuilder::ProcessAlphaMaskEffect(
    __in_ecount(1) const CHwBrushContext *pEffectContext,
    __in_ecount(1) const IMILEffectList *pIEffects,
    UINT uIndex,
    UINT cbSize,
    UINT cResources
    )
{
    HRESULT hr = S_OK;

    AlphaMaskParams alphaMaskParams;

    IUnknown *pIUnknown = NULL;
    IWGXBitmapSource *pMaskBitmap = NULL;
    CHwTexturedColorSource *pMaskColorSource = NULL;
    
    CMultiOutSpaceMatrix<CoordinateSpace::RealizationSampling> matBitmapToIdealRealization;

    CDelayComputedBounds<CoordinateSpace::RealizationSampling> rcRealizationBounds;

    BitmapToXSpaceTransform matRealizationToGivenSampleSpace;
    
    // check the parameter size
    if (cbSize != sizeof(alphaMaskParams))
    {
        AssertMsg(FALSE, "AlphaMask parameter has unexpected size.");
        IFC(WGXERR_UNSUPPORTED_OPERATION);
    }
    else if (cResources != 1)
    {
        AssertMsg(FALSE, "AlphaMask has unexpected number of resources.");
        IFC(WGXERR_UNSUPPORTED_OPERATION);
    }

    IFC(pIEffects->GetParameters(uIndex, cbSize, &alphaMaskParams));

    IFC(pIEffects->GetResources(uIndex, cResources, &pIUnknown));

    IFC(pIUnknown->QueryInterface(
            IID_IWGXBitmapSource,
            reinterpret_cast<void **>(&pMaskBitmap)));

    pEffectContext->GetRealizationBoundsAndTransforms(
        CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Effect>::ReinterpretBase(alphaMaskParams.matTransform),
        OUT matBitmapToIdealRealization,
        OUT matRealizationToGivenSampleSpace,
        OUT rcRealizationBounds
        );

    {
        CHwBitmapColorSource::CacheContextParameters oContextCacheParameters(
            MilBitmapInterpolationMode::Linear,
            pEffectContext->GetContextStatePtr()->RenderState->PrefilterEnable,
            pEffectContext->GetFormat(),
            MilBitmapWrapMode::Extend
            );

        IFC(CHwBitmapColorSource::DeriveFromBitmapAndContext(
            m_pHP->m_pDevice,
            pMaskBitmap,
            NULL,
            NULL,
            rcRealizationBounds,
            &matBitmapToIdealRealization,
            &matRealizationToGivenSampleSpace,
            pEffectContext->GetContextStatePtr()->RenderState->PrefilterThreshold,
            pEffectContext->CanFallback(),
            NULL,
            oContextCacheParameters,
            &pMaskColorSource
            ));

        IFC(Mul_AlphaMask(pMaskColorSource));
    }

Cleanup:

    ReleaseInterfaceNoNULL(pIUnknown);
    ReleaseInterfaceNoNULL(pMaskBitmap);
    ReleaseInterfaceNoNULL(pMaskColorSource);

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::ChooseVertexBuilder
//
//  Synopsis:
//      Create a vertex builder for the current pipeline
//

HRESULT
CHwPipelineBuilder::ChooseVertexBuilder(
    __deref_out_ecount(1) CHwVertexBuffer::Builder **ppVertexBuilder
    )
{
    HRESULT hr = S_OK;

    MilVertexFormatAttribute mvfaAALocation = MILVFAttrNone;

    if (m_fAntiAliasUsed)
    {
        mvfaAALocation   = HWPIPELINE_ANTIALIAS_LOCATION;
    }

    Assert((m_mvfIn & m_mvfGenerated) == 0);

    IFC(CHwVertexBuffer::Builder::Create(
        m_mvfIn,
        m_mvfIn | m_mvfGenerated,
        mvfaAALocation,
        m_pHP,
        m_pHP->m_pDevice,
        &m_pHP->m_dbScratch,
        ppVertexBuilder
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::TryToMultiplyConstantAlphaToExistingStage
//
//  Synopsis:
//      Tries to find an existing stage it can use to drop it's alpha multiply
//      into.  Should work on both shader and fixed function pipelines.
//
//------------------------------------------------------------------------------
bool
CHwPipelineBuilder::TryToMultiplyConstantAlphaToExistingStage(
    __in_ecount(1) const CHwConstantAlphaColorSource *pAlphaColorSource
    )
{
    HRESULT hr = S_OK;

    float flAlpha = pAlphaColorSource->GetAlpha();
    CHwConstantAlphaScalableColorSource *pScalableAlphaSource = NULL;
    bool fStageToMultiplyFound = false;
    INT iItemCount = static_cast<INT>(m_pHP->m_rgItem.GetCount());

    // Parameter Assertions
    Assert(flAlpha >= 0.0f);
    Assert(flAlpha <= 1.0f);

    // Member Assertions

    // There should be at least one stage
    Assert(iItemCount > 0);
    Assert(GetNumReservedStages() > 0);

    // An alpha scale of 1.0 is a nop; do nothing
    if (flAlpha == 1.0f)
    {
        fStageToMultiplyFound = true;
        goto Cleanup;
    }

    int iLastAlphaScalableItem = GetLastAlphaScalableItem();
    int iItemAvailableForAlphaMultiply = GetEarliestItemAvailableForAlphaMultiply();

    //  We can add logic to recognize that an alpha scale of 0 would give us a
    //  completely transparent result and then "compress" previous stages.

    // Check for existing stage at which constant alpha scale may be applied
    if (iItemAvailableForAlphaMultiply < iItemCount)
    {
        // Check for existing color source that will handle the alpha scale
        if (iLastAlphaScalableItem >= iItemAvailableForAlphaMultiply)
        {
            Assert(m_pHP->m_rgItem[iLastAlphaScalableItem].pHwColorSource);
            
            // Multiply with new scale factor
            m_pHP->m_rgItem[iLastAlphaScalableItem].pHwColorSource->AlphaScale(flAlpha);

            fStageToMultiplyFound = true;
        }
        else
        {
            //
            // Check for existing color source that can be reused to handle the
            // alpha scale.  Alpha scale can be applied to any constant color
            // source using the ConstantAlphaScalable class.
            //
            // The scale should technically come at the end of the current
            // operations; so, try to get as close to the end as possible.
            //
    
            for (INT iLastConstant = iItemCount-1;
                 iLastConstant >= iItemAvailableForAlphaMultiply;
                 iLastConstant--)
            {
                CHwColorSource *pHCS =
                    m_pHP->m_rgItem[iLastConstant].pHwColorSource;
    
                if (pHCS && (pHCS->GetSourceType() & CHwColorSource::Constant))
                {
                    // The ConstantAlphaScalable class only supports
                    // HBO_Multiply because it assumes premulitplied colors come
                    // in and go out.
                    Assert(m_eAlphaMultiplyOp == HBO_Multiply);
    
                    //
                    // Inject an alpha scalable color source in place
                    // of the current constant color source.
                    //
    
                    IFC(CHwConstantAlphaScalableColorSource::Create(
                        m_pHP->m_pDevice,
                        flAlpha,
                        DYNCAST(CHwConstantColorSource, pHCS),
                        &m_pHP->m_dbScratch,
                        &pScalableAlphaSource
                        ));
    
                    // Transfer pScalableAlphaSource reference
                    m_pHP->m_rgItem[iLastConstant].pHwColorSource =
                        pScalableAlphaSource;
                    pHCS->Release();

                    //
                    // Color Sources being added to a pipeline are
                    // required to have their mappings reset.  This
                    // normally happens when items are added to the
                    // pipeline, but since this is replacing an item
                    // we need to call it ourselves.
                    //
                    pScalableAlphaSource->ResetForPipelineReuse();

                    pScalableAlphaSource = NULL;

                    // Remember this location now holds an
                    // alpha scalable color source
                    SetLastAlphaScalableStage(iLastConstant);

                    fStageToMultiplyFound = true;
                    break;
                }
            }
        }
    }

Cleanup:
    ReleaseInterfaceNoNULL(pScalableAlphaSource);

    //
    // We only want to consider success if our HRESULT is S_OK.
    //
    return (hr == S_OK && fStageToMultiplyFound);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::CheckForBlendAlreadyPresentAtAALocation
//
//  Synopsis:
//      We may have already added a blend operation using the location we're
//      going to generate anti-aliasing in.  If this is the case we don't need
//      to add another blend operation.
//
//------------------------------------------------------------------------------
HRESULT
CHwPipelineBuilder::CheckForBlendAlreadyPresentAtAALocation(
    bool *pfNeedToAddAnotherStageToBlendAntiAliasing
    )
{
    HRESULT hr = S_OK;

    INT iAAPiggybackItem = GetAAPiggybackItem();

    *pfNeedToAddAnotherStageToBlendAntiAliasing = false;

    //
    // Validate that any AA piggybacking is okay.  If first location (item)
    // available for alpha multiply is greater than location of piggyback item,
    // then piggybacking is not allowed.
    //
    // AA piggyback item is -1 when not set so that case will also be detected.
    //

    if (iAAPiggybackItem < GetEarliestItemAvailableForAlphaMultiply())
    {
        //
        // Check if there was a piggyback item
        //
        if (iAAPiggybackItem != INVALID_PIPELINE_ITEM)
        {
            // Future Consideration:   Find new attribute for AA piggybacker
            //  and modify pipeline item with new properties.
            RIP("Fixed function pipeline does not expect invalid piggybacking");
            IFC(WGXERR_NOTIMPLEMENTED);
        }

        *pfNeedToAddAnotherStageToBlendAntiAliasing = true;
    }
    else
    {
        Assert(GetGeneratedComponents() & MILVFAttrDiffuse);
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::SetupVertexBuilder
//
//  Synopsis:
//      Choose the appropriate vertex builder class for the pipeline that has
//      just been set up and initialize the vertex builder
//

HRESULT
CHwPipelineBuilder::SetupVertexBuilder(
    __deref_out_ecount(1) CHwVertexBuffer::Builder **ppVertexBuilder
    )
{
    HRESULT hr = S_OK;

    // Select a vertex builder
    IFC(ChooseVertexBuilder(ppVertexBuilder));

    // Send vertex mappings for each color source
    HwPipelineItem *pItem = m_pHP->m_rgItem.GetDataBuffer();

    CHwVertexBuffer::Builder *pVertexBuilder = *ppVertexBuilder;

    if (VerticesArePreGenerated())
    {
        // Pass NULL builder to color source to indicate that vertices are
        // pre-generated and should not be modified.
        pVertexBuilder = NULL;
    }

    for (UINT uItem = 0; uItem < m_pHP->m_rgItem.GetCount(); uItem++, pItem++)
    {
        if (pItem->pHwColorSource && pItem->mvfaTextureCoordinates != MILVFAttrNone)
        {
            IFC(pItem->pHwColorSource->SendVertexMapping(
                pVertexBuilder,
                pItem->mvfaTextureCoordinates
                ));
        }
    }

    // Let vertex builder know that is the end of the vertex mappings
    IFC((*ppVertexBuilder)->FinalizeMappings());

Cleanup:

    if (FAILED(hr))
    {
        delete *ppVertexBuilder;
        *ppVertexBuilder = NULL;
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::Set_AAColorSource
//
//  Synopsis:
//      Adds an antialiasing colorsource.
//
//------------------------------------------------------------------------------
HRESULT
CHwPipelineBuilder::Set_AAColorSource(
    __in_ecount(1) CHwColorComponentSource *pAAColorSource
    )
{
    HRESULT hr = S_OK;

    //
    // Use Geometry Generator specified AA location (none, falloff, UV) to
    //   1) Append blend operation as needed
    //   2) Otherwise set proper indicators to vertex builder
    //

    Assert(pAAColorSource->GetComponentLocation() == CHwColorComponentSource::Diffuse);

    bool fNeedToAddAnotherStageToBlendAntiAliasing = true;

    IFC(CheckForBlendAlreadyPresentAtAALocation(
        &fNeedToAddAnotherStageToBlendAntiAliasing
        ));

    if (fNeedToAddAnotherStageToBlendAntiAliasing)
    {
        IFC(Mul_BlendColorsInternal(
            pAAColorSource
            ));
    }

    m_fAntiAliasUsed = true;

Cleanup:
    RRETURN(hr);
}

