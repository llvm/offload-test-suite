[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 15))) {
        if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 14))) {
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 8))) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
        }
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
      } else {
      if ((WaveGetLaneIndex() >= 14)) {
        result = (result + WaveActiveMin(3));
      }
      uint counter0 = 0;
      while ((counter0 < 3)) {
        counter0 = (counter0 + 1);
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
        if ((counter0 == 2)) {
          break;
        }
      }
    }
    break;
  }
  case 1: {
    if (((WaveGetLaneIndex() % 2) == 0)) {
      result = (result + WaveActiveSum(2));
    }
  }
  case 2: {
    if (true) {
      result = (result + WaveActiveSum(3));
    }
    break;
  }
  }
  uint counter1 = 0;
  while ((counter1 < 2)) {
    counter1 = (counter1 + 1);
    if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 10))) {
      result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
    }
    if ((counter1 == 1)) {
      break;
    }
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
        if ((((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 6))) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMin(result));
          }
        }
        if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 7))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
      }
      break;
    }
  case 1: {
      if ((WaveGetLaneIndex() == 9)) {
        if ((WaveGetLaneIndex() == 4)) {
          result = (result + WaveActiveMax(5));
        }
        if ((WaveGetLaneIndex() == 8)) {
          if ((WaveGetLaneIndex() == 14)) {
            result = (result + WaveActiveMin(result));
          }
        }
        if ((WaveGetLaneIndex() == 7)) {
          result = (result + WaveActiveMin(result));
        }
      } else {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveMax(WaveGetLaneIndex()));
      }
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
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveMin(6));
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
