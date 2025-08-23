[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 13))) {
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 5))) {
          result = (result + WaveActiveMin(result));
        }
        switch ((WaveGetLaneIndex() % 2)) {
        case 0: {
            if ((WaveGetLaneIndex() < 8)) {
              result = (result + WaveActiveSum(1));
            }
            break;
          }
        case 1: {
            if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 10))) {
              if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 10))) {
                result = (result + WaveActiveMax(result));
              }
            } else {
            if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 15))) {
              result = (result + WaveActiveMax(result));
            }
          }
          break;
        }
      }
      if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 5))) {
        result = (result + WaveActiveMax(2));
      }
    } else {
    if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 14))) {
      result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
    }
    switch ((WaveGetLaneIndex() % 3)) {
    case 0: {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMin(8));
          }
        }
        break;
      }
    case 1: {
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13))) {
          if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 8))) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
          }
        } else {
        if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMin(3));
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
  if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
    result = (result + WaveActiveMax(result));
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
  if ((WaveGetLaneIndex() < 3)) {
    if ((WaveGetLaneIndex() < 6)) {
      result = (result + WaveActiveSum(WaveGetLaneIndex()));
    }
    uint counter0 = 0;
    while ((counter0 < 2)) {
      counter0 = (counter0 + 1);
      if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 12))) {
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveSum(3));
        }
        uint counter1 = 0;
        while ((counter1 < 2)) {
          counter1 = (counter1 + 1);
          if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 3))) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
        }
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 12))) {
          result = (result + WaveActiveMin(result));
        }
      }
    }
  }
}
