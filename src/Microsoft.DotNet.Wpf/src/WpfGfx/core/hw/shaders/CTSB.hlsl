Texture2D g_FontTexture;
SamplerState g_FontSampler;

struct VS_OUTPUT
{
    float2 TextureUV  : TEXCOORD0;   // vertex texture coords 
};

cbuffer ConstantBuffer
{
	float4 c1;
	float4 c2;
	float4 c3;
	float4 c4;
	float4 c5;
};

float4 main(float2 uv : TEXCOORD0) : SV_Target {
    // Constant registers:
    // c1.rg = ds,dt,0,0   // red sub pixel offset in alpha texture coordinates
    // c2.rgb = a,a,a      // brush alpha
    // c3.rgb = 1,1,1
    // c4.rgb = g1*f + g2 // f = brush color (rgb)
    // c5.rgb = g3*f + g4 // f = brush color (rgb)
    // where g1, g2, g3 and g4 are alpha correction ratios

    // fetch glyph alpha for green channel
    float4 r0, r1, r2;

    r1 = g_FontTexture.Sample(g_FontSampler, uv);

    // fetch glyph alpha for blue channel
    r0.xy = uv + c1.xy;
    r2 = g_FontTexture.Sample(g_FontSampler, r0);

    // fetch glyph alpha for red channel
    r0.xy = uv - c1.xy;  // r0 = sampling position for red
    r0 = g_FontTexture.Sample(g_FontSampler, r0);

    // combine color component alphas to single register
    r0.r = r0.a;  // set red
    r0.g = r1.a;  // set green
    r0.b = r2.a;  // set blue

    r0.rgb = r0 * c2;   // multiply glyph alphas by brush alpha
    r0.a = r1.a * c2.r; // set final alpha to the green channel alpha to match greyscale behavior
                        // multiply final alpha by brush alpha

    // Corrected alpha is calculated in r0.rgb as
    // new a = a + a*(1-a)*((g1*f + g2)*a + (g3*f + g4))*4

    r1.rgb = c4 * r0;  // r1 = (g1*f + g2)*a
    r1.rgb = r1 + c5;  // r1 = (g1*f + g2)*a + (g3*f + g4)

    r2.rgb = c3 - r0;  // r2 = 1-a
    r1.rgb = r1 * r2;  // r1 = (1-a)*((g1*f + g2)*a + (g3*f + g4))
    r1.rgb = r1 + r1;  // r1 = (1-a)*((g1*f + g2)*a + (g3*f + g4))*2
    r1.rgb = r1 + r1;  // r1 = (1-a)*((g1*f + g2)*a + (g3*f + g4))*4
    r0.rgb = (r0 * r1) + r0;   // r0 = a + a*(1-a)*((g1*f + g2)*a + (g3*f + g4))*4

    return r0;
}
