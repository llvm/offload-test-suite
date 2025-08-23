[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
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
  case 2: {
      if (true) {
        result = (result + WaveActiveSum(3));
      }
      break;
    }
  case 3: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13))) {
            if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 15))) {
              result = (result + WaveActiveSum(result));
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
      }
      break;
    }
  default: {
      result = (result + WaveActiveSum(99));
      break;
    }
  }
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
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveMax(7));
        }
        if ((WaveGetLaneIndex() < 8)) {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(result));
          }
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveMin(result));
          }
        } else {
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 3))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
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
