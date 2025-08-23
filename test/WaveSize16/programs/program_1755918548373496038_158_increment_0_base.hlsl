[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  uint counter0 = 0;
  while ((counter0 < 2)) {
    counter0 = (counter0 + 1);
    if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 12))) {
      result = (result + WaveActiveMax(result));
    }
    if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 7))) {
      if ((WaveGetLaneIndex() < 1)) {
        if ((WaveGetLaneIndex() >= 14)) {
          result = (result + WaveActiveMax(result));
        }
      }
      if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 10))) {
        result = (result + WaveActiveMin(result));
      }
    } else {
    uint counter1 = 0;
    while ((counter1 < 3)) {
      counter1 = (counter1 + 1);
      if ((WaveGetLaneIndex() >= 9)) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
      if ((WaveGetLaneIndex() < 2)) {
        result = (result + WaveActiveSum(result));
      }
    }
    if ((WaveGetLaneIndex() >= 13)) {
      result = (result + WaveActiveMax(result));
    }
  }
  if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 15))) {
    result = (result + WaveActiveSum(result));
  }
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      uint counter2 = 0;
      while ((counter2 < 3)) {
        counter2 = (counter2 + 1);
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13))) {
          if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 11))) {
            result = (result + WaveActiveSum(3));
          }
        } else {
        if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveMin(result));
        }
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
        }
      }
      if ((counter2 == 2)) {
        break;
      }
    }
    break;
  }
  case 1: {
    if (((WaveGetLaneIndex() & 1) == 1)) {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
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
      }
    }
  }
  case 2: {
    uint counter3 = 0;
    while ((counter3 < 3)) {
      counter3 = (counter3 + 1);
      for (uint i4 = 0; (i4 < 2); i4 = (i4 + 1)) {
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
          result = (result + WaveActiveMax(result));
        }
        if ((i4 == 1)) {
          break;
        }
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
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
