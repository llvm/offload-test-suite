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
    if (InputA[1] == 1) {
        switch (WaveGetLaneIndex() & 3) {
            case 0:
            {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                break;
            }
            case 1:
            {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (testBit(uint4(0x89eabcfb, 0xd9b69862, 0xe646b896, 0xe35f35d1), WaveGetLaneIndex())) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                } else {
                }
                break;
            }
            case 2:
            {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                for (int loopIdx0 = 0;;loopIdx0++,                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (WaveIsFirstLane()) {
                        break;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10014;
                }
                break;
            }
            case 3:
            {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (testBit(uint4(0x8904da8a, 0xdca3721c, 0x4ad90d24, 0x745978c), WaveGetLaneIndex())) {
                    return;
                } else {
                    return;
                }
                break;
            }
        }
    } else {
        if (InputA[0] == 0) {
            if (gIndex >= InputA[0x2e]) {
            } else {
            }
            {
                int loopIdx0 = 0;
                do {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (WaveIsFirstLane()) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        break;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    loopIdx0++;
                } while (true);
            }
        } else {
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
            if (gIndex >= InputA[0x46]) {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
        }
        if (testBit(uint4(0x2252ee54, 0x8d2da5f1, 0x8bb2785c, 0x980e465), WaveGetLaneIndex())) {
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1003f;
            if (gIndex >= InputA[0x4c]) {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
        } else {
            if (testBit(uint4(0xd690d39c, 0x218e2b3e, 0x893185a1, 0x44e5c750), WaveGetLaneIndex())) {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            } else {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10048;
            }
            if (testBit(uint4(0xdec52586, 0x6b5046f6, 0xc800a33e, 0xd06851f7), WaveGetLaneIndex())) {
            }
        }
    }
    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
}

