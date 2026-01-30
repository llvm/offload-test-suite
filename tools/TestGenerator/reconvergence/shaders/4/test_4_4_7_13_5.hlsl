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
                        }
                        break;
                    }
                    case 2:
                    {
                        break;
                    }
                    case 3:
                    {
                        if (InputA[2] == 2) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        break;
                    }
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (InputA[0] == 0) {
                    if (gIndex >= InputA[0x33]) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1001b;
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
                } else {
                    if (gIndex >= InputA[0x33]) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1001b;
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
                }
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            break;
        }
        case 1:
        {
            if (gIndex >= InputA[0x29]) {
                if (testBit(uint4(0xd773c098, 0xa9130fe8, 0x7ab7d00c, 0x2d9acc74), WaveGetLaneIndex())) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
                if (gIndex >= InputA[0x40]) {
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10042;
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
            } else {
                {
                    int loopIdx0 = 0;
                    do {
                        if (gIndex >= InputA[0x47]) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10048;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveIsFirstLane()) {
                            break;
                        }
                        switch (loopIdx0) {
                            case 1: {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10050;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                            case 2: {
                                break;
                            }
                            default: {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveIsFirstLane()) {
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        loopIdx0++;
                    } while (true);
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
            break;
        }
        case 2:
        {
            if (testBit(uint4(0xadba7496, 0xdca3621e, 0x6e6723a8, 0x23fb3d07), WaveGetLaneIndex())) {
                switch (WaveGetLaneIndex() & 3) {
                    case 0:
                    {
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
                        break;
                    }
                    case 1:
                    {
                        {
                            int loopIdx0 = 0;
                            do {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                if (WaveIsFirstLane()) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                                loopIdx0++;
                            } while (true);
                        }
                        break;
                    }
                    case 2:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1007b;
                        break;
                    }
                    case 3:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1007e;
                        if (testBit(uint4(0x2d48218, 0x4beafcfd, 0x3d258fdc, 0xf9d6175a), WaveGetLaneIndex())) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10080;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        break;
                    }
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
            break;
        }
        case 3:
        {
            if (testBit(uint4(0x2d48218, 0x4beafcfd, 0x3d258fdc, 0xf9d6175a), WaveGetLaneIndex())) {
                {
                    int loopIdx0 = 0;
                    do {
                        loopIdx0++;
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (InputA[2] == 2) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        if (testBit(uint4(0x6dfa865e, 0x5a043f88, 0x2d9b688a, 0x3e9ec403), WaveGetLaneIndex())) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                    } while (loopIdx0 < InputA[3]);
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (testBit(uint4(0xadba7496, 0xdca3621e, 0x6e6723a8, 0x23fb3d07), WaveGetLaneIndex())) {
                    if (gIndex >= InputA[0x1b]) {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10098;
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (gIndex >= InputA[0x56]) {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1009e;
                    }
                }
            } else {
                if (testBit(uint4(0x875613ed, 0x21349191, 0xdf3985e7, 0xce73a89a), WaveGetLaneIndex())) {
                    if (testBit(uint4(0x2d48218, 0x4beafcfd, 0x3d258fdc, 0xf9d6175a), WaveGetLaneIndex())) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100a5;
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    if (gIndex >= InputA[0x33]) {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100aa;
                    }
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (WaveIsFirstLane()) {
                    if (WaveIsFirstLane()) {
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100b1;
                    if (gIndex >= InputA[0x25]) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100b4;
                    }
                }
            }
            break;
        }
    }
}

