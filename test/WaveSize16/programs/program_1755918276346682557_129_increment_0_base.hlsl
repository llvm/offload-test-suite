[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      uint counter0 = 0;
      while ((counter0 < 3)) {
        counter0 = (counter0 + 1);
        if ((WaveGetLaneIndex() < 1)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        if ((WaveGetLaneIndex() == 11)) {
          if ((WaveGetLaneIndex() == 9)) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
          if ((WaveGetLaneIndex() == 0)) {
            if ((WaveGetLaneIndex() == 1)) {
              result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
            }
            if ((WaveGetLaneIndex() == 15)) {
              result = (result + WaveActiveMax(result));
            }
          }
        }
      }
      break;
    }
  case 2: {
      for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
        if ((WaveGetLaneIndex() >= 14)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
          for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
            if ((WaveGetLaneIndex() >= 9)) {
              result = (result + WaveActiveMin(WaveGetLaneIndex()));
            }
          }
        }
        if ((i1 == 2)) {
          break;
        }
      }
      break;
    }
  }
}
