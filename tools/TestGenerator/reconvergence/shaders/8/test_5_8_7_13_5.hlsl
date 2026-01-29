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
    switch (WaveGetLaneIndex() & 3) {
        case 0:
        {
            if (testBit(uint4(0x6dfa865e, 0x5a043f88, 0x2d9b688a, 0x3e9ec403), WaveGetLaneIndex())) {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10005;
            } else {
                switch (WaveGetLaneIndex() & 3) {
                    case 0:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        break;
                    }
                    case 1:
                    {
                        if (testBit(uint4(0x6dfa865e, 0x5a043f88, 0x2d9b688a, 0x3e9ec403), WaveGetLaneIndex())) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (InputA[2] == 2) {
                            } else {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10010;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10013;
                        }
                        break;
                    }
                    case 2:
                    {
                        if (gIndex >= InputA[0x43]) {
                            if (gIndex >= InputA[0x46]) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                while (!WaveIsFirstLane()) {}
                            } else {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                while (!WaveIsFirstLane()) {}
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10020;
                            if (gIndex >= InputA[0x27]) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                        } else {
                            if (gIndex >= InputA[0x8]) {
                            } else {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10029;
                            if (InputA[2] == 2) {
                            } else {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1002c;
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1002f;
                        break;
                    }
                    case 3:
                    {
                        for (int loopIdx0 = 0;;loopIdx0++,                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                            if (WaveGetLaneIndex() == loopIdx0) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        break;
                    }
                }
            }
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10040;
            break;
        }
        case 1:
        {
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            if (gIndex >= InputA[0x12]) {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10046;
                switch (InputA[4]) {
                    case 5:
                    {
                        for (int loopIdx0 = 0;
                                 loopIdx0 < InputA[1];
                                 loopIdx0++) {
                            break;
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1004b;
                            if (WaveGetLaneIndex() == loopIdx0) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            } else {
                            }
                        }
                        break;
                    }
                    case 4:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        break;
                    }
                    case 6:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveIsFirstLane()) {
                            if (InputA[2] == 2) {
                            } else {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1005a;
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1005c;
                            if (testBit(uint4(0x2d48218, 0x4beafcfd, 0x3d258fdc, 0xf9d6175a), WaveGetLaneIndex())) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1005e;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        break;
                    }
                }
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            break;
        }
        case 2:
        {
            break;
        }
        case 3:
        {
            if (WaveIsFirstLane()) {
                if (InputA[0] == 0) {
                    for (int loopIdx0 = 0;;loopIdx0++,                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006f;
                        {
                            int loopIdx1 = 0;
                            do {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10071;
                                if (WaveIsFirstLane()) {
                                    if (testBit(uint4(0xadba7496, 0xdca3621e, 0x6e6723a8, 0x23fb3d07), WaveGetLaneIndex())) {
                                        break;
                                    } else {
                                        break;
                                    }
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10079;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                loopIdx1++;
                            } while (true);
                        }
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            break;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        {
                            int loopIdx1 = 0;
                            do {
                                loopIdx1++;
                            } while (loopIdx1 < InputA[3]);
                        }
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
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1008d;
                                break;
                            }
                        }
                    }
                    if (WaveIsFirstLane()) {
                        for (int loopIdx0 = 0;
                                 loopIdx0 < WaveGetLaneIndex() + 1;
                                 loopIdx0++) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10093;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10096;
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                    }
                }
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1009c;
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
            break;
        }
    }
    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
}

