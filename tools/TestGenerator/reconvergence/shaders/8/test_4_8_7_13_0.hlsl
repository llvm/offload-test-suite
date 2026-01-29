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
                    if (InputA[2] == 2) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    break;
                } while (loopIdx1 < InputA[1]);
            }
        }
        if (gIndex >= InputA[0x51]) {
            for (int loopIdx0 = 0;
                     loopIdx0 < InputA[5];
                     loopIdx0++) {
                continue;
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000e;
                break;
            }
            for (int loopIdx0 = 0;
                     loopIdx0 < InputA[5];
                     loopIdx0++) {
                if (testBit(uint4(0x1fa337ce, 0xd935d238, 0xb8165c16, 0x5dcd618f), WaveGetLaneIndex())) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                } else {
                }
                if (InputA[2] == 2) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        } else {
            {
                int loopIdx0 = 0;
                do {
                    loopIdx0++;
                    if (InputA[2] == 2) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10020;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10022;
                    if (WaveGetLaneIndex() == loopIdx0) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                } while (loopIdx0 < InputA[4]);
            }
            {
                int loopIdx0 = 0;
                do {
                    loopIdx0++;
                    {
                        int loopIdx1 = 0;
                        do {
                            loopIdx1++;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } while (loopIdx1 < InputA[1]);
                    }
                    if (testBit(uint4(0x78dd605f, 0x1cda5dc7, 0xc225a9aa, 0xbdfc0a37), WaveGetLaneIndex())) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                } while (loopIdx0 < InputA[2]);
            }
        }
    }
}

