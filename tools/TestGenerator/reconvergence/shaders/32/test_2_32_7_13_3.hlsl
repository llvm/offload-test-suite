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
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        }
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
        {
            int loopIdx1 = 0;
            do {
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10006;
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (WaveIsFirstLane()) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000a;
                    break;
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                loopIdx1++;
            } while (true);
        }
        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
    }
    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10011;
    switch (InputA[0]) {
        case 1:
        {
            break;
        }
        case 0:
        {
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10016;
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
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10020;
                    break;
                }
            }
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
            break;
        }
        case 2:
        {
            break;
        }
    }
}

