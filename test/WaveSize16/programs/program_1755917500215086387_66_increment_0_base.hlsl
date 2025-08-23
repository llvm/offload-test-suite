[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  uint counter0 = 0;
  while ((counter0 < 2)) {
    counter0 = (counter0 + 1);
    if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 15))) {
      result = (result + WaveActiveMax(result));
    }
    for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
      if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 11))) {
        result = (result + WaveActiveMax(result));
      }
      switch ((WaveGetLaneIndex() % 4)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
          }
          break;
        }
      case 1: {
          for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
            if ((WaveGetLaneIndex() == 1)) {
              result = (result + WaveActiveMax(result));
            }
            if ((WaveGetLaneIndex() == 0)) {
              result = (result + WaveActiveMin(result));
            }
          }
          break;
        }
      case 2: {
          if (true) {
            result = (result + WaveActiveSum(3));
          }
          break;
        }
      case 3: {
          if ((WaveGetLaneIndex() < 20)) {
            result = (result + WaveActiveSum(4));
          }
          break;
        }
      default: {
          result = (result + WaveActiveSum(99));
          break;
        }
      }
      if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 6))) {
        result = (result + WaveActiveMin(result));
      }
      if ((i1 == 1)) {
        continue;
      }
    }
    if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
      result = (result + WaveActiveMax(result));
    }
  }
}
