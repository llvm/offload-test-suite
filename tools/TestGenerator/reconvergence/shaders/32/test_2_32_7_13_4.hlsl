#define THREADS_X 7
#define THREADS_Y 13

StructuredBuffer<uint> InputA : register(t0);
RWStructuredBuffer<uint4> OutputB : register(u1);

bool testBit(uint4 mask, uint bit) { return ((mask[bit / 32] >> (bit % 32)) & 1) != 0; }
static int outLoc = 0;
static int invocationStride = 91;


[numthreads(THREADS_X, THREADS_Y, 1)]
void main(uint gIndex : SV_GroupIndex)
{
    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10000;
    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
    if (testBit(uint4(0x7c271cdc, 0xcb18f481, 0x6ec4196a, 0x550db835), WaveGetLaneIndex())) {
        if (InputA[2] == 2) {
        }
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10006;
        for (int loopIdx0 = 0;
                 loopIdx0 < InputA[5];
                 loopIdx0++) {
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10008;
        }
    } else {
        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000b;
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
    }
    if (InputA[3] == 3) {
        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000f;
        {
            int loopIdx0 = 0;
            do {
                loopIdx0++;
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            } while (loopIdx0 < InputA[3]);
        }
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
    } else {
        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000f;
        {
            int loopIdx0 = 0;
            do {
                loopIdx0++;
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            } while (loopIdx0 < InputA[3]);
        }
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
    }
    if (testBit(uint4(0xc06a63de, 0xdca35a29, 0x3b774a8, 0x4e2b27f5), WaveGetLaneIndex())) {
        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1001c;
        if (gIndex >= InputA[0x35]) {
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        }
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        if (InputA[1] == 1) {
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        }
    }
}

