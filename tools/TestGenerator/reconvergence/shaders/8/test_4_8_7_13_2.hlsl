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
                }
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10006;
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            } else {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
            if (InputA[3] == 3) {
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000c;
                switch (WaveGetLaneIndex() & 3) {
                    case 0:
                    {
                        break;
                    }
                    case 1:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        break;
                    }
                    case 2:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10014;
                        break;
                    }
                    case 3:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10017;
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        break;
                    }
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            } else {
                if (WaveIsFirstLane()) {
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
        }
        if (InputA[3] == 3) {
            if (gIndex >= InputA[0x25]) {
                if (WaveIsFirstLane()) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (WaveIsFirstLane()) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1002a;
                }
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1002c;
            }
        } else {
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            switch (WaveGetLaneIndex() & 3) {
                case 0:
                {
                    break;
                }
                case 1:
                {
                    if (gIndex >= InputA[0x4b]) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    } else {
                    }
                    break;
                }
                case 2:
                {
                    if (gIndex >= InputA[0x16]) {
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1003d;
                    }
                    break;
                }
                case 3:
                {
                    for (int loopIdx0 = 0;;loopIdx0++,                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            break;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    break;
                }
            }
        }
    } else {
        if (testBit(uint4(0x4be9eec2, 0x80c7a6f2, 0xe252fe7f, 0xf1ddec4d), WaveGetLaneIndex())) {
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            for (int loopIdx0 = 0;;loopIdx0++,            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                switch (WaveGetLaneIndex() & 3) {
                    case 0:
                    {
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
                        break;
                    }
                }
                if (testBit(uint4(0x4be9eec2, 0x80c7a6f2, 0xe252fe7f, 0xf1ddec4d), WaveGetLaneIndex())) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
                if (WaveIsFirstLane()) {
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1005f;
                    if (testBit(uint4(0x5247c0ae, 0xdca34a0b, 0x919b5668, 0xdc0696c0), WaveGetLaneIndex())) {
                        break;
                    } else {
                        break;
                    }
                }
                if (WaveGetLaneIndex() == loopIdx0) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10069;
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (WaveIsFirstLane()) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        }
        if (testBit(uint4(0x4be9eec2, 0x80c7a6f2, 0xe252fe7f, 0xf1ddec4d), WaveGetLaneIndex())) {
            if (InputA[0] == 0) {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (WaveIsFirstLane()) {
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10076;
                }
            }
            {
                int loopIdx0 = 0;
                do {
                    loopIdx0++;
                    {
                        int loopIdx1 = 0;
                        do {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (WaveIsFirstLane()) {
                                break;
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            loopIdx1++;
                        } while (true);
                    }
                } while (loopIdx0 < InputA[5]);
            }
        } else {
            if (InputA[1] == 1) {
                {
                    int loopIdx0 = 0;
                    do {
                        loopIdx0++;
                    } while (loopIdx0 < InputA[4]);
                }
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
                        break;
                    }
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
            if (WaveIsFirstLane()) {
                if (gIndex >= InputA[0x1]) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
                {
                    int loopIdx0 = 0;
                    do {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10099;
                        if (WaveIsFirstLane()) {
                            break;
                        }
                        loopIdx0++;
                    } while (true);
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100a0;
        }
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
    }
}

