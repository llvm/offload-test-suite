#define THREADS_X 7
#define THREADS_Y 13

StructuredBuffer<uint> InputA : register(t0);
RWStructuredBuffer<uint4> OutputB : register(u1);

bool testBit(uint4 mask, uint bit) { return ((mask[bit / 32] >> (bit % 32)) & 1) != 0; }
static int outLoc = 0;
static int invocationStride = 91;

        void func0(uint gIndex, int loopIdx0) {
            if (InputA[3] == 3) {
                if (InputA[2] == 2) {
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10026;
                }
            }
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10029;
            switch (WaveGetLaneIndex() & 3) {
                case 0:
                {
                    break;
                }
                case 1:
                {
                    if (testBit(uint4(0xbe1d7ec5, 0xeebe994d, 0x7b9a81b7, 0xf95758), WaveGetLaneIndex())) {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1002f;
                    }
                    break;
                }
                case 2:
                {
                    if (InputA[0] == 0) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10036;
                    break;
                }
                case 3:
                {
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10039;
                    if (testBit(uint4(0xe459f25a, 0xdca35218, 0x278524d4, 0x6a18b2f7), WaveGetLaneIndex())) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    break;
                }
            }
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10040;
        }

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
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (WaveIsFirstLane()) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            break;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000a;
                        loopIdx2++;
                    } while (true);
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (gIndex >= InputA[0x4d]) {
                } else {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
            }
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            if (WaveIsFirstLane()) {
                {
                    int loopIdx2 = 0;
                    do {
                        loopIdx2++;
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    } while (loopIdx2 < InputA[5]);
                }
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10017;
                switch (InputA[0]) {
                    case 1:
                    {
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
            }
        }
        func0(gIndex, loopIdx0);
    }
}

