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
  if ((WaveGetLaneIndex() >= 12)) {
    result = (result + WaveActiveSum(1));
  } else {
  if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 12))) {
    result = (result + WaveActiveMin(2));
  } else {
  if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
    result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
  }
  }
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 3)) {
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
          uint counter0 = 0;
          while ((counter0 < 3)) {
            counter0 = (counter0 + 1);
            if ((WaveGetLaneIndex() == 5)) {
              result = (result + WaveActiveSum(WaveGetLaneIndex()));
            }
          }
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
      uint counter1 = 0;
      while ((counter1 < 2)) {
        counter1 = (counter1 + 1);
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveSum(result));
        }
        if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 13))) {
          if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveSum(5));
          }
          if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMin(result));
          }
        }
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveMin(6));
        }
        if ((counter1 == 1)) {
          break;
        }
      }
      break;
    }
  }
}
