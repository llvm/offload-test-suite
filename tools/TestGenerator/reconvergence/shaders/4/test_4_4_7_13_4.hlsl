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
        if (InputA[2] == 2) {
            switch (WaveGetLaneIndex() & 3) {
                case 0:
                {
                    if (WaveIsFirstLane()) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    break;
                }
                case 1:
                {
                    {
                        int loopIdx0 = 0;
                        do {
                            loopIdx0++;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } while (loopIdx0 < InputA[5]);
                    }
                    break;
                }
                case 2:
                {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    break;
                }
                case 3:
                {
                    if (gIndex >= InputA[0x5a]) {
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10016;
                    }
                    break;
                }
            }
            if (InputA[3] == 3) {
                if (InputA[0] == 0) {
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1001c;
                } else {
                }
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1001f;
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (InputA[2] == 2) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        }
        switch (WaveGetLaneIndex() & 3) {
            case 0:
            {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                break;
            }
            case 1:
            {
                if (testBit(uint4(0x7c271cdc, 0xcb18f481, 0x6ec4196a, 0x550db835), WaveGetLaneIndex())) {
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1002d;
                    for (int loopIdx0 = 0;;loopIdx0++,                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveIsFirstLane()) {
                            break;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10035;
                }
                break;
            }
            case 2:
            {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                break;
            }
            case 3:
            {
                switch (InputA[1]) {
                    case 2:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10040;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        break;
                    }
                    case 1:
                    {
                        if (gIndex >= InputA[0xf]) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10047;
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        {
                            int loopIdx0 = 0;
                            do {
                                loopIdx0++;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            } while (loopIdx0 < InputA[5]);
                        }
                        break;
                    }
                    case 3:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10050;
                        {
                            int loopIdx0 = 0;
                            do {
                                loopIdx0++;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10053;
                            } while (loopIdx0 < InputA[4]);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        break;
                    }
                }
                break;
            }
        }
    } else {
        if (InputA[3] == 3) {
            switch (WaveGetLaneIndex() & 3) {
                case 0:
                {
                    if (WaveIsFirstLane()) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    break;
                }
                case 1:
                {
                    {
                        int loopIdx0 = 0;
                        do {
                            loopIdx0++;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } while (loopIdx0 < InputA[5]);
                    }
                    break;
                }
                case 2:
                {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    break;
                }
                case 3:
                {
                    if (gIndex >= InputA[0x5a]) {
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10016;
                    }
                    break;
                }
            }
            if (InputA[0] == 0) {
                if (InputA[2] == 2) {
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1001c;
                } else {
                }
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1001f;
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (InputA[0] == 0) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        }
        switch (WaveGetLaneIndex() & 3) {
            case 0:
            {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                break;
            }
            case 1:
            {
                if (testBit(uint4(0x7c271cdc, 0xcb18f481, 0x6ec4196a, 0x550db835), WaveGetLaneIndex())) {
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1002d;
                    for (int loopIdx0 = 0;;loopIdx0++,                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveIsFirstLane()) {
                            break;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10035;
                }
                break;
            }
            case 2:
            {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                break;
            }
            case 3:
            {
                switch (InputA[1]) {
                    case 2:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10040;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        break;
                    }
                    case 1:
                    {
                        if (gIndex >= InputA[0xf]) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10047;
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        {
                            int loopIdx0 = 0;
                            do {
                                loopIdx0++;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            } while (loopIdx0 < InputA[5]);
                        }
                        break;
                    }
                    case 3:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10050;
                        {
                            int loopIdx0 = 0;
                            do {
                                loopIdx0++;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10053;
                            } while (loopIdx0 < InputA[4]);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        break;
                    }
                }
                break;
            }
        }
    }
    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100b3;
}

