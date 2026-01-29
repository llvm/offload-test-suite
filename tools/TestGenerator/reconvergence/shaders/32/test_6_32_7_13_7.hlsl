#define THREADS_X 7
#define THREADS_Y 13

StructuredBuffer<uint> InputA : register(t0);
RWStructuredBuffer<uint4> OutputB : register(u1);

bool testBit(uint4 mask, uint bit) { return ((mask[bit / 32] >> (bit % 32)) & 1) != 0; }
static int outLoc = 0;
static int invocationStride = 91;

                    void func0(uint gIndex) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        return;
                    }
                    void func1(uint gIndex) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        return;
                    }

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
                    if (gIndex >= InputA[0x37]) {
                        if (gIndex >= InputA[0xb]) {
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000b;
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } else {
                            if (gIndex >= InputA[0x13]) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            } else {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                while (!WaveIsFirstLane()) {}
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (gIndex >= InputA[0x31]) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                } else {
                    func0(gIndex);
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (InputA[0] == 0) {
                        for (int loopIdx0 = 0;
                                 loopIdx0 < InputA[3];
                                 loopIdx0++) {
                            switch (InputA[2]) {
                                case 3:
                                {
                                    break;
                                }
                                case 2:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10029;
                                    break;
                                }
                                case 4:
                                {
                                    break;
                                }
                            }
                            {
                                int loopIdx1 = 0;
                                do {
                                    loopIdx1++;
                                } while (loopIdx1 < InputA[2]);
                            }
                        }
                    } else {
                        if (WaveIsFirstLane()) {
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
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1003d;
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
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10044;
                                    break;
                                }
                                case 3:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1004a;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1004d;
                        if (testBit(uint4(0x89eabcfb, 0xd9b69862, 0xe646b896, 0xe35f35d1), WaveGetLaneIndex())) {
                            if (testBit(uint4(0xdec52586, 0x6b5046f6, 0xc800a33e, 0xd06851f7), WaveGetLaneIndex())) {
                            }
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                        } else {
                            {
                                int loopIdx0 = 0;
                                do {
                                    loopIdx0++;
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10056;
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                } while (loopIdx0 < InputA[1]);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10059;
                            if (gIndex >= InputA[0x3f]) {
                            }
                        }
                    }
                }
                break;
            }
            case 2:
            {
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10061;
                if (InputA[0] == 0) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (gIndex >= InputA[0x3c]) {
                        if (InputA[3] == 3) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10066;
                            for (int loopIdx0 = 0;
                                     loopIdx0 < WaveGetLaneIndex() + 1;
                                     loopIdx0++) {
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10069;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006b;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (InputA[0] == 0) {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006f;
                        if (gIndex >= InputA[0x25]) {
                            if (WaveIsFirstLane()) {
                            }
                            for (int loopIdx0 = 0;;loopIdx0++,                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10074;
                                if (WaveIsFirstLane()) {
                                    break;
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10079;
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        switch (WaveGetLaneIndex() & 3) {
                            case 0:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10080;
                                if (InputA[0] == 0) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10083;
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10085;
                                break;
                            }
                            case 1:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                if (WaveIsFirstLane()) {
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1008a;
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                            case 2:
                            {
                                if (WaveIsFirstLane()) {
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                            case 3:
                            {
                                if (WaveIsFirstLane()) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10096;
                                }
                                break;
                            }
                        }
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006f;
                        if (gIndex >= InputA[0x25]) {
                            if (WaveIsFirstLane()) {
                            }
                            for (int loopIdx0 = 0;;loopIdx0++,                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10074;
                                if (WaveIsFirstLane()) {
                                    break;
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10079;
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        switch (WaveGetLaneIndex() & 3) {
                            case 0:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10080;
                                if (InputA[2] == 2) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10083;
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10085;
                                break;
                            }
                            case 1:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                if (WaveIsFirstLane()) {
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1008a;
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                            case 2:
                            {
                                if (WaveIsFirstLane()) {
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                            case 3:
                            {
                                if (WaveIsFirstLane()) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10096;
                                }
                                break;
                            }
                        }
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100c7;
                }
                break;
            }
            case 3:
            {
                break;
            }
        }
    } else {
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
                    if (gIndex >= InputA[0x37]) {
                        if (gIndex >= InputA[0xb]) {
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1000b;
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        } else {
                            if (gIndex >= InputA[0x13]) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            } else {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                while (!WaveIsFirstLane()) {}
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (gIndex >= InputA[0x31]) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                } else {
                    func1(gIndex);
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (InputA[2] == 2) {
                        for (int loopIdx0 = 0;
                                 loopIdx0 < InputA[3];
                                 loopIdx0++) {
                            switch (InputA[2]) {
                                case 3:
                                {
                                    break;
                                }
                                case 2:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10029;
                                    break;
                                }
                                case 4:
                                {
                                    break;
                                }
                            }
                            {
                                int loopIdx1 = 0;
                                do {
                                    loopIdx1++;
                                } while (loopIdx1 < InputA[2]);
                            }
                        }
                    } else {
                        if (WaveIsFirstLane()) {
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
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1003d;
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
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10044;
                                    break;
                                }
                                case 3:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1004a;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1004d;
                        if (testBit(uint4(0x89eabcfb, 0xd9b69862, 0xe646b896, 0xe35f35d1), WaveGetLaneIndex())) {
                            if (testBit(uint4(0xdec52586, 0x6b5046f6, 0xc800a33e, 0xd06851f7), WaveGetLaneIndex())) {
                            }
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                        } else {
                            {
                                int loopIdx0 = 0;
                                do {
                                    loopIdx0++;
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10056;
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                } while (loopIdx0 < InputA[1]);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10059;
                            if (gIndex >= InputA[0x3f]) {
                            }
                        }
                    }
                }
                break;
            }
            case 2:
            {
                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10061;
                if (InputA[2] == 2) {
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (gIndex >= InputA[0x3c]) {
                        if (InputA[3] == 3) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10066;
                            for (int loopIdx0 = 0;
                                     loopIdx0 < WaveGetLaneIndex() + 1;
                                     loopIdx0++) {
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10069;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006b;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    if (InputA[0] == 0) {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006f;
                        if (gIndex >= InputA[0x25]) {
                            if (WaveIsFirstLane()) {
                            }
                            for (int loopIdx0 = 0;;loopIdx0++,                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10074;
                                if (WaveIsFirstLane()) {
                                    break;
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10079;
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        switch (WaveGetLaneIndex() & 3) {
                            case 0:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10080;
                                if (InputA[3] == 3) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10083;
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10085;
                                break;
                            }
                            case 1:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                if (WaveIsFirstLane()) {
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1008a;
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                            case 2:
                            {
                                if (WaveIsFirstLane()) {
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                            case 3:
                            {
                                if (WaveIsFirstLane()) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10096;
                                }
                                break;
                            }
                        }
                    } else {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006f;
                        if (gIndex >= InputA[0x25]) {
                            if (WaveIsFirstLane()) {
                            }
                            for (int loopIdx0 = 0;;loopIdx0++,                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10074;
                                if (WaveIsFirstLane()) {
                                    break;
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10079;
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        switch (WaveGetLaneIndex() & 3) {
                            case 0:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10080;
                                if (InputA[0] == 0) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10083;
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10085;
                                break;
                            }
                            case 1:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                if (WaveIsFirstLane()) {
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1008a;
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                            case 2:
                            {
                                if (WaveIsFirstLane()) {
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                            case 3:
                            {
                                if (WaveIsFirstLane()) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10096;
                                }
                                break;
                            }
                        }
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100c7;
                }
                break;
            }
            case 3:
            {
                break;
            }
        }
    }
}

