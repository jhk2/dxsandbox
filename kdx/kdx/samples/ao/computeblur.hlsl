// input ao buffer
Texture2D source_texture : register(t0);

RWTexture2D<float4> output_texture : register(u0);

#define TILE_SIZE 16
#define FILTER_SIZE 5

static const uint2 tileSize = uint2(TILE_SIZE, TILE_SIZE);
static const uint2 filterOffset = uint2(FILTER_SIZE/2, FILTER_SIZE/2);
static const uint2 neighborSize = tileSize + 2*filterOffset;
groupshared float4 neighborhood[neighborSize.x][neighborSize.y];

// TODO: add gaussian filter, separable filters

float filterBox(uint x, uint y) {
    return 1.0 / (FILTER_SIZE * FILTER_SIZE);
}

uint2 clampLocation(uint2 input)
{
	uint width; uint height;
	source_texture.GetDimensions(width, height);
    return clamp(input, uint2(0,0), uint2(width, height)); // assuming that in/out images are same size
}

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void ComputeMain( uint3 tile_id : SV_GroupID, uint3 thread_id : SV_GroupThreadID )
{
	const uint2 pixel = tile_id.xy * tileSize + thread_id.xy;
	const uint x = thread_id.x; const uint y = thread_id.y;
	// copy into shared memory
	for (uint i = 0; i < neighborSize.y; i+=tileSize.y) {
        for (uint j = 0; j < neighborSize.x; j+=tileSize.x){
            if ((x+j) < neighborSize.x && (y+i) < neighborSize.y) {
                const uint2 read_coord = clampLocation(uint2(j,i) + pixel - filterOffset);
                neighborhood[x+j][y+i] = source_texture.Load(int3(read_coord, 0));
            }
        }
    }
	
	GroupMemoryBarrierWithGroupSync();

	float4 total;
    // next, perform the convolution
    // position of the current pixel in shared memory is thread_id + filterOffset
    const uint2 shared_pixel = thread_id.xy + filterOffset; // guaranteed to be within shared memory bounds
    // so do filter lookups relative to that position

    for (uint i = 0; i < FILTER_SIZE; i++) {
        for (uint j = 0; j < FILTER_SIZE; j++) {
            const uint2 offset = uint2(j, i) - filterOffset; // go from -filtersize/2 to +filtersize/2 both ways
            total += neighborhood[shared_pixel.x + offset.x][shared_pixel.y + offset.y] * filterBox(j, i);
        }
    }

	output_texture[pixel] = float4(total.r, total.r, total.r, 1.0);
}