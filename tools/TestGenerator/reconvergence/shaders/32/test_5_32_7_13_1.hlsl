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
                        break;
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    break;
                    if (WaveIsFirstLane()) {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10009;
                        break;
                    }
                    if (InputA[2] == 2) {
                        if (WaveGetLaneIndex() == loopIdx1) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                    }
                    if (WaveGetLaneIndex() == loopIdx1) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        for (int loopIdx2 = 0;;loopIdx2++,                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (WaveIsFirstLane()) {
                                break;
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (gIndex >= InputA[0x47]) {
                        } else {
                        }
                    }
                }
                if (InputA[0] == 0) {
                    continue;
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (WaveIsFirstLane()) {
                        if (gIndex >= InputA[0x2f]) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10027;
                        }
                        if (testBit(uint4(0xe891fb36, 0xf509aea0, 0x6713fbdf, 0x4dafce1c), WaveGetLaneIndex())) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1002d;
                        }
                    }
                }
            } else {
                break;
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (gIndex >= InputA[0x25]) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (WaveIsFirstLane()) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        {
                            int loopIdx1 = 0;
                            do {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                if (WaveIsFirstLane()) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                loopIdx1++;
                            } while (true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveGetLaneIndex() == loopIdx0) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10045;
                    if (testBit(uint4(0x8db206f9, 0x1ef87d91, 0x28c04adc, 0x9ac7eb8b), WaveGetLaneIndex())) {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10047;
                        if (WaveGetLaneIndex() == loopIdx0) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                } else {
                    {
                        int loopIdx1 = 0;
                        do {
                            loopIdx1++;
                            switch (WaveGetLaneIndex() & 3) {
                                case 0:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                                case 1:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10057;
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
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1005e;
                            {
                                int loopIdx2 = 0;
                                do {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    if (WaveIsFirstLane()) {
                                        break;
                                    }
                                    while (!WaveIsFirstLane()) {}
                                    loopIdx2++;
                                } while (true);
                            }
                        } while (loopIdx1 < InputA[5]);
                    }
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            break;
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006c;
        } while (loopIdx0 < InputA[2]);
    }
}

