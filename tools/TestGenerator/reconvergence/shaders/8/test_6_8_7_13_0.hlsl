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
    if (WaveIsFirstLane()) {
        for (int loopIdx0 = 0;
                 loopIdx0 < InputA[4];
                 loopIdx0++) {
            {
                int loopIdx1 = 0;
                do {
                    loopIdx1++;
                    if (InputA[2] == 2) {
                        if (WaveGetLaneIndex() == loopIdx1) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            for (int loopIdx2 = 0;
                                     loopIdx2 < InputA[3];
                                     loopIdx2++) {
                            }
                            for (int loopIdx2 = 0;
                                     loopIdx2 < InputA[2];
                                     loopIdx2++) {
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000b;
                        switch (loopIdx1) {
                            case 1: {
                                if (InputA[3] == 3) {
                                } else {
                                }
                                break;
                            }
                            case 2: {
                                break;
                                break;
                            }
                            default: {
                                if (gIndex >= InputA[0x47]) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                } else {
                                }
                                break;
                            }
                        }
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    if (WaveIsFirstLane()) {
                        if (WaveGetLaneIndex() == loopIdx1) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10021;
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10023;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10021;
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10023;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        if (InputA[2] == 2) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10031;
                            if (testBit(uint4(0x2909ad5e, 0x15194574, 0xc3379230, 0x9d6b11f2), WaveGetLaneIndex())) {
                            } else {
                            }
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10031;
                            if (testBit(uint4(0x2909ad5e, 0x15194574, 0xc3379230, 0x9d6b11f2), WaveGetLaneIndex())) {
                            } else {
                            }
                        }
                    }
                } while (loopIdx1 < InputA[1]);
            }
        }
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10040;
        for (int loopIdx0 = 0;
                 loopIdx0 < InputA[3];
                 loopIdx0++) {
            if (testBit(uint4(0x2909ad5e, 0x15194574, 0xc3379230, 0x9d6b11f2), WaveGetLaneIndex())) {
                if (testBit(uint4(0xc9debe4f, 0xcfc5fd3e, 0xf5d57afd, 0xff297fe2), WaveGetLaneIndex())) {
                    if (testBit(uint4(0x78dd605f, 0x1cda5dc7, 0xc225a9aa, 0xbdfc0a37), WaveGetLaneIndex())) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10046;
                        if (gIndex >= InputA[0x6]) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10048;
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1004b;
                        }
                        switch (InputA[4]) {
                            case 5:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                            case 4:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                            case 6:
                            {
                                break;
                            }
                        }
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10058;
                    if (WaveGetLaneIndex() == loopIdx0) {
                        continue;
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1005b;
                        {
                            int loopIdx1 = 0;
                            do {
                                loopIdx1++;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            } while (loopIdx1 < InputA[1]);
                        }
                    }
                } else {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    for (int loopIdx1 = 0;
                             loopIdx1 < InputA[4];
                             loopIdx1++) {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10063;
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveGetLaneIndex() == loopIdx1) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10067;
                        }
                    }
                    break;
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006b;
                }
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006d;
                switch (InputA[0]) {
                    case 1:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10071;
                        if (InputA[0] == 0) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (WaveIsFirstLane()) {
                            }
                            if (InputA[1] == 1) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            } else {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        break;
                    }
                    case 0:
                    {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10080;
                        {
                            int loopIdx1 = 0;
                            do {
                                loopIdx1++;
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10082;
                                if (WaveIsFirstLane()) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                }
                                continue;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            } while (loopIdx1 < InputA[5]);
                        }
                        break;
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1008a;
                        break;
                    }
                    case 2:
                    {
                        if (testBit(uint4(0xdca35e7e, 0xdca345ea, 0x1f7f4828, 0x52e219cc), WaveGetLaneIndex())) {
                            break;
                        } else {
                            break;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        break;
                    }
                }
            }
            if (testBit(uint4(0xdca35e7e, 0xdca345ea, 0x1f7f4828, 0x52e219cc), WaveGetLaneIndex())) {
                continue;
            } else {
                continue;
            }
        }
    }
}

