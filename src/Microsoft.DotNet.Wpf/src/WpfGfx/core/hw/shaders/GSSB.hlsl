Texture2D g_FontTexture;
SamplerState g_FontSampler;

struct VS_OUTPUT
{
    float2 TextureUV  : TEXCOORD0;   // vertex texture coords 
};

cbuffer ConstantBuffer
{
	float4 c0;
	float4 c1;
	float4 c2;
	float4 c3;
	float4 c4;
	float4 c5;
};

float4 main(float2 uv : TEXCOORD0) : SV_Target {
    // Constant registers:
    // c2.a = a           // brush alpha
    // c3.a = 1
    // c4.a = g1*f + g2 // f = brush color (rgb)
    // c5.a = g3*f + g4 // f = brush color (rgb)
    // where g1, g2, g3 and g4 are alpha correction ratios

    // fetch glyph alpha
    float4 r0, r1, r2;
    r0.a = g_FontTexture.Sample(g_FontSampler, uv).a;

    r0.a = r0.a * c2.a; // multiply by alpha

    // Corrected alpha is calculated in r0.rgb as
    // new a = a + a*(1-a)*((g1*f + g2)*a + (g3*f + g4))*4

    r1.a = c4.a * r0.a; // r1 = (g1*f + g2)*a
    r1.a = r1.a + c5.a;  // r1 = (g1*f + g2)*a + (g3*f + g4)

    r2.a = c3.a - r0.a;  // r2 = 1-a
    r1.a = r1.a * r2.a;  // r1 = (1-a)*((g1*f + g2)*a + (g3*f + g4))
    r1.a = r1.a + r1.a;  // r1 = (1-a)*((g1*f + g2)*a + (g3*f + g4))*2
    r1.a = r1.a + r1.a;  // r1 = (1-a)*((g1*f + g2)*a + (g3*f + g4))*4
    r0.a = r0.a * r1.a + r0.a;  // r0 = a + a*(1-a)*((g1*f + g2)*a + (g3*f + g4))*4

    return r0.aaaa;
}
