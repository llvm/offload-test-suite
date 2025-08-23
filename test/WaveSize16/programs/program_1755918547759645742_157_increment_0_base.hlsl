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
      if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 13))) {
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 12))) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
        uint counter0 = 0;
        while ((counter0 < 3)) {
          counter0 = (counter0 + 1);
          if ((WaveGetLaneIndex() == 12)) {
            result = (result + WaveActiveSum(result));
          }
          if ((counter0 == 2)) {
            break;
          }
        }
      } else {
      if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15))) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
      }
      if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 12))) {
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 11))) {
          result = (result + WaveActiveMax(5));
        }
      }
      if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 11))) {
        result = (result + WaveActiveMin(3));
      }
    }
    break;
  }
  case 2: {
    uint counter1 = 0;
    while ((counter1 < 2)) {
      counter1 = (counter1 + 1);
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMin(7));
      }
      for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
        if ((WaveGetLaneIndex() == 6)) {
          result = (result + WaveActiveMax(result));
        }
        if ((WaveGetLaneIndex() == 0)) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
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
}
