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
    if (WaveIsFirstLane()) {
        for (int loopIdx0 = 0;
                 loopIdx0 < InputA[4];
                 loopIdx0++) {
        }
        if (InputA[3] == 3) {
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        } else {
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        }
    }
    if (InputA[0] == 0) {
        {
            int loopIdx0 = 0;
            do {
                if (WaveIsFirstLane()) {
                    break;
                }
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000e;
                loopIdx0++;
            } while (true);
        }
        if (gIndex >= InputA[0x26]) {
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10011;
        } else {
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10013;
        }
        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10015;
    }
    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
    if (InputA[2] == 2) {
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1001a;
    }
    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
    if (testBit(uint4(0xdca35e7e, 0xdca345ea, 0x1f7f4828, 0x52e219cc), WaveGetLaneIndex())) {
        if (gIndex >= InputA[0xb]) {
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10020;
        }
    } else {
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        for (int loopIdx0 = 0;
                 loopIdx0 < InputA[2];
                 loopIdx0++) {
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        }
        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10027;
    }
}

