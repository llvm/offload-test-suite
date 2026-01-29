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
            if (InputA[2] == 2) {
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
                    if (InputA[3] == 3) {
                        if (WaveGetLaneIndex() == loopIdx1) {
                            switch (WaveGetLaneIndex() & 3) {
                                case 0:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                                case 1:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10013;
                                    break;
                                }
                                case 2:
                                {
                                    break;
                                }
                                case 3:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                            }
                            if (testBit(uint4(0x1b511116, 0xdca3421a, 0xd88d4708, 0x95104e5a), WaveGetLaneIndex())) {
                            }
                        } else {
                            for (int loopIdx2 = 0;;loopIdx2++,                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                if (WaveIsFirstLane()) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10026;
                            if (gIndex >= InputA[0x55]) {
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        switch (WaveGetLaneIndex() & 3) {
                            case 0:
                            {
                                break;
                            }
                            case 1:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                if (testBit(uint4(0x8db206f9, 0x1ef87d91, 0x28c04adc, 0x9ac7eb8b), WaveGetLaneIndex())) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                } else {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                }
                                break;
                            }
                            case 2:
                            {
                                break;
                            }
                            case 3:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1003a;
                                break;
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    if (WaveIsFirstLane()) {
                        if (WaveGetLaneIndex() == loopIdx1) {
                            if (InputA[2] == 2) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10042;
                            }
                            break;
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            switch (WaveGetLaneIndex() & 3) {
                                case 0:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                                case 1:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                                case 2:
                                {
                                    break;
                                }
                                case 3:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveGetLaneIndex() == loopIdx1) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10058;
                            break;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            switch (WaveGetLaneIndex() & 3) {
                                case 0:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                                case 1:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10060;
                                    break;
                                }
                                case 2:
                                {
                                    break;
                                }
                                case 3:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                            }
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10058;
                            break;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            switch (WaveGetLaneIndex() & 3) {
                                case 0:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                                case 1:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10060;
                                    break;
                                }
                                case 2:
                                {
                                    break;
                                }
                                case 3:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                            }
                        }
                    }
                }
                if (WaveGetLaneIndex() == loopIdx0) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (testBit(uint4(0x1b511116, 0xdca3421a, 0xd88d4708, 0x95104e5a), WaveGetLaneIndex())) {
                        break;
                    } else {
                        break;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    for (int loopIdx1 = 0;
                             loopIdx1 < WaveGetLaneIndex() + 1;
                             loopIdx1++) {
                        break;
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10088;
                }
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1008a;
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        } while (loopIdx0 < InputA[2]);
    }
}

