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
                if (InputA[0] == 0) {
                    switch (loopIdx0) {
                        case 1: {
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            break;
                        }
                        case 2: {
                            break;
                        }
                        default: {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10014;
                            if (gIndex >= InputA[0x47]) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10017;
                            } else {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10019;
                            }
                            break;
                        }
                    }
                } else {
                    {
                        int loopIdx1 = 0;
                        do {
                            loopIdx1++;
                            if (WaveGetLaneIndex() == loopIdx1) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10021;
                            }
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10024;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } while (loopIdx1 < InputA[3]);
                    }
                }
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1002a;
                {
                    int loopIdx1 = 0;
                    do {
                        loopIdx1++;
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        for (int loopIdx2 = 0;
                                 loopIdx2 < InputA[5];
                                 loopIdx2++) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1002e;
                            continue;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        break;
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    } while (loopIdx1 < InputA[1]);
                }
                if (WaveIsFirstLane()) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10037;
                    break;
                    break;
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                for (int loopIdx1 = 0;
                         loopIdx1 < InputA[4];
                         loopIdx1++) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1003e;
                    {
                        int loopIdx2 = 0;
                        do {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10040;
                            if (WaveGetLaneIndex() == loopIdx2) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10042;
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            break;
                            if (WaveIsFirstLane()) {
                                if (testBit(uint4(0xad560636, 0xdca36a0f, 0x6e8b91d0, 0x23174fba), WaveGetLaneIndex())) {
                                    break;
                                } else {
                                    break;
                                }
                            }
                            switch (InputA[0]) {
                                case 1:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1004f;
                                    break;
                                }
                                case 0:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                                case 2:
                                {
                                    break;
                                }
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10057;
                            {
                                int loopIdx3 = 0;
                                do {
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10059;
                                    if (WaveIsFirstLane()) {
                                        break;
                                    }
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    loopIdx3++;
                                } while (true);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1005f;
                            loopIdx2++;
                        } while (true);
                    }
                }
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10062;
                break;
            }
        }
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        if (testBit(uint4(0xb75d1502, 0xe151ecb9, 0xf35298f9, 0xcdb79989), WaveGetLaneIndex())) {
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        } else {
            switch (WaveGetLaneIndex() & 3) {
                case 0:
                {
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006c;
                    break;
                }
                case 1:
                {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    for (int loopIdx0 = 0;;loopIdx0++,                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10075;
                        for (int loopIdx1 = 0;
                                 loopIdx1 < WaveGetLaneIndex() + 1;
                                 loopIdx1++) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10077;
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10079;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            if (WaveGetLaneIndex() == loopIdx1) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1007e;
                            }
                        }
                        if (WaveIsFirstLane()) {
                            break;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        switch (loopIdx0) {
                            case 1: {
                                if (testBit(uint4(0x4823cf2e, 0x37def99d, 0x2183eb05, 0x60435a6a), WaveGetLaneIndex())) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                }
                                break;
                            }
                            case 2: {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                if (gIndex >= InputA[0xe]) {
                                }
                                break;
                            }
                            default: {
                                if (gIndex >= InputA[0x12]) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                }
                                break;
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    break;
                }
                case 2:
                {
                    if (gIndex >= InputA[0x4b]) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveIsFirstLane()) {
                            switch (WaveGetLaneIndex() & 3) {
                                case 0:
                                {
                                    break;
                                }
                                case 1:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100a1;
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
                        if (gIndex >= InputA[0x28]) {
                            if (testBit(uint4(0x4823cf2e, 0x37def99d, 0x2183eb05, 0x60435a6a), WaveGetLaneIndex())) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100ae;
                            } else {
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                    }
                    break;
                }
                case 3:
                {
                    if (testBit(uint4(0x4823cf2e, 0x37def99d, 0x2183eb05, 0x60435a6a), WaveGetLaneIndex())) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        for (int loopIdx0 = 0;
                                 loopIdx0 < WaveGetLaneIndex() + 1;
                                 loopIdx0++) {
                            if (InputA[1] == 1) {
                            } else {
                            }
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
                    break;
                }
            }
            if (testBit(uint4(0xb75d1502, 0xe151ecb9, 0xf35298f9, 0xcdb79989), WaveGetLaneIndex())) {
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100cd;
                if (gIndex >= InputA[0x46]) {
                    if (InputA[2] == 2) {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100d0;
                        if (gIndex >= InputA[0x8]) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (gIndex >= InputA[0x58]) {
                        }
                    }
                    for (int loopIdx0 = 0;
                             loopIdx0 < InputA[1];
                             loopIdx0++) {
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        break;
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100dd;
                    }
                } else {
                    if (gIndex >= InputA[0x21]) {
                        {
                            int loopIdx0 = 0;
                            do {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100e2;
                                if (WaveIsFirstLane()) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                loopIdx0++;
                            } while (true);
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
                                break;
                            }
                            case 2:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100f1;
                                break;
                            }
                            case 3:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100f5;
                                break;
                            }
                        }
                    }
                    if (InputA[0] == 12345) {
                        while (true) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                    }
                }
            } else {
                for (int loopIdx0 = 0;
                         loopIdx0 < WaveGetLaneIndex() + 1;
                         loopIdx0++) {
                    if (WaveGetLaneIndex() == loopIdx0) {
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (gIndex >= InputA[0x17]) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10106;
                        break;
                    }
                }
                if (gIndex >= InputA[0x47]) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (testBit(uint4(0xb75d1502, 0xe151ecb9, 0xf35298f9, 0xcdb79989), WaveGetLaneIndex())) {
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
                                break;
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10119;
                        if (gIndex >= InputA[0x5a]) {
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1011d;
                    }
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10122;
        }
    }
}

