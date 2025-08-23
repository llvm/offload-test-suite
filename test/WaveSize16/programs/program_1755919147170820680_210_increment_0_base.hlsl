[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((WaveGetLaneIndex() == 1)) {
        uint counter0 = 0;
        while ((counter0 < 2)) {
          counter0 = (counter0 + 1);
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 15))) {
            result = (result + WaveActiveSum(7));
          }
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 14))) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
        }
      } else {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
      uint counter1 = 0;
      while ((counter1 < 3)) {
        counter1 = (counter1 + 1);
        if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 9))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
        }
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
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
  case 3: {
    if ((WaveGetLaneIndex() < 20)) {
      result = (result + WaveActiveSum(4));
    }
    break;
  }
  }
  for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
    if (((WaveGetLaneIndex() & 1) == 0)) {
      result = (result + WaveActiveMin(result));
    }
    uint counter3 = 0;
    while ((counter3 < 3)) {
      counter3 = (counter3 + 1);
      if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 14))) {
        result = (result + WaveActiveMax(result));
      }
      if ((WaveGetLaneIndex() >= 9)) {
        if ((WaveGetLaneIndex() < 7)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
      }
    }
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveMax(result));
    }
    if ((i2 == 1)) {
      break;
    }
  }
}
