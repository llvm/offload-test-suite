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
    {
        int loopIdx0 = 0;
        do {
            loopIdx0++;
            if (InputA[0] == 0) {
            } else {
            }
            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10004;
            {
                int loopIdx1 = 0;
                do {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (WaveIsFirstLane()) {
                        break;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000b;
                    loopIdx1++;
                } while (true);
            }
        } while (loopIdx0 < InputA[2]);
    }
    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000e;
    switch (WaveGetLaneIndex() & 3) {
        case 0:
        {
            break;
        }
        case 1:
        {
            if (gIndex >= InputA[0x11]) {
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            }
            break;
        }
        case 2:
        {
            switch (InputA[2]) {
                case 3:
                {
                    break;
                }
                case 2:
                {
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1001c;
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    break;
                }
                case 4:
                {
                    break;
                }
            }
            break;
        }
        case 3:
        {
            break;
        }
    }
}

