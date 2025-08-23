[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15))) {
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        if ((WaveGetLaneIndex() < 6)) {
          if ((WaveGetLaneIndex() >= 12)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveMin(result));
        }
      } else {
      if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 6))) {
        result = (result + WaveActiveSum(result));
      }
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
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 5))) {
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
    for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
      if ((WaveGetLaneIndex() == 11)) {
        result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
      }
      uint counter1 = 0;
      while ((counter1 < 2)) {
        counter1 = (counter1 + 1);
        if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveSum(result));
        }
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveMax(6));
        }
      }
    }
    break;
  }
  case 3: {
    uint counter2 = 0;
    while ((counter2 < 3)) {
      counter2 = (counter2 + 1);
      for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
        if ((WaveGetLaneIndex() >= 13)) {
          result = (result + WaveActiveSum(result));
        }
        if ((i3 == 1)) {
          continue;
        }
      }
      if ((WaveGetLaneIndex() == 0)) {
        result = (result + WaveActiveMax(result));
      }
      if ((counter2 == 2)) {
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
}
