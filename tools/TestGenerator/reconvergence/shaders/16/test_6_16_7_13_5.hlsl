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
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (InputA[1] == 1) {
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
                                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10017;
                                        break;
                                    }
                                }
                            } else {
                                if (gIndex >= InputA[0x33]) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1001d;
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
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        break;
                    }
                    case 2:
                    {
                        if (gIndex >= InputA[0x29]) {
                            if (testBit(uint4(0xd773c098, 0xa9130fe8, 0x7ab7d00c, 0x2d9acc74), WaveGetLaneIndex())) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            if (gIndex >= InputA[0x40]) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10035;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                        } else {
                            {
                                int loopIdx0 = 0;
                                do {
                                    if (gIndex >= InputA[0x47]) {
                                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1003b;
                                    }
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    if (WaveIsFirstLane()) {
                                        break;
                                    }
                                    switch (loopIdx0) {
                                        case 1: {
                                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10043;
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
                    case 3:
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
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1006e;
                                    break;
                                }
                                case 3:
                                {
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10071;
                                    if (testBit(uint4(0x2d48218, 0x4beafcfd, 0x3d258fdc, 0xf9d6175a), WaveGetLaneIndex())) {
                                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10073;
                                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    }
                                    break;
                                }
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        }
                        break;
                    }
                }
                if (WaveIsFirstLane()) {
                    if (InputA[3] == 3) {
                        for (int loopIdx0 = 0;;loopIdx0++,                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10080;
                            {
                                int loopIdx1 = 0;
                                do {
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10082;
                                    if (WaveIsFirstLane()) {
                                        if (testBit(uint4(0xadba7496, 0xdca3621e, 0x6e6723a8, 0x23fb3d07), WaveGetLaneIndex())) {
                                            break;
                                        } else {
                                            break;
                                        }
                                    }
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1008a;
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    loopIdx1++;
                                } while (true);
                            }
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                break;
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            {
                                int loopIdx1 = 0;
                                do {
                                    loopIdx1++;
                                } while (loopIdx1 < InputA[3]);
                            }
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
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1009e;
                                    break;
                                }
                            }
                        }
                        if (WaveIsFirstLane()) {
                            for (int loopIdx0 = 0;
                                     loopIdx0 < WaveGetLaneIndex() + 1;
                                     loopIdx0++) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100a4;
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100a7;
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                        }
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100ad;
                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                }
            }
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
            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
            break;
        }
        case 3:
        {
            if (WaveIsFirstLane()) {
                for (int loopIdx0 = 0;
                         loopIdx0 < InputA[1];
                         loopIdx0++) {
                    if (WaveGetLaneIndex() == loopIdx0) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        if (InputA[0] == 0) {
                            for (int loopIdx1 = 0;
                                     loopIdx1 < InputA[5];
                                     loopIdx1++) {
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (WaveIsFirstLane()) {
                            }
                        } else {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (WaveGetLaneIndex() == loopIdx0) {
                            } else {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100c8;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100cb;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100cd;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100cf;
                    if (WaveIsFirstLane()) {
                        if (InputA[1] == 1) {
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100d2;
                            if (gIndex >= InputA[0x1]) {
                            }
                            {
                                int loopIdx1 = 0;
                                do {
                                    if (WaveIsFirstLane()) {
                                        break;
                                    }
                                    loopIdx1++;
                                } while (true);
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100da;
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100dc;
                        break;
                    }
                }
                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                if (InputA[0] == 12345) {
                    while (true) {
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                }
                if (testBit(uint4(0x6dfa865e, 0x5a043f88, 0x2d9b688a, 0x3e9ec403), WaveGetLaneIndex())) {
                    for (int loopIdx0 = 0;
                             loopIdx0 < InputA[5];
                             loopIdx0++) {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100e4;
                        if (InputA[0] == 0) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (gIndex >= InputA[0x53]) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100e9;
                            }
                            switch (loopIdx0) {
                                case 1: {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                                case 2: {
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                        }
                        {
                            int loopIdx1 = 0;
                            do {
                                if (WaveIsFirstLane()) {
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100f8;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                if (WaveGetLaneIndex() == loopIdx1) {
                                }
                                if (WaveIsFirstLane()) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                                for (int loopIdx2 = 0;
                                         loopIdx2 < WaveGetLaneIndex() + 1;
                                         loopIdx2++) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10102;
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                loopIdx1++;
                            } while (true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    if (WaveIsFirstLane()) {
                        switch (WaveGetLaneIndex() & 3) {
                            case 0:
                            {
                                if (testBit(uint4(0xadba7496, 0xdca3621e, 0x6e6723a8, 0x23fb3d07), WaveGetLaneIndex())) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                }
                                break;
                            }
                            case 1:
                            {
                                if (testBit(uint4(0x875613ed, 0x21349191, 0xdf3985e7, 0xce73a89a), WaveGetLaneIndex())) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10112;
                                } else {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10115;
                                }
                                break;
                            }
                            case 2:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                if (WaveIsFirstLane()) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                }
                                break;
                            }
                            case 3:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1011f;
                                if (InputA[2] == 2) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                }
                                break;
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        for (int loopIdx0 = 0;;loopIdx0++,                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                            break;
                            if (WaveIsFirstLane()) {
                            }
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1012b;
                                break;
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1012e;
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10133;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10135;
                } else {
                    for (int loopIdx0 = 0;
                             loopIdx0 < InputA[5];
                             loopIdx0++) {
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100e4;
                        if (InputA[2] == 2) {
                            OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            if (gIndex >= InputA[0x53]) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100e9;
                            }
                            switch (loopIdx0) {
                                case 1: {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                                case 2: {
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                        }
                        {
                            int loopIdx1 = 0;
                            do {
                                if (WaveIsFirstLane()) {
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x100f8;
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                if (WaveGetLaneIndex() == loopIdx1) {
                                }
                                if (WaveIsFirstLane()) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    break;
                                }
                                for (int loopIdx2 = 0;
                                         loopIdx2 < WaveGetLaneIndex() + 1;
                                         loopIdx2++) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10102;
                                }
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                loopIdx1++;
                            } while (true);
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                    }
                    if (WaveIsFirstLane()) {
                        switch (WaveGetLaneIndex() & 3) {
                            case 0:
                            {
                                if (testBit(uint4(0xadba7496, 0xdca3621e, 0x6e6723a8, 0x23fb3d07), WaveGetLaneIndex())) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                }
                                break;
                            }
                            case 1:
                            {
                                if (testBit(uint4(0x875613ed, 0x21349191, 0xdf3985e7, 0xce73a89a), WaveGetLaneIndex())) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10112;
                                } else {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10115;
                                }
                                break;
                            }
                            case 2:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                if (WaveIsFirstLane()) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                }
                                break;
                            }
                            case 3:
                            {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1011f;
                                if (InputA[0] == 0) {
                                    OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                                }
                                break;
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                        for (int loopIdx0 = 0;;loopIdx0++,                        OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true)) {
                            break;
                            if (WaveIsFirstLane()) {
                            }
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1012b;
                                break;
                            }
                            OutputB[(outLoc++)*invocationStride + gIndex].x = 0x1012e;
                            if (WaveIsFirstLane()) {
                                OutputB[(outLoc++)*invocationStride + gIndex] = WaveActiveBallot(true);
                            }
                        }
                        OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10133;
                    }
                    OutputB[(outLoc++)*invocationStride + gIndex].x = 0x10135;
                }
            }
            break;
        }
    }
}

