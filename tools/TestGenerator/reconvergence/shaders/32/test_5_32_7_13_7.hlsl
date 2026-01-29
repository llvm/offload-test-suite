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
    if (InputA[3] == 3) {
        switch (WaveGetLaneIndex() & 3) {
            case 0:
            {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                break;
            }
            case 1:
            {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (testBit(uint4(0x89eabcfb, 0xd9b69862, 0xe646b896, 0xe35f35d1), WaveGetLaneIndex())) {
                    if (gIndex >= InputA[0x37]) {
                        if (gIndex >= InputA[0xb]) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        if (InputA[2] == 2) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10010;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10012;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10014;
                } else {
                    if (gIndex >= InputA[0x37]) {
                        if (gIndex >= InputA[0xb]) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        if (InputA[0] == 0) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10010;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10012;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10014;
                }
                break;
            }
            case 2:
            {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                break;
            }
            case 3:
            {
                if (WaveIsFirstLane()) {
                    if (gIndex >= InputA[0x36]) {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1002b;
                        {
                            int loopIdx0 = 0;
                            do {
                                loopIdx0++;
                            } while (loopIdx0 < InputA[4]);
                        }
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (gIndex >= InputA[0x27]) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                }
                break;
            }
        }
    } else {
        if (InputA[1] == 1) {
            if (testBit(uint4(0x89eabcfb, 0xd9b69862, 0xe646b896, 0xe35f35d1), WaveGetLaneIndex())) {
                if (WaveIsFirstLane()) {
                    for (int loopIdx0 = 0;;loopIdx0++,                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                        if (WaveIsFirstLane()) {
                            break;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10045;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10047;
                    if (testBit(uint4(0xdec52586, 0x6b5046f6, 0xc800a33e, 0xd06851f7), WaveGetLaneIndex())) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1004b;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10050;
            }
        }
    }
}

