[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  uint counter0 = 0;
  while ((counter0 < 2)) {
    counter0 = (counter0 + 1);
    if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 14))) {
      result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
    }
    if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 6))) {
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 9))) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
      }
      switch ((WaveGetLaneIndex() % 4)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
          }
          break;
        }
      case 1: {
          if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 12))) {
            if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 8))) {
              result = (result + WaveActiveSum(WaveGetLaneIndex()));
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
      if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 5))) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
    } else {
    if ((WaveGetLaneIndex() >= 8)) {
      result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
    }
    for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
      if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
        if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMax(result));
        }
      }
      if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15))) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
      }
      if ((i1 == 1)) {
        continue;
      }
    }
    if ((WaveGetLaneIndex() >= 13)) {
      result = (result + WaveActiveMax(result));
    }
  }
  }
}
