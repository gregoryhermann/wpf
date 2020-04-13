Texture2D g_BrushTexture;
SamplerState g_BrushSampler;

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

float4 main(float2 uv0 : TEXCOORD0, float2 uv1 : TEXCOORD1) : SV_Target {
    // Constant registers:
    // c0: color channel mask: (1,0,0,0) = red; (0,1,0,0) = green; (0,0,1,0) = blue
    // c1: (d4,d4,d4,d1) // gamma
    // c2: (d5,d5,d5,d3) //   correction
    // c3: (d6,d6,d6,d2) //     ratios
    // c4: ( 1, 1, 1, 1)
    // c5.a: effect alpha 

    float4 r0, r1, r2, r3;

    r0 = g_BrushTexture.Sample(g_BrushSampler, uv0); // fetch brush color (rgba, alpha-premultiplied)
    r1 = g_FontTexture.Sample(g_FontSampler, uv1); // fetch the alpha (a)

    r0 = saturate(r0*c5.a);    // combine brush color with effect alpha
    r1 = saturate(r0*r1.a);    // combine alphas

    r1.rgb = saturate(dot(r1, c0));      // replicate desired channel to r1.rgb (r1.rgb = af, r1.a = a)

    // corrected alpha value is calculated in r0.a as:
    // new a = a + (1-a)*(d2*af + (d3*af + d1*a)*a)*4

    // corrected premultiplied foreground
    // is calculated in r0.rgb as following:
    // new af = af + af*(1-a)*(d4 + d5*a + d6*af)*4

    r0.a = c1 * r1;            // d1*a
    r0.rgb = c2 * r1.a;        // d5*a
    r0.rgb = r0 + c1;          // d4 + d5*a
    r0.a = (c2 * r1.b) + r0;   // d3*af + d1*a
    r0.rgb = c3 * r1 + r0;     // d4 + d5*a + d6*af
    r0.a = r1 * r0;            // d3*a*af + d1*a*a
    r3 = c4 - r1;
    r0.rgb = r3.a * r0;         // (1-a)*(d4 + d5*a + d6*af)*4
    r0.a = c3*r1.b + r0;        // (d2*af + d3*a*af + d1*a*a)*4
    r0 = r0 + r0;
    r0 = r0 + r0;
    r0.rgb = r1 * r0 + r1;      // af + af*(1-a)*(d4 + d5*a + d6*af)*4
    r0.a = r3 * r0 + r1;        // a + (1-a)*(d2*af + d3*a*af + d1*a*a)*4

    return r0;
}

