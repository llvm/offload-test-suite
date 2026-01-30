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
    {
        int loopIdx0 = 0;
        do {
            loopIdx0++;
            if (InputA[0] == 0) {
                for (int loopIdx1 = 0;;loopIdx1++,                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                    if (WaveIsFirstLane()) {
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (InputA[2] == 2) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    if (WaveIsFirstLane()) {
                        break;
                    }
                    if (InputA[2] == 2) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000e;
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10010;
                    }
                    switch (WaveGetLaneIndex() & 3) {
                        case 0:
                        {
                            break;
                        }
                        case 1:
                        {
                            break;
                        }
                        case 2:
                        {
                            break;
                        }
                        case 3:
                        {
                            break;
                        }
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1001f;
            }
            if (WaveIsFirstLane()) {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (testBit(uint4(0xe891fb36, 0xf509aea0, 0x6713fbdf, 0x4dafce1c), WaveGetLaneIndex())) {
                    if (gIndex >= InputA[0x19]) {
                    } else {
                    }
                    continue;
                } else {
                    if (InputA[2] == 2) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1002b;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (WaveIsFirstLane()) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10030;
                    }
                }
                continue;
            }
        } while (loopIdx0 < InputA[2]);
    }
}

