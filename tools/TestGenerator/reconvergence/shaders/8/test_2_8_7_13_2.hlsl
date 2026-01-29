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
    if (testBit(uint4(0x4be9eec2, 0x80c7a6f2, 0xe252fe7f, 0xf1ddec4d), WaveGetLaneIndex())) {
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        if (WaveIsFirstLane()) {
        }
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10005;
        for (int loopIdx0 = 0;
                 loopIdx0 < WaveGetLaneIndex() + 1;
                 loopIdx0++) {
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10007;
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        }
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
    }
    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000c;
    for (int loopIdx0 = 0;;loopIdx0++,    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
        {
            int loopIdx1 = 0;
            do {
                loopIdx1++;
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            } while (loopIdx1 < InputA[2]);
        }
        if (gIndex >= InputA[0x30]) {
        } else {
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10013;
        }
        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10015;
        if (WaveIsFirstLane()) {
            break;
        }
        if (WaveIsFirstLane()) {
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        }
        if (InputA[3] == 3) {
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1001d;
        }
    }
}

