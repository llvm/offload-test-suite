[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
    if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 13))) {
      result = (result + WaveActiveMax(result));
    }
    if ((WaveGetLaneIndex() == 7)) {
      if ((WaveGetLaneIndex() == 6)) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(result));
        }
      }
      if ((WaveGetLaneIndex() == 15)) {
        result = (result + WaveActiveSum(10));
      }
    }
    if ((i0 == 1)) {
      continue;
    }
  }
  if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 10))) {
    if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 0))) {
      result = (result + WaveActiveSum(result));
    }
    if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 13))) {
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 15))) {
        result = (result + WaveActiveMax(10));
      }
      switch ((WaveGetLaneIndex() % 4)) {
      case 0: {
          uint counter1 = 0;
          while ((counter1 < 2)) {
            counter1 = (counter1 + 1);
            if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 12))) {
              result = (result + WaveActiveMax(result));
            }
            if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 13))) {
              result = (result + WaveActiveMax(7));
            }
            if ((counter1 == 1)) {
              break;
            }
          }
          break;
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
          break;
        }
      case 3: {
          if ((WaveGetLaneIndex() < 20)) {
            result = (result + WaveActiveSum(4));
          }
          break;
        }
      }
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 14))) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
    }
    if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 5))) {
      result = (result + WaveActiveMax(result));
    }
  }
}
