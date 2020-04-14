// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
// File: DropShadow.fx
//
// Desc: Effect file for image post-processing sample.  This effect contains
//       a single pixel shader that offsets and colors a texture for use
//       in creating a shadow.
//

//-----------------------------------------------------------------------------

// Size is the height of the source texture for vertical shaders, and the width
// for horizontal shaders.
float2 offset : register(C0);
float3 shadowColor : register(c1);
float opacity : register(c2);

texture2D g_samSrcColor : register(t0);
SamplerState g_samSrcColorSamp : register(s0);

texture2D g_samShdColor : register(t1);
SamplerState g_samShdColorSamp : register(s1);

//--------------------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Pos  : POSITION;
    float4 Color: COLOR0;
    float2 TexCoord : TEXCOORD0;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VS( float4 position : POSITION, float4 color : COLOR0, float2 texcoord : TEXCOORD0)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
#pragma warning(disable:3206)
    output.Pos.x = position;
#pragma warning(default)
    output.Color = color;
    output.TexCoord = texcoord;
    return output;
}


//-----------------------------------------------------------------------------
// Pixel Shader: DropShadowPS
// Desc: Returns the source texture with an offset, colored shadow.
//       The shadow texture is intended to be a blurred image.
//-----------------------------------------------------------------------------
float4 PS( VS_OUTPUT input ) : SV_Target
{
    float2 Tex = input.TexCoord;

    float4 outColor;
    float4 srcColor = (g_samSrcColor.Sample(g_samSrcColorSamp, Tex));
    if (srcColor.a != 1.0f)
    {
        float2 shdTex;
        shdTex.x = Tex.x - offset.x; 
        shdTex.y = Tex.y - offset.y;
        float4 shdColor = (g_samShdColor.Sample(g_samShdColorSamp, shdTex));
        shdColor.rgb = shadowColor * shdColor.a * opacity;
        shdColor.a = shdColor.a * opacity;
        outColor = (1 - srcColor.a)*shdColor + srcColor;
    }
    else
    {
        outColor = srcColor;
    }

    return outColor;
}





