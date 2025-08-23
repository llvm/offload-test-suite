[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 10))) {
    if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 15))) {
      result = (result + WaveActiveMax(result));
    }
    switch ((WaveGetLaneIndex() % 3)) {
    case 0: {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
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
  switch ((WaveGetLaneIndex() % 2)) {
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
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 15))) {
          result = (result + WaveActiveMax(result));
        }
        if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10))) {
          if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
          }
          if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveMax(result));
          }
        }
        if ((counter0 == 2)) {
          break;
        }
      }
      break;
    }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        uint counter1 = 0;
        while ((counter1 < 2)) {
          counter1 = (counter1 + 1);
          if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 4))) {
            if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 5))) {
              result = (result + WaveActiveMax(result));
            }
            if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 1))) {
              result = (result + WaveActiveMax(result));
            }
          } else {
          if ((WaveGetLaneIndex() == 0)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
          if ((WaveGetLaneIndex() == 13)) {
            result = (result + WaveActiveMax(result));
          }
        }
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 9))) {
          result = (result + WaveActiveSum(result));
        }
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
      }
    }
  }
  case 1: {
    if (((WaveGetLaneIndex() & 1) == 1)) {
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
    }
    break;
  }
  }
}
