// SPDX-License-Identifier: Apache-2.0
// Ordinary D3D11 fullscreen triangle, no Mavk-specific shader/API operations.
float4 vsMain(uint id : SV_VertexID) : SV_Position {
    float2 p = id == 0 ? float2(-1, -1) : id == 1 ? float2(-1, 3) : float2(3, -1);
    return float4(p, 0, 1);
}
float4 psMain() : SV_Target { return float4(0.25, 0.75, 0.5, 1); }
Texture2D<float4> source : register(t0);
float4 psTextured(float4 p : SV_Position) : SV_Target {
    return source.Load(int3(uint2(p.xy) & 3u, 0));
}
SamplerState textureFilter : register(s0);
float4 psFiltered(float4 p : SV_Position) : SV_Target {
    return source.SampleLevel(textureFilter, p.xy / 8.0, 0);
}
