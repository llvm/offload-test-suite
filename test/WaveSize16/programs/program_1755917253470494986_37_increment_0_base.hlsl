[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13))) {
    result = (result + WaveActiveSum(1));
  } else {
  if ((((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 9))) {
    result = (result + WaveActiveMin(2));
  }
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
        uint counter1 = 0;
        while ((counter1 < 2)) {
          counter1 = (counter1 + 1);
          if ((WaveGetLaneIndex() == 0)) {
            result = (result + WaveActiveMax(result));
          }
        }
        if ((WaveGetLaneIndex() == 3)) {
          result = (result + WaveActiveSum(2));
        }
      }
    }
  case 2: {
      if ((WaveGetLaneIndex() == 8)) {
        if ((WaveGetLaneIndex() == 12)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
          if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 7))) {
            result = (result + WaveActiveMax(result));
          }
          if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveMin(result));
          }
        }
        if ((WaveGetLaneIndex() == 2)) {
          result = (result + WaveActiveMax(result));
        }
      }
    }
  case 3: {
      if ((WaveGetLaneIndex() < 20)) {
        result = (result + WaveActiveSum(4));
      }
      break;
    }
  }
  if ((WaveGetLaneIndex() == 11)) {
    uint counter3 = 0;
    while ((counter3 < 3)) {
      counter3 = (counter3 + 1);
      uint counter4 = 0;
      while ((counter4 < 2)) {
        counter4 = (counter4 + 1);
        if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 10))) {
          result = (result + WaveActiveMax(3));
        }
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15))) {
          if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11))) {
            result = (result + WaveActiveMax(result));
          }
        } else {
        if ((WaveGetLaneIndex() >= 14)) {
          result = (result + WaveActiveSum(9));
        }
        if ((WaveGetLaneIndex() < 2)) {
          result = (result + WaveActiveSum(result));
        }
      }
      if ((counter4 == 1)) {
        break;
      }
    }
    if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 14))) {
      result = (result + WaveActiveMax(result));
    }
  }
  }
}
