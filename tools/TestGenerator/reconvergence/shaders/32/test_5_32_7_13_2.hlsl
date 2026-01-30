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
            if (gIndex >= InputA[0x23]) {
                for (int loopIdx0 = 0;
                         loopIdx0 < WaveGetLaneIndex() + 1;
                         loopIdx0++) {
                    if (testBit(uint4(0x3b805da9, 0x57aea542, 0x98bb588a, 0xd3d87c8f), WaveGetLaneIndex())) {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10006;
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10009;
                    switch (WaveGetLaneIndex() & 3) {
                        case 0:
                        {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
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
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            break;
                        }
                    }
                }
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10017;
                switch (WaveGetLaneIndex() & 3) {
                    case 0:
                    {
                        if (gIndex >= InputA[0x13]) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1001c;
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        break;
                    }
                    case 1:
                    {
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        break;
                    }
                    case 2:
                    {
                        if (InputA[1] == 1) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10028;
                        }
                        break;
                    }
                    case 3:
                    {
                        if (gIndex >= InputA[0x25]) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1002d;
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10031;
                        break;
                    }
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            } else {
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10036;
                {
                    int loopIdx0 = 0;
                    do {
                        loopIdx0++;
                        continue;
                        if (testBit(uint4(0x4be9eec2, 0x80c7a6f2, 0xe252fe7f, 0xf1ddec4d), WaveGetLaneIndex())) {
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                    } while (loopIdx0 < InputA[3]);
                }
                {
                    int loopIdx0 = 0;
                    do {
                        switch (loopIdx0) {
                            case 1: {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                            case 2: {
                                break;
                            }
                            default: {
                                break;
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10048;
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveGetLaneIndex() == loopIdx0) {
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1004d;
                        }
                        if (WaveIsFirstLane()) {
                            break;
                        }
                        if (WaveGetLaneIndex() == loopIdx0) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
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
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                            case 3:
                            {
                                break;
                            }
                        }
                        loopIdx0++;
                    } while (true);
                }
            }
            if (testBit(uint4(0x83491595, 0xba1d7a6a, 0xd1585899, 0x93653d77), WaveGetLaneIndex())) {
                if (gIndex >= InputA[0x27]) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (gIndex >= InputA[0x43]) {
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10068;
                }
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006a;
                if (gIndex >= InputA[0x39]) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    for (int loopIdx0 = 0;
                             loopIdx0 < InputA[5];
                             loopIdx0++) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006f;
                    }
                }
            }
        }
        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10074;
    }
}

