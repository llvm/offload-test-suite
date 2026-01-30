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
    for (int loopIdx0 = 0;
             loopIdx0 < InputA[5];
             loopIdx0++) {
        for (int loopIdx1 = 0;
                 loopIdx1 < WaveGetLaneIndex() + 1;
                 loopIdx1++) {
            if (WaveIsFirstLane()) {
                {
                    int loopIdx2 = 0;
                    do {
                        for (int loopIdx3 = 0;;loopIdx3++,                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10005;
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (InputA[2] == 2) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000e;
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10010;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        if (WaveIsFirstLane()) {
                            break;
                        }
                        if (testBit(uint4(0x477bc001, 0xe05428d, 0x20ee35be, 0xa56b6ae9), WaveGetLaneIndex())) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10017;
                        }
                        {
                            int loopIdx3 = 0;
                            do {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                if (WaveIsFirstLane()) {
                                    break;
                                }
                                loopIdx3++;
                            } while (true);
                        }
                        loopIdx2++;
                    } while (true);
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (WaveGetLaneIndex() == loopIdx1) {
                    if (WaveIsFirstLane()) {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10023;
                    }
                } else {
                    if (testBit(uint4(0x477bc001, 0xe05428d, 0x20ee35be, 0xa56b6ae9), WaveGetLaneIndex())) {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10027;
                    } else {
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (WaveGetLaneIndex() == loopIdx1) {
                    }
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
            if (WaveGetLaneIndex() == loopIdx1) {
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10031;
                switch (WaveGetLaneIndex() & 3) {
                    case 0:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (InputA[3] == 3) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1003b;
                        break;
                    }
                    case 1:
                    {
                        if (WaveGetLaneIndex() == loopIdx1) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        break;
                    }
                    case 2:
                    {
                        {
                            int loopIdx2 = 0;
                            do {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                if (WaveIsFirstLane()) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                                loopIdx2++;
                            } while (true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        break;
                    }
                    case 3:
                    {
                        if (WaveGetLaneIndex() == loopIdx1) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1004e;
                        }
                        break;
                    }
                }
                break;
            } else {
                if (WaveIsFirstLane()) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    {
                        int loopIdx2 = 0;
                        do {
                            loopIdx2++;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } while (loopIdx2 < InputA[1]);
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
                {
                    int loopIdx2 = 0;
                    do {
                        loopIdx2++;
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10060;
                        if (testBit(uint4(0x477bc001, 0xe05428d, 0x20ee35be, 0xa56b6ae9), WaveGetLaneIndex())) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                    } while (loopIdx2 < InputA[5]);
                }
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10065;
            }
        }
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        if (WaveGetLaneIndex() == loopIdx0) {
            for (int loopIdx1 = 0;
                     loopIdx1 < WaveGetLaneIndex() + 1;
                     loopIdx1++) {
                for (int loopIdx2 = 0;
                         loopIdx2 < WaveGetLaneIndex() + 1;
                         loopIdx2++) {
                    if (WaveIsFirstLane()) {
                    }
                    if (testBit(uint4(0xbe1d7ec5, 0xeebe994d, 0x7b9a81b7, 0xf95758), WaveGetLaneIndex())) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                }
                switch (loopIdx1) {
                    case 1: {
                        break;
                        break;
                    }
                    case 2: {
                        switch (loopIdx0) {
                            case 1: {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1007b;
                                break;
                            }
                            case 2: {
                                break;
                            }
                            default: {
                                break;
                            }
                        }
                        break;
                    }
                    default: {
                        break;
                        break;
                    }
                }
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            if (WaveIsFirstLane()) {
                if (InputA[3] == 3) {
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1008b;
                    for (int loopIdx1 = 0;;loopIdx1++,                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            break;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10092;
                    }
                    if (testBit(uint4(0xe459f25a, 0xdca35218, 0x278524d4, 0x6a18b2f7), WaveGetLaneIndex())) {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10095;
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                } else {
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1008b;
                    for (int loopIdx1 = 0;;loopIdx1++,                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            break;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10092;
                    }
                    if (testBit(uint4(0xe459f25a, 0xdca35218, 0x278524d4, 0x6a18b2f7), WaveGetLaneIndex())) {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10095;
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (WaveGetLaneIndex() == loopIdx0) {
                    for (int loopIdx1 = 0;
                             loopIdx1 < InputA[4];
                             loopIdx1++) {
                    }
                }
            }
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100af;
        }
    }
}

