[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
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
      uint counter0 = 0;
      while ((counter0 < 3)) {
        counter0 = (counter0 + 1);
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 13))) {
          result = (result + WaveActiveMax(4));
        }
        if ((WaveGetLaneIndex() == 9)) {
          if ((WaveGetLaneIndex() == 2)) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
          switch ((WaveGetLaneIndex() % 3)) {
          case 0: {
              if ((WaveGetLaneIndex() < 8)) {
                result = (result + WaveActiveSum(1));
              }
              break;
            }
          case 1: {
              if (((WaveGetLaneIndex() % 2) == 0)) {
                result = (result + WaveActiveSum(2));
              }
            }
          case 2: {
              if (true) {
                result = (result + WaveActiveSum(3));
              }
              break;
            }
          }
          if ((WaveGetLaneIndex() == 3)) {
            result = (result + WaveActiveSum(result));
          }
        } else {
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
        switch ((WaveGetLaneIndex() % 2)) {
        case 0: {
            if ((WaveGetLaneIndex() < 8)) {
              result = (result + WaveActiveSum(1));
            }
            break;
          }
        case 1: {
            if (((WaveGetLaneIndex() % 2) == 0)) {
              result = (result + WaveActiveSum(2));
            }
            break;
          }
        }
      }
    }
    break;
  }
  default: {
    result = (result + WaveActiveSum(99));
    break;
  }
  }
}
