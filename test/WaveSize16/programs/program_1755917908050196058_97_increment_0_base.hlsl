[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 15))) {
          result = (result + WaveActiveMin(10));
        }
        for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
          if ((WaveGetLaneIndex() < 6)) {
            result = (result + WaveActiveMin(result));
          }
        }
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 15))) {
          result = (result + WaveActiveMin(result));
        }
      }
      break;
    }
  case 1: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 15))) {
            if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
            }
          }
          break;
        }
      case 1: {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            if (((WaveGetLaneIndex() & 1) == 0)) {
              result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
            }
            if (((WaveGetLaneIndex() & 1) == 1)) {
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
  if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 15))) {
    switch ((WaveGetLaneIndex() % 3)) {
    case 0: {
        if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 11))) {
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 0))) {
            result = (result + WaveActiveMin(result));
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
    default: {
        result = (result + WaveActiveSum(99));
        break;
      }
    }
  }
}
