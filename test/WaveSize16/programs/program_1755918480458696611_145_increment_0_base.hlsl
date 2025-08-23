[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((WaveGetLaneIndex() == 2)) {
    if ((WaveGetLaneIndex() == 10)) {
      result = (result + WaveActiveSum(WaveGetLaneIndex()));
    }
    switch ((WaveGetLaneIndex() % 2)) {
    case 0: {
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 15))) {
          if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 15))) {
            result = (result + WaveActiveMax(result));
          }
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMax(result));
          }
        } else {
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 15))) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
      }
      break;
    }
  case 1: {
      if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMin(result));
        }
      } else {
      if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 10))) {
        result = (result + WaveActiveSum(result));
      }
      if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 11))) {
        result = (result + WaveActiveSum(result));
      }
    }
    break;
  }
  default: {
    result = (result + WaveActiveSum(99));
    break;
  }
  }
  } else {
  if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15))) {
    result = (result + WaveActiveMin(2));
  }
  uint counter0 = 0;
  while ((counter0 < 2)) {
    counter0 = (counter0 + 1);
    if ((WaveGetLaneIndex() == 4)) {
      result = (result + WaveActiveMin(result));
    }
    if ((WaveGetLaneIndex() >= 9)) {
      if ((WaveGetLaneIndex() < 1)) {
        result = (result + WaveActiveSum(result));
      }
    }
    if ((WaveGetLaneIndex() == 9)) {
      result = (result + WaveActiveSum(result));
    }
  }
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 9))) {
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 1))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        uint counter1 = 0;
        while ((counter1 < 2)) {
          counter1 = (counter1 + 1);
          if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveMin(7));
          }
          for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
            if ((WaveGetLaneIndex() == 4)) {
              result = (result + WaveActiveMax(3));
            }
            if ((WaveGetLaneIndex() == 11)) {
              result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
            }
          }
          if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveSum(5));
          }
          if ((counter1 == 1)) {
            break;
          }
        }
      }
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
