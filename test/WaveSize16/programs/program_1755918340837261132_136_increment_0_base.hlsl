[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 1))) {
        if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 7))) {
          result = (result + WaveActiveMax(result));
        }
        switch ((WaveGetLaneIndex() % 4)) {
        case 0: {
            if ((WaveGetLaneIndex() < 8)) {
              result = (result + WaveActiveSum(1));
            }
          }
        case 1: {
            if (((WaveGetLaneIndex() % 2) == 0)) {
              result = (result + WaveActiveSum(2));
            }
            break;
          }
        case 2: {
            if (true) {
              result = (result + WaveActiveSum(3));
            }
          }
        case 3: {
            if ((WaveGetLaneIndex() < 20)) {
              result = (result + WaveActiveSum(4));
            }
            break;
          }
        }
        if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 9))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
        }
      }
    }
  case 1: {
      if (((WaveGetLaneIndex() % 2) == 0)) {
        result = (result + WaveActiveSum(2));
      }
      break;
    }
  }
}
