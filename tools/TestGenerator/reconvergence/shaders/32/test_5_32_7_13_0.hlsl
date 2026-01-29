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
            {
                int loopIdx1 = 0;
                do {
                    loopIdx1++;
                    if (InputA[0] == 0) {
                        if (WaveGetLaneIndex() == loopIdx1) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                    } else {
                        if (WaveGetLaneIndex() == loopIdx1) {
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (InputA[1] == 1) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10013;
                    if (testBit(uint4(0xdca35e7e, 0xdca345ea, 0x1f7f4828, 0x52e219cc), WaveGetLaneIndex())) {
                        continue;
                    } else {
                        continue;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                } while (loopIdx1 < InputA[1]);
            }
        }
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
    }
}

