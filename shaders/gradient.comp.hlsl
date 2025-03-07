
// bind RWTexture2D for rgba16f
// "u" register is for UAVs, "space0" corresponds to set 0
RWTexture2D<float4> image : register(u0, space0);

[numthreads(16, 16, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupThreadID : SV_GroupThreadID)
{
    int2 texelCoord = int2(dispatchThreadID.xy); // equivalent to gl_GlobalInvocationID.xy

    uint width, height;
    image.GetDimensions(width, height);
    int2 size = int2(width, height);

    if (texelCoord.x < size.x && texelCoord.y < size.y)
    {
        float4 color = float4(0.0, 0.0, 0.0, 1.0);

        if (groupThreadID.x != 0 && groupThreadID.y != 0)
        {
            color.x = float(texelCoord.x) / float(size.x);
            color.y = float(texelCoord.y) / float(size.y);
        }

        image[texelCoord] = color;
    }
}