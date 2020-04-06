// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------

//
// This file is automatically generated.  Please do not edit it directly.
//
// File name: data_generated.h
//---------------------------------------------------------------------------

#pragma once

#pragma warning (push)
#pragma warning (disable : 4995)

struct CMilAxisAngleRotation3DDuce_Data
{
    MilPoint3F m_axis;
    CMilSlaveVector3D *m_pAxisAnimation;
    DOUBLE m_angle;
    CMilSlaveDouble *m_pAngleAnimation;
};

struct CMilQuaternionRotation3DDuce_Data
{
    MilQuaternionF m_quaternion;
    CMilSlaveQuaternion *m_pQuaternionAnimation;
};

struct CMilPerspectiveCameraDuce_Data
{
    CMilTransform3DDuce *m_pTransform;
    DOUBLE m_nearPlaneDistance;
    CMilSlaveDouble *m_pNearPlaneDistanceAnimation;
    DOUBLE m_farPlaneDistance;
    CMilSlaveDouble *m_pFarPlaneDistanceAnimation;
    MilPoint3F m_position;
    CMilSlavePoint3D *m_pPositionAnimation;
    MilPoint3F m_lookDirection;
    CMilSlaveVector3D *m_pLookDirectionAnimation;
    MilPoint3F m_upDirection;
    CMilSlaveVector3D *m_pUpDirectionAnimation;
    DOUBLE m_fieldOfView;
    CMilSlaveDouble *m_pFieldOfViewAnimation;
};

struct CMilOrthographicCameraDuce_Data
{
    CMilTransform3DDuce *m_pTransform;
    DOUBLE m_nearPlaneDistance;
    CMilSlaveDouble *m_pNearPlaneDistanceAnimation;
    DOUBLE m_farPlaneDistance;
    CMilSlaveDouble *m_pFarPlaneDistanceAnimation;
    MilPoint3F m_position;
    CMilSlavePoint3D *m_pPositionAnimation;
    MilPoint3F m_lookDirection;
    CMilSlaveVector3D *m_pLookDirectionAnimation;
    MilPoint3F m_upDirection;
    CMilSlaveVector3D *m_pUpDirectionAnimation;
    DOUBLE m_width;
    CMilSlaveDouble *m_pWidthAnimation;
};

struct CMilMatrixCameraDuce_Data
{
    CMilTransform3DDuce *m_pTransform;
   DirectX::XMFLOAT4X4 m_viewMatrix;
   DirectX::XMFLOAT4X4 m_projectionMatrix;
};

struct CMilModel3DGroupDuce_Data
{
    CMilTransform3DDuce *m_pTransform;
    UINT32 m_cChildren;
    CMilModel3DDuce **m_rgpChildren;
};

struct CMilAmbientLightDuce_Data
{
    CMilTransform3DDuce *m_pTransform;
    MilColorF m_color;
    CMilSlaveColor *m_pColorAnimation;
};

struct CMilDirectionalLightDuce_Data
{
    CMilTransform3DDuce *m_pTransform;
    MilColorF m_color;
    CMilSlaveColor *m_pColorAnimation;
    MilPoint3F m_direction;
    CMilSlaveVector3D *m_pDirectionAnimation;
};

struct CMilPointLightDuce_Data
{
    CMilTransform3DDuce *m_pTransform;
    MilColorF m_color;
    CMilSlaveColor *m_pColorAnimation;
    MilPoint3F m_position;
    CMilSlavePoint3D *m_pPositionAnimation;
    DOUBLE m_range;
    CMilSlaveDouble *m_pRangeAnimation;
    DOUBLE m_constantAttenuation;
    CMilSlaveDouble *m_pConstantAttenuationAnimation;
    DOUBLE m_linearAttenuation;
    CMilSlaveDouble *m_pLinearAttenuationAnimation;
    DOUBLE m_quadraticAttenuation;
    CMilSlaveDouble *m_pQuadraticAttenuationAnimation;
};

struct CMilSpotLightDuce_Data
{
    CMilTransform3DDuce *m_pTransform;
    MilColorF m_color;
    CMilSlaveColor *m_pColorAnimation;
    MilPoint3F m_position;
    CMilSlavePoint3D *m_pPositionAnimation;
    DOUBLE m_range;
    CMilSlaveDouble *m_pRangeAnimation;
    DOUBLE m_constantAttenuation;
    CMilSlaveDouble *m_pConstantAttenuationAnimation;
    DOUBLE m_linearAttenuation;
    CMilSlaveDouble *m_pLinearAttenuationAnimation;
    DOUBLE m_quadraticAttenuation;
    CMilSlaveDouble *m_pQuadraticAttenuationAnimation;
    MilPoint3F m_direction;
    CMilSlaveVector3D *m_pDirectionAnimation;
    DOUBLE m_outerConeAngle;
    CMilSlaveDouble *m_pOuterConeAngleAnimation;
    DOUBLE m_innerConeAngle;
    CMilSlaveDouble *m_pInnerConeAngleAnimation;
};

struct CMilGeometryModel3DDuce_Data
{
    CMilTransform3DDuce *m_pTransform;
    CMilGeometry3DDuce *m_pGeometry;
    CMilMaterialDuce *m_pMaterial;
    CMilMaterialDuce *m_pBackMaterial;
};

struct CMilMeshGeometry3DDuce_Data
{
    UINT32 m_cbPositionsSize;
    MilPoint3F *m_pPositionsData;
    UINT32 m_cbNormalsSize;
    MilPoint3F *m_pNormalsData;
    UINT32 m_cbTextureCoordinatesSize;
    MilPoint2D *m_pTextureCoordinatesData;
    UINT32 m_cbTriangleIndicesSize;
    INT *m_pTriangleIndicesData;
};

struct CMilMaterialGroupDuce_Data
{
    UINT32 m_cChildren;
    CMilMaterialDuce **m_rgpChildren;
};

struct CMilDiffuseMaterialDuce_Data
{
    MilColorF m_color;
    MilColorF m_ambientColor;
    CMilBrushDuce *m_pBrush;
};

struct CMilSpecularMaterialDuce_Data
{
    MilColorF m_color;
    CMilBrushDuce *m_pBrush;
    DOUBLE m_specularPower;
};

struct CMilEmissiveMaterialDuce_Data
{
    MilColorF m_color;
    CMilBrushDuce *m_pBrush;
};

struct CMilTransform3DGroupDuce_Data
{
    UINT32 m_cChildren;
    CMilTransform3DDuce **m_rgpChildren;
};

struct CMilTranslateTransform3DDuce_Data
{
    DOUBLE m_offsetX;
    CMilSlaveDouble *m_pOffsetXAnimation;
    DOUBLE m_offsetY;
    CMilSlaveDouble *m_pOffsetYAnimation;
    DOUBLE m_offsetZ;
    CMilSlaveDouble *m_pOffsetZAnimation;
};

struct CMilScaleTransform3DDuce_Data
{
    DOUBLE m_scaleX;
    CMilSlaveDouble *m_pScaleXAnimation;
    DOUBLE m_scaleY;
    CMilSlaveDouble *m_pScaleYAnimation;
    DOUBLE m_scaleZ;
    CMilSlaveDouble *m_pScaleZAnimation;
    DOUBLE m_centerX;
    CMilSlaveDouble *m_pCenterXAnimation;
    DOUBLE m_centerY;
    CMilSlaveDouble *m_pCenterYAnimation;
    DOUBLE m_centerZ;
    CMilSlaveDouble *m_pCenterZAnimation;
};

struct CMilRotateTransform3DDuce_Data
{
    DOUBLE m_centerX;
    CMilSlaveDouble *m_pCenterXAnimation;
    DOUBLE m_centerY;
    CMilSlaveDouble *m_pCenterYAnimation;
    DOUBLE m_centerZ;
    CMilSlaveDouble *m_pCenterZAnimation;
    CMilRotation3DDuce *m_pRotation;
};

struct CMilMatrixTransform3DDuce_Data
{
    DirectX::XMFLOAT4X4 m_matrix;
};

struct CMilPixelShaderDuce_Data
{
    ShaderEffectShaderRenderMode::Enum m_ShaderRenderMode;
    UINT32 m_cbPixelShaderBytecodeSize;
    INT *m_pPixelShaderBytecodeData;
    BOOL m_CompileSoftwareShader;
};

struct CMilImplicitInputBrushDuce_Data
{
    DOUBLE m_Opacity;
    CMilSlaveDouble *m_pOpacityAnimation;
    CMilTransformDuce *m_pTransform;
    CMilTransformDuce *m_pRelativeTransform;
};

struct CMilBlurEffectDuce_Data
{
    DOUBLE m_Radius;
    CMilSlaveDouble *m_pRadiusAnimation;
    MilKernelType::Enum m_KernelType;
    MilEffectRenderingBias::Enum m_RenderingBias;
};

struct CMilDropShadowEffectDuce_Data
{
    DOUBLE m_ShadowDepth;
    CMilSlaveDouble *m_pShadowDepthAnimation;
    MilColorF m_Color;
    CMilSlaveColor *m_pColorAnimation;
    DOUBLE m_Direction;
    CMilSlaveDouble *m_pDirectionAnimation;
    DOUBLE m_Opacity;
    CMilSlaveDouble *m_pOpacityAnimation;
    DOUBLE m_BlurRadius;
    CMilSlaveDouble *m_pBlurRadiusAnimation;
    MilEffectRenderingBias::Enum m_RenderingBias;
};

struct CMilShaderEffectDuce_Data
{
    CMilPixelShaderDuce *m_pPixelShader;
    INT m_DdxUvDdyUvRegisterIndex;
    UINT32 m_cbShaderConstantFloatRegistersSize;
    SHORT *m_pShaderConstantFloatRegistersData;
    UINT32 m_cbDependencyPropertyFloatValuesSize;
    FLOAT *m_pDependencyPropertyFloatValuesData;
    UINT32 m_cbShaderConstantIntRegistersSize;
    SHORT *m_pShaderConstantIntRegistersData;
    UINT32 m_cbDependencyPropertyIntValuesSize;
    INT *m_pDependencyPropertyIntValuesData;
    UINT32 m_cbShaderConstantBoolRegistersSize;
    SHORT *m_pShaderConstantBoolRegistersData;
    UINT32 m_cbDependencyPropertyBoolValuesSize;
    BOOL *m_pDependencyPropertyBoolValuesData;
    UINT32 m_cbShaderSamplerRegistrationInfoSize;
    INT *m_pShaderSamplerRegistrationInfoData;
    UINT32 m_cbDependencyPropertySamplerValuesSize;
    INT *m_pDependencyPropertySamplerValuesData;
    DOUBLE m_TopPadding;
    DOUBLE m_BottomPadding;
    DOUBLE m_LeftPadding;
    DOUBLE m_RightPadding;
};

struct CMilDrawingImageDuce_Data
{
    CMilDrawingDuce *m_pDrawing;
};

struct CMilTransformGroupDuce_Data
{
    UINT32 m_cChildren;
    CMilTransformDuce **m_rgpChildren;
};

struct CMilTranslateTransformDuce_Data
{
    DOUBLE m_X;
    CMilSlaveDouble *m_pXAnimation;
    DOUBLE m_Y;
    CMilSlaveDouble *m_pYAnimation;
};

struct CMilScaleTransformDuce_Data
{
    DOUBLE m_ScaleX;
    CMilSlaveDouble *m_pScaleXAnimation;
    DOUBLE m_ScaleY;
    CMilSlaveDouble *m_pScaleYAnimation;
    DOUBLE m_CenterX;
    CMilSlaveDouble *m_pCenterXAnimation;
    DOUBLE m_CenterY;
    CMilSlaveDouble *m_pCenterYAnimation;
};

struct CMilSkewTransformDuce_Data
{
    DOUBLE m_AngleX;
    CMilSlaveDouble *m_pAngleXAnimation;
    DOUBLE m_AngleY;
    CMilSlaveDouble *m_pAngleYAnimation;
    DOUBLE m_CenterX;
    CMilSlaveDouble *m_pCenterXAnimation;
    DOUBLE m_CenterY;
    CMilSlaveDouble *m_pCenterYAnimation;
};

struct CMilRotateTransformDuce_Data
{
    DOUBLE m_Angle;
    CMilSlaveDouble *m_pAngleAnimation;
    DOUBLE m_CenterX;
    CMilSlaveDouble *m_pCenterXAnimation;
    DOUBLE m_CenterY;
    CMilSlaveDouble *m_pCenterYAnimation;
};

struct CMilMatrixTransformDuce_Data
{
    MilMatrix3x2D m_Matrix;
    CMilSlaveMatrix *m_pMatrixAnimation;
};

struct CMilLineGeometryDuce_Data
{
    CMilTransformDuce *m_pTransform;
    MilPoint2D m_StartPoint;
    CMilSlavePoint *m_pStartPointAnimation;
    MilPoint2D m_EndPoint;
    CMilSlavePoint *m_pEndPointAnimation;
};

struct CMilRectangleGeometryDuce_Data
{
    CMilTransformDuce *m_pTransform;
    DOUBLE m_RadiusX;
    CMilSlaveDouble *m_pRadiusXAnimation;
    DOUBLE m_RadiusY;
    CMilSlaveDouble *m_pRadiusYAnimation;
    MilPointAndSizeD m_Rect;
    CMilSlaveRect *m_pRectAnimation;
};

struct CMilEllipseGeometryDuce_Data
{
    CMilTransformDuce *m_pTransform;
    DOUBLE m_RadiusX;
    CMilSlaveDouble *m_pRadiusXAnimation;
    DOUBLE m_RadiusY;
    CMilSlaveDouble *m_pRadiusYAnimation;
    MilPoint2D m_Center;
    CMilSlavePoint *m_pCenterAnimation;
};

struct CMilGeometryGroupDuce_Data
{
    CMilTransformDuce *m_pTransform;
    MilFillMode::Enum m_FillRule;
    UINT32 m_cChildren;
    CMilGeometryDuce **m_rgpChildren;
};

struct CMilCombinedGeometryDuce_Data
{
    CMilTransformDuce *m_pTransform;
    MilCombineMode::Enum m_GeometryCombineMode;
    CMilGeometryDuce *m_pGeometry1;
    CMilGeometryDuce *m_pGeometry2;
};

struct CMilPathGeometryDuce_Data
{
    CMilTransformDuce *m_pTransform;
    MilFillMode::Enum m_FillRule;
    UINT32 m_cbFiguresSize;
    MilPathGeometry *m_pFiguresData;
};

struct CMilSolidColorBrushDuce_Data
{
    DOUBLE m_Opacity;
    CMilSlaveDouble *m_pOpacityAnimation;
    CMilTransformDuce *m_pTransform;
    CMilTransformDuce *m_pRelativeTransform;
    MilColorF m_Color;
    CMilSlaveColor *m_pColorAnimation;
};

struct CMilLinearGradientBrushDuce_Data
{
    DOUBLE m_Opacity;
    CMilSlaveDouble *m_pOpacityAnimation;
    CMilTransformDuce *m_pTransform;
    CMilTransformDuce *m_pRelativeTransform;
    MilColorInterpolationMode::Enum m_ColorInterpolationMode;
    MilBrushMappingMode::Enum m_MappingMode;
    MilGradientSpreadMethod::Enum m_SpreadMethod;
    UINT32 m_cbGradientStopsSize;
    MilGradientStop *m_pGradientStopsData;
    MilPoint2D m_StartPoint;
    CMilSlavePoint *m_pStartPointAnimation;
    MilPoint2D m_EndPoint;
    CMilSlavePoint *m_pEndPointAnimation;
};

struct CMilRadialGradientBrushDuce_Data
{
    DOUBLE m_Opacity;
    CMilSlaveDouble *m_pOpacityAnimation;
    CMilTransformDuce *m_pTransform;
    CMilTransformDuce *m_pRelativeTransform;
    MilColorInterpolationMode::Enum m_ColorInterpolationMode;
    MilBrushMappingMode::Enum m_MappingMode;
    MilGradientSpreadMethod::Enum m_SpreadMethod;
    UINT32 m_cbGradientStopsSize;
    MilGradientStop *m_pGradientStopsData;
    MilPoint2D m_Center;
    CMilSlavePoint *m_pCenterAnimation;
    DOUBLE m_RadiusX;
    CMilSlaveDouble *m_pRadiusXAnimation;
    DOUBLE m_RadiusY;
    CMilSlaveDouble *m_pRadiusYAnimation;
    MilPoint2D m_GradientOrigin;
    CMilSlavePoint *m_pGradientOriginAnimation;
};

struct CMilImageBrushDuce_Data
{
    DOUBLE m_Opacity;
    CMilSlaveDouble *m_pOpacityAnimation;
    CMilTransformDuce *m_pTransform;
    CMilTransformDuce *m_pRelativeTransform;
    MilBrushMappingMode::Enum m_ViewportUnits;
    MilBrushMappingMode::Enum m_ViewboxUnits;
    MilPointAndSizeD m_Viewport;
    CMilSlaveRect *m_pViewportAnimation;
    MilPointAndSizeD m_Viewbox;
    CMilSlaveRect *m_pViewboxAnimation;
    MilStretch::Enum m_Stretch;
    MilTileMode::Enum m_TileMode;
    MilHorizontalAlignment::Enum m_AlignmentX;
    MilVerticalAlignment::Enum m_AlignmentY;
    MilCachingHint::Enum m_CachingHint;
    DOUBLE m_CacheInvalidationThresholdMinimum;
    DOUBLE m_CacheInvalidationThresholdMaximum;
    CMilImageSource *m_pImageSource;
};

struct CMilDrawingBrushDuce_Data
{
    DOUBLE m_Opacity;
    CMilSlaveDouble *m_pOpacityAnimation;
    CMilTransformDuce *m_pTransform;
    CMilTransformDuce *m_pRelativeTransform;
    MilBrushMappingMode::Enum m_ViewportUnits;
    MilBrushMappingMode::Enum m_ViewboxUnits;
    MilPointAndSizeD m_Viewport;
    CMilSlaveRect *m_pViewportAnimation;
    MilPointAndSizeD m_Viewbox;
    CMilSlaveRect *m_pViewboxAnimation;
    MilStretch::Enum m_Stretch;
    MilTileMode::Enum m_TileMode;
    MilHorizontalAlignment::Enum m_AlignmentX;
    MilVerticalAlignment::Enum m_AlignmentY;
    MilCachingHint::Enum m_CachingHint;
    DOUBLE m_CacheInvalidationThresholdMinimum;
    DOUBLE m_CacheInvalidationThresholdMaximum;
    CMilDrawingDuce *m_pDrawing;
};

struct CMilVisualBrushDuce_Data
{
    DOUBLE m_Opacity;
    CMilSlaveDouble *m_pOpacityAnimation;
    CMilTransformDuce *m_pTransform;
    CMilTransformDuce *m_pRelativeTransform;
    MilBrushMappingMode::Enum m_ViewportUnits;
    MilBrushMappingMode::Enum m_ViewboxUnits;
    MilPointAndSizeD m_Viewport;
    CMilSlaveRect *m_pViewportAnimation;
    MilPointAndSizeD m_Viewbox;
    CMilSlaveRect *m_pViewboxAnimation;
    MilStretch::Enum m_Stretch;
    MilTileMode::Enum m_TileMode;
    MilHorizontalAlignment::Enum m_AlignmentX;
    MilVerticalAlignment::Enum m_AlignmentY;
    MilCachingHint::Enum m_CachingHint;
    DOUBLE m_CacheInvalidationThresholdMinimum;
    DOUBLE m_CacheInvalidationThresholdMaximum;
    CMilVisual *m_pVisual;
};

struct CMilBitmapCacheBrushDuce_Data
{
    DOUBLE m_Opacity;
    CMilSlaveDouble *m_pOpacityAnimation;
    CMilTransformDuce *m_pTransform;
    CMilTransformDuce *m_pRelativeTransform;
    CMilBitmapCacheDuce *m_pBitmapCache;
    CMilVisual *m_pInternalTarget;
};

struct CMilDashStyleDuce_Data
{
    DOUBLE m_Offset;
    CMilSlaveDouble *m_pOffsetAnimation;
    UINT32 m_cbDashesSize;
    DOUBLE *m_pDashesData;
};

struct CMilPenDuce_Data
{
    CMilBrushDuce *m_pBrush;
    DOUBLE m_Thickness;
    CMilSlaveDouble *m_pThicknessAnimation;
    MilPenCap::Enum m_StartLineCap;
    MilPenCap::Enum m_EndLineCap;
    MilPenCap::Enum m_DashCap;
    MilPenJoin::Enum m_LineJoin;
    DOUBLE m_MiterLimit;
    CMilDashStyleDuce *m_pDashStyle;
};

struct CMilGeometryDrawingDuce_Data
{
    CMilBrushDuce *m_pBrush;
    CMilPenDuce *m_pPen;
    CMilGeometryDuce *m_pGeometry;
};

struct CMilGlyphRunDrawingDuce_Data
{
    CGlyphRunResource *m_pGlyphRun;
    CMilBrushDuce *m_pForegroundBrush;
};

struct CMilImageDrawingDuce_Data
{
    CMilImageSource *m_pImageSource;
    MilPointAndSizeD m_Rect;
    CMilSlaveRect *m_pRectAnimation;
};

struct CMilVideoDrawingDuce_Data
{
    CMilSlaveVideo *m_pPlayer;
    MilPointAndSizeD m_Rect;
    CMilSlaveRect *m_pRectAnimation;
};

struct CMilDrawingGroupDuce_Data
{
    UINT32 m_cChildren;
    CMilDrawingDuce **m_rgpChildren;
    CMilGeometryDuce *m_pClipGeometry;
    DOUBLE m_Opacity;
    CMilSlaveDouble *m_pOpacityAnimation;
    CMilBrushDuce *m_pOpacityMask;
    CMilTransformDuce *m_pTransform;
    CMilGuidelineSetDuce *m_pGuidelineSet;
    MilEdgeMode::Enum m_EdgeMode;
    MilBitmapScalingMode::Enum m_bitmapScalingMode;
    MilClearTypeHint::Enum m_ClearTypeHint;
};

struct CMilGuidelineSetDuce_Data
{
    UINT32 m_cbGuidelinesXSize;
    DOUBLE *m_pGuidelinesXData;
    UINT32 m_cbGuidelinesYSize;
    DOUBLE *m_pGuidelinesYData;
    BOOL m_IsDynamic;
};

struct CMilBitmapCacheDuce_Data
{
    DOUBLE m_RenderAtScale;
    CMilSlaveDouble *m_pRenderAtScaleAnimation;
    BOOL m_SnapsToDevicePixels;
    BOOL m_EnableClearType;
};


