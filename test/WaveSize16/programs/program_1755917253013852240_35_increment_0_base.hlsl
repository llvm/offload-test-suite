[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if ((WaveGetLaneIndex() == 8)) {
        if ((WaveGetLaneIndex() == 7)) {
          result = (result + WaveActiveSum(result));
        }
        uint counter0 = 0;
        while ((counter0 < 2)) {
          counter0 = (counter0 + 1);
          if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 3))) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
        }
        if ((WaveGetLaneIndex() == 2)) {
          result = (result + WaveActiveSum(result));
        }
      }
      break;
    }
  case 1: {
      for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
        if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMax(result));
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
