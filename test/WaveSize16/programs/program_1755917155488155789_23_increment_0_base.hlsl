[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if ((WaveGetLaneIndex() >= 8)) {
        for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
          if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveSum(4));
          }
          for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
            if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15))) {
              result = (result + WaveActiveMin(result));
            }
          }
        }
        if ((WaveGetLaneIndex() < 7)) {
          result = (result + WaveActiveMax(result));
        }
      } else {
      uint counter2 = 0;
      while ((counter2 < 2)) {
        counter2 = (counter2 + 1);
        if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveMax(result));
        }
        for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
          if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveSum(result));
          }
          if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
            result = (result + WaveActiveSum(result));
          }
        }
      }
      if ((WaveGetLaneIndex() >= 9)) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
      }
    }
    break;
  }
  case 1: {
    if ((WaveGetLaneIndex() >= 14)) {
      if ((WaveGetLaneIndex() >= 10)) {
        result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
      }
      uint counter4 = 0;
      while ((counter4 < 3)) {
        counter4 = (counter4 + 1);
        if (((WaveGetLaneIndex() & 1) == 0)) {
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
          }
        } else {
        if ((WaveGetLaneIndex() == 12)) {
          result = (result + WaveActiveMax(result));
        }
      }
      if ((counter4 == 2)) {
        break;
      }
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
  }
}
