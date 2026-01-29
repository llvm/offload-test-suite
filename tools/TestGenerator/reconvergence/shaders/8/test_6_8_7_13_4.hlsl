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
    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
    if (testBit(uint4(0x7c271cdc, 0xcb18f481, 0x6ec4196a, 0x550db835), WaveGetLaneIndex())) {
        if (InputA[0] == 0) {
            switch (WaveGetLaneIndex() & 3) {
                case 0:
                {
                    if (WaveIsFirstLane()) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10008;
                        for (int loopIdx0 = 0;
                                 loopIdx0 < InputA[5];
                                 loopIdx0++) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000a;
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
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                                case 3:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10016;
                                    break;
                                }
                            }
                            if (WaveGetLaneIndex() == loopIdx0) {
                            } else {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1001c;
                            }
                        }
                    }
                    break;
                }
                case 1:
                {
                    break;
                }
                case 2:
                {
                    if (InputA[2] == 2) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        for (int loopIdx0 = 0;
                                 loopIdx0 < WaveGetLaneIndex() + 1;
                                 loopIdx0++) {
                            if (testBit(uint4(0xc06a63de, 0xdca35a29, 0x3b774a8, 0x4e2b27f5), WaveGetLaneIndex())) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            } else {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            {
                                int loopIdx1 = 0;
                                do {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    if (WaveIsFirstLane()) {
                                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                        break;
                                    }
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10032;
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    loopIdx1++;
                                } while (true);
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10036;
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1003a;
                    break;
                }
                case 3:
                {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    break;
                }
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            for (int loopIdx0 = 0;;loopIdx0++,            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                for (int loopIdx1 = 0;
                         loopIdx1 < WaveGetLaneIndex() + 1;
                         loopIdx1++) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (WaveGetLaneIndex() == loopIdx1) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        for (int loopIdx2 = 0;
                                 loopIdx2 < WaveGetLaneIndex() + 1;
                                 loopIdx2++) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10047;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveGetLaneIndex() == loopIdx1) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1004c;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        for (int loopIdx2 = 0;
                                 loopIdx2 < WaveGetLaneIndex() + 1;
                                 loopIdx2++) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10047;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveGetLaneIndex() == loopIdx1) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1004c;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                    }
                    if (testBit(uint4(0x7c271cdc, 0xcb18f481, 0x6ec4196a, 0x550db835), WaveGetLaneIndex())) {
                        break;
                        {
                            int loopIdx2 = 0;
                            do {
                                loopIdx2++;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            } while (loopIdx2 < InputA[5]);
                        }
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10066;
                }
                for (int loopIdx1 = 0;
                         loopIdx1 < InputA[2];
                         loopIdx1++) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006a;
                    if (gIndex >= InputA[0x12]) {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006c;
                        if (testBit(uint4(0x7c271cdc, 0xcb18f481, 0x6ec4196a, 0x550db835), WaveGetLaneIndex())) {
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                    }
                }
                if (WaveIsFirstLane()) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10075;
                    break;
                }
                {
                    int loopIdx1 = 0;
                    do {
                        if (WaveIsFirstLane()) {
                            if (WaveGetLaneIndex() == loopIdx1) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1007b;
                            }
                            if (testBit(uint4(0x56e4eb08, 0xc50314e5, 0xf94d540e, 0x41e77aae), WaveGetLaneIndex())) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            } else {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10080;
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10083;
                        if (testBit(uint4(0x7c271cdc, 0xcb18f481, 0x6ec4196a, 0x550db835), WaveGetLaneIndex())) {
                            for (int loopIdx2 = 0;;loopIdx2++,                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                                if (WaveIsFirstLane()) {
                                    break;
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1008a;
                            }
                            switch (InputA[0]) {
                                case 1:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1008e;
                                    break;
                                }
                                case 0:
                                {
                                    break;
                                }
                                case 2:
                                {
                                    break;
                                }
                            }
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            for (int loopIdx2 = 0;
                                     loopIdx2 < WaveGetLaneIndex() + 1;
                                     loopIdx2++) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                        }
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1009c;
                            break;
                        }
                        switch (WaveGetLaneIndex() & 3) {
                            case 0:
                            {
                                break;
                            }
                            case 1:
                            {
                                {
                                    int loopIdx2 = 0;
                                    do {
                                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100a5;
                                        if (WaveIsFirstLane()) {
                                            if (testBit(uint4(0xc06a63de, 0xdca35a29, 0x3b774a8, 0x4e2b27f5), WaveGetLaneIndex())) {
                                                break;
                                            } else {
                                                break;
                                            }
                                        }
                                        loopIdx2++;
                                    } while (true);
                                }
                                break;
                            }
                            case 2:
                            {
                                if (WaveGetLaneIndex() == loopIdx1) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                }
                                break;
                            }
                            case 3:
                            {
                                if (WaveGetLaneIndex() == loopIdx1) {
                                }
                                break;
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (InputA[0] == 0) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100bb;
                            if (InputA[3] == 3) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100bd;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        loopIdx1++;
                    } while (true);
                }
                if (InputA[0] == 0) {
                    if (testBit(uint4(0x55f9c0db, 0x2434d3fe, 0xbccf3e1c, 0x2fb1f2c0), WaveGetLaneIndex())) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveIsFirstLane()) {
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        {
                            int loopIdx1 = 0;
                            do {
                                loopIdx1++;
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100ca;
                            } while (loopIdx1 < InputA[5]);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    if (InputA[1] == 1) {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100cf;
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        for (int loopIdx1 = 0;;loopIdx1++,                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (WaveIsFirstLane()) {
                                break;
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100d9;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    } else {
                        if (InputA[3] == 3) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } else {
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100e2;
                        if (WaveIsFirstLane()) {
                        }
                    }
                } else {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    switch (InputA[3]) {
                        case 4:
                        {
                            if (InputA[1] == 1) {
                            }
                            break;
                        }
                        case 3:
                        {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (WaveGetLaneIndex() == loopIdx0) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            return;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            break;
                        }
                        case 5:
                        {
                            {
                                int loopIdx1 = 0;
                                do {
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100f8;
                                    if (WaveIsFirstLane()) {
                                        break;
                                    }
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    loopIdx1++;
                                } while (true);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100fe;
                            break;
                        }
                    }
                    if (WaveGetLaneIndex() == loopIdx0) {
                        switch (InputA[1]) {
                            case 2:
                            {
                                break;
                            }
                            case 1:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                            case 3:
                            {
                                break;
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1010b;
                        if (gIndex >= InputA[0x25]) {
                        }
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (InputA[3] == 3) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10112;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        for (int loopIdx1 = 0;
                                 loopIdx1 < InputA[5];
                                 loopIdx1++) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                    }
                }
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        }
    }
}

