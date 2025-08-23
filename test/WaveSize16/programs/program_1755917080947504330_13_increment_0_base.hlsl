[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
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
    }
  case 2: {
      if (true) {
        result = (result + WaveActiveSum(3));
      }
      break;
    }
  case 3: {
      uint counter0 = 0;
      while ((counter0 < 3)) {
        counter0 = (counter0 + 1);
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 15))) {
          result = (result + WaveActiveSum(1));
        }
        for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 11))) {
            result = (result + WaveActiveSum(result));
          }
          if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 8))) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
        }
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
        }
      }
      break;
    }
  }
}
