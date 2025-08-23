[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
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
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 12))) {
        for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
          if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 7))) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
          }
        }
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 3))) {
          result = (result + WaveActiveMin(result));
        }
      }
      break;
    }
  case 1: {
      if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 3))) {
        if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 9))) {
          result = (result + WaveActiveMin(result));
        }
        if ((WaveGetLaneIndex() < 6)) {
          if ((WaveGetLaneIndex() < 6)) {
            result = (result + WaveActiveSum(result));
          }
          if ((WaveGetLaneIndex() >= 10)) {
            result = (result + WaveActiveMax(result));
          }
        }
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveMin(result));
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
