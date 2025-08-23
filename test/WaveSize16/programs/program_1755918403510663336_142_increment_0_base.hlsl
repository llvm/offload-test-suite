[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if ((WaveGetLaneIndex() == 7)) {
        if ((WaveGetLaneIndex() == 6)) {
          result = (result + WaveActiveMax(result));
        }
        uint counter0 = 0;
        while ((counter0 < 3)) {
          counter0 = (counter0 + 1);
          if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 8))) {
            result = (result + WaveActiveMax(1));
          }
          if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 8))) {
            result = (result + WaveActiveSum(result));
          }
        }
      }
      break;
    }
  case 1: {
      for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
        if ((WaveGetLaneIndex() < 8)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveSum(result));
          }
        }
        if ((WaveGetLaneIndex() >= 12)) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
        if ((i1 == 1)) {
          continue;
        }
        if ((i1 == 1)) {
          break;
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
  default: {
      result = (result + WaveActiveSum(99));
      break;
    }
  }
}
