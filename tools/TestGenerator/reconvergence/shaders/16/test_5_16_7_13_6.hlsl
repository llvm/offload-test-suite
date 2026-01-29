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
    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
    if (WaveIsFirstLane()) {
        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10002;
        if (WaveIsFirstLane()) {
            if (testBit(uint4(0xd49562fa, 0x4d048210, 0x5ea3e45b, 0x4bec7494), WaveGetLaneIndex())) {
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10005;
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            for (int loopIdx0 = 0;;loopIdx0++,            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                if (InputA[2] == 2) {
                    switch (loopIdx0) {
                        case 1: {
                            break;
                        }
                        case 2: {
                            break;
                        }
                        default: {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10010;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            break;
                        }
                    }
                } else {
                    switch (loopIdx0) {
                        case 1: {
                            break;
                        }
                        case 2: {
                            break;
                        }
                        default: {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10010;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            break;
                        }
                    }
                }
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10020;
                if (WaveIsFirstLane()) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10023;
                    for (int loopIdx1 = 0;
                             loopIdx1 < WaveGetLaneIndex() + 1;
                             loopIdx1++) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10026;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    for (int loopIdx1 = 0;
                             loopIdx1 < WaveGetLaneIndex() + 1;
                             loopIdx1++) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1002b;
                    }
                }
                if (WaveIsFirstLane()) {
                    break;
                }
                {
                    int loopIdx1 = 0;
                    do {
                        loopIdx1++;
                        if (testBit(uint4(0xb75d1502, 0xe151ecb9, 0xf35298f9, 0xcdb79989), WaveGetLaneIndex())) {
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10034;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (testBit(uint4(0xad560636, 0xdca36a0f, 0x6e8b91d0, 0x23174fba), WaveGetLaneIndex())) {
                            break;
                        } else {
                            break;
                        }
                    } while (loopIdx1 < InputA[4]);
                }
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1003e;
                if (testBit(uint4(0xb75d1502, 0xe151ecb9, 0xf35298f9, 0xcdb79989), WaveGetLaneIndex())) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (WaveGetLaneIndex() == loopIdx0) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10044;
                    {
                        int loopIdx1 = 0;
                        do {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10046;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (WaveIsFirstLane()) {
                                break;
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1004b;
                            loopIdx1++;
                        } while (true);
                    }
                } else {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1004f;
                    {
                        int loopIdx1 = 0;
                        do {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10051;
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10053;
                                break;
                            }
                            loopIdx1++;
                        } while (true);
                    }
                    if (WaveIsFirstLane()) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
            }
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1005d;
        }
        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1005f;
        if (gIndex >= InputA[0xf]) {
            for (int loopIdx0 = 0;
                     loopIdx0 < WaveGetLaneIndex() + 1;
                     loopIdx0++) {
                if (WaveIsFirstLane()) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    switch (InputA[2]) {
                        case 3:
                        {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            break;
                        }
                        case 2:
                        {
                            break;
                        }
                        case 4:
                        {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            break;
                        }
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006e;
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
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            break;
                        }
                    }
                }
                for (int loopIdx1 = 0;
                         loopIdx1 < InputA[5];
                         loopIdx1++) {
                    switch (WaveGetLaneIndex() & 3) {
                        case 0:
                        {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1007f;
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
                    break;
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1008b;
                }
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1008f;
        } else {
            for (int loopIdx0 = 0;
                     loopIdx0 < WaveGetLaneIndex() + 1;
                     loopIdx0++) {
                if (WaveIsFirstLane()) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    switch (InputA[2]) {
                        case 3:
                        {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            break;
                        }
                        case 2:
                        {
                            break;
                        }
                        case 4:
                        {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            break;
                        }
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006e;
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
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            break;
                        }
                    }
                }
                for (int loopIdx1 = 0;
                         loopIdx1 < InputA[5];
                         loopIdx1++) {
                    switch (WaveGetLaneIndex() & 3) {
                        case 0:
                        {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1007f;
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
                    break;
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1008b;
                }
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1008f;
        }
    }
    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
}

